// MD5（十六进制小写输出）—— 自己实现，保持核心层不依赖 Qt/OpenSSL。
//
// 为什么要自己写：主项目 JmCrypto 的 Token 与 AES 密钥都建立在 MD5 之上，
// 而核心层（jmnext_core）刻意不引入第三方依赖，便于在没有包管理的环境里构建。
// 正确性由公开测试向量盯着（见 tests/test_md5.cpp）：md5("") 与 md5("abc") 等。
#pragma once
#include <string>

namespace jmnext::core {
/// 输入按 UTF-8 字节处理，输出 32 位小写十六进制串。
std::string md5Hex(const std::string& input);
}  // namespace jmnext::core
