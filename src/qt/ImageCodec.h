// 图片读写：用 Qt 的 QImage（自带 PNG/JPEG/PPM 等解码器，省一层依赖）。
// 与核心层的关系：这里只负责"文件 ↔ ARGB 缓冲"的搬运，还原算法仍在 core（已被测试覆盖）。
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace jmnext::qt {

struct Image {
    int width = 0;
    int height = 0;
    std::vector<uint32_t> pixels;   // 0xAARRGGBB，行优先
    std::string detectedFormat;     // 让调用方能证明"确实是 Qt 解码出来的"
};

std::optional<Image> loadImage(const std::string& path);
/// 供界面调用的同义入口（避免与 QWidget::loadImage 之类的名字混淆）
std::optional<Image> loadImageFile(const std::string& path);
bool savePpm(const std::string& path, const Image& image);

}  // namespace jmnext::qt
