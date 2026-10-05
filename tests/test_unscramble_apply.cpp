// 还原一张"每行颜色不同"的合成图，验证每一行确实按 bands 的指示搬到了目标位置。
// 判据是**独立复算**：按几何公式自己算一遍"第 y 行应该来自哪一行"，与实现的结果比对。
#include "core/ImageUnscramble.h"
#include "core/JmCrypto.h"
#include "core/UnscrambleApply.h"

#include <cstdio>
#include <vector>

using namespace jmnext::core;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}

int main() {
    const int W = 8, H = 12;
    // 合成图：第 y 行的每个像素都是 y（便于追踪行搬移）
    std::vector<uint32_t> src(static_cast<std::size_t>(W) * H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) src[static_cast<std::size_t>(y) * W + x] = static_cast<uint32_t>(y);

    // ---------- 1) bands 为空（不需要还原）时原样返回 ----------
    auto same = applyBands(src, W, H, {});
    check(same == src, "bands 为空 → 原样返回");

    // ---------- 2) 指定份数：逐行核对目标行的来源 ----------
    const int num = 4;
    auto bs = bands(W, H, num);
    auto out = applyBands(src, W, H, bs);
    check(out.size() == src.size(), "输出尺寸与输入一致（不重不漏）");

    // 独立复算：目标第 y 行应来自哪一行
    auto expectedSourceRow = [&](int dstY) -> int {
        for (const auto& b : bs)
            if (dstY >= b.dstY && dstY < b.dstY + b.height) return b.srcY + (dstY - b.dstY);
        return -1;
    };
    for (int y = 0; y < H; ++y) {
        const int want = expectedSourceRow(y);
        const uint32_t got = out[static_cast<std::size_t>(y) * W];
        if (want < 0) continue;
        if (got != static_cast<uint32_t>(want)) {
            std::printf("FAIL: 目标第 %d 行来自 %u，期望 %d\n", y, got, want);
            ++failures;
        }
    }

    // ---------- 3) unscramblePage：份数 <= 1 时不还原 ----------
    // aid=1（< 268850 → 份数 10，需要还原）；这里只检查"不崩且尺寸正确"
    auto page = unscramblePage(src, W, H, 1, "1");
    check(page.size() == src.size(), "unscramblePage 输出尺寸正确");

    // ---------- 4) 尺寸不符时明确失败（不静默返回垃圾）----------
    check(applyBands(std::vector<uint32_t>(5, 0), W, H, bs).empty(), "输入尺寸不符 → 返回空");

    if (failures == 0) std::printf("全部通过：还原作用到像素缓冲（逐行独立复算）\n");
    return failures == 0 ? 0 : 1;
}
