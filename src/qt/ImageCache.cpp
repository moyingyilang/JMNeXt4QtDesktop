#include "qt/ImageCache.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace jmnext::qt {

ImageCache::ImageCache(int maxImages) : images_(maxImages) {
    // 放系统缓存目录下（Linux: ~/.cache/<组织>/<应用>）
    cacheDir_ = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/images";
    QDir().mkpath(cacheDir_);
}

bool ImageCache::hasImage(const QString& key) const { return images_.contains(key); }

QImage ImageCache::image(const QString& key) {
    if (QImage* hit = images_.object(key)) {
        ++imageHits_;
        return *hit;
    }
    ++imageMisses_;
    return {};
}

void ImageCache::putImage(const QString& key, const QImage& image) {
    if (image.isNull()) return;
    images_.insert(key, new QImage(image));
}

QString ImageCache::rawPath(const QString& url) const {
    const QByteArray h = QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha1).toHex();
    return cacheDir_ + "/" + QString::fromLatin1(h) + ".bin";
}

bool ImageCache::hasRaw(const QString& url) const {
    const bool ok = QFile::exists(rawPath(url));
    if (ok) ++rawHits_; else ++rawMisses_;
    return ok;
}

QByteArray ImageCache::raw(const QString& url) const {
    QFile f(rawPath(url));
    if (!f.open(QIODevice::ReadOnly)) return {};
    return f.readAll();
}

void ImageCache::putRaw(const QString& url, const QByteArray& bytes) {
    if (bytes.isEmpty()) return;
    QFile f(rawPath(url));
    if (f.open(QIODevice::WriteOnly)) f.write(bytes);
}

}  // namespace jmnext::qt
