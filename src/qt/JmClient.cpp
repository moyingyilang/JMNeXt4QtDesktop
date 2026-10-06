#include "qt/JmClient.h"

#include "core/JmCore.h"
#include "qt/ImageCache.h"
#include "qt/ImageCodec.h"

#include <QDateTime>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QImage>

#include <vector>

namespace jmnext::qt {
using namespace jmnext::core;

namespace {
QImage toQImage(const std::vector<uint32_t>& px, int w, int h) {
    QImage img(w, h, QImage::Format_ARGB32);
    for (int y = 0; y < h; ++y) {
        auto* line = reinterpret_cast<uint32_t*>(img.scanLine(y));
        for (int x = 0; x < w; ++x) line[x] = px[static_cast<std::size_t>(y) * w + x];
    }
    return img;
}
}  // namespace

bool JmClient::bootstrap() {
    lastError_.clear();
    session_.refresh(QDateTime::currentSecsSinceEpoch());   // 用真实当前时间（服务端校验时效）

    // 先试**缓存的主机**：主机相当稳定，缓存命中就省掉"两个入口请求 + 解密"这一整轮往返
    // （实测首屏约 3.7 秒，其中主机发现占一部分）。缓存失效时会自然回退到下面的发现流程。
    const QString cached = cachedHost();
    if (!cached.isEmpty()) {
        session_.useHost(cached.toStdString());
        host_ = session_.apiBaseUrl();
        bootstrapped_ = true;
        return true;
    }

    auto base = discoverHost(session_, http_, [](const std::vector<std::string>& hosts) {
        return hosts.empty() ? std::optional<std::string>{} : std::optional<std::string>(hosts.front());
    });
    if (!base) {
        lastError_ = "主机发现失败：" + http_.lastError();
        return false;
    }
    host_ = *base;
    saveCachedHost(QString::fromStdString(host_));   // 记下来，下次直接用
    bootstrapped_ = true;
    return true;
}

QString JmClient::cachedHostPath() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/host.txt";
}

QString JmClient::cachedHost() const {
    QFile f(cachedHostPath());
    if (!f.open(QIODevice::ReadOnly)) return {};
    const QString s = QString::fromUtf8(f.readAll()).trimmed();
    if (s.isEmpty() || !s.startsWith("http")) return {};     // 不合法就当作没有缓存
    return s;
}

