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
    Q_INVOKABLE void searchOrdered(const QString& word, int page, const QString& order);
    Q_INVOKABLE void openChapter(const QString& aid, int page = 0);
    Q_INVOKABLE void loadAlbum(const QString& aid);
    Q_PROPERTY(QString currentAid READ currentAid NOTIFY currentAidChanged)
    // 阅读页：页面图在 GUI 线程落成临时 PNG 后，通过这两个属性给 QML（带 NOTIFY，绑定可用）
    Q_PROPERTY(QString pagePath READ pagePath NOTIFY pageChanged)
    Q_PROPERTY(int pageSeq READ pageSeq NOTIFY pageChanged)
    // 双页模式的"右页"：与 pagePath 同一手法（预览图落盘后把路径给 QML）
    Q_PROPERTY(QString previewPath READ previewPath NOTIFY previewChanged)
    Q_PROPERTY(int previewSeq READ previewSeq NOTIFY previewChanged)
    Q_INVOKABLE void openChapterId(const QString& chapterId, int page = 0);
    Q_INVOKABLE void step(int delta);
    Q_INVOKABLE void setBlockWords(const QStringList& words);
    Q_INVOKABLE void setTwoPage(bool on);
    Q_INVOKABLE void loadCovers(int n);
    /// 热门标签（分类浏览页的数据源）
    Q_INVOKABLE void loadHotTags();
    /// 按分类标签筛选作品（第 51 轮）
    Q_INVOKABLE void categoryFilter(const QString& c, int page = 1);
    /// 通用分页列表（收藏/画师/标签/周更…共用）
    Q_INVOKABLE void loadPaged(const QString& tag, const QString& path, const QString& query);
    Q_INVOKABLE void login(const QString& username, const QString& password);
    Q_INVOKABLE void logout();
    /// 通用动作（收藏/点赞/追更/签到/评论）
    Q_INVOKABLE void action(const QString& tag, const QString& path, const QString& form);
    Q_INVOKABLE void fetch(const QString& tag, const QString& path, const QString& query);
    Q_INVOKABLE void reportCacheStats();

signals:
    void listReady(const QStringList& titles, const QStringList& ids);
    void listAppended(const QStringList& titles, const QStringList& ids);
    /// 热门标签就绪（纯字符串数组）
    void hotTagsReady(const QStringList& tags);
    void categoryReady(const QStringList& titles, const QStringList& ids);
    void pagedReady(const QString& tag, const QStringList& titles, const QStringList& ids);
    void loginResult(bool ok, const QString& message);
    void actionDone(const QString& tag, bool ok, const QString& message);
    void textReady(const QString& tag, bool ok, const QString& text);
    void albumReady(const QString& name, const QString& author, const QStringList& tags);
    void albumCoverReady(const QImage& image);
    void currentAidChanged();
    void pageChanged();
    void previewChanged();
    void chaptersReady(const QStringList& names, const QStringList& ids);
    void pageReady(const QImage& image, const QString& status);
    /// 双页模式：右页就绪
    void previewReady(int index, const QImage& image, const QString& status);
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
    QString pagePath() const { return pagePath_; }
    int pageSeq() const { return pageSeq_; }
    QString previewPath() const { return previewPath_; }
    int previewSeq() const { return previewSeq_; }
public:
    /// 页面图落盘后调用（GUI 线程）
    void setPagePath(const QString& path) { pagePath_ = path; ++pageSeq_; emit pageChanged(); }
    void setPreviewPath(const QString& path) { previewPath_ = path; ++previewSeq_; emit previewChanged(); }
private:
    QString pagePath_;
    int pageSeq_ = 0;
    QString previewPath_;
    int previewSeq_ = 0;
    template <typename... Args>
    void invoke(const char* method, Args&&... args);
};

}  // namespace jmnext::qt
