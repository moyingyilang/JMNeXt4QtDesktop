#include "core/JmApi.h"

#include "core/JmCrypto.h"

#include <cstdlib>
#include <string>
#include <vector>

namespace jmnext::core {
namespace {

// 真实响应是信封：{"code":200,"data":"<base64 密文>"}（已用真实网络确认）。
// data 里是**密文**，且 JSON 会把 "/" 转义成 "\/" —— 必须还原回 "/"，否则 base64 会错。
// 返回 {code, data原文}；不是信封则返回 nullopt（此时按"整个 body 就是密文"处理）。
struct Envelope {
    int code = 0;
    std::string data;
};

std::optional<Envelope> extractEnvelope(const std::string& body) {
    if (body.empty() || body.front() != '{') return std::nullopt;
    const auto key = body.find("\"data\"");
    if (key == std::string::npos) return std::nullopt;
    auto colon = body.find(':', key);
    if (colon == std::string::npos) return std::nullopt;
    auto q1 = body.find('"\"', colon);
    if (q1 == std::string::npos) return std::nullopt;

    Envelope env;
    const auto codeKey = body.find("\"code\"");
    if (codeKey != std::string::npos) {
        auto c2 = body.find(':', codeKey);
        if (c2 != std::string::npos) env.code = std::atoi(body.c_str() + c2 + 1);
    }
    for (std::size_t i = q1 + 1; i < body.size(); ++i) {
        const char c = body[i];
        if (c == '\\' && i + 1 < body.size()) {
            const char n = body[i + 1];
            if (n == '/') { env.data.push_back('/'); ++i; continue; }        // \/ → /
            if (n == '"') { env.data.push_back('"'); ++i; continue; }        // \" → "
            if (n == 'n') { env.data.push_back('\n'); ++i; continue; }
            env.data.push_back(n); ++i; continue;
        }
        if (c == '"') return env;                                   // 字符串结束
        env.data.push_back(c);
    }
    return std::nullopt;
}

// 与主项目 JmRemote.kt 实际发送的三个头一致（登录后还会加 Authorization: Bearer <jwt>，
// 那部分等做登录再接；这里不带凭证）
std::vector<std::string> headersFor(const JmSession& s) {
    return {"Token: " + s.token(), "Tokenparam: " + s.tokenParam(),
            "Accept: application/json, text/plain, */*"};
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

    // 真实协议：响应是信封 {code,data}，密文在 data 里（真实网络已确认）；不是信封时按整 body 处理
    const std::string payload = [&] {
        if (auto env = extractEnvelope(resp.body)) return env->data;
        return resp.body;
    }();

    // 用**发起请求时**的时间戳解密（不是"现在"的时间戳）
    if (auto text = decryptApiData(payload, usedTime)) {
        return Result{resp.status, *text, /*retried=*/false};
    }

    // 解密失败 → 时间戳可能过期，重算一次再试（只重试一次）
    if (allowRetry) {
        session_.refresh(usedTime + 1);                // 由上层决定"现在"；这里只保证换一个新值
        const auto second = http_.get(url, headersFor(session_));
        if (second.ok()) {
            const std::string payload2 = [&] {
                if (auto env = extractEnvelope(second.body)) return env->data;
                return second.body;
            }();
            if (auto text = decryptApiData(payload2, session_.time())) {
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
