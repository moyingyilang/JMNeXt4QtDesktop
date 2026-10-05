// 漫画图片切片还原的几何计算 —— 自己重写版。
//
// 规格来源：JMNeXt（主项目，我们自己的代码）shared/src/main/kotlin/com/jmnext/data/image/ImageUnscramble.kt
// 几何关系与该文件、以及更上游的 utils/Function.js 保持一致（变量名对齐便于对账）。
//
// 这里**只做几何**：输入宽高与份数，输出"从源图哪一段搬到目标图哪一段"的条带列表。
// 与位图无关 —— 这样两端（Android/桌面）能用同一份计算，且能直接用数字写测试。
#pragma once
#include <cstdint>
#include <vector>

namespace jmnext::core {

/// 一条带：源图起始行、目标图起始行、高度（单位都是像素行）。
struct Band {
    int srcY;
    int dstY;
    int height;
    friend bool operator==(const Band& a, const Band& b) {
        return a.srcY == b.srcY && a.dstY == b.dstY && a.height == b.height;
    }
};

/// 计算还原所需的条带；返回空列表表示**不需要还原**（份数 <= 1 或参数不合法）。
///
/// 逐行对照主项目实现：
///   base = height / num; remainder = height % num
///   第 i 条：源起点 y = height - base*(i+1) - remainder，目标起点 py = base*i
///   i == 0 时把 remainder **补进条高**（而不是目标位置），这样所有条带拼起来正好铺满整页
///   再做一次边界裁剪（srcTop/usable/room），任何一条为空就跳过
std::vector<Band> bands(int width, int height, int num);

}  // namespace jmnext::core
