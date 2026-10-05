// jmnext4img —— 用 Qt 解码图片并做切片还原（把 core 的算法接到真实图片上）。
//
// 用法：jmnext4img <in.{png,jpg,ppm}> <aid> <page> <out.ppm>
//
// 为什么要它：core 里的还原算法已被测试覆盖，但"真图能否走通"要靠这一步证明 ——
// 解码走 Qt 的 QImageReader（会自动识别格式），还原仍调用那份被测过的 core 代码。
#include "core/ImageUnscramble.h"
#include "core/JmCrypto.h"
#include "core/UnscrambleApply.h"
#include "qt/ImageCodec.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "用法: jmnext4img <in.{png,jpg,ppm}> <aid> <page> <out.ppm>\n";
        return 2;
    }
    const std::string in = argv[1];
    const int aid = std::atoi(argv[2]);
    const std::string page = argv[3];
    const std::string outPath = argv[4];

    auto img = jmnext::qt::loadImage(in);
    if (!img) { std::cerr << "解码失败: " << in << "\n"; return 1; }
    std::cout << "解码成功: " << img->width << "x" << img->height
              << " 格式=" << (img->detectedFormat.empty() ? "?" : img->detectedFormat) << "\n";

    const int num = jmnext::core::sliceCount(aid, page);
    const auto bands = jmnext::core::bands(img->width, img->height, num);
    const auto out = jmnext::core::applyBands(img->pixels, img->width, img->height, bands);
    if (out.empty()) { std::cerr << "还原失败\n"; return 1; }

    jmnext::qt::Image result{img->width, img->height, out, {}};
    if (!jmnext::qt::savePpm(outPath, result)) { std::cerr << "写文件失败: " << outPath << "\n"; return 1; }
    std::cout << "已还原并写出 " << outPath << "（aid=" << aid << " page=" << page
              << " 份数=" << num << " 条带=" << bands.size() << "）\n";
    return 0;
}
