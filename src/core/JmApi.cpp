#include "core/JmApi.h"

#include "core/JmCrypto.h"

#include <vector>

namespace jmnext::core {
namespace {
std::vector<std::string> headersFor(const JmSession& s) {
    return {"Token: " + s.token(), "Tokenparam: " + s.tokenParam()};
}
}  // namespace

std::optional<JmApi::Result> JmApi::request(const std::string& path, const std::string& query) {
    auto base = session_.apiUrl(path);
    if (!base) {
        lastError_ = "API 主机尚未初始化";
        return std::nullopt;
    }
    std::string url = *base;
    if (!query.empty()) url += "?" + query;
    return doRequest(url, /*allowRetry=*/true);
}

std::optional<JmApi::Result> JmApi::requestPath(const std::string& fullPathWithQuery) {
    return request(fullPathWithQuery, "");
}

std::optional<JmApi::Result> JmApi::doRequest(const std::string& url, bool allowRetry) {
    lastError_.clear();
    const int64_t usedTime = session_.time();          // 关键：记住"发起请求时"的时间戳
    const auto resp = http_.get(url, headersFor(session_));

    if (resp.status == 0) {                            // 网络类失败（连不上/超时/DNS）
        lastError_ = "网络请求失败";
        session_.markHostSuspect();
        return std::nullopt;
    }
    if (!resp.ok()) {
        lastError_ = "HTTP " + std::to_string(resp.status);
        return std::nullopt;
    }

    // 用**发起请求时**的时间戳解密（不是"现在"的时间戳）
    if (auto text = decryptApiData(resp.body, usedTime)) {
        return Result{resp.status, *text, /*retried=*/false};
    }

    // 解密失败 → 时间戳可能过期，重算一次再试（只重试一次）
    if (allowRetry) {
        session_.refresh(usedTime + 1);                // 由上层决定"现在"；这里只保证换一个新值
        const auto second = http_.get(url, headersFor(session_));
        if (second.ok()) {
            if (auto text = decryptApiData(second.body, session_.time())) {
                return Result{second.status, *text, /*retried=*/true};
            }
        }
        lastError_ = "解密失败（已重试一次）";
        return std::nullopt;
    }
    lastError_ = "解密失败";
    return std::nullopt;
}

}  // namespace jmnext::core
