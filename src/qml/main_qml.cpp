// QML 前端的入口（P0）。刻意做得很小：只把版本、风格、渲染后端这些"能核对的字符串"
// 传给 QML，用来验证"Qt Quick 在这个容器里真的能起窗口"。
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QDebug>
#include <QQuickWindow>

#include "core/Version.h"
#include "qt/Theme.h"

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);

    // 与 widget 版共用同一份偏好（style.txt / dark.txt），这样两个前端看起来一致
    const jmnext::qt::Style style = jmnext::qt::loadSavedStyle();
    const bool dark = jmnext::qt::loadSavedDark();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appVersion"),
                                             QString::fromLatin1(jmnext::core::APP_VERSION));
    engine.rootContext()->setContextProperty(QStringLiteral("themeName"),
                                             QString::fromLatin1(jmnext::qt::styleName(style)));
    engine.rootContext()->setContextProperty(QStringLiteral("darkTheme"), dark);
    engine.rootContext()->setContextProperty(QStringLiteral("renderBackend"),
                                             QString::fromLatin1(qgetenv("QT_QUICK_BACKEND")));
    // Qt 6.4 没有 loadFromModule()；资源里 QML 文件保留源码路径作为别名，
    // 实测真实 URL 是 qrc:/JMNeXt/src/qml/Main.qml（见 build/.rcc/*_raw_qml_0.qrc）
    engine.load(QUrl(QStringLiteral("qrc:/JMNeXt/src/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        std::printf("QML 加载失败\n");
        return 1;
    }
    qInfo().noquote() << QStringLiteral("QML 前端已起：版本 %1，风格 %2，深浅 %3，后端 %4")
                             .arg(QString::fromLatin1(jmnext::core::APP_VERSION),
                                  QString::fromLatin1(jmnext::qt::styleName(style)),
                                  dark ? QStringLiteral("深色") : QStringLiteral("浅色"),
                                  QString::fromLatin1(qgetenv("QT_QUICK_BACKEND")));

    // --shot <png>：延时抓一张窗口图再退出（P0 用来证明"真的渲染出了画面"，P2 起用来看每屏效果）
    if (argc >= 3 && std::string(argv[1]) == "--shot") {
        const QString out = QString::fromUtf8(argv[2]);
        QTimer::singleShot(1500, &app, [&engine, out] {
            if (auto* w = qobject_cast<QQuickWindow*>(engine.rootObjects().value(0))) {
                const QImage img = w->grabWindow();
                qInfo().noquote() << QStringLiteral("截图%1：%2 %3x%4")
                                         .arg(img.save(out) ? QStringLiteral("成功") : QStringLiteral("失败"),
                                              out)
                                         .arg(img.width())
                                         .arg(img.height());
            } else {
                qInfo().noquote() << QStringLiteral("截图失败：拿不到窗口");
            }
            QGuiApplication::quit();
        });
    }
    return app.exec();
}
