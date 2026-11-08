#include "qml/JmBackend.h"

#include <QMetaObject>
#include <QThread>

#include "qt/JmWorker.h"
#include "core/FavoriteStore.h"
#include "core/HistoryStore.h"

namespace jmnext::qt {

JmBackend::JmBackend(QObject* parent) : QObject(parent) {
    thread_ = new QThread(this);
    worker_ = new JmWorker();
    worker_->moveToThread(thread_);
    connect(thread_, &QThread::finished, worker_, &QObject::deleteLater);
    // 把 worker 的信号转出去（跨线程自动走队列连接）
    connect(worker_, &JmWorker::listReady, this, &JmBackend::listReady);
    connect(worker_, &JmWorker::listAppended, this, &JmBackend::listAppended);
    connect(worker_, &JmWorker::hotTagsReady, this, &JmBackend::hotTagsReady);
    connect(worker_, &JmWorker::categoryReady, this, &JmBackend::categoryReady);
    connect(worker_, &JmWorker::pagedReady, this, &JmBackend::pagedReady);
    connect(worker_, &JmWorker::loginResult, this, &JmBackend::loginResult);
    connect(worker_, &JmWorker::actionDone, this, &JmBackend::actionDone);
    connect(worker_, &JmWorker::textReady, this, &JmBackend::textReady);
    connect(worker_, &JmWorker::albumReady, this, &JmBackend::albumReady);
    connect(worker_, &JmWorker::albumCoverReady, this, &JmBackend::albumCoverReady);
    connect(worker_, &JmWorker::chaptersReady, this, &JmBackend::chaptersReady);
    connect(worker_, &JmWorker::pageReady, this, &JmBackend::pageReady);
    connect(worker_, &JmWorker::previewReady, this, &JmBackend::previewReady);
    connect(worker_, &JmWorker::coverReady, this, &JmBackend::coverReady);
    connect(worker_, &JmWorker::coverUrlReady, this, &JmBackend::coverUrlReady);
    connect(worker_, &JmWorker::cacheStats, this, &JmBackend::cacheStats);
    connect(worker_, &JmWorker::status, this, &JmBackend::status);
    connect(worker_, &JmWorker::failed, this, &JmBackend::failed);
    thread_->start();
}

JmBackend::~JmBackend() {
    if (thread_) {
        thread_->quit();
        thread_->wait(5000);
    }
}

template <typename... Args>
void JmBackend::invoke(const char* method, Args&&... args) {
    if (!worker_) return;
    QMetaObject::invokeMethod(worker_, method, Qt::QueuedConnection, std::forward<Args>(args)...);
}

void JmBackend::loadList() { invoke("loadList"); }
void JmBackend::loadMore() { invoke("loadMore"); }
void JmBackend::loadHotTags() { invoke("loadHotTags"); }
void JmBackend::categoryFilter(const QString& c, int page) { invoke("categoryFilter", Q_ARG(QString, c), Q_ARG(int, page)); }
void JmBackend::login(const QString& u, const QString& p) { invoke("login", Q_ARG(QString, u), Q_ARG(QString, p)); }
void JmBackend::logout() { invoke("logout"); }
void JmBackend::fetch(const QString& tag, const QString& path, const QString& query) {
    invoke("fetch", Q_ARG(QString, tag), Q_ARG(QString, path), Q_ARG(QString, query));
}

void JmBackend::action(const QString& tag, const QString& path, const QString& form) {
    invoke("action", Q_ARG(QString, tag), Q_ARG(QString, path), Q_ARG(QString, form));
}

void JmBackend::loadPaged(const QString& tag, const QString& path, const QString& query) {
    invoke("loadPaged", Q_ARG(QString, tag), Q_ARG(QString, path), Q_ARG(QString, query));
}
void JmBackend::searchOrdered(const QString& w, int p, const QString& o) { invoke("searchOrdered", Q_ARG(QString, w), Q_ARG(int, p), Q_ARG(QString, o)); }
void JmBackend::search(const QString& word, int page) { invoke("search", Q_ARG(QString, word), Q_ARG(int, page)); }
void JmBackend::loadAlbum(const QString& aid) {
    currentAid_ = aid;
    emit currentAidChanged();
    invoke("loadAlbum", Q_ARG(QString, aid));
}
void JmBackend::openChapterId(const QString& chapterId, int page) {
    invoke("openChapterId", Q_ARG(QString, chapterId), Q_ARG(int, page));
}

void JmBackend::openChapter(const QString& aid, int page) { invoke("openChapter", Q_ARG(QString, aid), Q_ARG(int, page)); }
void JmBackend::step(int delta) { invoke("step", Q_ARG(int, delta)); }
void JmBackend::setBlockWords(const QStringList& words) { invoke("setBlockWords", Q_ARG(QStringList, words)); }
void JmBackend::setTwoPage(bool on) { invoke("setTwoPage", Q_ARG(bool, on)); }
void JmBackend::loadCovers(int n) { invoke("loadCovers", Q_ARG(int, n)); }
void JmBackend::reportCacheStats() { invoke("reportCacheStats"); }

// ── 本地阅读历史 ────────────────────────────────────────────────
QVariantList JmBackend::history() const {
    QVariantList out;
    for (const auto& e : jmnext::core::HistoryStore::instance().list()) {
        QVariantMap m;
        m.insert(QStringLiteral("aid"), e.aid);
        m.insert(QStringLiteral("title"), e.title);
        m.insert(QStringLiteral("ts"), static_cast<qlonglong>(e.timestamp));
        out.append(m);
    }
    return out;
}

void JmBackend::clearHistory() {
    jmnext::core::HistoryStore::instance().clear();
    emit historyChanged();
}

void JmBackend::rememberHistory(const QString& aid, const QString& title) {
    jmnext::core::HistoryStore::instance().remember(aid, title);
    emit historyChanged();
}

// ── 本地收藏 ────────────────────────────────────────────────────
QVariantList JmBackend::favorites() const {
    QVariantList out;
    for (const auto& e : jmnext::core::FavoriteStore::instance().list()) {
        QVariantMap m;
        m.insert(QStringLiteral("aid"), e.aid);
        m.insert(QStringLiteral("title"), e.title);
        m.insert(QStringLiteral("folder"), e.folder);
        m.insert(QStringLiteral("ts"), static_cast<qlonglong>(e.timestamp));
        out.append(m);
    }
    return out;
}

void JmBackend::rememberFavorite(const QString& aid, const QString& title, const QString& folder) {
    jmnext::core::FavoriteStore::instance().remember(aid, title, folder);
    emit favoritesChanged();
}

void JmBackend::removeFavorite(const QString& aid) {
    jmnext::core::FavoriteStore::instance().remove(aid);
    emit favoritesChanged();
}

void JmBackend::clearFavorites() {
    jmnext::core::FavoriteStore::instance().clear();
    emit favoritesChanged();
}
}  // namespace jmnext::qt
