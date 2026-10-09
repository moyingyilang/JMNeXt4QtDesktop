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

    /// POST（表单）。默认实现返回失败，真实实现（Qt 侧）覆盖它；
    /// 这样加登录不会破坏既有的测试桩（它们只实现 get）。
    virtual HttpResponse post(const std::string& url, const std::string& body,
                              const std::vector<std::string>& headers) {
        (void)url; (void)body; (void)headers;
        return HttpResponse{};
    }
};

}  // namespace jmnext::net
