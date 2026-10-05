// 切片几何的测试。判据来自主项目注释里点明的那条"最容易出错的性质"：
// **条带不重不漏地铺满整页**。另加若干可手算的具体用例与边界条件。
#include "core/ImageUnscramble.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

using jmnext::core::Band;
using jmnext::core::bands;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

/// 目标区间必须两两不相交、且都在 [0, height) 内；源区间同理。
static void assert_well_formed(const std::vector<Band>& bs, int height, const char* tag) {
    std::vector<Band> sorted = bs;
    std::sort(sorted.begin(), sorted.end(),
              [](const Band& a, const Band& b) { return a.dstY < b.dstY; });
    int expectNext = 0;
    for (const auto& b : sorted) {
        check(b.height > 0, (std::string(tag) + ": height > 0").c_str());
        check(b.dstY == expectNext, (std::string(tag) + ": 目标区间连续无缝（不重不漏）").c_str());
        check(b.srcY >= 0 && b.srcY + b.height <= height,
              (std::string(tag) + ": 源区间在界内").c_str());
        check(b.dstY >= 0 && b.dstY + b.height <= height,
              (std::string(tag) + ": 目标区间在界内").c_str());
        expectNext = b.dstY + b.height;
    }
    check(expectNext == height, (std::string(tag) + ": 正好铺满整页").c_str());
}

int main() {
    // 1) 边界：不需要还原的情形一律返回空
    check(bands(100, 100, 1).empty(), "num==1 返回空");
    check(bands(100, 100, 0).empty(), "num==0 返回空");
    check(bands(100, 100, -3).empty(), "num<0 返回空");
    check(bands(100, 100, 200).empty(), "份数大于页高 返回空");
    check(bands(0, 100, 3).empty(), "宽度为 0 返回空");
    check(bands(100, 0, 3).empty(), "高度为 0 返回空");

    // 2) 可手算的具体用例：height=10, num=3 → base=3, remainder=1
    //    i=0: src[6,10) → dst[0,4)   i=1: src[3,6) → dst[4,7)   i=2: src[0,3) → dst[7,10)
    {
        auto bs = bands(50, 10, 3);
        check(bs.size() == 3, "height=10 num=3 → 3 条带");
        if (bs.size() == 3) {
            check(bs[0] == Band{6, 0, 4}, "第 0 条 = src6 dst0 h4（remainder 补进第 0 条高度）");
            check(bs[1] == Band{3, 4, 3}, "第 1 条 = src3 dst4 h3");
            check(bs[2] == Band{0, 7, 3}, "第 2 条 = src0 dst7 h3");
        }
        assert_well_formed(bs, 10, "h10n3");
    }

    // 3) 能整除的情形（remainder=0）：height=12, num=4 → base=3
    {
        auto bs = bands(50, 12, 4);
        assert_well_formed(bs, 12, "h12n4");
    }

    // 4) 不能被整除、且份数较多：把"不重不漏"这条性质大量抽查
    for (int h = 2; h <= 400; ++h) {
        for (int n = 2; n <= std::min(h, 40); ++n) {
            auto bs = bands(64, h, n);
            if (bs.empty()) continue;
            assert_well_formed(bs, h, "sweep");
            if (failures > 5) break;  // 只报前几个，避免刷屏
        }
        if (failures > 5) break;
    }

    if (failures == 0) std::printf("全部通过：切片几何与边界条件\n");
    return failures == 0 ? 0 : 1;
}
