// 最小界面骨架：打开一张图 → 按 aid/page 还原 → 并排看"原始/还原"。
#include "core/JmParse.h"
#include "qt/JmClient.h"
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

namespace jmnext::qt {

class MainWindow : public QMainWindow {
public:
    MainWindow();

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
    CacheStats cacheStats() const {
        return {client_.cache().imageHits(), client_.cache().imageMisses(),
                client_.cache().rawHits(), client_.cache().rawMisses()};
    }

private:
    void setupUi();
    void applyCjkFont();

    QLabel* originalView_ = nullptr;
    QLabel* restoredView_ = nullptr;
    QLineEdit* aidEdit_ = nullptr;
    QLineEdit* pageEdit_ = nullptr;
    QPlainTextEdit* logView_ = nullptr;
    QListWidget* listView_ = nullptr;
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
