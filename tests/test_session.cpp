// 会话层测试。关键判据：**Token 与响应体 AES 密钥是同一个值** —— 用会话自己算出的 token
// 去加密一段 JSON，再交给 decryptApiData 解，必须原样解回来（跨组件互证，不是自证）。
#include "core/JmCore.h"
#include "net/HttpClient.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace jmnext::core;

static int failures = 0;
static void eqStr(const std::string& got, const std::string& want, const char* what) {
    if (got != want) {
        std::printf("FAIL: %s\n  实际 %s\n  期望 %s\n", what, got.c_str(), want.c_str());
        ++failures;
    }
}
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}

/// 用给定密钥（hex 文本）加密一段文本并按 base64 编码 —— 模拟服务端
static std::string serverEncrypt(const std::string& plain, const std::string& keyHex) {
    uint8_t key[32];
    keyFromHexString(keyHex, key);
    std::vector<uint8_t> data(plain.begin(), plain.end());
    const auto ct = aes256EcbEncrypt(data, key);
    return base64Encode(ct);
}

/// 假网络客户端：按 URL 返回预置响应（证明链路可注入）
class FakeHttp : public jmnext::net::HttpClient {
public:
    jmnext::net::HttpResponse get(const std::string& url,
                                 const std::vector<std::string>& headers) override {
        lastUrl = url;
        lastHeaders = headers;
        return {200, R"({"status":"ok"})"};
    }
    std::string lastUrl;
    std::vector<std::string> lastHeaders;
};

int main() {
    // ---------- 主机规范化 ----------
    eqStr(JmSession::normalizeHost("api.example.com"), "https://api.example.com/", "裸域名补 https 与尾斜杠");
    eqStr(JmSession::normalizeHost("https://a.b/c/"), "https://a.b/c/", "已带协议则只规范尾斜杠");
    eqStr(JmSession::normalizeHost("api.example.com\n"), "https://api.example.com/", "去掉行尾换行");

    // ---------- 未初始化主机时明确失败（不拼半截 URL）----------
    {
        JmSession s("2.1.9", 1700000000);
        check(!s.apiUrl("latest").has_value(), "主机未初始化 → apiUrl 返回空");
        check(!s.imageUrl("x.jpg").has_value(), "图床未初始化 → imageUrl 返回空");
    }

    // ---------- URL 拼接 ----------
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        eqStr(*s.apiUrl("latest"), "https://api.example.com/latest", "apiUrl 拼路径");
        eqStr(*s.apiUrl("/latest"), "https://api.example.com/latest", "apiUrl 容忍多余斜杠");
        eqStr(*s.imageUrl("media/1.jpg"), "https://api.example.com/media/1.jpg", "图床缺省回退 API 主机");
        s.setImageHost("https://img.example.com/");
        eqStr(*s.imageUrl("/media/1.jpg"), "https://img.example.com/media/1.jpg", "图床优先且容忍斜杠");
    }

    // ---------- 会话固定时间戳与 refresh ----------
    {
        JmSession s("2.1.9", 1700000000);
        eqStr(s.tokenParam(), "1700000000,2.1.9", "Tokenparam 形如 <时间戳>,<版本>");
        const std::string t1 = s.token();
        eqStr(t1, md5Hex("1700000000" + std::string(TOKEN_SEED)), "Token = md5(时间戳+seed)");
        check(s.token() == t1, "时间戳在会话内固定（token 稳定）");
        s.refresh(1700009999);
        check(s.token() != t1, "refresh 之后 token 变化");
        eqStr(s.tokenParam(), "1700009999,2.1.9", "refresh 之后 Tokenparam 同步");
    }

    // ---------- 关键规则：Token 就是响应体的 AES 密钥 ----------
    {
        JmSession s("2.1.9", 1700000000);
        const std::string payload = R"({"status":"ok","data":{"total":3}})";
        // 服务端用同一个 token 加密
        const std::string cipher = serverEncrypt(payload, s.token());
        auto dec = decryptApiData(cipher, s.time());
        check(dec.has_value(), "用会话 token 加密的响应能解开");
        if (dec) eqStr(*dec, payload, "解出的内容与原文一致");

        // 用**另一个时间戳**去解同一个响应 → 必须失败或解不出原文（说明时间戳必须复用）
        auto wrong = decryptApiData(cipher, s.time() + 1);
        check(!wrong.has_value() || *wrong != payload, "换时间戳解同一响应 → 拿不到原文（时间戳必须复用）");
    }

    // ---------- "人话"响应不能被当成解密失败 ----------
    {
        JmSession s("2.1.9", 1700000000);
        const std::string message = "已追踪!";
        const std::string cipher = serverEncrypt(message, s.token());
        auto dec = decryptApiData(cipher, s.time());
        check(dec.has_value(), "非 JSON 的一句话也要返回（否则上层会重发非幂等 POST）");
        if (dec) eqStr(*dec, message, "人话内容原样返回");
    }

    // ---------- 主机发现：固定 seed 且必须形如 JSON ----------
    {
        const std::string json = R"({"hosts":["a.com","b.com"]})";
        const std::string cipher = serverEncrypt(json, md5Hex(HOST_SEED));
        auto dec = decryptHostPayload(cipher);
        check(dec.has_value(), "主机发现载荷能解开");
        if (dec) eqStr(*dec, json, "主机发现载荷内容一致");

        const std::string notJson = serverEncrypt("hello", md5Hex(HOST_SEED));
        check(!decryptHostPayload(notJson).has_value(), "主机发现载荷不是 JSON → 判失败");
    }

    // ---------- 可注入网络层 ----------
    {
        FakeHttp http;
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        const auto url = *s.apiUrl("latest");
        const std::vector<std::string> headers = {"Token: " + s.token(), "Tokenparam: " + s.tokenParam()};
        const auto resp = http.get(url, headers);
        check(resp.ok(), "假客户端返回 200");
        eqStr(http.lastUrl, "https://api.example.com/latest", "假客户端收到拼好的 URL");
        check(http.lastHeaders.size() == 2, "假客户端收到 Token 与 Tokenparam 两个头");
    }

    if (failures == 0) std::printf("全部通过：会话层（URL/时间戳复用/人话响应/主机发现）与可注入网络层\n");
    return failures == 0 ? 0 : 1;
}
