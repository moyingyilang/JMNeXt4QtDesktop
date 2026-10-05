// 业务请求层测试：拼 URL、带 Token/Tokenparam/Accept、按"发起请求时"的时间戳解密、
// **先取信封 {code,data} 再解密**（真实网络已确认的形状）、解密失败 refresh 重试一次、
// 网络失败标记主机可疑。全部用假客户端，不碰真实网络。
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

/// 真实响应形状：{"code":200,"data":"<base64 密文>"}
static std::string envelope(const std::string& cipher) {
    return std::string("{\"code\":200,\"data\":\"") + cipher + "\"}";
}

class FakeHttp : public jmnext::net::HttpClient {
public:
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
    // ---------- 正常一次成功（响应是信封）----------
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        FakeHttp http;
        const std::string body = R"({"status":"ok","data":{"content":[]}})";
        http.scripted.push_back(jmnext::net::HttpResponse{200, envelope(serverEncrypt(body, s.token()))});

        JmApi api(s, http);
        auto r = api.request("latest", "page=0");
        check(r.has_value(), "请求成功");
        if (r) {
            eqStr(r->text, body, "解出的正文与原文一致（信封里的密文被正确取出并解密）");
            check(!r->retried, "一次成功时不标记重试");
        }
        eqStr(http.seenUrls.at(0), "https://api.example.com/latest?page=0", "URL 含查询串");
        check(http.seenHeaders.at(0).size() == 3, "带了 Token / Tokenparam / Accept 三个头");
        check(http.seenHeaders.at(0)[0].rfind("Token: ", 0) == 0, "第一个头是 Token");
        check(http.seenHeaders.at(0)[2] == "Accept: application/json, text/plain, */*",
              "Accept 与主项目 JmRemote 一致");
        check(!s.hostSuspect(), "成功时主机不可疑");
    }

    // ---------- 信封里的 \/ 转义必须还原（真实响应就是这么给的）----------
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        FakeHttp http;
        const std::string body = R"({"status":"ok"})";
        std::string cipher = serverEncrypt(body, s.token());
        // 模拟服务端的 JSON 转义：把 '/' 变成 "\/"
        std::string escaped;
        for (char c : cipher) {
            if (c == '/') escaped += "\\/";
            else escaped.push_back(c);
        }
        http.scripted.push_back(jmnext::net::HttpResponse{200, envelope(escaped)});
        JmApi api(s, http);
        auto r = api.request("latest");
        check(r.has_value(), "带 \\/ 转义的信封也能解开");
        if (r) eqStr(r->text, body, "转义还原后内容正确");
    }

    // ---------- 非信封响应：按"整个 body 就是密文"兼容处理 ----------
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        FakeHttp http;
        const std::string body = R"({"ok":true})";
        http.scripted.push_back(jmnext::net::HttpResponse{200, serverEncrypt(body, s.token())});
        JmApi api(s, http);
        auto r = api.request("latest");
        check(r.has_value(), "非信封（纯密文）也能解开");
        if (r) eqStr(r->text, body, "内容正确");
    }

    // ---------- 解密失败 → refresh 重试一次 ----------
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        FakeHttp http;
        const std::string body = R"({"status":"ok"})";
        const std::string future = md5Hex("1700000001" + std::string(TOKEN_SEED));
        http.scripted.push_back(jmnext::net::HttpResponse{200, envelope(serverEncrypt(body, future))});
        http.scripted.push_back(jmnext::net::HttpResponse{200, envelope(serverEncrypt(body, future))});
        JmApi api(s, http);
        auto r = api.request("latest");
        check(r.has_value(), "重试后成功");
        if (r) {
            eqStr(r->text, body, "重试后内容正确");
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
        http.scripted.push_back(jmnext::net::HttpResponse{200, envelope("不是密文")});
        http.scripted.push_back(jmnext::net::HttpResponse{200, envelope("仍然不是密文")});
        JmApi api(s, http);
        check(!api.request("latest").has_value(), "两次都失败 → 返回空");
        eqStr(api.lastError(), "解密失败（已重试一次）", "错误说明写明重试过");
    }

    // ---------- HTTP 错误码不算网络类失败 ----------
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

    // ---------- 网络类失败 → 标记主机可疑 ----------
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        FakeHttp http;
        http.scripted.push_back(jmnext::net::HttpResponse{0, ""});
        JmApi api(s, http);
        check(!api.request("latest").has_value(), "网络失败 → 返回空");
        check(s.hostSuspect(), "网络类失败要标记主机可疑");
    }

    // ---------- 主机未初始化 → 不发请求 ----------
    {
        JmSession s("2.1.9", 1700000000);
        FakeHttp http;
        JmApi api(s, http);
        check(!api.request("latest").has_value(), "未初始化主机 → 失败");
        check(http.seenUrls.empty(), "未初始化主机 → 一个请求都不发");
    }

    if (failures == 0)
        std::printf("全部通过：业务请求层（信封/转义/URL/三个头/时间戳复用/重试一次/主机可疑）\n");
    return failures == 0 ? 0 : 1;
}
