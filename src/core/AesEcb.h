// AES-256-ECB（含 PKCS#5/#7 补位）—— 自己实现，核心层不依赖第三方库。
//
// 规格来源：主项目 shared/.../crypto/JmCrypto.kt 用 `AES/ECB/PKCS5Padding`，
// 密钥是 **hex 字符串本身的 UTF-8 字节**（32 个字符 → 32 字节 → AES-256）。
// 正确性由公开测试向量 + openssl 交叉验证盯着（见 tests/test_aes.cpp），
// 不使用"自己加密自己解密"这种自证式测试。
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace jmnext::core {

/// 单块 AES-256 加密/解密（输入输出都是 16 字节）
void aes256EncryptBlock(const uint8_t key[32], const uint8_t in[16], uint8_t out[16]);
void aes256DecryptBlock(const uint8_t key[32], const uint8_t in[16], uint8_t out[16]);

/// ECB + PKCS#7 补位
std::vector<uint8_t> aes256EcbEncrypt(const std::vector<uint8_t>& plain, const uint8_t key[32]);
/// ECB 解密并**去掉** PKCS#7 补位；补位非法时返回空 vector（调用方据此判失败）
std::vector<uint8_t> aes256EcbDecrypt(const std::vector<uint8_t>& cipher, const uint8_t key[32]);

/// 把 16 进制字符串的 UTF-8 字节当作密钥（与主项目语义一致）；长度不足 32 字节时补 0
void keyFromHexString(const std::string& keyHex, uint8_t out[32]);

}  // namespace jmnext::core
