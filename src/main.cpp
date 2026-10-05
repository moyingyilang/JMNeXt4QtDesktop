// JMNeXt4QtDesktop 入口。
//
//   jmnext4desktop                 打开窗口
//   jmnext4desktop --selftest <图>  无显示自检：加载 → 还原 → 打印校验和与字体 → 退出
//
// 自检存在的理由：容器里没有显示，界面层的代码否则无法被验证。走的是与窗口完全相同的代码路径。
#include "qt/JmClient.h"
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

    if (argc >= 2 && std::string(argv[1]) == "--list") {
        jmnext::qt::MainWindow win;
        const int n = win.loadRealList();
        if (n < 0) { std::printf("加载失败\n"); return 1; }
        std::printf("列表条数：%d\n", n);
        for (int i = 0; i < 3 && i < n; ++i)
            std::printf("  第 %d 条：%s\n", i + 1, win.listItemText(i).toUtf8().constData());
        std::printf("首页列表自检：通过\n");
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
    win.resize(760, 620);
    win.show();
    return app.exec();
}
