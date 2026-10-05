// 最小界面骨架：打开一张图 → 按 aid/page 还原 → 并排看"原始/还原"。
#include "core/JmParse.h"
#include "qt/JmClient.h"
#include "qt/JmWorker.h"
//
// 现在刻意只做这一件事（对应目标里的"先跑通能看图"），列表、阅读器交互随后再加。
// 无显示环境可用 --selftest 走同一条代码路径并打印结果（见 main.cpp），
// 这样界面层也能在容器里被验证，而不是"写完就算"。
#pragma once
#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QListWidget;
class QThread;
class QComboBox;
class QKeyEvent;

namespace jmnext::qt {

class MainWindow : public QMainWindow {
public:
    MainWindow();
    ~MainWindow() override;

    /// 加载图片（原图与还原图都更新到界面上）
    bool loadImage(const QString& path);
    /// 按当前 aid/page 重新还原
    void unscrambleNow();
    /// 把一串说明写到底部日志（自检与排错都看它）
    void log(const QString& line);
    /// 当前选中的界面字体族名（CJK 自检用）
    QString chosenFontFamily() const { return fontFamily_; }

    /// 拉取真实首页列表并填进左侧列表（返回条数；-1 表示失败）
    int loadRealList();
    /// 当前列表条数（自检用）
    /// 为列表前 n 项加载封面缩略图（惰性，避免一次下 80 张），返回成功张数
    int loadCoversFirst(int n);
    int coverLoadedCount() const { return coverLoaded_; }

    /// 异步拉列表并等待结果（自检用；返回条数，-1 表示超时/失败）
    int requestListAndWait(int timeoutMs = 60000);
    /// 异步取前 n 张封面并等待（自检用）
    int requestCoversAndWait(int n, int timeoutMs = 120000);
    /// 异步打开章节并翻页（自检用）：返回收到的页数，-1 表示失败/超时
    int requestReaderAndWait(const QString& aid, int page, int steps, int timeoutMs = 180000);
    /// 自检：打开某作品的章节列表并切到第 index 个章节，返回其页数（-1 表示失败）
    int requestChapterPickAndWait(const QString& aid, int index, int timeoutMs = 180000);
    /// 自检用：把屏蔽关键词交给 worker（正式路径由界面输入框触发）
    void setBlockWordsForTest(const QStringList& words) { if (worker_) worker_->setBlockWords(words); }
    /// 触发一次异步拉列表（截图自检用；不等待结果）
    void requestListAsync() { if (worker_) worker_->loadList(); }
    /// 触发 worker 上报缓存统计并等它回来（自检用）
    void requestCacheStatsAndWait(int timeoutMs = 5000);
    QString lastPageStatus() const { return pendingPageStatus_; }
    int listCount() const;
    /// 列表第 i 条的显示文本（自检用）
    QString listItemText(int i) const;
    bool showPage(int index);

    /// 载入某作品的某章节，并跳到指定页（真实链路：详情 → 章节 → 下载 → 还原）
    bool openChapter(const QString& aid, int pageIndex);
    /// 翻到下一页（越界返回 false）
    bool nextPage();
    /// 翻到上一页
    bool prevPage();
    /// 当前页信息（自检用）
    QString pageStatus() const { return pageStatus_; }
    struct CacheStats { int imageHits, imageMisses, rawHits, rawMisses; };
    /// 注意：统计来自**工作线程**里那个 JmClient（干活的是它），由 cacheStats 信号上报
    CacheStats cacheStats() const { return lastCacheStats_; }
    CacheStats lastCacheStats_{0, 0, 0, 0};

private:
    void setupUi();
    /// 键盘翻页：← / PageUp 上一页，→ / PageDown / 空格 下一页
    void keyPressEvent(QKeyEvent* event) override;
    /// 滚轮翻页（只挂在图片区上，避免抢走左侧列表的滚动）
    bool eventFilter(QObject* watched, QEvent* event) override;
    void applyCjkFont();

    QLabel* originalView_ = nullptr;
    QLabel* restoredView_ = nullptr;
    QLineEdit* aidEdit_ = nullptr;
    QLineEdit* pageEdit_ = nullptr;
    QPlainTextEdit* logView_ = nullptr;
    QListWidget* listView_ = nullptr;
    QLineEdit* blockEdit_ = nullptr;
    QComboBox* chapterBox_ = nullptr;
    int coverLoaded_ = 0;
    QThread* workerThread_ = nullptr;
    JmWorker* worker_ = nullptr;
    bool pumpList_ = false;
    int pendingListResult_ = -1;
    QString pendingPageStatus_;
    jmnext::qt::JmClient client_;
    bool clientReady_ = false;
    jmnext::core::ChapterImages chapter_;
    std::string chapterAid_;
    int pageIndex_ = 0;
    QString pageStatus_;
    QString fontFamily_;
    QString loadedPath_;
    int width_ = 0;
    int height_ = 0;
    std::vector<uint32_t> pixels_;   // 原图像素（ARGB）
};

}  // namespace jmnext::qt
