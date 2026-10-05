// net::HttpClient 的 Qt 实现（QNetworkAccessManager）。
//
// 为什么单独放 src/qt：核心层（src/core）刻意不依赖 Qt，便于在任何环境里编测试；
// 真机网络与界面都住在这一层。
//
// 未验证：真实网络请求（容器里没有可用目标与账号），只验证了编译与接口契合。
#pragma once
#include "net/HttpClient.h"

#include <QNetworkAccessManager>
#include <QString>

namespace jmnext::qt {

class QtHttpClient : public jmnext::net::HttpClient {
public:
    /// 同步 get（桌面端在后台线程上调用；超时毫秒数默认 20 秒）
    jmnext::net::HttpResponse get(const std::string& url,
                                 const std::vector<std::string>& headers) override;
    /// 最近一次失败的说明（网络类失败时上层据此标记主机可疑）
    const std::string& lastError() const { return lastError_; }

private:
    QNetworkAccessManager manager_;
    std::string lastError_;
};

}  // namespace jmnext::qt
