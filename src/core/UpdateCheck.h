// 版本比较与"该下哪个包" —— 语义对照主项目 shared/.../UpdateCheck.kt，
// 并**包含主项目 2026-10-05 新定的 fix(n) 规则**（那边目前只落在文档里，这里先实现）。
//
// 两条规则最容易踩：
//  1. 归一化：去掉 v 前缀、忽略变体后缀（.lite/.debug）—— 主项目 versionName 带后缀而 tag 是干净的，
//     直接字符串比较会永远判定"有新版本"；
//  2. **fix(n) 是版本的一部分**：`2.1.7.fix1` 必须当成 [2,1,7,1]，
//     否则它与 `2.1.7` 相等 → 修复版会被判定成"已是最新"而**永远推不出去**。
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace jmnext::core {

inline constexpr char RELEASE_REPO[] = "moyingyilang/JMNeXt";

/// 版本号 → 数字段。解析不出返回 nullopt。
/// `v2.1.7` → [2,1,7]；`2.1.7.lite` → [2,1,7]；`2.1.7.fix1.lite` → [2,1,7,1]
std::optional<std::vector<int>> versionParts(const std::string& raw);
/// 远端是否比本地新（缺的段按 0 计算）
bool isNewer(const std::string& latest, const std::string& current);

/// 用于拼附件名的版本串：保留 fixN，丢弃 .lite/.debug 这类变体后缀
std::string cleanVersion(const std::string& raw);
/// 发布 tag 的规范形式（附件直链必须带 v 前缀）
std::string tagOf(const std::string& raw);

/// Android 该下哪个包（发布脚本的命名：Android-full-<版本>.apk / Android-lite-<版本>.apk）
std::string androidAssetName(const std::string& version, bool lite);
/// 桌面端候选附件名（**统一包排第一**：内含多架构运行时，用户不用分辨）
std::vector<std::string> desktopAssetNames(const std::string& version, const std::string& os,
                                          const std::string& arch);
/// release 附件直链
std::string assetUrl(const std::string& tag, const std::string& assetName,
                     const std::string& repo = RELEASE_REPO);

}  // namespace jmnext::core
