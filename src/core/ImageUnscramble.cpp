#include "core/ImageUnscramble.h"

#include <vector>
#include <algorithm>

namespace jmnext::core {

std::vector<Band> bands(int width, int height, int num) {
    if (num <= 1 || height < num || width <= 0 || height <= 0) return {};

    const int base = height / num;
    const int remainder = height % num;
    std::vector<Band> out;
    out.reserve(static_cast<std::size_t>(num));

    for (int i = 0; i < num; ++i) {
        int copyH = base;
        int py = base * i;
        const int y = height - base * (i + 1) - remainder;
        if (i == 0) {
            copyH += remainder;
        } else {
            py += remainder;
        }
        if (copyH <= 0) continue;

        const int srcTop = std::max(y, 0);
        const int srcBottom = std::min(y + copyH, height);
        int usable = srcBottom - srcTop;
        if (usable <= 0) continue;

        const int room = height - py;
        if (usable > room) usable = room;
        if (usable <= 0) continue;

        out.push_back(Band{srcTop, py, usable});
    }
    return out;
}

}  // namespace jmnext::core
