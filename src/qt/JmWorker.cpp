#include "qt/JmWorker.h"

#include "qt/JmClient.h"

#include <QDir>
#include <QStandardPaths>

#include <cstdlib>

namespace jmnext::qt {

JmWorker::~JmWorker() = default;

JmClient& JmWorker::client() {
    if (!client_) client_ = std::make_unique<JmClient>();
    return *client_;
}

bool JmWorker::ensureStarted() {
    if (started_) return true;
    if (!client().bootstrap()) {
        emit failed(QStringLiteral("主机发现失败：%1").arg(QString::fromStdString(client().lastError())));
        return false;
    }
    started_ = true;
    emit status(QStringLiteral("主机：%1").arg(QString::fromStdString(client().host())));
    return true;
}

void JmWorker::loadList() {
    lastQuery_.clear();          // 回到首页列表
    if (!ensureStarted()) return;
    auto list = client().latest(0);
    if (!list) {
        emit failed(QStringLiteral("列表失败：%1").arg(QString::fromStdString(client().lastError())));
        return;
    }
    // 屏蔽过滤：关键词按**子串**匹配作品名/作者，分类按**精确**匹配（语义见 core/BlockRules）
    core::BlockRules rules;
    rules.words = blockWords_;

    QStringList titles, ids;
    int hidden = 0;
    std::vector<core::ListEntry> kept;
    for (const auto& e : *list) {
        const core::ListItem item{e.name, e.author, e.categoryTitle, e.categorySubTitle};
        if (rules.hides(item)) { ++hidden; continue; }
        kept.push_back(e);
        titles << QStringLiteral("%1\n   %2　[%3]")
                      .arg(QString::fromStdString(e.name))
                      .arg(QString::fromStdString(e.author))
                      .arg(QString::fromStdString(e.categoryTitle));
        ids << QString::fromStdString(e.id);
    }
    if (hidden > 0)
        emit status(QStringLiteral("已按屏蔽规则隐藏 %1 条（关键词 %2 个）").arg(hidden).arg(blockWords_.size()));
    currentEntries_ = kept;
    pendingIds_ = ids;
    page_ = 0;
    emit listReady(titles, ids);
}

void JmWorker::search(const QString& word, int page) {
    lastQuery_ = word;           // 记住当前是搜索结果，"加载更多"要翻搜索的第 N 页
    if (!ensureStarted()) return;
    emit status(QStringLiteral("正在搜索：%1").arg(word));
    auto list = client().search(word.toStdString(), page);
    if (!list) {
        emit failed(QStringLiteral("搜索失败：%1").arg(QString::fromStdString(client().lastError())));
        return;
    }
    core::BlockRules rules;
    rules.words = blockWords_;
    QStringList titles, ids;
    int hidden = 0;
    std::vector<core::ListEntry> kept;
    for (const auto& e : *list) {
        const core::ListItem item{e.name, e.author, e.categoryTitle, e.categorySubTitle};
        if (rules.hides(item)) { ++hidden; continue; }
        kept.push_back(e);
        titles << QStringLiteral("%1\n   %2　[%3]")
                      .arg(QString::fromStdString(e.name))
                      .arg(QString::fromStdString(e.author))
                      .arg(QString::fromStdString(e.categoryTitle));
        ids << QString::fromStdString(e.id);
    }
    if (hidden > 0) emit status(QStringLiteral("搜索结果里按屏蔽规则隐藏 %1 条").arg(hidden));
    pendingIds_ = ids;
    page_ = 0;
    currentEntries_ = kept;
    emit listReady(titles, ids);          // 界面收到后替换列表（与"加载首页列表"同一套机制）
}

void JmWorker::loadCovers(int n) {
    if (currentEntries_.empty()) { emit failed(QStringLiteral("还没有列表，无法取封面")); return; }
    // 用"当前列表"的条目（含搜索结果），而不是重新拉一次首页列表：
    // 以前重拉首页列表，导致（1）搜索结果会配到首页作品的封面；（2）屏蔽隐藏条目后下标错位；
    // （3）白费一次网络往返。现在下标与界面显示项严格一致。
    const int limit = qMin(n, static_cast<int>(currentEntries_.size()));
    for (int i = 0; i < limit; ++i) {
        auto img = client().cover(currentEntries_[static_cast<std::size_t>(i)]);
        if (!img) continue;
        emit coverReady(i, *img);
    }
}

void JmWorker::openChapter(const QString& aid, int page) {
    emit status(QStringLiteral("正在加载…"));
    if (!ensureStarted()) return;
    currentAid_ = aid;

    // 首图优先：详情（标签 + 章节列表）与封面挪到首图**之后**再取，不再挡在阅读前面。
    // 依据：实测 series[0].id 与作品 id 相同，所以第一话可以直接用 aid 取；
    // 万一某个作品不同，下面的 directOk==false 分支会回退到"先详情、再用 series[0].id"的老路。
    bool directOk = false;

    // 1) 进度恢复优先（不需要详情）
    if (page == 0) {
        if (auto saved = core::loadReadProgress(progressPath()); saved && saved->aid == aid.toStdString()) {
            if (auto ch = client().chapter(saved->chapterId)) {
                chapter_ = *ch;
                pageIndex_ = -1;
                directOk = true;
                emit status(QStringLiteral("恢复上次进度：第 %1 页（作品 %2）").arg(saved->page + 1).arg(aid));
                showPageAt(saved->page);
            }
        }
    }

    // 2) 没有进度就用作品 id 当第一话
    if (!directOk) {
        if (auto ch = client().chapter(aid.toStdString())) {
            chapter_ = *ch;
            pageIndex_ = -1;
            directOk = true;
            emit status(QStringLiteral("章节：%1 共 %2 页，scramble_id=%3")
                            .arg(QString::fromStdString(chapter_.id))
                            .arg(chapter_.totalPage)
                            .arg(chapter_.scrambleId));
            showPageAt(page);
        }
    }

    // 3) 详情与章节列表随后再取（界面上的标签与章节列表）
    auto al = client().album(aid.toStdString());
    if (!al) {
        if (!directOk) emit failed(QStringLiteral("详情失败：%1").arg(QString::fromStdString(client().lastError())));
        else emit status(QStringLiteral("详情获取失败（阅读不受影响）：%1")
                             .arg(QString::fromStdString(client().lastError())));
        return;
    }
    emit status(QStringLiteral("详情：%1（标签 %2，章节 %3）")
                    .arg(QString::fromStdString(al->name))
                    .arg(al->tags.size())
                    .arg(al->series.size()));

    {
        QStringList tags;
        for (const auto& t : al->tags) tags << QString::fromStdString(t);
        emit albumReady(QString::fromStdString(al->name), QString::fromStdString(al->author), tags);
        fetchAlbumCover(aid);
    }
    {
        QStringList names, ids;
        for (const auto& se : al->series) {
            names << (se.name.empty() ? QStringLiteral("(未命名)") : QString::fromStdString(se.name));
            ids << QString::fromStdString(se.id);
        }
        emit chaptersReady(names, ids);
    }

    // 4) 回退路径：直接用 aid 取不到章节时，才按老办法用章节表里的第一项
    if (!directOk) {
        if (al->series.empty()) { emit failed(QStringLiteral("该作品没有章节")); return; }
        auto ch = client().chapter(al->series.front().id);
        if (!ch) { emit failed(QStringLiteral("章节失败：%1").arg(QString::fromStdString(client().lastError()))); return; }
        chapter_ = *ch;
        pageIndex_ = -1;
        emit status(QStringLiteral("章节：%1 共 %2 页，scramble_id=%3")
                        .arg(QString::fromStdString(chapter_.id))
                        .arg(chapter_.totalPage)
                        .arg(chapter_.scrambleId));
        showPageAt(page);
    }
}

void JmWorker::step(int delta) {
    if (chapter_.images.empty()) { emit failed(QStringLiteral("尚未载入章节")); return; }
    const int base = pageIndex_ < 0 ? 0 : pageIndex_;   // 未显示任何页时从第 0 页算起
    const int next = base + delta;
    if (delta != 0 && pageIndex_ < 0) { showPageAt(0); return; }
    showPageAt(next);
}

void JmWorker::showPageAt(int index) {
    emit status(QStringLiteral("正在加载第 %1 页…").arg(index + 1));   // 让界面先给出反馈，而不是像卡住
    if (chapter_.images.empty()) { emit failed(QStringLiteral("章节没有图片")); return; }
    if (index < 0 || index >= static_cast<int>(chapter_.images.size())) {
        emit status(QStringLiteral("页码越界：%1（共 %2 页）").arg(index + 1).arg(chapter_.images.size()));
        return;
    }
    const auto& p = chapter_.images[static_cast<std::size_t>(index)];
    auto img = client().pageImage(p.url, std::atoi(chapter_.id.c_str()), chapter_.scrambleId);
    if (!img) { emit failed(QStringLiteral("取图失败：%1").arg(QString::fromStdString(client().lastError()))); return; }
    pageIndex_ = index;
    const int pct = chapter_.images.empty()
                        ? 0
                        : static_cast<int>((index + 1) * 100 / static_cast<int>(chapter_.images.size()));
    emit pageReady(*img, QStringLiteral("第 %1/%2 页 %3x%4 · %5%")
                              .arg(index + 1)
                              .arg(chapter_.images.size())
                              .arg(img->width())
                              .arg(img->height())
                              .arg(pct));
    saveProgressNow();          // 记录进度（下次打开同一作品接着读）
    // 预取下一页与上一页：读漫画大部分时间在往后翻，回翻也是常见动作
    prefetch(index + 1);
    prefetch(index - 1);
}

}  // namespace jmnext::qt

