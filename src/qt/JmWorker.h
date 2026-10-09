// 后台工作对象：把 JmClient 的网络/解码工作搬离界面线程。
//
// 为什么需要它：JmClient 的方法都是同步的（一次列表/一页图可能要几百毫秒到数秒），
// 直接在主线程调用会让界面卡住 —— 这正是从"能跑"到"能用"的最后一道坎。
//
// 约定：
//  - JmClient 在**工作线程内**惰性创建（它内部的 QNetworkAccessManager 必须与使用它的线程同属）；
//  - 跨线程只传 Qt 已有元类型的类型（QString / QStringList / QImage），不引入自定义类型；
//  - 失败一律通过 failed(QString) 上报，界面据此提示，不静默。
#pragma once
#include <QImage>
#include <QObject>
#include <QStringList>

#include <vector>

#include "core/JmParse.h"
#include "core/JmCore.h"
#include "qt/JmClient.h"

#include <memory>

namespace jmnext::qt {

class JmWorker : public QObject {
    Q_OBJECT
public:
    JmWorker() = default;
    ~JmWorker() override;

public slots:
    /// 取某作品的封面（详情区用）
    void fetchAlbumCover(const QString& id);
    /// 主机发现 + 拉首页列表：成功发 listReady（标题与 id 两个平行列表），失败发 failed
    void loadList();
    /// 只取详情与章节列表（进详情页用；不拉任何页面图）
    void loadAlbum(const QString& aid);
    /// 搜索（结果走 listReady，界面按"替换列表"处理；屏蔽规则同样生效）
    void search(const QString& word, int page = 1);
    /// 设置屏蔽关键词（来自界面输入）。列表加载时按 BlockRules 过滤，并上报隐藏条数
    void setBlockWords(const QStringList& words);
    /// 单页/双页：开时在 originalView_ 位置显示下一页
    void setTwoPage(bool on);
    /// 加载下一页（追加到列表末尾）；到末页时上报状态
    void loadMore();
    /// 为列表前 n 项取封面：成功发 coverReady(index, image)
    void loadCovers(int n);

    /// 热门标签（分类浏览页的数据源；JmClient::hotTags）
    void loadHotTags();

    /// 按分类标签筛选作品（c 为空表示不筛选；参数会被省略）
    void categoryFilter(const QString& c, int page);

    /// 通用分页列表（tag 用于把结果对上是哪个请求）
    void loadPaged(const QString& tag, const QString& path, const QString& query);
    /// 打开某作品的某章节并显示第 page 页；之后 step(±1) 翻页
    void openChapter(const QString& aid, int page);
    /// 按章节 id 直接打开（章节选择器用；跳过先取详情再取第一话那一步）
    void openChapterId(const QString& chapterId, int page);
    void step(int delta);
    /// 直接跳到第 index 页（0 起）；openChapter 与 step 都经由它
    void showPageAt(int index);
    /// 确保主机已发现（各入口共用，避免详情失败：尚未初始化主机）
    bool ensureStarted();
    /// 预取第 index 页到缓存（失败静默，不影响当前页）
    /// 进度文件路径（AppDataLocation 下 progress.txt）
    /// 立刻写入当前进度（翻页成功后调用）
    void saveProgressNow();
    static std::string progressPath();
    void prefetch(int index);
    /// 上报缓存统计（跨线程不能返回值，所以用信号）
    void reportCacheStats();

signals:
    void listReady(const QStringList& titles, const QStringList& ids);
    void listAppended(const QStringList& titles, const QStringList& ids);
    void chaptersReady(const QStringList& names, const QStringList& ids);
    void albumReady(const QString& name, const QString& author, const QStringList& tags);
    void albumCoverReady(const QImage& image);
    void previewReady(int index, const QImage& image, const QString& status);   // 双页模式下"下一页"就绪
    void coverReady(int index, const QImage& image);
    /// 与 coverReady 配套：先告知该下标的封面 URL（QML 侧图片提供器据此命中同一份磁盘缓存）
    void coverUrlReady(int index, const QString& url);

    /// 热门标签就绪（纯字符串数组）
    void hotTagsReady(const QStringList& tags);

    /// 分类筛选结果就绪（title/id 两个平行列表，与 listReady 同形态）
    void categoryReady(const QStringList& titles, const QStringList& ids);

    /// 通用分页列表就绪
    void pagedReady(const QString& tag, const QStringList& titles, const QStringList& ids);
    void pageReady(const QImage& image, const QString& status);
    void status(const QString& text);
    void cacheStats(int imageHits, int imageMisses, int rawHits, int rawMisses);
    void failed(const QString& text);

private:
    JmClient& client();          // 在工作线程内惰性创建

    std::unique_ptr<JmClient> client_;
    std::vector<jmnext::core::ListEntry> currentEntries_;   // **过滤后**的当前列表（下标与界面显示项一致，供封面用）
    QString lastQuery_;          // 上次搜索词（空=当前看的是首页列表）；"加载更多"据此决定翻哪个列表
    QStringList pendingIds_;     // loadList 之后记住 id，供 loadCovers / openChapter 用
    jmnext::core::ChapterImages chapter_;
    bool started_ = false;
    int page_ = 0;                          // 当前已加载到第几页（0 起）
    bool twoPage_ = false;                  // 双页模式
    void previewNext(int index);            // 取第 index 页并 emit previewReady（不改变当前页）
    std::vector<std::string> blockWords_;   // 屏蔽关键词（本地规则）
    QString currentAid_;
    int pageIndex_ = -1;
};

}  // namespace jmnext::qt
