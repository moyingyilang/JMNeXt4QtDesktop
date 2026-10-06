// QML 图片提供器（P1c）：让 QML 按 URL 显示封面。
//
// 三条硬约束（详见 STATUS.md 的 P1c 设计）：
// 1. requestImage 在 Qt Quick **渲染线程**被调用 -> 不能用界面侧的 JmClient（QNetworkAccessManager 有线程归属），
//    这里用**自己的 ImageCache 实例**，且只碰磁盘接口（hasRaw/raw），不碰它的内存 LRU；
// 2. 磁盘缓存键是**完整 URL**（含 ?v=<updateAt>），所以 id 里直接带 URL，避免任何拼装差异；
// 3. 未命中返回**占位图**而不是空图：QML 侧会显示一块灰底，而不是静默空白。
#pragma once
#include <QQuickImageProvider>

#include "qt/ImageCache.h"

namespace jmnext::qt {

class JmImageProvider : public QQuickImageProvider {
public:
    JmImageProvider();
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    ImageCache diskOnly_{1};   // 只当磁盘缓存用（内存 LRU 不跨线程共享）
};

}  // namespace jmnext::qt
