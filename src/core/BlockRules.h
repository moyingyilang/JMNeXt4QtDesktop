// 屏蔽规则 —— 语义逐条对照主项目 shared/.../data/BlockRules.kt（80 行）。
//
// 三份名单（都是本地数据、用户自己维护）：
//   1. words      —— **关键词**：作品名或作者**包含**即隐藏（子串匹配）
//   2. tags       —— **标签**：标签**等于**名单项即屏蔽（精确匹配）
//   3. categories —— **分类**：分类名或子分类名**等于**名单项即隐藏（精确匹配）
//
// **最容易写错的区分**：关键词是**子串**，标签与分类是**精确**匹配。
// 把分类也写成子串，就会出现"屏蔽了『同人』结果『同人志』也被隐藏"这类连坐。
// 匹配一律**大小写无关**（用户想屏蔽 NTR 时不该还要考虑 ntr/Ntr）。
//
// 大小写折叠的范围说明（如实）：本实现只做 **ASCII** 的大小写折叠，核心层刻意不依赖 Qt/ICU。
// 对中文/日文标签，折叠是恒等变换，不影响结果；对 "NTR"/"Ntr" 这类缩写则按预期生效。
// 若将来需要完整的 Unicode 折叠（例如希腊字母、带变音符的拉丁字母），应在 Qt 层做归一化后再传入。
#pragma once
#include <string>
#include <vector>

namespace jmnext::core {

/// 列表项里本实现真正需要的字段（对应主项目的 ListItem 子集）
struct ListItem {
    std::string name;
    std::string author;
    std::string category;       // category.title
    std::string categorySub;    // category_sub.title
};

struct BlockRules {
    std::vector<std::string> words;
    std::vector<std::string> tags;
    std::vector<std::string> categories;

    /// 没有任何规则时，整条过滤路径都可以跳掉
    bool isEmpty() const { return words.empty() && tags.empty() && categories.empty(); }

    /// 这个列表项是否该被隐藏（只用列表项真正带的字段）
    bool hides(const ListItem& item) const;

    /// 这组标签命中了名单里的哪些规则 —— 返回的是**规则自己的写法**（界面要说"是你的『巨乳』这条挡的"）
    std::vector<std::string> hitRuleTags(const std::vector<std::string>& values) const;

    /// 这组标签命中了名单里的哪些项 —— 返回**作品上的写法**（详情页要拿它去"不再屏蔽"）
    std::vector<std::string> hitsTags(const std::vector<std::string>& values) const;

    /// 这组标签是否命中任一标签规则
    bool matchesTags(const std::vector<std::string>& values) const;

    /// 作者是否命中关键词
    bool hitsAuthor(const std::vector<std::string>& authors) const;
};

/// 大小写无关的相等 / 包含（ASCII 折叠）
bool equalsIgnoreCase(const std::string& a, const std::string& b);
bool containsIgnoreCase(const std::string& haystack, const std::string& needle);

}  // namespace jmnext::core
