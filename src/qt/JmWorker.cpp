#include "qt/JmWorker.h"

#include "qt/JmClient.h"

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
    for (const auto& e : *list) {
        const core::ListItem item{e.name, e.author, e.categoryTitle, e.categorySubTitle};
        if (rules.hides(item)) { ++hidden; continue; }
        titles << QStringLiteral("%1\n   %2　[%3]")
                      .arg(QString::fromStdString(e.name))
                      .arg(QString::fromStdString(e.author))
                      .arg(QString::fromStdString(e.categoryTitle));
        ids << QString::fromStdString(e.id);
    }
    if (hidden > 0)
        emit status(QStringLiteral("已按屏蔽规则隐藏 %1 条（关键词 %2 个）").arg(hidden).arg(blockWords_.size()));
    pendingIds_ = ids;
    emit listReady(titles, ids);
}

void JmWorker::loadCovers(int n) {
    if (pendingIds_.isEmpty()) { emit failed(QStringLiteral("还没有列表，无法取封面")); return; }
    auto list = client().latest(0);        // 封面地址参数（id/image/update_at）来自同一次列表
    if (!list) { emit failed(QStringLiteral("取封面失败：%1").arg(QString::fromStdString(client().lastError()))); return; }
    const int limit = qMin(n, static_cast<int>(list->size()));
    for (int i = 0; i < limit; ++i) {
        auto img = client().cover((*list)[static_cast<std::size_t>(i)]);
        if (!img) continue;                // 单张失败不影响其余（下一轮加载时会补）
        emit coverReady(i, *img);
    }
}

void JmWorker::openChapter(const QString& aid, int page) {
    if (!ensureStarted()) return;
    currentAid_ = aid;
    auto al = client().album(aid.toStdString());
    if (!al) { emit failed(QStringLiteral("详情失败：%1").arg(QString::fromStdString(client().lastError()))); return; }
    emit status(QStringLiteral("详情：%1（标签 %2，章节 %3）")
                    .arg(QString::fromStdString(al->name))
                    .arg(al->tags.size())
                    .arg(al->series.size()));
    if (al->series.empty()) { emit failed(QStringLiteral("该作品没有章节")); return; }

    // 把章节列表交给界面（章节选择器）
    {
        QStringList names, ids;
        for (const auto& se : al->series) {
            names << (se.name.empty() ? QStringLiteral("(未命名)") : QString::fromStdString(se.name));
            ids << QString::fromStdString(se.id);
        }
        emit chaptersReady(names, ids);
    }

    auto ch = client().chapter(al->series.front().id);
    if (!ch) { emit failed(QStringLiteral("章节失败：%1").arg(QString::fromStdString(client().lastError()))); return; }
    chapter_ = *ch;
    pageIndex_ = -1;                       // 尚未显示任何一页
    emit status(QStringLiteral("章节：%1 共 %2 页，scramble_id=%3")
                    .arg(QString::fromStdString(chapter_.id))
                    .arg(chapter_.totalPage)
                    .arg(chapter_.scrambleId));
    showPageAt(page);                      // 修正后的语义：直接跳到目标页（原来用 step 做相对移动会差一页）
}

void JmWorker::step(int delta) {
    if (chapter_.images.empty()) { emit failed(QStringLiteral("尚未载入章节")); return; }
    const int base = pageIndex_ < 0 ? 0 : pageIndex_;   // 未显示任何页时从第 0 页算起
    const int next = base + delta;
    if (delta != 0 && pageIndex_ < 0) { showPageAt(0); return; }
    showPageAt(next);
}

void JmWorker::showPageAt(int index) {
    if (chapter_.images.empty()) { emit failed(QStringLiteral("章节没有图片")); return; }
    if (index < 0 || index >= static_cast<int>(chapter_.images.size())) {
        emit status(QStringLiteral("页码越界：%1（共 %2 页）").arg(index + 1).arg(chapter_.images.size()));
        return;
    }
    const auto& p = chapter_.images[static_cast<std::size_t>(index)];
    auto img = client().pageImage(p.url, std::atoi(chapter_.id.c_str()), chapter_.scrambleId);
    if (!img) { emit failed(QStringLiteral("取图失败：%1").arg(QString::fromStdString(client().lastError()))); return; }
    pageIndex_ = index;
    emit pageReady(*img, QStringLiteral("第 %1/%2 页 %3x%4")
                              .arg(index + 1)
                              .arg(chapter_.images.size())
                              .arg(img->width())
                              .arg(img->height()));
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
