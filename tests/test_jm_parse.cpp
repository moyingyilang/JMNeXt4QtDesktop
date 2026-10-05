// 三个真实接口的解析测试。fixture 严格照**真实网络确认过的形状**写（字段名一致），
// 并把真实响应里的 \/ 转义原样带上，确保解析后 URL 可用。
#include "core/JmCore.h"

#include <cstdio>
#include <string>

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

// 真实 latest 的形状（顶层数组；category/category_sub 是嵌套对象；image 在列表里为空）
static const char* kLatest = R"([
 {"id":"209827","author":"某作者","name":"某作品","image":"",
  "category":{"id":"2","title":"單本"},"category_sub":{"id":null,"title":null},
  "liked":false,"is_favorite":false,"update_at":1791191078},
 {"id":"1479408","author":"另一位","name":"另一部","image":"",
  "category":{"id":"1","title":"同人"},"category_sub":{"id":"3","title":"短篇"},
  "liked":true,"is_favorite":false,"update_at":1791187583}
])";

// 真实 album 的形状（含 series 章节表；tags 为字符串数组）
static const char* kAlbum = R"({
 "id":"209827","name":"某作品","author":"某作者","image":"",
 "tags":["純愛","日常"],
 "series":[{"id":"209827","name":"","sort":"1"},{"id":"209828","name":"11-20","sort":"2"}],
 "series_id":"209827","total_page":285,"likes":"12"
})";

// 真实 comic_read 的形状（scramble_id 是字符串；image 里的 / 被转义成 \/）
static const char* kRead = R"({
 "id":209827,"scramble_id":"220980","name":"某作品","total_page":2,
 "images":[{"page":1,"image":"https:\/\/cdn.example.com\/media\/209827\/00001.webp?t=1791191078"},
           {"page":2,"image":"https:\/\/cdn.example.com\/media\/209827\/00002.webp?t=1791191078"}]
})";

int main() {
    // ---------- latest ----------
    {
        auto list = parseLatestList(kLatest);
        check(list.has_value(), "解析 latest 成功");
        if (list) {
            check(list->size() == 2, "两条列表项");
            eqStr((*list)[0].id, "209827", "id");
            eqStr((*list)[0].name, "某作品", "name");
            eqStr((*list)[0].author, "某作者", "author");
            eqStr((*list)[0].categoryTitle, "單本", "category.title");
            eqStr((*list)[0].categorySubTitle, "", "category_sub.title 为 null 时留空");
            eqStr((*list)[1].categorySubTitle, "短篇", "category_sub.title 有值时取到");
        }
    }

    // ---------- album ----------
    {
        auto a = parseAlbum(kAlbum);
        check(a.has_value(), "解析 album 成功");
        if (a) {
            eqStr(a->id, "209827", "album.id");
            check(a->tags.size() == 2, "两个标签");
            eqStr(a->tags[1], "日常", "标签内容");
            check(a->series.size() == 2, "两个章节");
            if (a->series.size() == 2) {
                eqStr(a->series[1].id, "209828", "章节 id");
                eqStr(a->series[1].name, "11-20", "章节名");
                check(a->series[1].sort == 2, "章节 sort 为数字");
            }
        }
    }

    // ---------- comic_read ----------
    {
        auto c = parseChapterImages(kRead);
        check(c.has_value(), "解析 comic_read 成功");
        if (c) {
            eqStr(c->id, "209827", "章节 id");
            check(c->scrambleId == 220980, "scramble_id 从字符串解出数字");
            check(c->totalPage == 2, "total_page");
            check(c->images.size() == 2, "两页");
            if (c->images.size() == 2) {
                eqStr(c->images[0].url,
                      "https://cdn.example.com/media/209827/00001.webp?t=1791191078",
                      "图片 URL 的 \\/ 转义被还原");
                check(c->images[1].page == 2, "第二页的页码");
            }
        }
    }

    // ---------- needsUnscramble：真实语义 ----------
    {
        check(!needsUnscrambleFor(209827, 220980, "https://x/a.webp"), "aid 小于 scramble_id 不还原（真实作品即如此）");
        check(needsUnscrambleFor(300000, 220980, "https://x/a.webp"), "aid 大于等于 scramble_id 需要还原");
        check(!needsUnscrambleFor(300000, 220980, "https://x/a.gif"), ".gif 一律不还原");
    }

    // ---------- 异常输入不崩、明确返回 ----------
    {
        check(!parseLatestList("not json").has_value(), "非 JSON 的 latest → 空");
        check(!parseAlbum("[]").has_value(), "数组当 album → 空");
        check(!parseChapterImages("{}").has_value(), "空对象 → 空（没有 id 也没有 images）");
        eqStr(unescapeJson(R"(a\/b\"c)"), "a/b\"c", "转义还原");
    }

    if (failures == 0) std::printf("全部通过：三个真实接口的解析（含 \\/ 转义与真实字段名）\n");
    return failures == 0 ? 0 : 1;
}
