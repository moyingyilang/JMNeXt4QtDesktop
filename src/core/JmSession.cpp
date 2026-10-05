#include "core/JmSession.h"

#include "core/JmCrypto.h"

namespace jmnext::core {
namespace {
std::string trimStartSlash(const std::string& s) {
    std::size_t i = 0;
    while (i < s.size() && s[i] == '/') ++i;
    return s.substr(i);
}
std::string trimEndSlash(std::string s) {
    while (!s.empty() && s.back() == '/') s.pop_back();
    return s;
}
}  // namespace

JmSession::JmSession(std::string clientVersion, int64_t timeSeconds)
    : clientVersion_(std::move(clientVersion)), timeSeconds_(timeSeconds) {}

// 必须写成 jmnext::core::token：成员 token() 会遮蔽同名自由函数，不限定就编译不过
std::string JmSession::token() const { return jmnext::core::token(timeSeconds_, TOKEN_SEED); }

std::string JmSession::tokenParam() const { return jmnext::core::tokenParam(timeSeconds_, clientVersion_); }

int64_t JmSession::refresh(int64_t nowSeconds) {
    timeSeconds_ = nowSeconds;
    return timeSeconds_;
}

void JmSession::useHost(const std::string& base) {
    apiBaseUrl_ = normalizeHost(base);
    hostSuspect_ = false;
}

std::optional<std::string> JmSession::apiUrl(const std::string& path) const {
    if (apiBaseUrl_.empty()) return std::nullopt;   // 未初始化：明确失败，不要拼出半截 URL
    return apiBaseUrl_ + trimStartSlash(path);
}

std::optional<std::string> JmSession::imageUrl(const std::string& path) const {
    std::string base = imageHost_.empty() ? apiBaseUrl_ : imageHost_;
    if (base.empty()) return std::nullopt;
    return trimEndSlash(base) + "/" + trimStartSlash(path);
}

std::string JmSession::normalizeHost(const std::string& host) {
    // 与主项目一致：已带协议就只补尾斜杠；否则补 https://
    std::string h = host;
    while (!h.empty() && (h.front() == ' ' || h.front() == '\r' || h.front() == '\n')) h.erase(h.begin());
    while (!h.empty() && (h.back() == ' ' || h.back() == '\r' || h.back() == '\n')) h.pop_back();
    if (h.rfind("http", 0) == 0) return trimEndSlash(h) + "/";
    return "https://" + trimEndSlash(h) + "/";
}

}  // namespace jmnext::core
