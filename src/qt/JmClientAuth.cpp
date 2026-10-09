// 登录相关实现（单独成文件：JmClient.cpp 的末尾追加容易落到命名空间之外）。
#include "JmClient.h"

#include "core/JmApi.h"
#include "core/JmPaths.h"

#include <cctype>

namespace jmnext::qt {

namespace {
// 只取"键名"（不取值），用于在日志里安全地排查凭证字段名 —— 令牌本身绝不落日志。
std::string keyNames(const std::string& json) {
    std::string out;
    std::size_t i = 0;
    while ((i = json.find('"', i)) != std::string::npos) {
        const auto e = json.find('"', i + 1);
        if (e == std::string::npos) break;
        const auto c = json.find(':', e + 1);
        if (c == std::string::npos) break;
        bool onlyWs = true;
        for (std::size_t k = e + 1; k < c; ++k)
            if (!std::isspace(static_cast<unsigned char>(json[k]))) { onlyWs = false; break; }
        if (onlyWs) { if (!out.empty()) out += ", "; out += json.substr(i + 1, e - i - 1); }
        i = e + 1;
    }
    return out;
}
}  // namespace


namespace {
// 从 "key":"value" 里取值（登录返回体字段很少，不值得引一套解析）
std::string fieldOf(const std::string& json, const std::string& key) {
    const auto k = json.find("\"" + key + "\"");
    if (k == std::string::npos) return {};
    auto c = json.find(':', k);
    if (c == std::string::npos) return {};
    auto q = json.find('"', c + 1);
    if (q == std::string::npos) return {};
    auto e = json.find('"', q + 1);
    if (e == std::string::npos) return {};
    return json.substr(q + 1, e - q - 1);
}
}  // namespace

// 依据 shared/data/JmRepository.kt 的 login()：POST login，表单 username/password，
// 返回体里取 jwt_token 作为凭证；Kotlin 侧还会加密落盘，这里当前只在进程内生效。
bool JmClient::login(const std::string& username, const std::string& password) {
    if (!bootstrapped_) { lastError_ = "尚未初始化主机"; return false; }
    core::JmApi api(session_, http_);
    const std::string body = "username=" + username + "&password=" + password;
    auto r = api.post(core::paths::LOGIN, body);
    if (!r) { lastError_ = api.lastError(); return false; }
    std::string jwt = fieldOf(r->text, "jwt_token");
    if (jwt.empty()) jwt = fieldOf(r->text, "jwtToken");
    if (jwt.empty()) jwt = fieldOf(r->text, "token");
    if (jwt.empty()) jwt = fieldOf(r->text, "s");          // 实测：JM 的登录令牌字段名是 s
    if (jwt.empty()) { lastRaw_ = r->text;
        lastError_ = "登录成功但未取到凭证；响应字段：" + keyNames(r->text); return false; }
    core::setAuthJwt(jwt);
    return true;
}

void JmClient::logout() { core::setAuthJwt({}); }

}  // namespace jmnext::qt
