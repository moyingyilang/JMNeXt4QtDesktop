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
                onAboutRequested: root.showAbout = true
                onSettingsRequested: root.showSettings = true
                onCategoryRequested: root.showCategory = true
                onMoreRequested: root.showMore = true
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
                onRandomClicked: {
                    root.listTitle = "随机推荐"
                    root.listPath = "random_recommend"
                    root.listQuery = ""
                    root.listTag = "random"
                    root.showList = true
                }
            }
        }

        // 右栏：阅读器（有页面后覆盖详情视图）
        ReaderScreen {
            id: readerPane
            visible: root.reading
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
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
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            Layout.fillWidth: true
            Layout.fillHeight: true
            albumAid: root.albumAid
            pageStatus: root.pageStatus
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
    /// 是否显示"关于"屏（第 37 轮新增）
    property bool showAbout: false
    property bool showSettings: false
    property bool showCategory: false
    property bool showMore: false
    /// 通用列表屏（第 61 轮）：任意"作品列表"型接口共用一屏
    property bool showList: false
    property bool showLogin: false
    property string listTitle: ""
    property string listPath: ""
    property string listQuery: ""
    property string listTag: "list"
    property bool twoPage: false
    // 诊断用：页面就绪 1.5 秒后再量一次几何（onStatusChanged 那一刻可能还没完成布局）
    Timer {
        id: geomProbe
        interval: 1500
        onTriggered: console.log("几何：阅读器 " + Math.round(readerPane.width) + "x" + Math.round(readerPane.height)
                                + "，图片 " + Math.round(pageImage.width) + "x" + Math.round(pageImage.height)
                                + "，绘制 " + Math.round(pageImage.paintedWidth) + "x" + Math.round(pageImage.paintedHeight))
    }
    property string pageUrl: ""
    property string albumAid: ""

    Connections {
        target: backend

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

        AboutScreen {
            id: aboutPane
            visible: root.showAbout
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            Layout.fillWidth: true
            Layout.fillHeight: true
            surfaceColor: root.cSurface1
            strokeColor: root.cStroke
            titleColor: root.cText
            bodyColor: root.cTextSecondary
            accentColor: root.cAccent
            onCloseRequested: root.showAbout = false
        }

        SettingsScreen {
            id: settingsPane
            visible: root.showSettings
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            Layout.fillWidth: true
            Layout.fillHeight: true
            surfaceColor: root.cSurface1
            strokeColor: root.cStroke
            titleColor: root.cText
            bodyColor: root.cTextSecondary
            accentColor: root.cAccent
            twoPage: root.twoPage
            onCloseRequested: root.showSettings = false
            onTwoPageToggled: (on) => {
                root.twoPage = on
                backend.setTwoPage(on)
            }
            onBlockWordsChanged: (words) => backend.setBlockWords(words)
        }

        CategoryScreen {
            id: categoryPane
            visible: root.showCategory
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            Layout.fillWidth: true
            Layout.fillHeight: true
            surfaceColor: root.cSurface1
            strokeColor: root.cStroke
            titleColor: root.cText
            bodyColor: root.cTextSecondary
            accentColor: root.cAccent
            onCloseRequested: root.showCategory = false
            onTagClicked: (tag) => console.log("QML 分类标签被点击（筛选待接）：" + tag)
        }

        MoreScreen {
            id: morePane
            visible: root.showMore
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            Layout.fillWidth: true
            Layout.fillHeight: true
            surfaceColor: root.cSurface1
            strokeColor: root.cStroke
            titleColor: root.cText
            bodyColor: root.cTextSecondary
            accentColor: root.cAccent
            onCloseRequested: root.showMore = false
            onEntryClicked: (key) => {
                if (key === "category") { root.showMore = false; root.showCategory = true }
                else if (key === "about") { root.showMore = false; root.showAbout = true }
                else if (key === "settings") { root.showMore = false; root.showSettings = true }
                else {
                    // 接口路径与参数取自 shared/data/JmRepository.kt（权威参照）
                    var m = ({
                        "random":    ["随机推荐", "random_recommend", ""],
                        "creator":   ["画师列表", "creator_author", "page=1"],
                        "week":      ["每周更新", "week", ""],
                        "favorites": ["我的收藏（需登录）", "favorite", "page=1"],
                        "history":   ["阅读历史（需登录）", "watch_list", "page=1"],
                        "tags":      ["标签收藏", "tags_favorite", ""],
                        "notify":    ["通知（需登录）", "notifications", ""],
                        "auth":      ["__login__", "", ""]
                    })[key]
                    if (m && m[1] === "__login__") { root.showMore = false; root.showLogin = true; return }
                    if (m) {
                        root.listTitle = m[0]; root.listPath = m[1]; root.listQuery = m[2]
                        root.listTag = key
                        root.showMore = false
                        root.showList = true
                    } else {
                        console.log("QML 更多：该项还没有对应接口 -> " + key)
                    }
                }
            }
        }

        ComicListScreen {
            id: comicListPane
            visible: root.showList
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: root.listTitle
            apiPath: root.listPath
            apiQuery: root.listQuery
            tagId: root.listTag
            titleColor: root.cText
            bodyColor: root.cTextSecondary
            surfaceColor: root.cSurface2
            accentColor: root.cAccent
            pageStatus: root.pageStatus
            onCloseRequested: root.showList = false
            onComicClicked: (aid) => {
                root.albumAid = aid
                backend.loadAlbum(aid)
                root.showList = false
            }
        }

        LoginScreen {
            id: loginPane
            visible: root.showLogin
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            Layout.fillWidth: true
            Layout.fillHeight: true
            surfaceColor: root.cSurface1
            strokeColor: root.cStroke
            titleColor: root.cText
            bodyColor: root.cTextSecondary
            accentColor: root.cAccent
            onCloseRequested: root.showLogin = false
            onSubmit: (u, p) => backend.login(u, p)
            onLogoutRequested: backend.logout()
            Connections {
                target: backend
                function onLoginResult(ok, msg) { loginPane.message = (ok ? "成功：" : "失败：") + msg }
            }
        }
}
