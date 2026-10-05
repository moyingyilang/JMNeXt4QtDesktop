// jmnext4cli —— 无头最小闭环（先把"纯计算"那半跑通，网络与界面随后接）。
//
// 用法：
//   jmnext4cli decrypt   <base64 文本> <秒级时间戳>     # 走 JmCrypto.decryptApiData 的语义
//   jmnext4cli unscramble <in.argb> <w> <h> <aid> <page> <out.ppm>
//
// 为什么先做这两个：它们覆盖"解密 → 算份数 → 还原"这条链路的全部纯计算部分，
// 不依赖网络、账号与显示，可以在任何环境里复现与自查（真实网络请求留到接入 Qt 网络层后）。
#include "core/ImageUnscramble.h"
#include "core/JmCrypto.h"
#include "core/UnscrambleApply.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace jmnext::core;

static std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

static bool writePpm(const std::string& path, const std::vector<uint32_t>& px, int w, int h) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "P6\n" << w << " " << h << "\n255\n";
    for (uint32_t v : px) {
        out.put(static_cast<char>((v >> 16) & 0xFF));
        out.put(static_cast<char>((v >> 8) & 0xFF));
        out.put(static_cast<char>(v & 0xFF));
    }
    return static_cast<bool>(out);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "用法:\n  jmnext4cli decrypt <base64> <秒级时间戳>\n"
                     "  jmnext4cli unscramble <in.argb> <w> <h> <aid> <page> <out.ppm>\n";
        return 2;
    }
    const std::string cmd = argv[1];

    if (cmd == "decrypt" && argc == 4) {
        const std::string b64 = argv[2];
        const auto ts = static_cast<int64_t>(std::strtoll(argv[3], nullptr, 10));
        auto text = decryptApiData(b64, ts);
        if (!text) { std::cerr << "解密失败（两个 seed 都不通）\n"; return 1; }
        std::cout << *text << "\n";
        return 0;
    }

    if (cmd == "unscramble" && argc == 8) {
        const auto raw = readFile(argv[2]);
        const int w = std::atoi(argv[3]);
        const int h = std::atoi(argv[4]);
        const int aid = std::atoi(argv[5]);
        const std::string page = argv[6];
        const std::string outPath = argv[7];
        const std::size_t want = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4;
        if (raw.size() != want) {
            std::cerr << "输入大小不符：期望 " << want << " 字节（w*h*4），实际 " << raw.size() << "\n";
            return 1;
        }
        std::vector<uint32_t> px(raw.size() / 4);
        for (std::size_t i = 0; i < px.size(); ++i) {
            px[i] = (static_cast<uint32_t>(raw[i * 4]) << 24) | (static_cast<uint32_t>(raw[i * 4 + 1]) << 16) |
                    (static_cast<uint32_t>(raw[i * 4 + 2]) << 8) | static_cast<uint32_t>(raw[i * 4 + 3]);
        }
        const int num = sliceCount(aid, page);
        const auto bs = bands(w, h, num);
        const auto out = applyBands(px, w, h, bs);
        if (out.empty()) { std::cerr << "还原失败（尺寸或条带不合法）\n"; return 1; }
        if (!writePpm(outPath, out, w, h)) { std::cerr << "写文件失败：" << outPath << "\n"; return 1; }
        std::cout << "已还原并写出 " << outPath << "（aid=" << aid << " page=" << page
                  << " 份数=" << num << " 条带=" << bs.size() << "）\n";
        return 0;
    }

    std::cerr << "参数不对\n";
    return 2;
}
