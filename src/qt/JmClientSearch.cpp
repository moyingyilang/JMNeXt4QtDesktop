// 带排序的搜索（单独文件：追加到 JmClient.cpp 末尾会落到命名空间之外）。
// 依据 shared/data/JmRepository.kt 的 search()：参数 search_query / page / o(order) / search_type / y / m。
#include "JmClient.h"

#include "core/JmApi.h"
#include "core/JmPaths.h"

namespace jmnext::qt {

using jmnext::core::parseLatestList;
using jmnext::core::parseLooseList;
using jmnext::core::parseSearchPage;

std::optional<std::vector<jmnext::core::ListEntry>> JmClient::searchOrdered(const std::string& word, int page,
                                                              const std::string& order) {
    if (!bootstrapped_) { lastError_ = "尚未初始化主机"; return std::nullopt; }
    core::JmApi api(session_, http_);
    std::string query = "search_query=" + word + "&page=" + std::to_string(page);
    if (!order.empty()) query += "&o=" + order;   // 空则整个参数省略（与 Kotlin 的 let 语义一致）
    auto r = api.request(core::paths::SEARCH, query);
    if (!r) { lastError_ = api.lastError(); return std::nullopt; }
    lastRaw_ = r->text;
    if (auto parsed = parseSearchPage(r->text)) {
        bool named = false;
        for (const auto& e : parsed->items) if (!e.name.empty()) { named = true; break; }
        if (named || parsed->items.empty()) return parsed->items;
    }
    if (auto loose = parseLooseList(r->text)) return loose;
    if (auto flat = parseLatestList(r->text)) return flat;
    lastError_ = "搜索解析失败";
    return std::nullopt;
}

}  // namespace jmnext::qt
