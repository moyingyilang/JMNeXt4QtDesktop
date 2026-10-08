import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// P2 第一屏：首页列表（标题 + 封面 + 点击进入阅读）。
// 数据与图片都来自共用后端：backend（JmBackend）负责数据，image://jm/ 由 JmImageProvider 负责图片。
// 说明：这里的布局是"结构级"复刻的起点，P3 才会按主项目五套表面工艺细化观感。
ApplicationWindow {
    id: root
    width: 1180; height: 780
    visible: true
    title: "JMNeXt4QtDesktop (QML) " + appVersion
    color: root.cBg

    // 列表数据：标题、aid、封面 URL（URL 由后端在取封面时给出，见 coverUrlReady）
    ListModel { id: listModel }

    RowLayout {
        id: rootLayout
        anchors.fill: parent
        spacing: 0

        // 左栏：作品列表
        ColumnLayout {
            id: leftColumn
            Layout.preferredWidth: 380
            Layout.minimumWidth: 380
            Layout.maximumWidth: 380
            Layout.fillHeight: true
            spacing: 6

            SearchScreen {
                id: searchScreen
                Layout.fillWidth: true
                listModel: listModel
                secondaryColor: "#9aa0a8"
                onSearchRequested: (word) => backend.search(word, 1)
            }

            HomeScreen {
                id: homeScreen
                Layout.fillWidth: true
                Layout.fillHeight: true
                listModel: listModel
                pageStatus: root.pageStatus
                hoverColor: root.cSurface2
                textColor: root.cText
                secondaryColor: root.cTextSecondary
                cardRadius: root.radiusMd
                onCardClicked: (aid) => {
                    root.albumAid = aid
                    backend.loadAlbum(aid)
                }
                onRetryClicked: backend.loadList()
                onMoreClicked: {
                    backend.loadMore()
                }
            }
        }

        // 右栏：阅读器（有页面后覆盖详情视图）
        ReaderScreen {
            id: readerPane
            visible: root.reading
            Layout.fillWidth: true
            Layout.fillHeight: true
            pageUrl: root.pageUrl
            pageStatus: root.pageStatus
            bgColor: root.cBg
            secondaryColor: root.cTextSecondary
            onStepRequested: (delta) => backend.step(delta)
        }

        // 右栏：详情视图（封面图待扩展图片提供器后补上；本轮先文字 + 章节列表）
        DetailScreen {
            id: rightPane
            visible: !root.reading
            Layout.fillWidth: true
            Layout.fillHeight: true
            albumAid: root.albumAid
            albumName: root.albumName
            albumAuthor: root.albumAuthor
            albumTags: root.albumTags
            pageStatus: root.pageStatus
            chapterModel: chapterModel
            surfaceColor: root.cSurface1
            strokeColor: root.cStroke
            onChapterClicked: (cid) => backend.openChapterId(cid, 0)
        }
    }

    // 状态行（后端 status 信号）
    // 主题 token（由 C++ 在 load 之前以 QVariantMap 传入，onCompleted 里复制过来）
    property string cBg: "#1e1f22"
    property string cSurface1: "#26282c"
    property string cSurface2: "#2f3237"
    property string cText: "#e6e6e6"
    property string cTextSecondary: "#b9bcc2"
    property string cAccent: "#5b8def"
    property string cStroke: "#3a3d43"
    property int radiusMd: 6

    property string pageStatus: ""
    // 列表分页状态：条数不再增长即视为"已到底"（后端暂未暴露 hasMore，先在界面侧推导）
    property int lastListCount: 0
    property bool listExhausted: false
    property bool listLoading: false
    // 临时布局探针：启动 3 秒后打印各层宽度（不需要页面，避免网络波动影响测量）
    Timer {
        interval: 3000; running: true; repeat: false
        onTriggered: console.log("布局探针：窗口 " + Math.round(root.width) + "x" + Math.round(root.height)
                                 + "，RowLayout " + Math.round(rootLayout.width) + "x" + Math.round(rootLayout.height)
                                 + "，左栏 " + Math.round(leftColumn.width)
                                 + "，右栏 " + Math.round(rightPane.width) + "x" + Math.round(rightPane.height)
                                 + "（右栏可见=" + rightPane.visible + "）")
    }
    // 阅读器状态：**不在绑定里直接读 context property**（编译后的 QML 绑定看不到它们，会当 null），
    // 改由 Connections 的 onPageChanged 计算后赋给根属性，绑定只读根属性。
    property bool reading: false
    // 诊断用：页面就绪 1.5 秒后再量一次几何（onStatusChanged 那一刻可能还没完成布局）
    Timer {
        id: geomProbe
        interval: 1500
        onTriggered: console.log("几何：阅读器 " + Math.round(readerPane.width) + "x" + Math.round(readerPane.height)
                                + "，图片 " + Math.round(pageImage.width) + "x" + Math.round(pageImage.height)
                                + "，绘制 " + Math.round(pageImage.paintedWidth) + "x" + Math.round(pageImage.paintedHeight))
    }
    property string pageUrl: ""
    property string albumName: ""
    property string albumAuthor: ""
    property string albumTags: ""
    property string albumAid: ""
    ListModel { id: chapterModel }

    Connections {
        target: backend

        function onListReady(titles, ids) {
            listModel.clear()
            for (var i = 0; i < titles.length; ++i)
                listModel.append({ "title": titles[i], "aid": ids[i], "coverUrl": "" })
            console.log("QML 列表已填充：" + listModel.count + " 条")
            root.listLoading = false
            if (listModel.count <= root.lastListCount) {
                root.listExhausted = true
                console.log("QML 列表已到底：" + listModel.count + " 条")
            }
            root.lastListCount = listModel.count
            backend.loadCovers(20)          // 让前 20 条的封面 URL 与图片就位
        }

        function onAlbumReady(name, author, tags) {
            root.albumName = name; root.albumAuthor = author
            root.albumTags = "标签：" + tags.join("、")
            console.log("QML 收到详情：" + name + "（标签 " + tags.length + "）")
        }

        function onChaptersReady(names, ids) {
            chapterModel.clear()
            for (var i = 0; i < names.length; ++i)
                chapterModel.append({ "name": names[i], "cid": ids[i] })
            console.log("QML 章节列表已填充：" + chapterModel.count + " 条")
            if (typeof autoReadAid !== "undefined" && autoReadAid === root.albumAid && ids.length > 0)
                backend.openChapterId(ids[0], 0)
        }

        function onCurrentAidChanged() { root.albumAid = backend.currentAid }

        function onStatus(text) { root.pageStatus = text }
        function onFailed(text) { root.pageStatus = "失败：" + text }
        function onPageReady(image, statusText) { root.pageStatus = statusText }

        function onPageChanged() {
            root.reading = backend.pageSeq > 0
            root.pageUrl = root.reading
                    ? "file://" + backend.pagePath + "?v=" + backend.pageSeq
                    : ""
        }
    }

    Component.onCompleted: {
        // --search 带词时不加载首页（否则会把搜索结果覆盖掉——第一次验证时就撞上了）
        if (typeof initialSearch === "undefined" || initialSearch.length === 0) {
            if (typeof themeInit !== "undefined" && themeInit) {
            root.cBg = themeInit.surfaceMica
            root.cSurface1 = themeInit.surface1
            root.cSurface2 = themeInit.surface2
            root.cText = themeInit.text
            root.cTextSecondary = themeInit.textSecondary
            root.cAccent = themeInit.accent
            root.cStroke = themeInit.stroke
            root.radiusMd = themeInit.radiusMd
        }
        console.log("QML 动效：列表悬停过渡与按压弹性已启用")
        console.log("QML 主题：" + themeName + " 背景 " + root.cBg + " 强调 " + root.cAccent
                    + " 文字 " + root.cText + " 圆角 " + root.radiusMd)
        console.log("QML 首页列表：开始加载")
            backend.loadList()
        }
    }
}
