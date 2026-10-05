// 版本号的**单一来源**。
//
// 命名规则（沿用主项目）：
//   x.x.x        小功能 +001（大功能 +010；只有特大可行性验证才动大版本）
//   x.x.x.fix(n) 已发布版本仍有待修项时的修复版
//
// 打包脚本 package-linux.sh 会从这里读版本，避免脚本与代码对不上。
// v0.1.0 已发布；此后的改动（章节列表、界面清理、更新检查等）累加为 0.1.1。
#pragma once

namespace jmnext::core {
inline constexpr char APP_VERSION[] = "0.1.1";
}  // namespace jmnext::core
