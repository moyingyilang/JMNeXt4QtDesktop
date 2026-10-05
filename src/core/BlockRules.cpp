#include "core/BlockRules.h"

#include <cctype>

namespace jmnext::core {
namespace {
std::string asciiLower(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}
bool containsAny(const std::string& haystack, const std::vector<std::string>& needles) {
    for (const auto& n : needles)
        if (!n.empty() && containsIgnoreCase(haystack, n)) return true;
    return false;
}
bool equalsAny(const std::string& value, const std::vector<std::string>& candidates) {
    if (value.empty()) return false;      // 空字段不参与匹配（对应主项目 orEmpty() 的行为）
    for (const auto& c : candidates)
        if (!c.empty() && equalsIgnoreCase(value, c)) return true;
    return false;
}
}  // namespace

bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    return asciiLower(a) == asciiLower(b);
}

bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return false;
    return asciiLower(haystack).find(asciiLower(needle)) != std::string::npos;
}

bool BlockRules::hides(const ListItem& item) const {
    if (isEmpty()) return false;
    if (containsAny(item.name, words) || containsAny(item.author, words)) return true;
    return equalsAny(item.category, categories) || equalsAny(item.categorySub, categories);
}

std::vector<std::string> BlockRules::hitRuleTags(const std::vector<std::string>& values) const {
    std::vector<std::string> out;
    for (const auto& rule : tags) {
        if (rule.empty()) continue;
        for (const auto& v : values) {
            if (equalsIgnoreCase(rule, v)) { out.push_back(rule); break; }   // 回显规则自己的写法
        }
    }
    return out;
}

std::vector<std::string> BlockRules::hitsTags(const std::vector<std::string>& values) const {
    std::vector<std::string> out;
    for (const auto& v : values) {
        for (const auto& rule : tags) {
            if (!rule.empty() && equalsIgnoreCase(rule, v)) { out.push_back(v); break; }  // 回显作品上的写法
        }
    }
    return out;
}

bool BlockRules::matchesTags(const std::vector<std::string>& values) const {
    return !tags.empty() && !hitRuleTags(values).empty();
}

bool BlockRules::hitsAuthor(const std::vector<std::string>& authors) const {
    for (const auto& a : authors)
        if (containsAny(a, words)) return true;
    return false;
}

}  // namespace jmnext::core
