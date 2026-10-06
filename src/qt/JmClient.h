// 界面要用的数据服务：把"会话 + Qt 网络 + 解析 + 解码 + 还原"包成几个直接可用的方法。
//
// 存在的意义：把协议细节关在这一层里，界面只看到 ListEntry / AlbumInfo / ChapterImages / QImage。
// 全部方法都是同步的（桌面端在后台线程调用；容器里用 --live 自检时直接在主线程调用即可）。
#pragma once
#include "core/JmParse.h"
#include "core/JmSession.h"
#include "qt/ImageCache.h"
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
    /// 主机缓存（省掉每次启动的"两个入口请求 + 解密"）
    QString cachedHostPath() const;
    QString cachedHost() const;
    void saveCachedHost(const QString& host) const;
    std::optional<std::vector<jmnext::core::ListEntry>> latest(int page = 0);
    /// 搜索（参数名 search_query 是探出来的；条目字段与首页列表相同）
    std::optional<std::vector<jmnext::core::ListEntry>> search(const std::string& word, int page);
    std::optional<jmnext::core::AlbumInfo> album(const std::string& id);
    std::optional<jmnext::core::ChapterImages> chapter(const std::string& id);
    ImageCache& cache() { return cache_; }
    const ImageCache& cache() const { return cache_; }

    /// 下载一页 → Qt 解码 → 需要则切片还原 → 返回 QImage（带内存与磁盘缓存）

    /// 取列表项的封面缩略图（3x4 模板，带缓存）—— 真实网络已验证该地址返回 JPEG 400x533
    std::optional<QImage> cover(const jmnext::core::ListEntry& entry);
    std::optional<QImage> pageImage(const std::string& url, int aid, int scrambleId);

    const std::string& lastError() const { return lastError_; }
    const std::string& host() const { return host_; }
    jmnext::core::JmSession& session() { return session_; }

private:
    jmnext::core::JmSession session_{"2.1.9", 0};
    QtHttpClient http_;
    ImageCache cache_;
    std::string host_;
    std::string lastError_;
    bool bootstrapped_ = false;
};

}  // namespace jmnext::qt
