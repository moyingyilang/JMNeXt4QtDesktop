// QML 侧的后端桥：把共用的 JmWorker 放到工作线程，用 Q_INVOKABLE 暴露给 QML。
//
// 为什么这样做：JmClient 的方法都是同步的（一次列表/一页图可能几百毫秒到数秒），放界面线程会卡住；
// 而 JmWorker 的 QNetworkAccessManager 必须在**使用它的线程**里创建，所以由 worker 自己在工作线程里惰性创建。
// 这里只做"转发"，不复制任何网络/缓存逻辑 —— widget 与 QML 两个前端共用同一份实现。
#pragma once
#include <QObject>
#include <QStringList>

class QThread;

namespace jmnext::qt {

class JmWorker;

class JmBackend : public QObject {
    Q_OBJECT
public:
    explicit JmBackend(QObject* parent = nullptr);
    ~JmBackend() override;

    // 与 JmWorker 的槽一一对应；全部走队列连接（跨线程），不在界面线程里执行网络。
    Q_INVOKABLE void loadList();
    Q_INVOKABLE void loadMore();
    Q_INVOKABLE void search(const QString& word, int page = 1);
    Q_INVOKABLE void openChapter(const QString& aid, int page = 0);
    Q_INVOKABLE void loadAlbum(const QString& aid);
    Q_PROPERTY(QString currentAid READ currentAid NOTIFY currentAidChanged)
    Q_INVOKABLE void openChapterId(const QString& chapterId, int page = 0);
    Q_INVOKABLE void step(int delta);
    Q_INVOKABLE void setBlockWords(const QStringList& words);
    Q_INVOKABLE void setTwoPage(bool on);
    Q_INVOKABLE void loadCovers(int n);
    Q_INVOKABLE void reportCacheStats();

signals:
    void listReady(const QStringList& titles, const QStringList& ids);
    void listAppended(const QStringList& titles, const QStringList& ids);
    void albumReady(const QString& name, const QString& author, const QStringList& tags);
    void albumCoverReady(const QImage& image);
    void currentAidChanged();
    void chaptersReady(const QStringList& names, const QStringList& ids);
    void pageReady(const QImage& image, const QString& status);
    void coverReady(int index, const QImage& image);
    void coverUrlReady(int index, const QString& url);
    void cacheStats(int imageHits, int imageMisses, int rawHits, int rawMisses);
    void status(const QString& text);
    void failed(const QString& text);

private:
    QThread* thread_ = nullptr;
    JmWorker* worker_ = nullptr;
    QString currentAid() const { return currentAid_; }
    QString currentAid_;
    template <typename... Args>
    void invoke(const char* method, Args&&... args);
};

}  // namespace jmnext::qt
