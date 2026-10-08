#include "qml/JmBackend.h"

#include <QMetaObject>
#include <QThread>

#include "qt/JmWorker.h"

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
    connect(worker_, &JmWorker::albumReady, this, &JmBackend::albumReady);
    connect(worker_, &JmWorker::albumCoverReady, this, &JmBackend::albumCoverReady);
    connect(worker_, &JmWorker::chaptersReady, this, &JmBackend::chaptersReady);
    connect(worker_, &JmWorker::pageReady, this, &JmBackend::pageReady);
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

}  // namespace jmnext::qt
