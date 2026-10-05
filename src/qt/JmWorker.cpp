#include "qt/JmWorker.h"

#include "qt/JmClient.h"

namespace jmnext::qt {

JmWorker::~JmWorker() = default;

JmClient& JmWorker::client() {
    if (!client_) client_ = std::make_unique<JmClient>();
    return *client_;
}

void JmWorker::loadList() {
    if (!client().bootstrap()) {
        emit failed(QStringLiteral("主机发现失败：%1").arg(QString::fromStdString(client().lastError())));
        return;
    }
    emit status(QStringLiteral("主机：%1").arg(QString::fromStdString(client().host())));
    auto list = client().latest(0);
    if (!list) {
        emit failed(QStringLiteral("列表失败：%1").arg(QString::fromStdString(client().lastError())));
        return;
    }
    QStringList titles, ids;
    for (const auto& e : *list) {
        titles << QStringLiteral("%1\n   %2　[%3]")
                      .arg(QString::fromStdString(e.name))
                      .arg(QString::fromStdString(e.author))
                      .arg(QString::fromStdString(e.categoryTitle));
        ids << QString::fromStdString(e.id);
    }
    pendingIds_ = ids;
    emit listReady(titles, ids);
}

void JmWorker::loadCovers(int n) {
    if (pendingIds_.isEmpty()) { emit failed(QStringLiteral("还没有列表，无法取封面")); return; }
    const int limit = qMin(n, pendingIds_.size());
    for (int i = 0; i < limit; ++i) {
        // 复用 JmClient 的列表结果：这里只取封面，参数从最近一次列表里拿
        // （骨架阶段：为简单起见重新拉一次列表，封面地址参数一致）
    }
    auto list = client().latest(0);
    if (!list) { emit failed(QStringLiteral("取封面失败：%1").arg(QString::fromStdString(client().lastError()))); return; }
    for (int i = 0; i < limit && i < static_cast<int>(list->size()); ++i) {
        auto img = client().cover((*list)[static_cast<std::size_t>(i)]);
        if (!img) continue;
        emit coverReady(i, *img);
    }
}

void JmWorker::openChapter(const QString& aid, int page) {
    currentAid_ = aid;
    auto al = client().album(aid.toStdString());
    if (!al) { emit failed(QStringLiteral("详情失败：%1").arg(QString::fromStdString(client().lastError()))); return; }
    emit status(QStringLiteral("详情：%1（标签 %2，章节 %3）")
                    .arg(QString::fromStdString(al->name))
                    .arg(al->tags.size())
                    .arg(al->series.size()));
    if (al->series.empty()) { emit failed(QStringLiteral("该作品没有章节")); return; }
    auto ch = client().chapter(al->series.front().id);
    if (!ch) { emit failed(QStringLiteral("章节失败：%1").arg(QString::fromStdString(client().lastError()))); return; }
    chapter_ = *ch;
    step(page - (pageIndex_ < 0 ? 0 : pageIndex_));      // 相对当前位置移动
}

void JmWorker::step(int delta) {
    if (chapter_.images.empty()) {
        // 还没有章节：openChapter 会先建立
        emit failed(QStringLiteral("尚未载入章节"));
        return;
    }
    const int next = pageIndex_ + delta;
    if (next < 0 || next >= static_cast<int>(chapter_.images.size())) {
        emit status(QStringLiteral("页码越界：%1（共 %2 页）").arg(next).arg(chapter_.images.size()));
        return;
    }
    const auto& p = chapter_.images[static_cast<std::size_t>(next)];
    auto img = client().pageImage(p.url, std::atoi(chapter_.id.c_str()), chapter_.scrambleId);
    if (!img) { emit failed(QStringLiteral("取图失败：%1").arg(QString::fromStdString(client().lastError()))); return; }
    pageIndex_ = next;
    emit pageReady(*img, QStringLiteral("第 %1/%2 页 %3x%4")
                              .arg(next + 1)
                              .arg(chapter_.images.size())
                              .arg(img->width())
                              .arg(img->height()));
}

}  // namespace jmnext::qt
