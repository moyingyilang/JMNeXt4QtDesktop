// 主机发现测试。用假网络客户端喂"真形状"的密文载荷，验证整条路径：
// 解密 → 解析（Server 是字符串数组、jm3_Server 是成对数组）→ 选主机 → 写入 session。
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

/// 假客户端：记录收到的头与 URL，按预置表单返回
class FakeHttp : public jmnext::net::HttpClient {
public:
    std::vector<jmnext::net::HttpResponse> scripted;
    std::vector<std::string> seenUrls;
    std::vector<std::size_t> seenHeaderCounts;
    std::size_t call = 0;

    jmnext::net::HttpResponse get(const std::string& url,
                                 const std::vector<std::string>& headers) override {
        seenUrls.push_back(url);
        seenHeaderCounts.push_back(headers.size());
        if (call < scripted.size()) return scripted[call++];
        return {0, ""};
    }
};

// 与主项目注释里给出的实测明文同形状
static const char* kPlain =
    R"({"Setting":["www.cdnhjk.net"],)" 
    R"("Server":["www.cdnhjk.net","www.cdngwc.cc"],)"
    R"("jm3_Server":[["www.cdnhjk.net","線路1"],["www.cdngwc.cc","線路2"]]})";

int main() {
    const std::string cipher = serverEncrypt(kPlain, md5Hex(HOST_SEED));

    // ---------- 解析：Server 是字符串数组，jm3_Server 是成对数组 ----------
    {
        auto p = parseHostPayload(kPlain);
        check(p.has_value(), "能解析出主机载荷");
        if (p) {
            check(p->servers.size() == 2, "Server 两个主机");
            eqStr(p->servers[0], "www.cdnhjk.net", "Server[0]");
            eqStr(p->setting[0], "www.cdnhjk.net", "Setting[0]");
            check(p->lines.size() == 2, "jm3_Server 两对");
            if (p->lines.size() == 2) {
                eqStr(p->lines[0].first, "www.cdnhjk.net", "线路 1 主机");
                eqStr(p->lines[0].second, "線路1", "线路 1 名称");
            }
        }
    }
    check(!parseHostPayload("not json at all").has_value(), "完全不是 JSON → 判失败");

    // ---------- 发现：解密 → 选主机 → 写入 session ----------
    {
        FakeHttp http;
        http.scripted.push_back(jmnext::net::HttpResponse{200, cipher});
        JmSession s("2.1.9", 1700000000);
        // 注入确定性挑法（默认是随机，测试不能靠运气）
        auto picked = discoverHost(s, http, [](const std::vector<std::string>& hosts) {
            return hosts.empty() ? std::nullopt : std::optional<std::string>(hosts[1]);
        });
        check(picked.has_value(), "发现成功");
        if (picked) eqStr(*picked, "https://www.cdngwc.cc/", "选中的主机补 scheme 与尾斜杠");
        eqStr(s.apiBaseUrl(), "https://www.cdngwc.cc/", "session 已写入主机");
        check(http.seenUrls.size() == 1, "第一个入口就成功，不再试第二个");
        check(http.seenHeaderCounts.size() == 1 && http.seenHeaderCounts[0] == 0,
              "隐私规则：主机发现不带任何请求头（不把凭证送给第三方）");
    }

    // ---------- 第一个入口失败则试第二个 ----------
    {
        FakeHttp http;
        http.scripted.push_back(jmnext::net::HttpResponse{500, "boom"});     // 第一个入口挂了
        http.scripted.push_back(jmnext::net::HttpResponse{200, cipher});      // 第二个入口可用
        JmSession s("2.1.9", 1700000000);
        auto picked = discoverHost(s, http, [](const std::vector<std::string>& h) {
            return h.empty() ? std::nullopt : std::optional<std::string>(h[0]);
        });
        check(picked.has_value(), "第一个入口失败后仍能成功");
        eqStr(*picked, "https://www.cdnhjk.net/", "用的是第二个入口的主机");
        check(http.seenUrls.size() == 2, "确实试了两个入口");
    }

    // ---------- 全部失败 → nullopt，且不污染 session ----------
    {
        FakeHttp http;
        http.scripted.push_back(jmnext::net::HttpResponse{200, "这不是密文"});
        http.scripted.push_back(jmnext::net::HttpResponse{200, serverEncrypt(R"({"Server":[]})", md5Hex(HOST_SEED))});
        JmSession s("2.1.9", 1700000000);
        auto picked = discoverHost(s, http, [](const std::vector<std::string>& h) {
            return h.empty() ? std::nullopt : std::optional<std::string>(h[0]);
        });
        check(!picked.has_value(), "全部入口不可用（解密失败 / 主机列表为空）→ 返回空");
        check(!s.hasHost(), "失败时不写入主机");
    }

    if (failures == 0) std::printf("全部通过：主机发现（解密、解析、挑选、隐私规则）\n");
    return failures == 0 ? 0 : 1;
}
