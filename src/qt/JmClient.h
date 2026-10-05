// 界面要用的数据服务：把"会话 + Qt 网络 + 解析 + 解码 + 还原"包成几个直接可用的方法。
//
// 存在的意义：把协议细节关在这一层里，界面只看到 ListEntry / AlbumInfo / ChapterImages / QImage。
// 全部方法都是同步的（桌面端在后台线程调用；容器里用 --live 自检时直接在主线程调用即可）。
#pragma once
#include "core/JmParse.h"
#include "core/JmSession.h"
#include "qt/QtHttpClient.h"

#include <QImage>

#include <optional>
#include <string>
#include <vector>

namespace jmnext::qt {

class JmClient {
public:
    /// 主机发现。返回是否成功；失败原因见 lastError()
    bool bootstrap();
    std::optional<std::vector<jmnext::core::ListEntry>> latest(int page = 0);
    std::optional<jmnext::core::AlbumInfo> album(const std::string& id);
    std::optional<jmnext::core::ChapterImages> chapter(const std::string& id);
    /// 下载一页 → Qt 解码 → 需要则切片还原 → 返回 QImage
    std::optional<QImage> pageImage(const std::string& url, int aid, int scrambleId);

    const std::string& lastError() const { return lastError_; }
    const std::string& host() const { return host_; }
    jmnext::core::JmSession& session() { return session_; }

private:
    jmnext::core::JmSession session_{"2.1.9", 0};
    QtHttpClient http_;
    std::string host_;
    std::string lastError_;
    bool bootstrapped_ = false;
};

}  // namespace jmnext::qt
