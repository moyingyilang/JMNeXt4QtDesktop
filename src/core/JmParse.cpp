#include "core/JmParse.h"

#include "core/JmCrypto.h"

#include <cctype>
#include <cstdlib>

namespace jmnext::core {
namespace {

std::vector<std::string> stringArrayField(const std::string& obj, const std::string& key);
std::string scalarField(const std::string& obj, const std::string& key);

/// 跳过空白
std::size_t skipWs(const std::string& s, std::size_t i) {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    return i;
}

/// 从引号开始读一个字面串（含转义处理），返回值与结束位置（指向收尾引号之后）
bool readString(const std::string& s, std::size_t i, std::string& out, std::size_t& end) {
    i = skipWs(s, i);
    if (i >= s.size() || s[i] != '"') return false;
    ++i;
    std::string raw;
    while (i < s.size()) {
        const char c = s[i];
        if (c == '\\' && i + 1 < s.size()) { raw.push_back(c); raw.push_back(s[i + 1]); i += 2; continue; }
        if (c == '"') { out = unescapeJson(raw); end = i + 1; return true; }
        raw.push_back(c);
        ++i;
    }
    return false;
}

/// 找 "key" 后面第一个 ':'，再返回其后的位置（找不到返回 npos）
std::size_t valuePosAfterKey(const std::string& s, const std::string& key, std::size_t from = 0) {
    const std::string needle = "\"" + key + "\"";
    const auto at = s.find(needle, from);
    if (at == std::string::npos) return std::string::npos;
    const auto colon = s.find(':', at + needle.size());
    if (colon == std::string::npos) return std::string::npos;
    return skipWs(s, colon + 1);
}

/// 在 pos（指向 '[' 或 '{'）之后找匹配的收尾符号，返回其位置（npos 表示不配平）
std::size_t matchingBracket(const std::string& s, std::size_t pos) {
    if (pos >= s.size()) return std::string::npos;
    const char open = s[pos];
    const char close = (open == '[') ? ']' : '}';
    int depth = 0;
    bool inStr = false;
    for (std::size_t i = pos; i < s.size(); ++i) {
        const char c = s[i];
        if (inStr) {
            if (c == '\\') { ++i; continue; }
            if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; continue; }
        if (c == open) ++depth;
        else if (c == close) { if (--depth == 0) return i; }
    }
    return std::string::npos;
}

/// 把数组/对象切成顶层片段（只按顶层逗号切，字符串内的逗号不算）
std::vector<std::string> splitTopLevel(const std::string& body) {
    std::vector<std::string> out;
    std::size_t start = 0;
    int depth = 0;
    bool inStr = false;
    for (std::size_t i = 0; i < body.size(); ++i) {
        const char c = body[i];
        if (inStr) {
            if (c == '\\') { ++i; continue; }
            if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; continue; }
        if (c == '[' || c == '{') ++depth;
        else if (c == ']' || c == '}') --depth;
        else if (c == ',' && depth == 0) { out.push_back(body.substr(start, i - start)); start = i + 1; }
    }
    if (start < body.size()) out.push_back(body.substr(start));
    return out;
}

std::string stringField(const std::string& obj, const std::string& key) {
    const auto pos = valuePosAfterKey(obj, key);
    if (pos == std::string::npos) return {};
    std::string out;
    std::size_t end = 0;
    if (!readString(obj, pos, out, end)) return {};
    return out;
}

/// 读标量字段：带引号的字符串与裸数字都吃（真实接口两种都有：
/// comic_read 的 id 是 209827 裸数字，而 scramble_id 是 "220980" 字符串）
/// 详情里的 author 可能是字符串，也可能是**字符串数组**（真实响应是数组，主项目的 hitsAuthor
/// 正是遍历列表），这里统一拼成一个字符串。
std::string authorField(const std::string& obj) {
    const auto pos = valuePosAfterKey(obj, "author");
    if (pos == std::string::npos) return {};
    if (obj[pos] == '[') {
        auto arr = stringArrayField(obj, "author");
        std::string out;
        for (const auto& a : arr) { if (!out.empty()) out += " / "; out += a; }
        return out;
    }
    return scalarField(obj, "author");
}

std::string scalarField(const std::string& obj, const std::string& key) {
    const auto pos = valuePosAfterKey(obj, key);
    if (pos == std::string::npos) return {};
    if (obj[pos] == '"') {
        std::string out;
        std::size_t end = 0;
        return readString(obj, pos, out, end) ? out : std::string{};
    }
    std::size_t i = pos;
    while (i < obj.size() && obj[i] != ',' && obj[i] != '}' && obj[i] != ']') ++i;
    std::string v = obj.substr(pos, i - pos);
    while (!v.empty() && std::isspace(static_cast<unsigned char>(v.back()))) v.pop_back();
    return v;
}

int intField(const std::string& obj, const std::string& key) {
    const auto pos = valuePosAfterKey(obj, key);
    if (pos == std::string::npos) return 0;
    if (obj[pos] == '"') {                       // 真实接口里数字有时是字符串
        std::string v; std::size_t end = 0;
        if (readString(obj, pos, v, end)) return std::atoi(v.c_str());
        return 0;
    }
    return std::atoi(obj.c_str() + pos);
}

std::vector<std::string> stringArrayField(const std::string& obj, const std::string& key) {
    std::vector<std::string> out;
    const auto pos = valuePosAfterKey(obj, key);
    if (pos == std::string::npos || obj[pos] != '[') return out;
    const auto end = matchingBracket(obj, pos);
    if (end == std::string::npos) return out;
    for (auto& piece : splitTopLevel(obj.substr(pos + 1, end - pos - 1))) {
        std::string v;
        std::size_t e = 0;
        if (readString(piece, 0, v, e) && !v.empty()) out.push_back(v);
    }
    return out;
}

/// 取一个对象数组字段（例如 series / images）
std::vector<std::string> objectArrayField(const std::string& obj, const std::string& key) {
    std::vector<std::string> out;
    const auto pos = valuePosAfterKey(obj, key);
    if (pos == std::string::npos || obj[pos] != '[') return out;
    const auto end = matchingBracket(obj, pos);
    if (end == std::string::npos) return out;
    for (auto& piece : splitTopLevel(obj.substr(pos + 1, end - pos - 1))) {
        const auto brace = piece.find('{');
        if (brace != std::string::npos) out.push_back(piece.substr(brace));
    }
    return out;
}

}  // namespace

std::string unescapeJson(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] != '\\' || i + 1 >= raw.size()) { out.push_back(raw[i]); continue; }
        const char n = raw[++i];
        switch (n) {
            case '/': out.push_back('/'); break;      // 真实响应里大量出现 \/
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case 'n': out.push_back('\n'); break;
            case 't': out.push_back('\t'); break;
            case 'r': out.push_back('\r'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'u': {                                 // \uXXXX → 仅处理 ASCII，其余丢弃占位
                if (i + 4 < raw.size()) {
                    const std::string hex = raw.substr(i + 1, 4);
                    const long cp = std::strtol(hex.c_str(), nullptr, 16);
                    if (cp > 0 && cp < 0x80) out.push_back(static_cast<char>(cp));
                    else out.append("?");               // 非 ASCII 码点：不静默出错，留可见占位
                    i += 4;
                }
                break;
            }
            default: out.push_back(n); break;
        }
    }
    return out;
}