namespace jmnext::qt {

void JmWorker::setBlockWords(const QStringList& words) {
    blockWords_.clear();
    for (const auto& w : words) {
        const std::string s = w.trimmed().toStdString();
        if (!s.empty()) blockWords_.push_back(s);
    }
    emit status(QStringLiteral("屏蔽关键词已设为 %1 个").arg(blockWords_.size()));
}

}  // namespace jmnext::qt

namespace jmnext::qt {

void JmWorker::openChapterId(const QString& chapterId, int page) {
    if (!ensureStarted()) return;
    auto ch = client().chapter(chapterId.toStdString());
    if (!ch) { emit failed(QStringLiteral("章节失败：%1").arg(QString::fromStdString(client().lastError()))); return; }
    chapter_ = *ch;
    pageIndex_ = -1;
    emit status(QStringLiteral("章节：%1 共 %2 页，scramble_id=%3")
                    .arg(QString::fromStdString(chapter_.id))
                    .arg(chapter_.totalPage)
                    .arg(chapter_.scrambleId));
    showPageAt(page);
}

}  // namespace jmnext::qt

namespace jmnext::qt {

void JmWorker::prefetch(int index) {
    if (chapter_.images.empty()) return;
    if (index < 0 || index >= static_cast<int>(chapter_.images.size())) return;
    const auto& p = chapter_.images[static_cast<std::size_t>(index)];
    // 复用与正式取图完全相同的路径：命中缓存就直接返回，未命中才下载并解码
    auto img = client().pageImage(p.url, std::atoi(chapter_.id.c_str()), chapter_.scrambleId);
    if (img) emit status(QStringLiteral("已预取第 %1 页（翻页时应当无需等待）").arg(index + 1));
}

}  // namespace jmnext::qt

