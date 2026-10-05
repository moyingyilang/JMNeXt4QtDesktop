// 把切片几何真正作用到像素缓冲上（还原一页图）。
//
// 与平台无关：输入输出都是 ARGB 像素（每像素 32 位，按行优先），平台侧只负责
// "把位图读成缓冲、把缓冲写回位图/文件"。这样桌面与将来可能的其它端共用同一份还原逻辑。
#pragma once
#include "core/ImageUnscramble.h"

#include <string>
#include <cstdint>
#include <vector>

namespace jmnext::core {

/// 按 [bands] 把 [src] 的行搬成还原后的图。src 长度必须是 width*height。
/// bands 为空（不需要还原）时原样返回副本。
std::vector<uint32_t> applyBands(const std::vector<uint32_t>& src, int width, int height,
                                 const std::vector<Band>& bands);

/// 一步到位：按 aid/page 算份数并还原（份数 <= 1 时原样返回）
std::vector<uint32_t> unscramblePage(const std::vector<uint32_t>& src, int width, int height,
                                     int aid, const std::string& page);

}  // namespace jmnext::core
