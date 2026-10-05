// 业务请求层：把"会话 + 可注入网络 + 解密"串成一步。
//
// 它做的事，对应主项目里 Repository 每次请求都要走的几步：
//   1. 用会话的 apiUrl() 拼 URL，带上 Token / Tokenparam 两个头；
//   2. 响应体用**发起请求时的**时间戳做 AES 解密（decryptApiData）；
//   3. **解密失败就 refresh 一次重试** —— 主项目 JmSession 注释写明了原因：
//      时间戳在会话内固定，长时间挂后台后服务端可能判过期，于是拿新密钥解旧响应必然乱码，
//      所以"解密失败"要显式重算一次再试，而不是直接报错给用户；
//   4. 网络类失败标记 session.hostSuspect，由上层重新发现主机（否则用户会遇到"怎么刷新都没用"）。
//
// **未验证**：真实服务端的响应格式（容器内无账号与可用主机），目前只有假响应路径。
#pragma once
#include "core/JmSession.h"
#include "net/HttpClient.h"

#include <optional>
#include <string>

namespace jmnext::core {

class JmApi {
public:
    struct Result {
        int status = 0;
        std::string text;      // 解密后的正文（JSON 或一句"人话"，见 decryptApiData 的语义）
        bool retried = false;  // 是否因解密失败 refresh 后重试过（便于测试与日志）
    };

    JmApi(JmSession& session, jmnext::net::HttpClient& http) : session_(session), http_(http) {}

    /// 发一次业务请求。query 是**原样**拼到 URL 后面的查询串（不含 '?'），可为空。
    std::optional<Result> request(const std::string& path, const std::string& query = "");
    std::optional<Result> requestPath(const std::string& fullPathWithQuery);

    const std::string& lastError() const { return lastError_; }

private:
    std::optional<Result> doRequest(const std::string& url, bool allowRetry);

    JmSession& session_;
    jmnext::net::HttpClient& http_;
    std::string lastError_;
};

}  // namespace jmnext::core
