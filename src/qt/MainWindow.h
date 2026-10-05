// 最小界面骨架：打开一张图 → 按 aid/page 还原 → 并排看"原始/还原"。
//
// 现在刻意只做这一件事（对应目标里的"先跑通能看图"），列表、阅读器交互随后再加。
// 无显示环境可用 --selftest 走同一条代码路径并打印结果（见 main.cpp），
// 这样界面层也能在容器里被验证，而不是"写完就算"。
#pragma once
#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPlainTextEdit;

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

private:
    void setupUi();
    void applyCjkFont();

    QLabel* originalView_ = nullptr;
    QLabel* restoredView_ = nullptr;
    QLineEdit* aidEdit_ = nullptr;
    QLineEdit* pageEdit_ = nullptr;
    QPlainTextEdit* logView_ = nullptr;
    QString fontFamily_;
    QString loadedPath_;
    int width_ = 0;
    int height_ = 0;
    std::vector<uint32_t> pixels_;   // 原图像素（ARGB）
};

}  // namespace jmnext::qt
