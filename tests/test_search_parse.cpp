// 搜索响应解析的离线测试（罐装 JSON，形状取自真实响应）。
#include "core/JmCore.h"

#include <cstdio>
#include <string>

using namespace jmnext::core;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}

int main() {
    const std::string json =
        R"({"search_query":"test","search_type":"site","y":0,"m":0,"total":45,"content":[)"
        R"({"id":"1471982","author":"kuzlin","description":null,"name":"摸鱼图+颜色测试",)"
        R"("image":"","category":{"id":"1","title":"同人"},"category_sub":{"id":"1","title":"同人"},)"
        R"("liked":false,"is_favorite":false,"update_at":1789185061,"adddate":"2026-09-12"},)"
        R"({"id":"1464581","author":"VN Simp","description":null,"name":"第二条","image":"",)"
        R"("category":{"id":"2","title":"單本"},"category_sub":{"id":null,"title":null},)"
        R"("liked":false,"is_favorite":false,"update_at":1789185060,"adddate":"2026-09-11"}]})";

    auto page = parseSearchPage(json);
    check(page.has_value(), "能解析");
    if (page) {
        check(page->total == 45, "total 取到 45");
        check(page->items.size() == 2, "两条条目");
        if (page->items.size() == 2) {
            check(page->items[0].id == "1471982", "第一条 id");
            check(page->items[0].name == "摸鱼图+颜色测试", "第一条名称");
            check(page->items[0].categoryTitle == "同人", "第一条分类");
            check(page->items[1].id == "1464581", "第二条 id");
            check(page->items[1].categoryTitle == "單本", "第二条分类");
        }
    }

    // 空结果（真实见过：参数名错时服务端返回 total=0 且 content 为空）
    auto empty = parseSearchPage(R"({"search_query":"","search_type":"site","total":0,"content":[]})");
    check(empty.has_value() && empty->items.empty() && empty->total == 0, "空结果可解析且为 0 条");

    // 缺 content 字段 -> 明确失败，不猜
    check(!parseSearchPage(R"({"total":3})").has_value(), "缺 content 返回 nullopt");

    if (failures == 0) std::printf("全部通过：搜索响应解析（条目字段、total、空结果、缺字段）\n");
    return failures == 0 ? 0 : 1;
}
