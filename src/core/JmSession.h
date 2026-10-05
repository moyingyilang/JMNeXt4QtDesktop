// 一次会话状态（时间戳、Token、API 主机、图床主机）—— 语义逐条对照主项目
// shared/.../data/remote/JmSession.kt。
//
// **最重要的一条规则**（主项目注释里的原话）：时间戳在会话内**固定**，因为它同时决定两件事 ——
//   1. `Token` 请求头 = md5("<时间戳><seed>")
//   2. **响应体 AES 密钥** = 同一个值
// 也就是说密钥与请求头是同一个值，解密时必须复用**发起请求时**的那个时间戳；
// 每次请求各算一个时间戳，就会拿新密钥解旧响应，必然解出乱码。
// 长时间挂后台可能被判过期，因此提供 refresh()，由上层在"解密失败"时重试一次。
#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace jmnext::core {

/// 与所抽取的 APK 对齐：服务端会校验版本号合法性，不能随便改
inline constexpr char DEFAULT_CLIENT_VERSION[] = "2.1.9";

class JmSession {
public:
    explicit JmSession(std::string clientVersion = DEFAULT_CLIENT_VERSION, int64_t timeSeconds = 0);

    int64_t time() const { return timeSeconds_; }
    /// Token 请求头，同时也是响应体的 AES 密钥
    std::string token() const;
    /// Tokenparam 请求头，形如 "<时间戳>,<版本>"
    std::string tokenParam() const;
    /// 重算时间戳（下次取 token/tokenParam 即生效）
    int64_t refresh(int64_t nowSeconds);

    /// API 主机，形如 "https://api.example.com/"（**含尾斜杠**）；主机发现成功后才有效
    void useHost(const std::string& base);
    bool hasHost() const { return !apiBaseUrl_.empty(); }
    const std::string& apiBaseUrl() const { return apiBaseUrl_; }

    /// 主机是否可疑（出现网络类失败）→ 由上层重新发现
    bool hostSuspect() const { return hostSuspect_; }
    void markHostSuspect() { hostSuspect_ = true; }
    void clearHostSuspect() { hostSuspect_ = false; }

    /// 图床主机（来自 setting 接口）；为空时回退到 apiBaseUrl
    void setImageHost(const std::string& base) { imageHost_ = base; }

    /// 拼业务接口 URL：base + path（去掉 path 开头的斜杠）
    std::optional<std::string> apiUrl(const std::string& path) const;
    /// 拼图片 URL：imageHost（或 apiBaseUrl）+ "/" + path
    std::optional<std::string> imageUrl(const std::string& path) const;

    /// 主机发现返回的一行主机名 → 规范 base
    static std::string normalizeHost(const std::string& host);

private:
    std::string clientVersion_;
    int64_t timeSeconds_;
    std::string apiBaseUrl_;
    std::string imageHost_;
    bool hostSuspect_ = false;
};

}  // namespace jmnext::core