void JmClient::saveCachedHost(const QString& host) const {
    QFile f(cachedHostPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write(host.toUtf8());
}

std::optional<std::vector<ListEntry>> JmClient::latest(int page) {
    if (!bootstrapped_) { lastError_ = "尚未初始化主机"; return std::nullopt; }
    JmApi api(session_, http_);
    auto r = api.request("latest", "page=" + std::to_string(page));
    if (!r) { lastError_ = api.lastError(); return std::nullopt; }
    auto parsed = parseLatestList(r->text);
    if (!parsed) lastError_ = "列表解析失败";
    return parsed;
}

std::optional<std::vector<ListEntry>> JmClient::search(const std::string& word, int page) {
    if (!bootstrapped_) { lastError_ = "尚未初始化主机"; return std::nullopt; }
    JmApi api(session_, http_);
    auto r = api.request("search", "search_query=" + word + "&page=" + std::to_string(page));
    if (!r) { lastError_ = api.lastError(); return std::nullopt; }
    auto parsed = parseSearchPage(r->text);
    if (!parsed) { lastError_ = "搜索解析失败"; return std::nullopt; }
    return parsed->items;
}

std::optional<AlbumInfo> JmClient::album(const std::string& id) {
    if (!bootstrapped_) { lastError_ = "尚未初始化主机"; return std::nullopt; }
    JmApi api(session_, http_);
    auto r = api.request("album", "id=" + id);
    if (!r) { lastError_ = api.lastError(); return std::nullopt; }
    auto parsed = parseAlbum(r->text);
    if (!parsed) lastError_ = "详情解析失败";
    return parsed;
}

std::optional<ChapterImages> JmClient::chapter(const std::string& id) {
    if (!bootstrapped_) { lastError_ = "尚未初始化主机"; return std::nullopt; }
    JmApi api(session_, http_);
    auto r = api.request("comic_read", "id=" + id);
    if (!r) { lastError_ = api.lastError(); return std::nullopt; }
    auto parsed = parseChapterImages(r->text);
    if (!parsed) lastError_ = "章节解析失败";
    return parsed;
}

std::optional<QImage> JmClient::pageImage(const std::string& url, int aid, int scrambleId) {
    const QString key = QString::fromStdString(url) + QStringLiteral("#") + QString::number(aid) +
                        QStringLiteral(":") + QString::number(scrambleId);
    // 1) 内存缓存：已解码且已还原的成品（image() 内部会正确计入命中/未命中）
    if (const QImage cached = cache_.image(key); !cached.isNull()) return cached;

    // 2) 磁盘缓存：原始字节（未还原，所以下面仍要解码 + 还原一次）
    const QString qurl = QString::fromStdString(url);
    QByteArray bytes;
    if (cache_.hasRaw(qurl)) {
        bytes = cache_.raw(qurl);
    } else {
        const auto resp = http_.get(url, {});          // 图床不加密、也不需要凭证
        if (!resp.ok()) { lastError_ = "图片下载失败：" + http_.lastError(); return std::nullopt; }
        bytes = QByteArray(resp.body.data(), static_cast<int>(resp.body.size()));
        cache_.putRaw(qurl, bytes);
    }

    // 落盘交给 Qt 解码（复用已测过的 ImageCodec 路径）
    const std::string tmp = cache_.cacheDir().toStdString() + "/page.tmp";
    {
        FILE* f = std::fopen(tmp.c_str(), "wb");
        if (!f) { lastError_ = "无法写临时文件"; return std::nullopt; }
        std::fwrite(bytes.constData(), 1, static_cast<std::size_t>(bytes.size()), f);
        std::fclose(f);
    }
    auto img = loadImage(tmp);
    if (!img) { lastError_ = "图片解码失败（是否缺少 webp 插件？）"; return std::nullopt; }

    QImage result;
    if (!needsUnscrambleFor(aid, scrambleId, url)) {
        result = toQImage(img->pixels, img->width, img->height);
    } else {
        const auto out = unscramblePage(img->pixels, img->width, img->height, aid, "1");
        if (out.empty()) { lastError_ = "切片还原失败"; return std::nullopt; }
        result = toQImage(out, img->width, img->height);
    }
    cache_.putImage(key, result);        // 3) 成品入内存缓存
    return result;
}

}  // namespace jmnext::qt

namespace jmnext::qt {

std::optional<QImage> JmClient::cover(const core::ListEntry& entry) {
    const std::string base = session_.imageBaseForCover();   // 图床缺省时用 API 主机（骨架阶段够用）
    auto url = core::coverUrl(entry.id, entry.image, entry.updateAt, base);
    if (!url) { lastError_ = "无法拼出封面地址"; return std::nullopt; }
    // 封面不需要还原，所以 scrambleId 传 0 且 aid 传 0（needsUnscramble 会判为 false 之外的路径）
    // —— 这里直接走"下载 + 解码"，不走页面图那条带还原的路径
    const QString key = QString::fromStdString(*url) + QStringLiteral("#cover");
    if (const QImage cached = cache_.image(key); !cached.isNull()) return cached;

    const QString qurl = QString::fromStdString(*url);
    QByteArray bytes;
    if (cache_.hasRaw(qurl)) {
        bytes = cache_.raw(qurl);
    } else {
        const auto resp = http_.get(*url, {});
        if (!resp.ok()) { lastError_ = "封面下载失败：" + http_.lastError(); return std::nullopt; }
        bytes = QByteArray(resp.body.data(), static_cast<int>(resp.body.size()));
        cache_.putRaw(qurl, bytes);
    }
    const std::string tmp = cache_.cacheDir().toStdString() + "/cover.tmp";
    {
        FILE* f = std::fopen(tmp.c_str(), "wb");
        if (!f) { lastError_ = "无法写临时文件"; return std::nullopt; }
        std::fwrite(bytes.constData(), 1, static_cast<std::size_t>(bytes.size()), f);
        std::fclose(f);
    }
    auto img = loadImage(tmp);
    if (!img) { lastError_ = "封面解码失败"; return std::nullopt; }
    const QImage qimg = toQImage(img->pixels, img->width, img->height);
    cache_.putImage(key, qimg);
    return qimg;
}

}  // namespace jmnext::qt
