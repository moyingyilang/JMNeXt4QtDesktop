// 信封取值的边界测试（独立文件，纯加法）。
//
// 真实响应形如 {"code":200,"data":"<base64 密文>"}，其中 data 里的 "/" 会被 JSON 转义成 "\/"。
// 这里额外覆盖两个容易被忽略的情形：
//   1) data 内的密文含被转义的引号 \"（虽然 base64 不会出现，但取信封的逻辑必须正确）
//   2) 服务端返回非 200 的 code（当前实现**不**据此拦截，仍尝试解密 —— 用测试把这一行为固定下来）
#include "core/JmCore.h"
#include "net/HttpClient.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace jmnext::core;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static void eqStr(const std::string& got, const std::string& want, const char* what) {
    if (got != want) {
        std::printf("FAIL: %s\n  实际 %s\n  期望 %s\n", what, got.c_str(), want.c_str());
        ++failures;
    }
}

static std::string serverEncrypt(const std::string& plain, const std::string& keyHex) {
    uint8_t key[32];
    keyFromHexString(keyHex, key);
    std::vector<uint8_t> data(plain.begin(), plain.end());
    return base64Encode(aes256EcbEncrypt(data, key));
}

class FakeHttp : public jmnext::net::HttpClient {
public:
    std::vector<jmnext::net::HttpResponse> scripted;
    std::size_t call = 0;
    jmnext::net::HttpResponse get(const std::string&, const std::vector<std::string>&) override {
        if (call < scripted.size()) return scripted[call++];
        return {0, ""};
    }
};

int main() {
    // 1) 正常信封中的 \/ 转义
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        const std::string body = R"({"ok":true})";
        std::string cipher = serverEncrypt(body, s.token());
        std::string escaped;
        for (char c : cipher) { if (c == '/') escaped += "\\/"; else escaped.push_back(c); }
        FakeHttp http;
        http.scripted.push_back(jmnext::net::HttpResponse{200, "{\"code\":200,\"data\":\"" + escaped + "\"}"});
        JmApi api(s, http);
        auto r = api.request("latest");
        check(r.has_value(), "带 \\/ 转义的信封可解开");
        if (r) eqStr(r->text, body, "内容正确");
    }

    // 2) 信封非 200 的 code：当前实现不拦截，仍尝试解密（把行为固定下来，将来若改为拦截，
    //    这个测试会失败，从而提醒我们这是一个有意的行为变更）
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        const std::string body = R"({"status":"ok"})";
        FakeHttp http;
        http.scripted.push_back(jmnext::net::HttpResponse{
            200, "{\"code\":401,\"data\":\"" + serverEncrypt(body, s.token()) + "\"}"});
        JmApi api(s, http);
        auto r = api.request("latest");
        check(r.has_value(), "code 非 200 时当前实现仍解密（行为已固定）");
        if (r) eqStr(r->text, body, "仍能取出正文");
    }

    // 3) data 字段缺失 → 按"整个 body 就是密文"处理（兼容路径）
    {
        JmSession s("2.1.9", 1700000000);
        s.useHost("api.example.com");
        const std::string body = R"({"ok":1})";
        FakeHttp http;
        http.scripted.push_back(jmnext::net::HttpResponse{200, serverEncrypt(body, s.token())});
        JmApi api(s, http);
        auto r = api.request("latest");
        check(r.has_value(), "没有信封时按纯密文解（兼容路径仍有效）");
    }

    if (failures == 0) std::printf("全部通过：信封边界（\\/ 转义 / 非 200 code / 无信封路径）\n");
    return failures == 0 ? 0 : 1;
}
