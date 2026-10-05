// 业务请求层测试：拼 URL、带 Token/Tokenparam、按"发起请求时"的时间戳解密、
// 解密失败 refresh 重试一次、网络失败标记主机可疑。全部用假客户端，不碰真实网络。
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

static std::string serverEncrypt(const std::string& plain, const std::string& keyHex) {
    uint8_t key[32];
    keyFromHexString(keyHex, key);
    std::vector<uint8_t> data(plain.begin(), plain.end());
    return base64Encode(aes256EcbEncrypt(data, key));
}

class FakeHttp : public jmnext::net::HttpClient {
public:
    // 每次调用依次取一条脚本（返回 status=0 表示网络类失败）
    std::vector<jmnext::net::HttpResponse> scripted;
    std::vector<std::string> seenUrls;
    std::vector<std::vector<std::string>> seenHeaders;
    std::size_t call = 0;

    jmnext::net::HttpResponse get(const std::string& url,
                                 const std::vector<std::string>& headers) override {
        seenUrls.push_back(url);
        seenHeaders.push_back(headers);
        if (call < scripted.size()) return scripted[call++];
        return {0, ""};
    }
};

int main() {
    // ---------- 正常一次成功 ----------
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        FakeHttp http;
        const std::string body = R"({"status":"ok","data":{"content":[]}})";
        http.scripted.push_back(jmnext::net::HttpResponse{200, serverEncrypt(body, s.token())});

        JmApi api(s, http);
        auto r = api.request("latest", "page=0");
        check(r.has_value(), "请求成功");
        if (r) {
            eqStr(r->text, body, "解出的正文与原文一致");
            check(!r->retried, "一次成功时不标记重试");
        }
        eqStr(http.seenUrls.at(0), "https://api.example.com/latest?page=0", "URL 含查询串");
        check(http.seenHeaders.at(0).size() == 2, "带了 Token 与 Tokenparam 两个头");
        check(http.seenHeaders.at(0)[0].rfind("Token: ", 0) == 0, "第一个头是 Token");
        check(!s.hostSuspect(), "成功时主机不可疑");
    }

    // ---------- 解密失败 → refresh 重试一次（用新时间戳加密的响应）----------
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        FakeHttp http;
        const std::string body = R"({"status":"ok"})";
        // 第一次：服务端用**旧** token 加密（模拟过期）→ 客户端拿旧时间戳能解，
        // 所以这里换个更贴近真实的情形：第一次返回用**未来** token 加密的响应（旧时间戳解不开）
        http.scripted.push_back(jmnext::net::HttpResponse{
            200, serverEncrypt(body, md5Hex("1700000001" + std::string(TOKEN_SEED)))});
        // 第二次（refresh 之后，时间戳 +1）：服务端用新 token 加密
        http.scripted.push_back(jmnext::net::HttpResponse{
            200, serverEncrypt(body, md5Hex("1700000001" + std::string(TOKEN_SEED)))});

        JmApi api(s, http);
        auto r = api.request("latest");
        check(r.has_value(), "重试后成功");
        if (r) {
            eqStr(r->text, body, "重试后解出的正文正确");
            check(r->retried, "标记了重试");
        }
        check(http.seenUrls.size() == 2, "确实发了两次请求");
        check(s.time() == 1700000001, "会话时间戳被 refresh 过");
    }

    // ---------- 两次都解不开 → 明确失败 ----------
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        FakeHttp http;
        http.scripted.push_back(jmnext::net::HttpResponse{200, "不是密文"});
        http.scripted.push_back(jmnext::net::HttpResponse{200, "仍然不是密文"});
        JmApi api(s, http);
        check(!api.request("latest").has_value(), "两次都失败 → 返回空");
        eqStr(api.lastError(), "解密失败（已重试一次）", "错误说明写明重试过");
    }

    // ---------- HTTP 错误码 ----------
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        FakeHttp http;
        http.scripted.push_back(jmnext::net::HttpResponse{500, "boom"});
        JmApi api(s, http);
        check(!api.request("latest").has_value(), "500 → 失败");
        eqStr(api.lastError(), "HTTP 500", "错误说明含状态码");
        check(!s.hostSuspect(), "HTTP 错误码不算网络类失败（主机本身可达）");
    }

    // ---------- 网络类失败 → 标记主机可疑（上层据此换主机）----------
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        FakeHttp http;
        http.scripted.push_back(jmnext::net::HttpResponse{0, ""});
        JmApi api(s, http);
        check(!api.request("latest").has_value(), "网络失败 → 返回空");
        check(s.hostSuspect(), "网络类失败要标记主机可疑（否则用户会遇到怎么刷新都没用）");
    }

    // ---------- 主机未初始化 → 明确失败，不发请求 ----------
    {
        JmSession s("2.1.9", 1700000000);
        FakeHttp http;
        JmApi api(s, http);
        check(!api.request("latest").has_value(), "未初始化主机 → 失败");
        check(http.seenUrls.empty(), "未初始化主机 → 一个请求都不发");
    }

    if (failures == 0) std::printf("全部通过：业务请求层（URL/请求头/时间戳复用/重试一次/主机可疑）\n");
    return failures == 0 ? 0 : 1;
}
