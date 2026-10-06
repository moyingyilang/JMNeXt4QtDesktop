#include "qml/JmImageProvider.h"

#include <QFile>
#include <QStandardPaths>
#include <QUrl>


namespace jmnext::qt {
namespace {
/// 占位图：纯色 + 简单边框，尺寸随请求（默认 3:4）
QImage placeholder(const QSize& requested) {
    const QSize s = requested.isValid() ? requested : QSize(120, 160);
    QImage img(s, QImage::Format_RGB32);
    img.fill(QColor(0x2b, 0x2d, 0x31));
    for (int x = 0; x < s.width(); ++x) {
        img.setPixelColor(x, 0, QColor(0x3a, 0x3d, 0x43));
        img.setPixelColor(x, s.height() - 1, QColor(0x3a, 0x3d, 0x43));
    }
    return img;
}
}  // namespace

JmImageProvider::JmImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage JmImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    // id 形如 "cover?<percent-encoded url>"；取不到合法 URL 时直接给占位图
    QString url = id;
    if (url.startsWith(QStringLiteral("cover?"))) url = url.mid(6);
    else url.clear();
    url = QUrl::fromPercentEncoding(url.toUtf8());
    // 详情页专辑封面：id 形如 "albumcover?<aid>"。
    // 图片基址与 worker 保持一致：读同一个 host.txt（AppDataLocation 下的缓存主机），
    // 并按 fetchAlbumCover 的同一规则拼 URL（只用 id 走模板、**不带 ?v=**），否则命不中磁盘缓存。
    if (url.isEmpty() && id.startsWith(QStringLiteral("albumcover?"))) {
        const QString aid = id.mid(11);
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QFile f(dir + QStringLiteral("/host.txt"));
        if (f.open(QIODevice::ReadOnly) && !aid.isEmpty()) {
            QString base = QString::fromUtf8(f.readAll()).trimmed();
            while (base.endsWith(QLatin1Char('/'))) base.chop(1);
            if (!base.isEmpty()) url = base + QStringLiteral("/media/albums/") + aid + QStringLiteral("_3x4.jpg");
        }
    }
    if (url.isEmpty()) {
        const QImage ph = placeholder(requestedSize);
        if (size) *size = ph.size();
        return ph;
    }
    const QByteArray bytes = diskOnly_.raw(url);      // 磁盘缓存：worker 线程写、渲染线程读
    if (!bytes.isEmpty()) {
        QImage img;
        img.loadFromData(bytes);   // 缓存里是原始下载字节（JPEG/WebP 等），直接用 Qt 解码最稳
        if (!img.isNull()) {
            if (size) *size = img.size();
            return img;
        }
    }
    const QImage ph = placeholder(requestedSize);
    if (size) *size = ph.size();
    return ph;
}

}  // namespace jmnext::qt