namespace jmnext::qt {

void JmWorker::reportCacheStats() {
    const auto& c = client().cache();
    emit cacheStats(c.imageHits(), c.imageMisses(), c.rawHits(), c.rawMisses());
}

}  // namespace jmnext::qt

namespace jmnext::qt {

void JmWorker::loadMore() {
    if (!ensureStarted()) return;
    const int next = page_ + 1;
    // 加载更多要跟着当前列表走：搜索结果显示时翻搜索的第 N 页，否则翻首页列表。
    // （此前一律翻首页列表，于是搜索后点加载更多会把首页内容混进搜索结果里。）
    const bool inSearch = !lastQuery_.isEmpty();
    auto list = inSearch ? client().search(lastQuery_.toStdString(), next) : client().latest(next);
    if (!list) {
        emit failed(QStringLiteral("加载第 %1 页失败：%2").arg(next + 1).arg(QString::fromStdString(client().lastError())));
        return;
    }
    if (list->empty()) {
        emit status(QStringLiteral("已到末页（当前共加载 %1 页）").arg(page_ + 1));
        return;
    }
    core::BlockRules rules;
    rules.words = blockWords_;
    QStringList titles, ids;
    int hidden = 0;
    std::vector<core::ListEntry> kept;
    for (const auto& e : *list) {
        const core::ListItem item{e.name, e.author, e.categoryTitle, e.categorySubTitle};
        if (rules.hides(item)) { ++hidden; continue; }
        kept.push_back(e);
        titles << QStringLiteral("%1\n   %2　[%3]")
                      .arg(QString::fromStdString(e.name))
                      .arg(QString::fromStdString(e.author))
                      .arg(QString::fromStdString(e.categoryTitle));
        ids << QString::fromStdString(e.id);
    }
    page_ = next;
    currentEntries_.insert(currentEntries_.end(), kept.begin(), kept.end());
    pendingIds_ += ids;
    emit status(QStringLiteral("第 %1 页：新增 %2 条（隐藏 %3 条）").arg(next + 1).arg(titles.size()).arg(hidden));
    emit listAppended(titles, ids);
}

}  // namespace jmnext::qt

namespace jmnext::qt {

void JmWorker::fetchAlbumCover(const QString& id) {
    if (!ensureStarted()) return;
    core::ListEntry entry;          // 只用 id 走模板路径（服务端列表里的 image 常为空）
    entry.id = id.toStdString();
    auto img = client().cover(entry);
    if (!img) { emit status(QStringLiteral("封面未取到：%1").arg(QString::fromStdString(client().lastError()))); return; }
    emit albumCoverReady(*img);
    emit status(QStringLiteral("封面已加载 %1x%2").arg(img->width()).arg(img->height()));
}

}  // namespace jmnext::qt

namespace jmnext::qt {

std::string JmWorker::progressPath() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return (dir + "/progress.txt").toStdString();
}

void JmWorker::saveProgressNow() {
    if (pageIndex_ < 0 || chapter_.id.empty()) return;
    const core::ReadProgress p{currentAid_.toStdString(), chapter_.id, pageIndex_};
    if (core::saveReadProgress(progressPath(), p))
        emit status(QStringLiteral("已记录进度：第 %1 页（作品 %2）").arg(pageIndex_ + 1).arg(currentAid_));
}

}  // namespace jmnext::qt
