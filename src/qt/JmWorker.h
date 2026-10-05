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
    /// 主机发现 + 拉首页列表：成功发 listReady（标题与 id 两个平行列表），失败发 failed
    void loadList();
    /// 设置屏蔽关键词（来自界面输入）。列表加载时按 BlockRules 过滤，并上报隐藏条数
    void setBlockWords(const QStringList& words);
    /// 为列表前 n 项取封面：成功发 coverReady(index, image)
    void loadCovers(int n);
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
    void prefetch(int index);
    /// 上报缓存统计（跨线程不能返回值，所以用信号）
    void reportCacheStats();

signals:
    void listReady(const QStringList& titles, const QStringList& ids);
    void chaptersReady(const QStringList& names, const QStringList& ids);
    void coverReady(int index, const QImage& image);
    void pageReady(const QImage& image, const QString& status);
    void status(const QString& text);
    void cacheStats(int imageHits, int imageMisses, int rawHits, int rawMisses);
    void failed(const QString& text);

private:
    JmClient& client();          // 在工作线程内惰性创建

    std::unique_ptr<JmClient> client_;
    QStringList pendingIds_;     // loadList 之后记住 id，供 loadCovers / openChapter 用
    jmnext::core::ChapterImages chapter_;
    bool started_ = false;
    std::vector<std::string> blockWords_;   // 屏蔽关键词（本地规则）
    QString currentAid_;
    int pageIndex_ = -1;
};

}  // namespace jmnext::qt
