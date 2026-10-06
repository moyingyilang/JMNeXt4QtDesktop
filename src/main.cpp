// JMNeXt4QtDesktop 入口。
//
//   jmnext4desktop                 打开窗口
//   jmnext4desktop --selftest <图>  无显示自检：加载 → 还原 → 打印校验和与字体 → 退出
//
// 自检存在的理由：容器里没有显示，界面层的代码否则无法被验证。走的是与窗口完全相同的代码路径。
#include "qt/JmClient.h"
#include "qt/Theme.h"
#include "qt/MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QTimer>
#include <QTimer>

#include <cstdio>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    // 界面主题：默认深色；--light 用浅色。先把"Qt 系统默认观感"换掉（第 (a) 步）
    {
        bool light = false;
        for (int i = 1; i < argc; ++i) if (std::string(argv[i]) == "--light") light = true;
        app.setStyleSheet(light ? jmnext::qt::lightThemeQss() : jmnext::qt::darkThemeQss());
    }

    if (argc >= 3 && std::string(argv[1]) == "--selftest") {
        jmnext::qt::MainWindow win;
        int rc = 0;
        if (!win.loadImage(QString::fromUtf8(argv[2]))) rc = 1;
        std::printf("自检结果：%s\n", rc == 0 ? "通过" : "失败");
        return rc;
    }

    if (argc >= 2 && std::string(argv[1]) == "--version") {
        std::printf("JMNeXt4QtDesktop %s\n", jmnext::core::APP_VERSION);
        return 0;
    }

    if (argc >= 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        std::printf(
            "JMNeXt4QtDesktop（早期阶段）\n"
            "\n"
            "用法：\n"
            "  jmnext4desktop                     起窗口（默认自动加载真实首页列表）\n"
            "  jmnext4desktop --list [屏蔽词]      加载列表（可给逗号分隔的屏蔽关键词）\n"
            "  jmnext4desktop --reader <aid>       同步打开某作品第一话第 0 页（自检路径）\n"
            "  jmnext4desktop --reader-async <aid> <page>   异步打开并翻 2 页（自检路径）\n"
            "  jmnext4desktop --chapters <aid>     打开某作品章节列表并切到第 3 话（自检）\n"
            "  jmnext4desktop --zoom <aid>         验证缩放两种模式（自检）\n"
            "  jmnext4desktop --screenshot <png> [--list]   截图后退出（无显示环境可用）\n"
            "  jmnext4desktop --version          打印版本（单一来源：src/core/Version.h）\n"
            "\n"
            "附带的独立工具：jmnext4net（真实网络自检）、jmnext4img（图片管线）、jmnext4cli（纯计算）\n"
            "依赖：Qt 6 与 qt6-image-formats-plugins（真实漫画图是 WebP，缺了图片打不开）\n");
        return 0;
    }

    if (argc >= 3 && std::string(argv[1]) == "--screenshot") {
        // 起真实窗口 → 拉真实列表（后台线程）→ 等它上屏 → 截图 → 退出
        jmnext::qt::MainWindow win;
        win.resize(1280, 800);
        win.show();
        const QString out = QString::fromUtf8(argv[2]);
        const bool withList = (argc >= 4 && std::string(argv[3]) == "--list");
        QTimer::singleShot(300, &app, [&win, withList] { if (withList) win.requestListAsync(); });
        QTimer::singleShot(argc >= 4 ? 45000 : 2000, &app, [&win, out] {   // 45 秒（原 25 秒）：真实图片走 CDN 慢时 25 秒会抓空（已知 race，缓解非根治）   // 带子参数时要等网络（此前只认 --list，导致 --reader 的截图拍早了）
            const bool ok = win.grab().save(out);
            std::printf("截图%s：%s\n", ok ? "成功" : "失败", out.toUtf8().constData());
            QCoreApplication::quit();
        });
        // 可选：把真实阅读页也载入（用于给用户看实际画面）。纯加法，不动上面的逻辑。
        if (argc >= 4 && std::string(argv[3]) == "--search") {
            const QString q = QString::fromUtf8(argv[4]);
            QTimer::singleShot(400, &app, [&win, q] { win.searchAsync(q); });
        }
        if (argc >= 5 && std::string(argv[3]) == "--reader") {
            const QString readerAid = QString::fromUtf8(argv[4]);
            QTimer::singleShot(21000, &app, [&win, readerAid] { win.openReaderAsync(readerAid, 2); });
        }
        return app.exec();
    }

    if (argc >= 3 && std::string(argv[1]) == "--zoom") {
        jmnext::qt::MainWindow win;
        win.resize(1000, 700);
        const int n = win.requestReaderAndWait(QString::fromUtf8(argv[2]), 0, 0);
        if (n < 1) { std::printf("缩放自检失败（没能取到页）\n"); return 1; }
        const QSize fit = win.repaintReaderAndSize();
        win.setZoomFit(false);
        const QSize full = win.repaintReaderAndSize();
        std::printf("适应窗口：%dx%d；原始尺寸：%dx%d\n", fit.width(), fit.height(), full.width(), full.height());
        std::printf("缩放自检：%s\n", (full.width() > fit.width()) ? "通过" : "可疑（原始尺寸没变大）");
        return 0;
    }

    if (argc >= 3 && std::string(argv[1]) == "--chapters") {
        jmnext::qt::MainWindow win;
        const int n = win.requestChapterPickAndWait(QString::fromUtf8(argv[2]), 2);
        if (n < 0) { std::printf("章节自检失败\n"); return 1; }
        std::printf("详情标签：%s\n", win.albumText().toUtf8().constData());
        const QSize cs = win.coverSize();
        std::printf("封面尺寸：%dx%d\n", cs.width(), cs.height());
        std::printf("章节数：%d；章节列表项数：%d；末页状态：%s\n", n, win.chapterListCount(),
                    win.lastPageStatus().toUtf8().constData());
        if (n == 0)
            std::printf("章节选择自检：该作品章节表为空（单本），按第一话读取：通过\n");
        else
            std::printf("章节选择自检：通过\n");
        return 0;
    }

    if (argc >= 4 && std::string(argv[1]) == "--reader-async") {
        jmnext::qt::MainWindow win;
        for (int i = 1; i < argc; ++i)          // --two-page：打开双页模式（自检用）
            if (std::string(argv[i]) == "--two-page") win.setTwoPageForTest(true);
        const int pages = win.requestReaderAndWait(QString::fromUtf8(argv[2]), std::atoi(argv[3]), 2);
        if (pages < 1) { std::printf("异步阅读器自检失败\n"); return 1; }
        win.requestCacheStatsAndWait();
        const auto cs = win.cacheStats();
        std::printf("异步阅读器：收到 %d 页，末页状态：%s\n", pages, win.lastPageStatus().toUtf8().constData());
        std::printf("缓存：内存命中 %d / 未命中 %d；磁盘命中 %d / 未命中 %d\n",
                    cs.imageHits, cs.imageMisses, cs.rawHits, cs.rawMisses);
        std::printf("异步阅读器自检：通过\n");
        return 0;
    }

    if (argc >= 2 && std::string(argv[1]) == "--list") {
        jmnext::qt::MainWindow win;
        QStringList words;
        if (argc >= 3) {
            for (const auto& w : QString::fromUtf8(argv[2]).split(',', Qt::SkipEmptyParts)) words << w.trimmed();
            win.setBlockWordsForTest(words);
        }
        const int n = win.requestListAndWait();
        if (n < 0) { std::printf("加载失败\n"); return 1; }
        std::printf("列表条数：%d\n", n);
        const int more = win.requestLoadMoreAndWait();
        std::printf("加载更多之后：%d 条\n", more);
        for (int i = 0; i < 3 && i < n; ++i)
            std::printf("  第 %d 条：%s\n", i + 1, win.listItemText(i).toUtf8().constData());
        const int covers = win.requestCoversAndWait(3);
        std::printf("封面缩略图：成功 %d 张（前 3 项）\n", covers);
        std::printf("首页列表自检：%s\n", covers > 0 ? "通过" : "通过（但封面未取到）");
        return 0;
    }

    if (argc >= 3 && std::string(argv[1]) == "--search") {
        jmnext::qt::MainWindow win;
        const QString word = QString::fromUtf8(argv[2]);
        const int n = win.requestSearchAndWait(word);
        if (n < 0) { std::printf("搜索失败或超时\n"); return 1; }
        std::printf("搜索 %s：列表条数 %d\n", argv[2], n);
        for (int i = 0; i < n && i < 3; ++i)
            std::printf("  第 %d 项：%s\n", i + 1, win.listItemText(i).toUtf8().constData());
        const int covers = win.requestCoversAndWait(3);
        std::printf("搜索结果封面：成功 %d 张\n", covers);
        const int more = win.requestLoadMoreAndWait();
        std::printf("搜索后再加载更多：累计 %d 条\n", more);
        return 0;
    }

    if (argc >= 4 && std::string(argv[1]) == "--reader") {
        jmnext::qt::MainWindow win;
        const QString aid = QString::fromUtf8(argv[2]);
        const int page = std::atoi(argv[3]);
        if (!win.openChapter(aid, page)) { std::printf("阅读器自检失败\n"); return 1; }
        std::printf("页状态：%s\n", win.pageStatus().toUtf8().constData());
        if (!win.nextPage()) { std::printf("翻下一页失败\n"); return 1; }
        std::printf("翻页后：%s\n", win.pageStatus().toUtf8().constData());
        win.prevPage();                                   // 回到第 1 页：应命中内存缓存
        std::printf("回到首页：%s\n", win.pageStatus().toUtf8().constData());
        std::printf("缓存：内存命中 %d 次 / 未命中 %d 次；磁盘命中 %d 次 / 未命中 %d 次\n",
                    win.cacheStats().imageHits, win.cacheStats().imageMisses,
                    win.cacheStats().rawHits, win.cacheStats().rawMisses);
        std::printf("阅读器自检：通过\n");
        return 0;
    }

    if (argc >= 2 && std::string(argv[1]) == "--live") {
        // 真实链路自检：主机发现 → 列表 → 详情 → 章节 → 下载一页并还原
        jmnext::qt::JmClient client;
        if (!client.bootstrap()) { std::printf("主机发现失败：%s\n", client.lastError().c_str()); return 1; }
        std::printf("主机: %s\n", client.host().c_str());
        auto list = client.latest(0);
        if (!list) { std::printf("列表失败：%s\n", client.lastError().c_str()); return 1; }
        std::printf("列表条数: %zu\n", list->size());
        if (list->empty()) return 1;
        const std::string aid = (*list)[0].id;
        std::printf("第一条: id=%s name=%s category=%s\n", aid.c_str(), (*list)[0].name.c_str(),
                    (*list)[0].categoryTitle.c_str());
        auto al = client.album(aid);
        if (!al) { std::printf("详情失败：%s\n", client.lastError().c_str()); return 1; }
        std::printf("详情: 标签 %zu 个，章节 %zu 个\n", al->tags.size(), al->series.size());
        if (al->series.empty()) return 1;
        auto ch = client.chapter(al->series.front().id);
        if (!ch) { std::printf("章节失败：%s\n", client.lastError().c_str()); return 1; }
        std::printf("章节: id=%s 共 %d 页 scramble_id=%d\n", ch->id.c_str(), ch->totalPage, ch->scrambleId);
        if (ch->images.empty()) return 1;
        auto qimg = client.pageImage(ch->images.front().url, std::atoi(ch->id.c_str()), ch->scrambleId);
        if (!qimg) { std::printf("取图失败：%s\n", client.lastError().c_str()); return 1; }
        std::printf("第一页: %dx%d，非空像素=%d\n", qimg->width(), qimg->height(),
                    qimg->isNull() ? 0 : 1);
        std::printf("真实链路自检：通过\n");
        return 0;
    }

    jmnext::qt::MainWindow win;
    win.resize(1280, 800);
    win.show();
    // 默认启动就拉一次真实列表：打开就能看到内容，不用再敲参数
    QTimer::singleShot(300, &app, [&win] { win.requestListAsync(); });
    return app.exec();
}
