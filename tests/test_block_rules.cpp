// 屏蔽规则测试。重点盯住"关键词是子串、标签与分类是精确"这个最容易写错的区分。
#include "core/JmCore.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace jmnext::core;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static ListItem item(const char* name, const char* author, const char* cat = "", const char* sub = "") {
    return ListItem{name, author, cat, sub};
}

int main() {
    // ---------- 空规则：什么都不隐藏 ----------
    {
        BlockRules r;
        check(r.isEmpty(), "空规则 isEmpty");
        check(!r.hides(item("任何作品", "任何作者")), "空规则不隐藏");
        check(!r.matchesTags({"NTR"}), "空规则不命中标签");
    }

    // ---------- 关键词：**子串**匹配，大小写无关 ----------
    {
        BlockRules r;
        r.words = {"ntr"};
        check(r.hides(item("我的NTR作品", "作者")), "关键词命中作品名（子串、大小写无关）");
        check(r.hides(item("作品", "NTR作者")), "关键词命中作者（子串）");
        check(r.hides(item("xxNTRxx", "作者")), "关键词是子串：夹在中间也算");
        check(!r.hides(item("纯爱作品", "作者")), "不含关键词则不隐藏");
        check(r.hitsAuthor({"某Ntr作者"}), "hitsAuthor 也是子串且大小写无关");
        check(!r.hitsAuthor({"纯爱作者"}), "作者不含关键词");
    }

    // ---------- 分类：**精确**匹配（与关键词的差别就在这里）----------
    {
        BlockRules r;
        r.categories = {"同人"};
        check(r.hides(item("作品", "作者", "同人", "")), "分类精确命中");
        check(r.hides(item("作品", "作者", "", "同人")), "子分类精确命中");
        check(!r.hides(item("作品", "作者", "同人志", "")), "分类是精确匹配：『同人志』不该被『同人』连坐");
        check(r.hides(item("作品", "作者", "同人", "")), "大小写无关（对中文为恒等）");
        check(!r.hides(item("作品", "作者", "", "")), "空分类不参与匹配");
    }

    // ---------- 标签：**精确**匹配、大小写无关 ----------
    {
        BlockRules r;
        r.tags = {"NTR", "巨乳"};
        check(r.matchesTags({"纯爱", "ntr"}), "标签大小写无关命中");
        check(!r.matchesTags({"NTR作品"}), "标签是精确匹配：『NTR作品』不该被『NTR』连坐");
        check(!r.matchesTags({""}), "空标签不命中");
        check(r.matchesTags({"巨乳"}), "中文标签精确命中");

        // 两者返回值不同：一个回显规则写法，一个回显作品写法
        auto byRule = r.hitRuleTags({"ntr"});
        auto byItem = r.hitsTags({"ntr"});
        check(byRule.size() == 1 && byRule[0] == "NTR", "hitRuleTags 回显**规则**的写法（界面说「是你的 NTR 挡的」）");
        check(byItem.size() == 1 && byItem[0] == "ntr", "hitsTags 回显**作品**的写法（详情页要拿它去取消屏蔽）");
    }

    // ---------- 三份名单共同生效，且关键词优先 ----------
    {
        BlockRules r;
        r.words = {"作者名"};
        r.tags = {"某标签"};
        r.categories = {"某分类"};
        check(!r.isEmpty(), "有规则时 isEmpty 为假");
        check(r.hides(item("作品", "作者名", "别的分类")), "任一份名单命中即隐藏（关键词）");
        check(r.hides(item("作品", "作者", "某分类")), "任一份名单命中即隐藏（分类）");
        check(!r.hides(item("作品", "作者", "别的分类")), "都不命中则不隐藏");
    }

    if (failures == 0) std::printf("全部通过：屏蔽规则（子串与精确之分、大小写无关、两种回显）\n");
    return failures == 0 ? 0 : 1;
}
