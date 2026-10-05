// JMNeXt4Desktop —— 骨架
//
// 现在只做一件事：起一个空窗口，确认 Qt 工程能被 CMake 构建出来。
// 真正的实现见 docs/PORT-SPEC.md（按清单逐项对齐主项目桌面端的功能）。
#include <QApplication>
#include <QLabel>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QLabel window;
    window.setWindowTitle(QStringLiteral("JMNeXt4Desktop"));
    // 用中文做一次显示自检：CJK 是否正常（主项目在字体上踩过坑，这里一开始就盯住）
    window.setText(QStringLiteral("JMNeXt4Desktop 骨架 · 汉字显示自检"));
    window.resize(560, 160);
    window.show();
    return app.exec();
}
