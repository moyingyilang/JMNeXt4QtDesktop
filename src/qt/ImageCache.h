// 图片缓存：内存 LRU（存已解码/已还原的 QImage）+ 磁盘缓存（存下载到的原始字节）。
//
// 对应主项目里 Coil 承担的两件事：
//  1. 内存缓存 —— 来回翻页时不重复解码与还原；
//  2. 磁盘缓存 —— 冷启动后不重复下载（主项目注释里写明的边界：磁盘存的是**未还原**的原始字节，
//     所以冷启动首见某页仍要解码 + 还原一次，这不是配置漏了）。
#pragma once
#include <QByteArray>
#include <QCache>
#include <QImage>
#include <QString>

namespace jmnext::qt {

class ImageCache {
public:
    /// 内存缓存上限（张）。单页 852x1280 约 4.3MB，默认 24 张 ≈ 100MB 上限
    explicit ImageCache(int maxImages = 24);

    bool hasImage(const QString& key) const;
    /// 命中返回副本并计一次命中；未命中返回空图
    QImage image(const QString& key);
    void putImage(const QString& key, const QImage& image);

    /// 磁盘缓存：原始下载字节（键是图片 URL）
    bool hasRaw(const QString& url) const;
    QByteArray raw(const QString& url) const;
    void putRaw(const QString& url, const QByteArray& bytes);

    int imageHits() const { return imageHits_; }
    int imageMisses() const { return imageMisses_; }
    int rawHits() const { return rawHits_; }
    int rawMisses() const { return rawMisses_; }
    QString cacheDir() const { return cacheDir_; }

private:
    QString rawPath(const QString& url) const;

    mutable QCache<QString, QImage> images_;
    mutable int imageHits_ = 0;
    mutable int imageMisses_ = 0;
    mutable int rawHits_ = 0;
    mutable int rawMisses_ = 0;
    QString cacheDir_;
};

}  // namespace jmnext::qt
