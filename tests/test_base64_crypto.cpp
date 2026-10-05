// Base64（RFC 4648 向量 + 清洗规则）与 JmCrypto 门面（域规则）的测试。
#include "core/Base64.h"
#include "core/JmCrypto.h"
#include "core/Md5.h"

#include <cstdio>
#include <string>

using namespace jmnext::core;

static int failures = 0;
static void eqStr(const std::string& got, const std::string& want, const char* what) {
    if (got != want) {
        std::printf("FAIL: %s\n  实际 %s\n  期望 %s\n", what, got.c_str(), want.c_str());
        ++failures;
    }
}
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}

static std::vector<uint8_t> bytes(const std::string& s) { return {s.begin(), s.end()}; }

int main() {
    // ---------- Base64：RFC 4648 §10 的向量 ----------
    eqStr(base64Encode(bytes("")), "", "encode(\"\")");
    eqStr(base64Encode(bytes("f")), "Zg==", "encode(\"f\")");
    eqStr(base64Encode(bytes("fo")), "Zm8=", "encode(\"fo\")");
    eqStr(base64Encode(bytes("foo")), "Zm9v", "encode(\"foo\")");
    eqStr(base64Encode(bytes("foob")), "Zm9vYg==", "encode(\"foob\")");
    eqStr(base64Encode(bytes("fooba")), "Zm9vYmE=", "encode(\"fooba\")");
    eqStr(base64Encode(bytes("foobar")), "Zm9vYmFy", "encode(\"foobar\")");
    for (const char* s : {"", "f", "fo", "foo", "foob", "fooba", "foobar"}) {
        const auto enc = base64Encode(bytes(s));
        const auto dec = base64Decode(enc);
        check(std::string(dec.begin(), dec.end()) == s, "Base64 往返一致");
    }

    // ---------- 清洗规则：只保留 A-Za-z0-9+/= ----------
    eqStr(sanitizeBase64("Zm9v\nYmFy"), "Zm9vYmFy", "清洗：丢弃换行");
    eqStr(sanitizeBase64("\"Zm9v YmFy\""), "Zm9vYmFy", "清洗：丢弃引号与空格");
    check(base64Decode("Zm9v\nYmFy") == bytes("foobar"), "解码前自动清洗");
    // 清洗规则会把 "!" 丢掉，所以 "Zm9v!" 依然能解开 —— 这是主项目的行为，测试要照它写
    check(base64Decode("Zm9v!") == bytes("foo"), "清洗掉非法字符后仍可解码（与主项目一致）");
    check(base64Decode("Zm9vY").empty(), "长度不是 4 的倍数 → 空（不猜）");
    check(base64Decode("Zg===").empty(), "'=' 过多 → 空");

    // ---------- Token 与 Tokenparam ----------
    // token = md5("<时间戳><seed>")：这里用 md5 自身做交叉断言（md5 已由 RFC 1321 向量锁定）
    eqStr(token(1700000000), md5Hex("1700000000" + std::string(TOKEN_SEED)), "token = md5(时间戳+seed)");
    eqStr(tokenParam(1700000000, "2.2.1"), "1700000000,2.2.1", "tokenParam 形如 <时间戳>,<版本>");

    // ---------- needsUnscramble 的三条规则 ----------
    check(!needsUnscramble("https://x/y.gif", 999999, 100), ".gif 一律不需要还原");
    check(needsUnscramble("https://x/y.jpg", 100, 100), "aid == scrambleId → 需要");
    check(!needsUnscramble("https://x/y.jpg", 99, 100), "aid < scrambleId → 不需要");

    // ---------- sliceCount：只可能是 2..20 的偶数，且规则可复算 ----------
    for (int aid : {1, 268850, 300000, 421925, 421926, 500000}) {
        for (const char* page : {"1", "2", "17"}) {
            const int n = sliceCount(aid, page);
            check(n >= 2 && n <= 20 && n % 2 == 0, "sliceCount 落在 2..20 的偶数上");
            // 独立复算一遍规则（两份实现互证，避免"自证式测试"）
            int key = static_cast<unsigned char>(md5Hex(std::to_string(aid) + page).back());
            if (aid >= 268850 && aid <= 421925) key %= 10;
            else if (aid >= 421926) key %= 8;
            const int table[10] = {2, 4, 6, 8, 10, 12, 14, 16, 18, 20};
            const int expect = (key >= 0 && key <= 9) ? table[key] : 10;  // else 分支给 10
            check(n == expect, "sliceCount 与独立复算一致");
        }
    }

    if (failures == 0) std::printf("全部通过：Base64（RFC 4648 + 清洗规则）与 JmCrypto 门面域规则\n");
    return failures == 0 ? 0 : 1;
}
