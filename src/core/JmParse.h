// 三个真实接口的响应解析（字段名已用**真实网络**确认，见 README 的"真实数据验证情况"）。
//
//   latest     顶层**数组**：{id,name,author,image,category{id,title},category_sub{id,title},...}
//   album      {id,name,author,image,tags:[...],series:[{id,name,sort}],series_id,images,...}
//   comic_read {id,scramble_id,name,total_page,images:[{page,image}]}，图片 URL 在 JSON 里被 \/ 转义
//
// 核心层刻意不依赖 Qt，所以这里自己写一个**面向这三个形状**的解析器（不是通用 JSON 库）：
// 够用、可读、且完全由测试覆盖。遇到不认识的字段一律跳过，不猜。
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace jmnext::core {

/// 列表项（latest / 搜索等列表接口的真实字段）
struct ListEntry {
    std::string id;
    std::string name;
    std::string author;
    std::string image;
    std::string categoryTitle;
    std::string categorySubTitle;
    std::string updateAt;          // update_at：作为封面 URL 的版本号（让缓存失效）
};

struct SeriesEntry {          // 章节（album.series）
    std::string id;
    std::string name;
    int sort = 0;
};

struct AlbumInfo {
    std::string id;
    std::string name;
    std::string author;
    std::vector<std::string> tags;
    std::vector<SeriesEntry> series;
};

struct PageImage {            // 一页
    int page = 0;
    std::string url;
};

struct ChapterImages {
    std::string id;
    int scrambleId = 0;
    std::string name;
    int totalPage = 0;
    std::vector<PageImage> images;
};

std::optional<std::vector<ListEntry>> parseLatestList(const std::string& json);
std::optional<AlbumInfo> parseAlbum(const std::string& json);
std::optional<ChapterImages> parseChapterImages(const std::string& json);

/// 搜索结果页（search 接口）
struct SearchPage {
    int total = 0;                  // 服务端报的总数（可能被截断/给上限值，仅作参考）
    std::vector<ListEntry> items;   // content 数组，条目字段与首页列表相同
};

/// 解析 search 响应：{"search_query":..,"total":N,"content":[ ... ]}
/// 实现上复用 parseLatestList（它找第一个 '[' 再配对括号），把 content 的数组切片交给它。
std::optional<SearchPage> parseSearchPage(const std::string& json);

/// JSON 字符串里的转义还原（至少处理 \/ \" \\ \n \t \r \uXXXX 的常见情形）
std::string unescapeJson(const std::string& raw);

/// 该图是否需要还原（真实接口给了 scramble_id；.gif 一律不需要）
bool needsUnscrambleFor(int aid, int scrambleId, const std::string& url);

}  // namespace jmnext::core
