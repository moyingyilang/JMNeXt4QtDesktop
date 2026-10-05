// 主项目加密门面的 C++ 版 —— 语义逐条对照 shared/.../crypto/JmCrypto.kt。
//
// 三条容易踩的域规则（主项目用注释记下来的，这里原样保留）：
//  1. `decryptApiData` **不能只认 JSON**：有些接口的 data 就是一句人话（实测 album_sertracking 回
//     "已追踪!"/"已取消追踪!"）。把它当"解密失败"会让上层**重发非幂等的 POST**，等于把刚做的操作撤销 ——
//     所以除 JSON 外还要记住第一个非空结果并返回。
//  2. `decryptHostPayload` 用**固定 seed**（不带时间戳），且必须形如 JSON 才认。
//  3. `needsUnscramble`：`.gif` 一律不需要还原；其余按 `aid >= scrambleId` 判定。
#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace jmnext::core {

/// 与主项目一致的两个 Token 种子与一个主机发现种子
inline constexpr char TOKEN_SEED[] = "185Hcomic3PAPP7R";
inline constexpr char TOKEN_SEED_ALT[] = "18comicAPPContent";
inline constexpr char HOST_SEED[] = "diosfjckwpqpdfjkvnqQjsik";

/// Token 头：md5("<秒级时间戳><seed>")
std::string token(int64_t timeSeconds, const std::string& seed = TOKEN_SEED);
/// Tokenparam 头："<时间戳>,<客户端版本>"
std::string tokenParam(int64_t timeSeconds, const std::string& clientVersion);

/// AES-256-ECB 解密（base64 文本 + hex 文本密钥），失败返回 nullopt
std::optional<std::string> decryptEcb(const std::string& cipherBase64, const std::string& keyHex);
/// 依次尝试两个 seed；JSON 优先，其次返回第一段非空"人话"
std::optional<std::string> decryptApiData(const std::string& dataBase64, int64_t timeSeconds);
/// 主机发现接口：固定 seed，且必须形如 JSON
std::optional<std::string> decryptHostPayload(const std::string& encryptedText);

/// 该图是否需要切片还原
bool needsUnscramble(const std::string& url, int aid, int scrambleId);
/// 还原份数（2..20）
int sliceCount(int aid, const std::string& page);

}  // namespace jmnext::core
