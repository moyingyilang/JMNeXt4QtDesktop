// 极小的 HTTP 抽象：把"发请求"这一步做成可注入的，这样整条链路（拼 URL → 取响应 → 解密 → 还原）
// 可以在**没有网络、没有账号**的环境里用假响应完整测试；真机再用 Qt 的 QNetworkAccessManager 实现它。
#pragma once
#include <string>
#include <vector>

namespace jmnext::net {

struct HttpResponse {
    int status = 0;
    std::string body;
    bool ok() const { return status >= 200 && status < 300; }
};

class HttpClient {
public:
    virtual ~HttpClient() = default;
    virtual HttpResponse get(const std::string& url, const std::vector<std::string>& headers) = 0;
};

}  // namespace jmnext::net
