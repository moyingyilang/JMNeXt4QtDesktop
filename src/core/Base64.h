// Base64 解码（含主项目 JmCrypto.sanitizeBase64 的清洗规则）—— 自己实现，核心层不依赖 Qt。
//
// 清洗规则来源：主项目 shared/.../crypto/JmCrypto.kt 的 sanitizeBase64：
// **只保留** A-Z a-z 0-9 + / =，其余（换行、空格、引号等）一律丢弃再解码。
#pragma once
#include <string>
#include <vector>

namespace jmnext::core {
/// 按主项目的清洗规则过滤
std::string sanitizeBase64(const std::string& text);
/// 标准 Base64 解码；含非法字符或长度不合法时返回空 vector
std::vector<uint8_t> base64Decode(const std::string& text);
/// 标准 Base64 编码（测试与自检用）
std::string base64Encode(const std::vector<uint8_t>& data);
}  // namespace jmnext::core
