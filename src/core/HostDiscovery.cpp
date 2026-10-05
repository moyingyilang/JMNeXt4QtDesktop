#include "core/HostDiscovery.h"

#include "core/JmCrypto.h"

#include <cstdlib>
#include <random>

namespace jmnext::core {
namespace {

/// 从 [pos] 处的 '[' 开始，取出其中所有字符串（只处理字符串数组这一种形状）
std::vector<std::string> extractStringArray(const std::string& s, std::size_t pos, bool& ok) {
    std::vector<std::string> out;
    ok = false;
    if (pos >= s.size() || s[pos] != '[') return out;
    ++pos;
    while (pos < s.size()) {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r' ||
                                  s[pos] == ',')) {
            ++pos;
        }
        if (pos >= s.size()) return out;
        if (s[pos] == ']') { ok = true; return out; }
        if (s[pos] == '[') return out;              // 嵌套数组（如 jm3_Server）不走这条路
        if (s[pos] != '"') return out;
        ++pos;
        std::string value;
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\' && pos + 1 < s.size()) ++pos;   // 简单转义：原样收下后一个字符
            value.push_back(s[pos++]);
        }
        if (pos >= s.size()) return out;
        ++pos;                                     // 跳过收尾引号
        out.push_back(value);
    }
    return out;
}

/// 取出 [[a,b],[c,d]] 这种成对数组
std::vector<std::pair<std::string, std::string>> extractPairs(const std::string& s, std::size_t pos,
                                                             bool& ok) {
    std::vector<std::pair<std::string, std::string>> out;
    ok = false;
    if (pos >= s.size() || s[pos] != '[') return out;
    ++pos;
    while (pos < s.size()) {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == ',' || s[pos] == '\n' || s[pos] == '\r')) ++pos;
        if (pos >= s.size()) return out;
        if (s[pos] == ']') { ok = true; return out; }
        bool innerOk = false;
        auto inner = extractStringArray(s, pos, innerOk);
        if (!innerOk) return out;
        // 跳到内层数组之后
        const auto innerEnd = s.find(']', pos);
        if (innerEnd == std::string::npos) return out;
        pos = innerEnd + 1;
        if (inner.size() >= 2) out.emplace_back(inner[0], inner[1]);
        else if (inner.size() == 1) out.emplace_back(inner[0], std::string());
    }
    return out;
}

/// 找到 "key" 之后第一个 ':' 再往后第一个 '[' 的位置
std::size_t arrayAfterKey(const std::string& s, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    auto at = s.find(needle);
    if (at == std::string::npos) return std::string::npos;
    auto colon = s.find(':', at + needle.size());
    if (colon == std::string::npos) return std::string::npos;
    return s.find('[', colon);
}

}  // namespace

std::optional<HostPayload> parseHostPayload(const std::string& json) {
    HostPayload out;
    bool any = false;

    if (const auto pos = arrayAfterKey(json, "Server"); pos != std::string::npos) {
        bool ok = false;
        out.servers = extractStringArray(json, pos, ok);
        any = any || ok;
    }
    if (const auto pos = arrayAfterKey(json, "Setting"); pos != std::string::npos) {
        bool ok = false;
        out.setting = extractStringArray(json, pos, ok);
        any = any || ok;
    }
    if (const auto pos = arrayAfterKey(json, "jm3_Server"); pos != std::string::npos) {
        bool ok = false;
        out.lines = extractPairs(json, pos, ok);
        any = any || ok;
    }
    if (!any) return std::nullopt;
    return out;
}

std::optional<std::string> pickRandom(const std::vector<std::string>& hosts) {
    if (hosts.empty()) return std::nullopt;
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> dist(0, hosts.size() - 1);
    return hosts[dist(rng)];
}

std::optional<std::string> discoverHost(JmSession& session, jmnext::net::HttpClient& http,
                                       const HostPicker& pick) {
    for (const char* url : HOST_ENDPOINTS) {
        // 注意：这里**只发 URL，不带任何头** —— 不把凭证送给第三方对象存储
        const auto resp = http.get(url, {});
        if (!resp.ok()) continue;

        auto plain = decryptHostPayload(resp.body);
        if (!plain) continue;

        auto payload = parseHostPayload(*plain);
        if (!payload || payload->servers.empty()) continue;

        auto host = pick(payload->servers);
        if (!host) continue;

        session.useHost(JmSession::normalizeHost(*host));
        return session.apiBaseUrl();
    }
    return std::nullopt;
}

}  // namespace jmnext::core
