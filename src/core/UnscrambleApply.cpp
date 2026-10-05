#include "core/UnscrambleApply.h"

#include "core/JmCrypto.h"

#include <vector>
#include <string>
#include <algorithm>

namespace jmnext::core {

std::vector<uint32_t> applyBands(const std::vector<uint32_t>& src, int width, int height,
                                 const std::vector<Band>& bands) {
    if (width <= 0 || height <= 0 ||
        src.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {
        return {};
    }
    if (bands.empty()) return src;

    std::vector<uint32_t> out(src.size());
    for (const auto& b : bands) {
        if (b.dstY < 0 || b.dstY + b.height > height) continue;
        if (b.srcY < 0 || b.srcY + b.height > height) continue;
        const auto* from = src.data() + static_cast<std::size_t>(b.srcY) * width;
        auto* to = out.data() + static_cast<std::size_t>(b.dstY) * width;
        std::copy(from, from + static_cast<std::size_t>(b.height) * width, to);
    }
    return out;
}

std::vector<uint32_t> unscramblePage(const std::vector<uint32_t>& src, int width, int height,
                                     int aid, const std::string& page) {
    const int num = sliceCount(aid, page);
    return applyBands(src, width, height, bands(width, height, num));
}

}  // namespace jmnext::core
