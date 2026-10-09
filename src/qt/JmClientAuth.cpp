// 登录相关实现（单独成文件：JmClient.cpp 的末尾追加容易落到命名空间之外）。
#include "JmClient.h"

#include "core/JmApi.h"
#include "core/JmPaths.h"

namespace jmnext::qt {

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
    if (jwt.empty()) { lastError_ = "登录成功但未取到凭证"; return false; }
    core::setAuthJwt(jwt);
    return true;
}

void JmClient::logout() { core::setAuthJwt({}); }

}  // namespace jmnext::qt