std::optional<std::vector<ListEntry>> parseLatestList(const std::string& json) {
    const auto start = json.find('[');
    if (start == std::string::npos) return std::nullopt;
    const auto end = matchingBracket(json, start);
    if (end == std::string::npos) return std::nullopt;

    std::vector<ListEntry> out;
    for (auto& obj : splitTopLevel(json.substr(start + 1, end - start - 1))) {
        if (obj.find('{') == std::string::npos) continue;
        ListEntry e;
        e.id = scalarField(obj, "id");
        e.name = stringField(obj, "name");
        e.author = stringField(obj, "author");
        e.image = stringField(obj, "image");
        // category / category_sub 是嵌套对象
        if (const auto cp = valuePosAfterKey(obj, "category"); cp != std::string::npos && obj[cp] == '{') {
            const auto ce = matchingBracket(obj, cp);
            if (ce != std::string::npos) e.categoryTitle = stringField(obj.substr(cp, ce - cp + 1), "title");
        }
        if (const auto sp = valuePosAfterKey(obj, "category_sub"); sp != std::string::npos && obj[sp] == '{') {
            const auto se = matchingBracket(obj, sp);
            if (se != std::string::npos) e.categorySubTitle = stringField(obj.substr(sp, se - sp + 1), "title");
        e.updateAt = scalarField(obj, "update_at");
        }
        if (!e.id.empty()) out.push_back(e);
    }
    return out;
}

