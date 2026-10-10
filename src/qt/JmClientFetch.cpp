// 通用 GET 原始正文（单独文件，避免追加到 JmClient.cpp 末尾落到命名空间之外）。
#include "JmClient.h"

#include "core/JmApi.h"
#include "core/JmPaths.h"

namespace jmnext::qt {

bool JmClient::fetchText(const std::string& path, const std::string& query, std::string* out) {
    if (!bootstrapped_) { lastError_ = "尚未初始化主机"; return false; }
    core::JmApi api(session_, http_);
    auto r = api.request(path, query);
    if (!r) { lastError_ = api.lastError(); return false; }
    lastRaw_ = r->text;
    if (out) *out = r->text;
    return true;
}

}  // namespace jmnext::qt
