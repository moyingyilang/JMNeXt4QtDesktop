// 阅读进度的存取（对应主项目的 ReadProgressStore）。
//
// 为什么放核心层：格式与规则是纯逻辑，放这里可以在任何环境跑测试；界面只调用它。
// 存储格式刻意用**三行文本**（aid / 章节 id / 页码），便于人工查看与排错：
//   <aid>
//   <chapterId>
//   <page>
// 任何一行缺失或不是数字都视为**没有进度**（返回 nullopt），而不是猜。
#pragma once
#include <optional>
#include <string>

namespace jmnext::core {

struct ReadProgress {
    std::string aid;
    std::string chapterId;
    int page = 0;                 // 0 起
};

/// 写入进度文件（覆盖）。返回是否成功。
bool saveReadProgress(const std::string& path, const ReadProgress& progress);

/// 读回进度；文件不存在/内容不合法时返回 nullopt（不猜、不抛）。
std::optional<ReadProgress> loadReadProgress(const std::string& path);

/// 把进度序列化成三行文本（测试与排错用）
std::string serializeReadProgress(const ReadProgress& progress);

}  // namespace jmnext::core