std::optional<AlbumInfo> parseAlbum(const std::string& json) {
    if (json.find('{') == std::string::npos) return std::nullopt;
    AlbumInfo a;
    a.id = scalarField(json, "id");
    a.name = stringField(json, "name");
    a.author = authorField(json);      // 真实响应里 author 是数组，别按字符串读
    a.tags = stringArrayField(json, "tags");
    for (auto& obj : objectArrayField(json, "series")) {
        SeriesEntry se;
        se.id = scalarField(obj, "id");
        se.name = stringField(obj, "name");
        se.sort = intField(obj, "sort");
        if (!se.id.empty()) a.series.push_back(se);
    }
    if (a.id.empty() && a.series.empty()) return std::nullopt;
    return a;
}

std::optional<ChapterImages> parseChapterImages(const std::string& json) {
    if (json.find('{') == std::string::npos) return std::nullopt;
    ChapterImages c;
    c.id = scalarField(json, "id");
    c.scrambleId = intField(json, "scramble_id");
    c.name = stringField(json, "name");
    c.totalPage = intField(json, "total_page");
    for (auto& obj : objectArrayField(json, "images")) {
        PageImage p;
        p.page = intField(obj, "page");
        p.url = stringField(obj, "image");
        if (!p.url.empty()) c.images.push_back(p);
    }
    if (c.id.empty() && c.images.empty()) return std::nullopt;
    return c;
}

bool needsUnscrambleFor(int aid, int scrambleId, const std::string& url) {
    return needsUnscramble(url, aid, scrambleId);
}

std::optional<std::vector<ListEntry>> parseLooseList(const std::string& json) {
    std::vector<ListEntry> out;
    std::size_t start = json.find('[');
    while (start != std::string::npos) {
        const auto end = matchingBracket(json, start);
        if (end == std::string::npos) break;
        for (auto& obj : splitTopLevel(json.substr(start + 1, end - start - 1))) {
            if (obj.find('{') == std::string::npos) continue;
            ListEntry e;
            e.id = scalarField(obj, "id");
            if (e.id.empty()) e.id = scalarField(obj, "aid");
            e.name = stringField(obj, "name");
            if (e.name.empty()) e.name = stringField(obj, "work_title");
            if (e.name.empty()) e.name = stringField(obj, "title");
            e.author = stringField(obj, "author");
            if (e.author.empty()) e.author = stringField(obj, "author_name");
            e.image = stringField(obj, "image");
            if (e.image.empty()) e.image = stringField(obj, "work_image");
            e.categoryTitle = stringField(obj, "platform_name");
            if (!e.id.empty() || !e.name.empty()) out.push_back(e);
        }
        if (!out.empty()) return out;
        start = json.find('[', start + 1);   // 本层为空则下钻
    }
    if (out.empty()) return std::nullopt;
    return out;
}

std::optional<std::vector<std::string>> parseHotTags(const std::string& json) {
    const auto start = json.find('[');
    if (start == std::string::npos) return std::nullopt;
    const auto end = matchingBracket(json, start);
    if (end == std::string::npos) return std::nullopt;

    std::vector<std::string> out;
    for (auto& item : splitTopLevel(json.substr(start + 1, end - start - 1))) {
        std::string value;
        std::size_t stop = 0;
        if (readString(item, skipWs(item, 0), value, stop) && !value.empty()) out.push_back(value);
    }
    return out;
}

std::optional<SearchPage> parseSearchPage(const std::string& json) {
    // 找 "content" 后面的数组，交给 parseLatestList（它对"数组切片"同样适用）
    const auto key = json.find("\"content\"");
    if (key == std::string::npos) return std::nullopt;
    const auto start = json.find('[', key);
    if (start == std::string::npos) return std::nullopt;
    const auto end = matchingBracket(json, start);
    if (end == std::string::npos) return std::nullopt;
    auto items = parseLatestList(json.substr(start, end - start + 1));
    if (!items) return std::nullopt;

    SearchPage page;
    page.items = *items;
    if (const auto t = scalarField(json, "total"); !t.empty()) page.total = std::atoi(t.c_str());
    return page;
}
}  // namespace jmnext::core
