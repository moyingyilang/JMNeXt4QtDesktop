// 通用 POST 动作（单独文件，避免追加到 JmClient.cpp 末尾落到命名空间之外）。
#include "JmClient.h"

#include "core/JmApi.h"
#include "core/JmPaths.h"

namespace jmnext::qt {

bool JmClient::action(const std::string& path, const std::string& form, std::string* message) {
    if (!bootstrapped_) { lastError_ = "尚未初始化主机"; return false; }
    core::JmApi api(session_, http_);
    auto r = api.post(path, form);
    if (!r) { lastError_ = api.lastError(); if (message) *message = lastError_; return false; }
    lastRaw_ = r->text;
    // 服务端一般返回 {"status":"200",...}；取不到 status 时不算失败（有些接口只回一句提示）
    const auto code = r->text.find("\"status\":\"200\"");
    if (code == std::string::npos && r->text.find("\"status\"") != std::string::npos) {
        if (message) *message = r->text.substr(0, 200);
        lastError_ = "服务端返回非 200";
        return false;
    }
    if (message) *message = "成功";
    return true;
}

}  // namespace jmnext::qt
