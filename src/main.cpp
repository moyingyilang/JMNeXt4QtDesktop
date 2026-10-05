// JMNeXt4QtDesktop 入口。
//
//   jmnext4desktop                 打开窗口
//   jmnext4desktop --selftest <图>  无显示自检：加载 → 还原 → 打印校验和与字体 → 退出
//
// 自检存在的理由：容器里没有显示，界面层的代码否则无法被验证。走的是与窗口完全相同的代码路径。
#include "qt/MainWindow.h"

#include <QApplication>
#include <QTimer>

#include <cstdio>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    if (argc >= 3 && std::string(argv[1]) == "--selftest") {
        jmnext::qt::MainWindow win;
        int rc = 0;
        if (!win.loadImage(QString::fromUtf8(argv[2]))) rc = 1;
        std::printf("自检结果：%s\n", rc == 0 ? "通过" : "失败");
        return rc;
    }

    jmnext::qt::MainWindow win;
    win.resize(760, 620);
    win.show();
    return app.exec();
}
