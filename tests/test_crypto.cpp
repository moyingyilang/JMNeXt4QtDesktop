// 密码学原语的测试。判据全部来自**外部**：
//   MD5   —— RFC 1321 附录 A.5 的测试套件
//   AES   —— NIST SP 800-38A F.1.5（ECB-AES256）向量，并用 openssl 交叉验证过一个"项目语义"下的已知密文
// 不用"自己加密自己解密"这种自证式测试。
#include "core/AesEcb.h"
#include "core/Md5.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

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

static std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> out;
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2)
        out.push_back(static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
    return out;
}
static std::string bytesToHex(const std::vector<uint8_t>& b) {
    static const char* h = "0123456789abcdef";
    std::string s;
    for (uint8_t x : b) { s.push_back(h[x >> 4]); s.push_back(h[x & 0xF]); }
    return s;
}

int main() {
    // ---------- MD5：RFC 1321 附录 A.5 ----------
    eqStr(md5Hex(""), "d41d8cd98f00b204e9800998ecf8427e", "md5(\"\")");
    eqStr(md5Hex("a"), "0cc175b9c0f1b6a831c399e269772661", "md5(\"a\")");
    eqStr(md5Hex("abc"), "900150983cd24fb0d6963f7d28e17f72", "md5(\"abc\")");
    eqStr(md5Hex("message digest"), "f96b697d7cb7938d525a2f31aaf161d0", "md5(\"message digest\")");
    eqStr(md5Hex("abcdefghijklmnopqrstuvwxyz"), "c3fcd3d76192e4007dfb496cca67e13b", "md5(a..z)");
    eqStr(md5Hex("12345678901234567890123456789012345678901234567890123456789012345678901234567890"),
          "57edf4a22be3c955ac49da2e2107b67a", "md5(80 位数字)");

    // ---------- AES-256 单块：NIST SP 800-38A F.1.5（密钥是**解码后的 32 字节**）----------
    {
        auto k32 = hexToBytes("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
        check(k32.size() == 32, "NIST 密钥解码后为 32 字节");
        uint8_t key[32];
        std::memcpy(key, k32.data(), 32);

        auto p1 = hexToBytes("6bc1bee22e409f96e93d7e117393172a");
        uint8_t c1[16];
        aes256EncryptBlock(key, p1.data(), c1);
        eqStr(bytesToHex(std::vector<uint8_t>(c1, c1 + 16)), "f3eed1bdb5d2a03c064b5a7e3db181f8",
              "AES-256 单块加密（NIST ECB-AES256 第 1 块）");

        uint8_t back[16];
        aes256DecryptBlock(key, c1, back);
        eqStr(bytesToHex(std::vector<uint8_t>(back, back + 16)), "6bc1bee22e409f96e93d7e117393172a",
              "AES-256 单块解密（回原文）");

        auto p2 = hexToBytes("ae2d8a571e03ac9c9eb76fac45af8e51");
        uint8_t c2[16];
        aes256EncryptBlock(key, p2.data(), c2);
        eqStr(bytesToHex(std::vector<uint8_t>(c2, c2 + 16)), "591ccb10d410ed26dc5ba74a31362870",
              "AES-256 单块加密（NIST 第 2 块）");
    }

    // ---------- 项目语义：密钥 = hex 文本的 ASCII 字节（32 字符）----------
    // 期望值由 openssl 独立算出：printf '6bc1bee2...' | openssl enc -aes-256-ecb -nopad -K <该密钥的hex> -nosalt
    {
        const std::string keyAscii = "603deb1015ca71be2b73aef0857d7781";  // 32 个字符
        uint8_t key[32];
        keyFromHexString(keyAscii, key);
        check(key[0] == '6' && key[1] == '0', "keyFromHexString：取的是文本的 ASCII 字节");
        check(key[31] == static_cast<uint8_t>(keyAscii[31]), "keyFromHexString：第 32 字节 = 第 32 个字符");
        auto p1 = hexToBytes("6bc1bee22e409f96e93d7e117393172a");
        uint8_t c1[16];
        aes256EncryptBlock(key, p1.data(), c1);
        eqStr(bytesToHex(std::vector<uint8_t>(c1, c1 + 16)), "e939fba9bb659b65b129d8aba1e4d11d",
              "项目语义（ASCII 密钥）下与 openssl 一致");
    }

    // ---------- PKCS#7 与拒绝路径 ----------
    {
        uint8_t key[32];
        keyFromHexString("0123456789abcdef0123456789abcdef", key);
        std::vector<uint8_t> plain(16, 0x41);
        auto ct = aes256EcbEncrypt(plain, key);
        check(ct.size() == 32, "PKCS#7：明文正好 16 字节时补一整块（密文 32 字节）");
        check(aes256EcbDecrypt(ct, key) == plain, "PKCS#7：往返一致");

        auto ct2 = aes256EcbEncrypt(std::vector<uint8_t>{1, 2, 3}, key);
        ct2.back() ^= 0xFF;
        check(aes256EcbDecrypt(ct2, key).empty(), "补位非法时返回空（拒绝悄悄放行）");

        std::vector<uint8_t> many;
        for (int i = 0; i < 100; ++i) many.push_back(static_cast<uint8_t>(i));
        auto ct3 = aes256EcbEncrypt(many, key);
        check(ct3.size() % 16 == 0, "ECB 密文长度是 16 的倍数");
        check(aes256EcbDecrypt(ct3, key) == many, "ECB 多块往返一致");
    }

    if (failures == 0)
        std::printf("全部通过：MD5（RFC 1321）+ AES-256-ECB（NIST SP800-38A + openssl 交叉验证）\n");
    return failures == 0 ? 0 : 1;
}
