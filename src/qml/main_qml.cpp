// QML 前端的入口（P0）。刻意做得很小：只把版本、风格、渲染后端这些"能核对的字符串"
// 传给 QML，用来验证"Qt Quick 在这个容器里真的能起窗口"。
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>
#include <QVariantMap>
#include <QTimer>
#include <QDebug>
#include <QQuickWindow>

#include "core/Version.h"
#include "qt/Theme.h"
#include "qt/StartupInfo.h"
#include "qml/JmBackend.h"
#include "qml/JmImageProvider.h"

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
    // --ui-style <名>：覆盖界面风格（无头验证与截图对比用）。
    // **不能用 --style**：那是 Qt/QML 自己的保留参数（会被 QML 引擎吃掉，
    // 表现为 module "miuix" is not installed）。必须在主题映射构建之前生效。
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--ui-style") {
            engine.rootContext()->setContextProperty(QStringLiteral("themeName"),
                                                     QString::fromUtf8(argv[i + 1]));
        }
    }
    engine.rootContext()->setContextProperty(QStringLiteral("darkTheme"), dark);
    auto* imageProvider = new jmnext::qt::JmImageProvider();
    engine.addImageProvider(QStringLiteral("jm"), imageProvider);   // QML 里用 image://jm/cover?<url>
    jmnext::qt::JmBackend backend;
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
    // 页面图到达 -> GUI 线程落成临时 PNG -> 通过 backend 的属性给 QML（QML 的 Image 不能直接吃 QImage）
    QObject::connect(&backend, &jmnext::qt::JmBackend::pageReady, &app,
                     [&backend](const QImage& img, const QString&) {
                         static int seq = 0;
                         const QString path = QDir(QDir::tempPath())
                                                  .filePath(QStringLiteral("jmnext-page-%1.png").arg(++seq % 2));
                         if (img.save(path, "PNG")) backend.setPagePath(path);
                     });

    engine.rootContext()->setContextProperty(QStringLiteral("renderBackend"),
                                             QString::fromLatin1(qgetenv("QT_QUICK_BACKEND")));
    // Qt 6.4 没有 loadFromModule()；资源里 QML 文件保留源码路径作为别名，
    // 实测真实 URL 是 qrc:/JMNeXt/src/qml/Main.qml（见 build/.rcc/*_raw_qml_0.qrc）
    // 主题 token 传入 QML（必须在 engine.load 之前，见下面的顺序说明）。
    // 用 QVariantMap：QML 在 Component.onCompleted 里把它复制到根属性，之后绑定只读根属性 ——
    // 因为编译后的 QML 绑定读不到 context property（会当 null），这个坑在 P2 阅读器里踩过一次。
    {
        using namespace jmnext::qt;
        // 从已经设好的 context property 读回风格名：不依赖任何局部变量名，避免与
        // Theme.h 的函数 styleName(Style) 撞名（撞了两次）。
        const QString styleKey =
            engine.rootContext()->contextProperty(QStringLiteral("themeName")).toString();
        const auto pal = [styleKey, dark] {
            if (styleKey == QLatin1String("translucent")) return tokens::translucent(dark);
            if (styleKey == QLatin1String("flatBlur"))    return tokens::flatBlur(dark);
            if (styleKey == QLatin1String("miuix"))       return tokens::miuix(dark);
            return tokens::windowGlass(dark);
        }();
        const auto rad = (styleKey == QLatin1String("flatBlur")) ? tokens::flatBlurRadius()
                       : (styleKey == QLatin1String("miuix"))    ? tokens::miuixRadius()
                                                                  : tokens::windowGlassRadius();
        QVariantMap m;
        const auto put = [&m](const char* k, const char* v) { m.insert(QString::fromLatin1(k), QString::fromUtf8(v ? v : "")); };
        put("accent", pal.accent);            put("accentHover", pal.accentHover);
        put("accentActive", pal.accentActive); put("accentFg", pal.accentFg);
        put("accentSoft", pal.accentSoft);     put("surfaceMica", pal.surfaceMica);
        put("surface1", pal.surface1);         put("surface2", pal.surface2);
        put("surface3", pal.surface3);         put("surfaceSunken", pal.surfaceSunken);
        put("surfaceHover", pal.surfaceHover); put("surfaceActive", pal.surfaceActive);
        put("stroke", pal.stroke);             put("strokeStrong", pal.strokeStrong);
        put("strokeInner", pal.strokeInner);   put("text", pal.text);
        put("textSecondary", pal.textSecondary); put("textTertiary", pal.textTertiary);
        put("textOnAccent", pal.textOnAccent); put("error", pal.error);
        m.insert(QStringLiteral("radiusXs"), rad.xs);
        m.insert(QStringLiteral("radiusSm"), rad.sm);
        m.insert(QStringLiteral("radiusMd"), rad.md);
        m.insert(QStringLiteral("radiusLg"), rad.lg);
        put("styleName", styleKey.toUtf8().constData());
        engine.rootContext()->setContextProperty(QStringLiteral("themeInit"), m);
    }

    // 先扫一遍参数、只设置 QML 启动时就要读到的属性。
    // 为什么必须在 load 之前：engine.load() 是同步的，QML 的 Component.onCompleted 在它内部执行，
    // 那时若拿不到 initialSearch，就会先把首页列表加载上来，把搜索结果覆盖掉（第一次验证就撞到了）。
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--search") {
            engine.rootContext()->setContextProperty(QStringLiteral("initialSearch"),
                                                     QString::fromUtf8(argv[i + 1]));
        } else if (std::string(argv[i]) == "--read") {
        }
    }

    engine.load(QUrl(QStringLiteral("qrc:/JMNeXt/src/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        std::printf("QML 加载失败\n");
        return 1;
    }
        bool verbose = false;
        for (int i = 1; i < argc; ++i) if (std::string(argv[i]) == "--verbose") verbose = true;
        app.setProperty("jmVerbose", verbose);
        qInfo().noquote() << jmnext::qt::startupEnvSummary();
    qInfo().noquote() << QStringLiteral("QML 前端已起：版本 %1，风格 %2，深浅 %3，后端 %4")
                             .arg(QString::fromLatin1(jmnext::core::APP_VERSION),
                                  QString::fromLatin1(jmnext::qt::styleName(style)),
                                  dark ? QStringLiteral("深色") : QStringLiteral("浅色"),
                                  QString::fromLatin1(qgetenv("QT_QUICK_BACKEND")));

    // --page <png>：直接把本地图片喂给阅读器（**测试通道**，绕开网络）。
    // 用途：离线验证"阅读器能不能把图画出来、尺寸对不对"，不受 CDN 波动影响。
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--page") {
            const QString png = QString::fromUtf8(argv[i + 1]);
            QTimer::singleShot(500, &backend, [&backend, png] {
                const QImage img(png);
                if (img.isNull()) {
                    qInfo().noquote() << QStringLiteral("--page：读不到图片 %1").arg(png);
                    return;
                }
                backend.setPagePath(png);
                qInfo().noquote() << QStringLiteral("--page：已把 %1（%2x%3）交给阅读器")
                                         .arg(png).arg(img.width()).arg(img.height());
            });
        }
    }

    // --search <词>：启动后按关键词搜索（无头验证搜索屏用）
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--search") {
            const QString word = QString::fromUtf8(argv[i + 1]);
            QTimer::singleShot(500, &backend, [&backend, word] { backend.search(word, 1); });
        }
    }

    // --read <aid>：打开该作品并自动读第一话（无头验证阅读器）。
    // 注意：只设 autoReadAid 不会加载任何东西，必须真的调用 loadAlbum（上一次尝试就是漏了这一步）。
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--read") {
            const QString aid = QString::fromUtf8(argv[i + 1]);
            engine.rootContext()->setContextProperty(QStringLiteral("autoReadAid"), aid);
            QTimer::singleShot(500, &backend, [&backend, aid] { backend.loadAlbum(aid); });
        }
    }

    // --shot <png>：延时抓一张窗口图再退出（P0 用来证明"真的渲染出了画面"，P2 起用来看每屏效果）

    // --selftest：不加载 QML，直接用后端跑真实数据链路（列表 -> 章节 -> 一页图），
    // 用于回答"QML 侧到底能不能拿到真实数据"这个问题（而不是只看窗口起没起）。
    if (argc >= 2 && std::string(argv[1]) == "--selftest") {
        int pages = 0;
        QObject::connect(&backend, &jmnext::qt::JmBackend::listReady, &app,
                         [&backend](const QStringList& titles, const QStringList&) {
                             qInfo().noquote() << QStringLiteral("自检：列表 %1 条，首条：%2")
                                                      .arg(titles.size())
                                                      .arg(titles.value(0).section('\n', 0, 0));
                             backend.loadCovers(1);   // 先让 worker 把一张封面写入磁盘缓存
                         });
        QString coverUrl;
        QObject::connect(&backend, &jmnext::qt::JmBackend::coverUrlReady, &app,
                         [&coverUrl](int index, const QString& url) {
                             if (index == 0) coverUrl = url;
                         });
        QObject::connect(&backend, &jmnext::qt::JmBackend::coverReady, &app,
                         [&app, &coverUrl, imageProvider](int index, const QImage& img) {
                             if (index != 0) return;
                             qInfo().noquote() << QStringLiteral("自检：worker 取到封面 %1x%2，URL 是否拿到：%3")
                                                      .arg(img.width()).arg(img.height())
                                                      .arg(coverUrl.isEmpty() ? QStringLiteral("否")
                                                                              : QStringLiteral("是"));
                             QSize got;
                             const QImage hit = imageProvider->requestImage(
                                 QStringLiteral("cover?") +
                                     QString::fromUtf8(QUrl::toPercentEncoding(coverUrl)), &got, QSize(160, 213));
                             qInfo().noquote() << QStringLiteral("自检：提供器按同一 URL 取到 %1x%2")
                                                      .arg(hit.width()).arg(hit.height());
                             const QImage miss = imageProvider->requestImage(
                                 QStringLiteral("cover?%2Fnot-exist"), &got, QSize(120, 160));
                             qInfo().noquote() << QStringLiteral("自检：不存在的 id 返回 %1x%2（占位图）")
                                                      .arg(miss.width()).arg(miss.height());
                             QTimer::singleShot(100, &app, &QCoreApplication::quit);
                         });
        QObject::connect(&backend, &jmnext::qt::JmBackend::pageReady, &app,
                         [&app, &pages](const QImage& img, const QString& statusText) {
                             ++pages;
                             qInfo().noquote() << QStringLiteral("自检：第 %1 页状态 %2，图片 %3x%4")
                                                      .arg(pages)
                                                      .arg(statusText)
                                                      .arg(img.width())
                                                      .arg(img.height());
                             if (pages >= 1) QTimer::singleShot(100, &app, &QCoreApplication::quit);
                         });
        QObject::connect(&backend, &jmnext::qt::JmBackend::failed, &app, [&app](const QString& e) {
            qInfo().noquote() << QStringLiteral("自检失败：%1").arg(e);
            QTimer::singleShot(100, &app, &QCoreApplication::quit);
        });
        QTimer::singleShot(0, &backend, [&backend] { backend.loadList(); });
        QTimer::singleShot(120000, &app, [&app] {
            qInfo().noquote() << QStringLiteral("自检超时（120 秒）");
            app.quit();
        });
        return app.exec();
    }
    // --ui-selftest：验证"点了加载更多真的出第二页"。
    //
    // 为什么需要它：--shot 是静态拍照，无法点击；此前"80 → 160 条"只在 widget 版验证过，
    // QML 侧只能证明"接线正确、能加载"。这里用 QML 侧同一条路径（JmBackend::loadMore）
    // 走一遍：列表就绪 -> 主动 loadMore -> 核对条数是否增长。
    if (argc >= 2 && std::string(argv[1]) == "--ui-selftest") {
        int round = 0;
        int firstCount = 0;
        QObject::connect(&backend, &jmnext::qt::JmBackend::listReady, &app,
                         [&backend, &round, &firstCount, &app](const QStringList& titles, const QStringList&) {
                             ++round;
                             if (round == 1) {
                                 firstCount = titles.size();
                                 qInfo().noquote() << QStringLiteral("界面自检：第一页 %1 条，请求加载更多…")
                                                          .arg(firstCount);
                                 backend.loadMore();
                                 return;
                             }
                             // 第二次 listReady 不作断言：loadMore 的产出走 listAppended（见下面的连接）。
                             // 这里只提示一句，避免与 listAppended 的结论混在一起造成误读。
                             qInfo().noquote() << QStringLiteral("界面自检：又收到一次 listReady（%1 条），忽略")
                                                      .arg(titles.size());
                         });
        QObject::connect(&backend, &jmnext::qt::JmBackend::failed, &app, [&app](const QString& e) {
            qInfo().noquote() << QStringLiteral("界面自检失败：%1").arg(e);
            QTimer::singleShot(100, &app, &QCoreApplication::quit);
        });
        QObject::connect(&backend, &jmnext::qt::JmBackend::listAppended, &app,
                         [&app, &firstCount](const QStringList& added, const QStringList&) {
                             qInfo().noquote()
                                 << QStringLiteral("界面自检：listAppended 追加 %1 条（第一页 %2 条，%3）")
                                        .arg(added.size())
                                        .arg(firstCount)
                                        .arg(added.isEmpty() ? QStringLiteral("未追加：失败")
                                                             : QStringLiteral("增长：通过"));
                             QTimer::singleShot(100, &app, &QCoreApplication::quit);
                         });
        QTimer::singleShot(0, &backend, [&backend] { backend.loadList(); });
        QTimer::singleShot(120000, &app, [&app] {
            qInfo().noquote() << QStringLiteral("界面自检超时（120 秒）");
            app.quit();
        });
        return app.exec();
    }
    // --category-filter <标签>：验证"点分类标签 -> 取作品列表"这条链路
    // （JmClient::categoryFilter -> JmWorker::categoryFilter -> JmBackend::categoryReady）。
    // 用法示例：./build/jmnext4qml --category-filter 女高中生
    if (argc >= 2 && std::string(argv[1]) == "--category-filter") {
        const QString tag = (argc >= 3) ? QString::fromUtf8(argv[2]) : QString();
        QObject::connect(&backend, &jmnext::qt::JmBackend::categoryReady, &app,
                         [&app](const QStringList& titles, const QStringList&) {
                             qInfo().noquote() << QStringLiteral("分类筛选自检：%1 条，首条：%2")
                                                      .arg(titles.size())
                                                      .arg(titles.value(0).section('\n', 0, 0));
                             QTimer::singleShot(100, &app, &QCoreApplication::quit);
                         });
        QObject::connect(&backend, &jmnext::qt::JmBackend::failed, &app, [&app](const QString& e) {
            qInfo().noquote() << QStringLiteral("分类筛选自检失败：%1").arg(e);
            QTimer::singleShot(100, &app, &QCoreApplication::quit);
        });
        QTimer::singleShot(0, &backend, [&backend, tag] { backend.categoryFilter(tag, 1); });
        QTimer::singleShot(60000, &app, [&app] {
            qInfo().noquote() << QStringLiteral("分类筛选自检超时（60 秒）");
            app.quit();
        });
        return app.exec();
    }

    // --raw <path> [query]：直出响应原文，用于一次看清未知响应形态（调试用）
    if (argc >= 3 && std::string(argv[1]) == "--raw") {
        const QString path = QString::fromUtf8(argv[2]);
        const QString query = (argc >= 4) ? QString::fromUtf8(argv[3]) : QString();
        QObject::connect(&backend, &jmnext::qt::JmBackend::status, &app, [](const QString& s) {
            qInfo().noquote() << QStringLiteral("状态：%1").arg(s);
        });
        QTimer::singleShot(0, &backend, [&backend, path, query] { backend.loadPaged("raw", path, query); });
        QTimer::singleShot(30000, &app, [&app] { app.quit(); });
        return app.exec();
    }

    // --paged <path> [query]：直接打任意"作品列表"型接口（收藏/历史/画师/周更/随机推荐…）。
    // 存在理由：通用列表屏 ComicListScreen 的每个入口只差 path/query，用它可以一次性核对多个接口，
    // 不必对每屏各写一条自检。示例：
    //   ./build/jmnext4qml --paged random_recommend
    //   ./build/jmnext4qml --paged creator_author page=1
    //   ./build/jmnext4qml --paged week
    if (argc >= 3 && std::string(argv[1]) == "--paged") {
        const QString path = QString::fromUtf8(argv[2]);
        const QString query = (argc >= 4) ? QString::fromUtf8(argv[3]) : QString();
        QObject::connect(&backend, &jmnext::qt::JmBackend::pagedReady, &app,
                         [&app](const QString& tag, const QStringList& titles, const QStringList&) {
                             qInfo().noquote() << QStringLiteral("列表自检[%1]：%2 条，首条：%3")
                                                      .arg(tag)
                                                      .arg(titles.size())
                                                      .arg(titles.value(0).section('\n', 0, 0));
                             QTimer::singleShot(100, &app, &QCoreApplication::quit);
                         });
        QObject::connect(&backend, &jmnext::qt::JmBackend::failed, &app, [&app](const QString& e) {
            qInfo().noquote() << QStringLiteral("列表自检失败：%1").arg(e);
            QTimer::singleShot(100, &app, &QCoreApplication::quit);
        });
        // 把后端 status 也打出来：未知响应形态的定位就靠它（例：字段名没对上时的响应片段）
        QObject::connect(&backend, &jmnext::qt::JmBackend::status, &app, [](const QString& s) {
            qInfo().noquote() << QStringLiteral("状态：%1").arg(s.left(260));
        });
        QTimer::singleShot(0, &backend, [&backend, path, query] { backend.loadPaged("selfcheck", path, query); });
        QTimer::singleShot(45000, &app, [&app] { qInfo().noquote() << QStringLiteral("列表自检超时"); app.quit(); });
        return app.exec();
    }

    // --hot-tags：验证分类屏的数据链路（JmParse::parseHotTags -> JmClient::hotTags
    // -> JmWorker::loadHotTags -> JmBackend::hotTagsReady）。与 --ui-selftest 同一套路：
    // 走真实网络、把结果打到日志，用于回答"热标签到底取到没有"。
    if (argc >= 2 && std::string(argv[1]) == "--hot-tags") {
        QObject::connect(&backend, &jmnext::qt::JmBackend::hotTagsReady, &app,
                         [&app](const QStringList& tags) {
                             qInfo().noquote() << QStringLiteral("热标签自检：%1 个，前三个：%2")
                                                      .arg(tags.size())
                                                      .arg(tags.mid(0, 3).join(QStringLiteral(" / ")));
                             QTimer::singleShot(100, &app, &QCoreApplication::quit);
                         });
        QObject::connect(&backend, &jmnext::qt::JmBackend::failed, &app, [&app](const QString& e) {
            qInfo().noquote() << QStringLiteral("热标签自检失败：%1").arg(e);
            QTimer::singleShot(100, &app, &QCoreApplication::quit);
        });
        QTimer::singleShot(0, &backend, [&backend] { backend.loadHotTags(); });
        QTimer::singleShot(60000, &app, [&app] {
            qInfo().noquote() << QStringLiteral("热标签自检超时（60 秒）");
            app.quit();
        });
        return app.exec();
    }
    // --open <aid>：启动后直接打开某作品的详情页（无头验证用：点不了鼠标，但要能拍到详情屏）
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--open") {
            const QString aid = QString::fromUtf8(argv[i + 1]);
            QTimer::singleShot(500, &backend, [&backend, aid] { backend.loadAlbum(aid); });
        }
    }

    if (argc >= 3 && std::string(argv[1]) == "--shot") {
        const QString out = QString::fromUtf8(argv[2]);
        // 第三个参数是**最长等待**毫秒（默认 1500）：数据/页面先就绪就先拍，最长等这么久。
        // 为什么从"固定延时"改成"条件等待"：真实网络时机波动大，固定延时下同一条命令
        // 有时拍到内容、有时拍到空窗（P2 阅读器那次 40 秒都没等到页面）。
        const int maxWaitMs = (argc >= 4) ? std::atoi(argv[3]) : 1500;
        bool grabbed = false;
        auto grabAndQuit = [&engine, out, &grabbed]() {
            if (grabbed) return;
            grabbed = true;
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
        };
        // 页面图就绪后再等 1.2 秒（让布局与绘制落定）再拍
        QObject::connect(&backend, &jmnext::qt::JmBackend::pageReady, &app,
                         [grabAndQuit, &app](const QImage&, const QString&) {
                             QTimer::singleShot(1200, &app, grabAndQuit);
                         });
        QTimer::singleShot(maxWaitMs, &app, grabAndQuit);   // 兜底：最多等 maxWaitMs
    }
    return app.exec();
}
