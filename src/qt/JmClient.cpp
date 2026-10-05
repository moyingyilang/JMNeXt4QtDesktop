#include "qt/JmClient.h"

#include "core/HostDiscovery.h"
#include "core/JmApi.h"
#include "core/JmCrypto.h"
#include "core/UnscrambleApply.h"
#include "qt/ImageCodec.h"

#include <QDateTime>
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
    auto base = discoverHost(session_, http_, [](const std::vector<std::string>& hosts) {
        return hosts.empty() ? std::optional<std::string>{} : std::optional<std::string>(hosts.front());
    });
    if (!base) {
        lastError_ = "主机发现失败：" + http_.lastError();
        return false;
    }
    host_ = *base;
    bootstrapped_ = true;
    return true;
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
    const auto resp = http_.get(url, {});          // 图床不加密、也不需要凭证
    if (!resp.ok()) { lastError_ = "图片下载失败：" + http_.lastError(); return std::nullopt; }

    // 落盘再接 Qt 解码（复用已测过的 ImageCodec 路径）
    const std::string tmp = "/tmp/jmnext-page.tmp";
    {
        FILE* f = std::fopen(tmp.c_str(), "wb");
        if (!f) { lastError_ = "无法写临时文件"; return std::nullopt; }
        std::fwrite(resp.body.data(), 1, resp.body.size(), f);
        std::fclose(f);
    }
    auto img = loadImage(tmp);
    if (!img) { lastError_ = "图片解码失败（是否缺少 webp 插件？）"; return std::nullopt; }

    if (!needsUnscrambleFor(aid, scrambleId, url)) {
        return toQImage(img->pixels, img->width, img->height);
    }
    const auto out = unscramblePage(img->pixels, img->width, img->height, aid, "1");
    if (out.empty()) { lastError_ = "切片还原失败"; return std::nullopt; }
    return toQImage(out, img->width, img->height);
}

}  // namespace jmnext::qt
