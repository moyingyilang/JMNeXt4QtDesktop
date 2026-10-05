#include "core/JmCrypto.h"

#include "core/AesEcb.h"
#include "core/Base64.h"
#include "core/Md5.h"

#include <optional>
#include <string>
#include <algorithm>
#include <cctype>

namespace jmnext::core {
namespace {
std::string trim(const std::string& s) {
    const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    auto begin = std::find_if(s.begin(), s.end(), notSpace);
    auto end = std::find_if(s.rbegin(), s.rend(), notSpace).base();
    return begin < end ? std::string(begin, end) : std::string();
}
bool looksLikeJson(const std::string& s) {
    return !s.empty() && (s.front() == '{' || s.front() == '[');
}
}  // namespace

std::string token(int64_t timeSeconds, const std::string& seed) {
    return md5Hex(std::to_string(timeSeconds) + seed);
}

std::string tokenParam(int64_t timeSeconds, const std::string& clientVersion) {
    return std::to_string(timeSeconds) + "," + clientVersion;
}

std::optional<std::string> decryptEcb(const std::string& cipherBase64, const std::string& keyHex) {
    const auto raw = base64Decode(cipherBase64);
    if (raw.empty()) return std::nullopt;
    uint8_t key[32];
    keyFromHexString(keyHex, key);
    const auto plain = aes256EcbDecrypt(raw, key);
    if (plain.empty()) return std::nullopt;
    return std::string(plain.begin(), plain.end());
}

std::optional<std::string> decryptApiData(const std::string& dataBase64, int64_t timeSeconds) {
    std::optional<std::string> messageLike;
    for (const char* seed : {TOKEN_SEED, TOKEN_SEED_ALT}) {
        const std::string key = md5Hex(std::to_string(timeSeconds) + seed);
        auto dec = decryptEcb(dataBase64, key);
        if (!dec) continue;
        const std::string trimmed = trim(*dec);
        if (trimmed.empty()) continue;
        if (looksLikeJson(trimmed)) return trimmed;   // 密钥对了
        if (!messageLike) messageLike = trimmed;      // 记住"人话"，绝不当作失败
    }
    return messageLike;
}

std::optional<std::string> decryptHostPayload(const std::string& encryptedText) {
    auto dec = decryptEcb(encryptedText, md5Hex(HOST_SEED));
    if (!dec) return std::nullopt;
    const std::string trimmed = trim(*dec);
    if (!looksLikeJson(trimmed)) return std::nullopt;
    return trimmed;
}

bool needsUnscramble(const std::string& url, int aid, int scrambleId) {
    if (url.find(".gif") != std::string::npos) return false;
    return aid >= scrambleId;
}

int sliceCount(int aid, const std::string& page) {
    const std::string h = md5Hex(std::to_string(aid) + page);
    int key = static_cast<unsigned char>(h.back());
    if (aid >= 268850 && aid <= 421925) {
        key %= 10;
    } else if (aid >= 421926) {
        key %= 8;
    }
    // 注意：aid < 268850 时既不取模、也匹配不到 0..9 —— 源码的 else 分支给 10。
    // 主项目注释明确写了"份数保持 10，这是源码的行为，不是遗漏"，移植时别"修正"它。
    const int table[10] = {2, 4, 6, 8, 10, 12, 14, 16, 18, 20};
    if (key < 0 || key > 9) return 10;
    return table[key];
}

}  // namespace jmnext::core
