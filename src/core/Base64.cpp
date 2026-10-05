#include "core/Base64.h"

#include <vector>
#include <string>
#include <array>

namespace jmnext::core {
namespace {
constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
int decodeChar(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}
}  // namespace

std::string sanitizeBase64(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' ||
            c == '/' || c == '=') {
            out.push_back(c);
        }
    }
    return out;
}

std::vector<uint8_t> base64Decode(const std::string& text) {
    const std::string s = sanitizeBase64(text);
    std::vector<uint8_t> out;
    if (s.empty()) return out;
    if (s.size() % 4 != 0) return {};  // 长度不合法一律判失败，不要猜

    int buf[4];
    for (std::size_t i = 0; i < s.size(); i += 4) {
        int pad = 0;
        for (int j = 0; j < 4; ++j) {
            const char c = s[i + static_cast<std::size_t>(j)];
            if (c == '=') {
                buf[j] = 0;
                ++pad;
            } else {
                buf[j] = decodeChar(c);
                if (buf[j] < 0) return {};
                if (pad > 0) return {};  // '=' 之后不能再有数据
            }
        }
        if (pad > 2) return {};
        const uint32_t triple = (static_cast<uint32_t>(buf[0]) << 18) |
                                (static_cast<uint32_t>(buf[1]) << 12) |
                                (static_cast<uint32_t>(buf[2]) << 6) |
                                static_cast<uint32_t>(buf[3]);
        out.push_back(static_cast<uint8_t>((triple >> 16) & 0xFF));
        if (pad < 2) out.push_back(static_cast<uint8_t>((triple >> 8) & 0xFF));
        if (pad < 1) out.push_back(static_cast<uint8_t>(triple & 0xFF));
    }
    return out;
}

std::string base64Encode(const std::vector<uint8_t>& data) {
    std::string out;
    std::size_t i = 0;
    while (i + 2 < data.size()) {
        const uint32_t t = (static_cast<uint32_t>(data[i]) << 16) |
                           (static_cast<uint32_t>(data[i + 1]) << 8) | data[i + 2];
        out.push_back(kAlphabet[(t >> 18) & 63]);
        out.push_back(kAlphabet[(t >> 12) & 63]);
        out.push_back(kAlphabet[(t >> 6) & 63]);
        out.push_back(kAlphabet[t & 63]);
        i += 3;
    }
    const std::size_t rest = data.size() - i;
    if (rest == 1) {
        const uint32_t t = static_cast<uint32_t>(data[i]) << 16;
        out.push_back(kAlphabet[(t >> 18) & 63]);
        out.push_back(kAlphabet[(t >> 12) & 63]);
        out.push_back('=');
        out.push_back('=');
    } else if (rest == 2) {
        const uint32_t t = (static_cast<uint32_t>(data[i]) << 16) |
                           (static_cast<uint32_t>(data[i + 1]) << 8);
        out.push_back(kAlphabet[(t >> 18) & 63]);
        out.push_back(kAlphabet[(t >> 12) & 63]);
        out.push_back(kAlphabet[(t >> 6) & 63]);
        out.push_back('=');
    }
    return out;
}

}  // namespace jmnext::core
