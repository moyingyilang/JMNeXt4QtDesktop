#include "core/UpdateCheck.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace jmnext::core {
namespace {

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}
std::vector<std::string> splitDots(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == '.') { out.push_back(cur); cur.clear(); } else { cur.push_back(c); }
    }
    out.push_back(cur);
    return out;
}
bool allDigits(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(),
                                     [](unsigned char c) { return std::isdigit(c) != 0; });
}
/// `fix1` → 1；不是该形态则返回 nullopt
std::optional<int> parseFix(const std::string& token) {
    const std::string t = lower(token);
    if (t.size() <= 3 || t.compare(0, 3, "fix") != 0) return std::nullopt;
    const std::string digits = t.substr(3);
    if (!allDigits(digits)) return std::nullopt;
    return std::stoi(digits);
}

}  // namespace

std::optional<std::vector<int>> versionParts(const std::string& raw) {
    std::string s = raw;
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    if (!s.empty() && (s.front() == 'v' || s.front() == 'V')) s.erase(s.begin());
    if (s.empty()) return std::nullopt;

    std::vector<int> out;
    for (const auto& tok : splitDots(s)) {
        if (allDigits(tok)) { out.push_back(std::stoi(tok)); continue; }
        if (auto fix = parseFix(tok)) {           // fixN 作为第四段
            out.push_back(*fix);
        }
        break;                                     // 其余（.lite/.debug/...）都是变体后缀，忽略
    }
    if (out.empty()) return std::nullopt;
    return out;
}

bool isNewer(const std::string& latest, const std::string& current) {
    const auto r = versionParts(latest);
    const auto l = versionParts(current);
    if (!r || !l) return false;
    const std::size_t n = std::max(r->size(), l->size());
    for (std::size_t i = 0; i < n; ++i) {
        const int rv = i < r->size() ? (*r)[i] : 0;
        const int lv = i < l->size() ? (*l)[i] : 0;
        if (rv != lv) return rv > lv;
    }
    return false;
}

std::string cleanVersion(const std::string& raw) {
    const std::string s = raw;
    // 找出"数字段 + 可选 fixN"，按原样拼回
    std::string head = s;
    while (!head.empty() && std::isspace(static_cast<unsigned char>(head.front()))) head.erase(head.begin());
    if (!head.empty() && (head.front() == 'v' || head.front() == 'V')) head.erase(head.begin());
    std::string out;
    for (const auto& tok : splitDots(head)) {
        if (allDigits(tok)) {
            if (!out.empty()) out += ".";
            out += tok;
            continue;
        }
        if (auto fix = parseFix(tok)) {
            if (!out.empty()) out += ".";
            out += "fix" + std::to_string(*fix);
        }
        break;
    }
    return out.empty() ? raw : out;
}

std::string tagOf(const std::string& raw) {
    const std::string v = cleanVersion(raw);
    if (!v.empty() && (v.front() == 'v' || v.front() == 'V')) return v;
    return "v" + v;
}

std::string androidAssetName(const std::string& version, bool lite) {
    return std::string("Android-") + (lite ? "lite" : "full") + "-" + cleanVersion(version) + ".apk";
}

std::vector<std::string> desktopAssetNames(const std::string& version, const std::string& os,
                                           const std::string& arch) {
    const std::string v = cleanVersion(version);
    const std::string o = lower(os);
    const std::string a = lower(arch);
    const bool isArm = a.find("arm") != std::string::npos || a.find("aarch64") != std::string::npos;
    const bool isWin = o.find("win") != std::string::npos;
    if (isWin) {
        return {"Windows-universal-" + v + ".exe",
                std::string("Windows-") + (isArm ? "arm64" : "x64") + "-" + v + ".zip"};
    }
    return {"Linux-universal-" + v + ".tar.gz",
            std::string("Linux-") + (isArm ? "aarch64" : "x86_64") + "-" + v + ".tar.gz"};
}

std::string assetUrl(const std::string& tag, const std::string& assetName, const std::string& repo) {
    return "https://github.com/" + repo + "/releases/download/" + tagOf(tag) + "/" + assetName;
}

}  // namespace jmnext::core
