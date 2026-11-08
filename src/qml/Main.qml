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

    /** 壁纸地址（file://…），空则只用渐变。由 --wallpaper 或设置页写入。 */
    property url wallpaperSource: (typeof startupWallpaper !== "undefined") ? startupWallpaper : ""

    /** 全局背景引用：供 Glass 做真背景模糊（Qt 6.5+）。 */
    property Item appBackdrop: null
    /** 玻璃调试输出计数（JMNEXT_GLASS_DEBUG=1 时最多打印 4 条）。 */
    property int glassDebugCount: 0
    color: root.cBg

    // 列表数据：标题、aid、封面 URL（URL 由后端在取封面时给出，见 coverUrlReady）
    ListModel { id: listModel }

    // 环境背景（对应 Kotlin 的 AmbientBackdrop）：铺满窗口，内容画在其上
    AmbientBackdrop {
        id: ambientBg
        anchors.fill: parent
        baseColor: root.cBg
        glowColor: root.cAccent
        wallpaper: root.wallpaperSource
        Component.onCompleted: root.appBackdrop = ambientBg
    }

    // 整窗玻璃层：对应主项目的 WindowGlass 材质。
    // 它铺在壁纸之上、内容之下，于是"玻璃模糊壁纸"这件事在界面上直接可见。
    Glass {
        id: windowGlass
        anchors.fill: parent
        tint: root.cSurface2
        tintAlpha: 0.72
        strokeColor: "transparent"
        radiusMd: 0
    }

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
            spacing: 8

            SearchScreen {
                id: searchScreen
                Layout.fillWidth: true
                listModel: listModel
                secondaryColor: "#9aa0a8"
                onSearchRequested: (word) => backend.search(word, 1)
                onSearchOrderedRequested: (word, order) => backend.searchOrdered(word, 1, order)
                titleColor: root.cText
                accentColor: root.cAccent
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
                reading: root.reading
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
            previewUrl: root.previewUrl
            twoPage: root.twoPage
            chapterNames: root.chapterNames
            chapterIds: root.chapterIds
            onChapterChosen: (cid) => backend.openChapterId(cid, 0)
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
                onFavoriteToggled: (aid) => {
                    backend.action("favorite", "favorite", "aid=" + aid)
                    // 本地也记一份：断网或未登录时收藏页仍有内容（对应主项目 FavoriteStore）
                    backend.rememberFavorite(aid, root.albumName ? root.albumName : "", "")
                }
                onTrackToggled: (aid) => backend.action("track", "album_sertracking", "id=" + aid)
                onCommentsRequested: (aid) => { root.commentAid = aid; root.showComments = true }
        }
    }

    // 随机 FAB：对应主项目 home/RandomFab.kt —— 悬挂在窗口右下角。
    // 之前它放在左栏 ColumnLayout 内，会与「加载更多」抢位置；移到窗口层后与主项目一致。
    FloatingBottomBar {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 24
        anchors.bottomMargin: 24
        visible: listModel.count > 0
        tint: root.cSurface2
        strokeColor: root.cStroke
        Button {
            text: "随机"
            background: null
            padding: 16
            contentItem: Text {
                text: parent.text
                color: root.cText
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: {
                console.log("QML 随机 FAB")
                homeScreen.randomClicked()
            }
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
    property bool showHistory: false
    property bool showFavorites: false
    property bool showNotifications: false
    property bool showCategory: false
    property bool showMore: false
    /// 通用列表屏（第 61 轮）：任意"作品列表"型接口共用一屏
    property bool showList: false
    property bool showLogin: false
    property bool showWeek: false
    property bool showComments: false
    property bool showProfile: false
    property string commentAid: ""
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
    property string previewUrl: ""
    property string albumAid: ""
    property string albumName: ""
    // 章节缓存：给阅读器的章节选择器用（chaptersReady 时写入）
    property var chapterNames: []
    property var chapterIds: []

    /// 把当前 themeInit 的 token 复制到根属性（启动与运行时切换都走这里）。
    function applyThemeTokens() {
        if (typeof themeInit === "undefined" || !themeInit) return
        root.cBg = themeInit.surfaceMica
        root.cSurface1 = themeInit.surface1
        root.cSurface2 = themeInit.surface2
        root.cText = themeInit.text
        root.cTextSecondary = themeInit.textSecondary
        root.cAccent = themeInit.accent
        root.cStroke = themeInit.stroke
        root.radiusMd = themeInit.radiusMd
    }

    // 运行时切换主题后重读 token（ThemeController::apply 会重写 themeInit 并发 applied）。
    Connections {
        target: (typeof themeController !== "undefined") ? themeController : null
        function onApplied() {
            root.applyThemeTokens()
            console.log("QML 主题已切换：" + themeName + " / " + (darkTheme ? "深色" : "浅色"))
        }
    }

    Connections {
        target: backend

        function onCurrentAidChanged() { root.albumAid = backend.currentAid }

        // 成功时要清掉上次失败的提示：onFailed 写进 pageStatus 的文字不会自己消失，
        // 否则界面上会一直挂着"失败：列表失败：网络请求失败"。
        function onListReady(titles, ids) {
            if (root.pageStatus.indexOf("失败") >= 0) root.pageStatus = ""
        }
        function onAlbumReady(name, author, tags) {
            if (root.pageStatus.indexOf("失败") >= 0) root.pageStatus = ""
            root.albumName = name
            // 本地历史：打开作品即记录（对应主项目 HistoryStore 的写入时机）
            if (typeof backend !== "undefined" && backend)
                backend.rememberHistory(backend.currentAid, name)
        }
        function onChaptersReady(names, ids) {
            if (root.pageStatus.indexOf("失败") >= 0) root.pageStatus = ""
            root.chapterNames = names
            root.chapterIds = ids
        }

        function onStatus(text) { root.pageStatus = text }
        function onFailed(text) { root.pageStatus = "失败：" + text }
        function onPageReady(image, statusText) { root.pageStatus = statusText }

        // 双页右页：与 pageUrl 同一套（读 backend 属性，不在绑定里直接读 context property）
        function onPreviewChanged() {
            root.previewUrl = backend.previewSeq > 0
                    ? "file://" + backend.previewPath + "?v=" + backend.previewSeq
                    : ""
        }

        function onPageChanged() {
            root.reading = backend.pageSeq > 0
            root.pageUrl = root.reading
                    ? "file://" + backend.pagePath + "?v=" + backend.pageSeq
                    : ""
        }
    }

    Component.onCompleted: {
        // --panel：启动即打开指定面板（favorites/notifications/history/settings/about/week）
        if (typeof startupPanel !== "undefined" && startupPanel) {
            if (startupPanel === "favorites") root.showFavorites = true
            else if (startupPanel === "notifications") root.showNotifications = true
            else if (startupPanel === "history") root.showHistory = true
            else if (startupPanel === "settings") root.showSettings = true
            else if (startupPanel === "about") root.showAbout = true
            else if (startupPanel === "week") root.showWeek = true
        }

        // --search 带词时不加载首页（否则会把搜索结果覆盖掉——第一次验证时就撞上了）
        if (typeof initialSearch === "undefined" || initialSearch.length === 0) {
            root.applyThemeTokens()
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
            // 这些全屏面板是 ApplicationWindow 的直接子项，不在布局里，
            // Layout.fill* 对它们无效，尺寸会塌成一小块（截图里挤在左上角）。
            anchors.fill: parent
            anchors.margins: 32
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
            // 全屏面板是 ApplicationWindow 的直接子项，不在布局内，
            // Layout.fill* 对它无效，尺寸会塌成左上角一小块（截图里已复现）。
            anchors.fill: parent
            anchors.margins: 32
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

        HistoryScreen {
            id: historyPane
            visible: root.showHistory
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            anchors.fill: parent
            anchors.margins: 32
            surfaceColor: root.cSurface1
            strokeColor: root.cStroke
            titleColor: root.cText
            bodyColor: root.cTextSecondary
            accentColor: root.cAccent
            onCloseRequested: root.showHistory = false
            onComicClicked: (aid) => {
                root.showHistory = false
                root.albumAid = aid
                backend.loadAlbum(aid)
            }
        }

        FavoritesScreen {
            id: favoritesPane
            visible: root.showFavorites
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            anchors.fill: parent
            anchors.margins: 32
            surfaceColor: root.cSurface1
            strokeColor: root.cStroke
            titleColor: root.cText
            bodyColor: root.cTextSecondary
            accentColor: root.cAccent
            onCloseRequested: root.showFavorites = false
            onComicClicked: (aid) => {
                root.showFavorites = false
                root.albumAid = aid
                backend.loadAlbum(aid)
            }
        }

        NotificationsScreen {
            id: notificationsPane
            visible: root.showNotifications
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            anchors.fill: parent
            anchors.margins: 32
            surfaceColor: root.cSurface1
            strokeColor: root.cStroke
            titleColor: root.cText
            bodyColor: root.cTextSecondary
            accentColor: root.cAccent
            onCloseRequested: root.showNotifications = false
        }

        CategoryScreen {
            id: categoryPane
            visible: root.showCategory
            opacity: visible ? 1 : 0
            anchors.fill: parent
            anchors.margins: 32
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            surfaceColor: root.cSurface1
            strokeColor: root.cStroke
            titleColor: root.cText
            bodyColor: root.cTextSecondary
            accentColor: root.cAccent
            onCloseRequested: root.showCategory = false
            onTagClicked: (tag) => {
                // 标签点击后把结果页接上（此前只打日志，点标签没有反应）
                root.listTitle = "标签：" + tag
                root.listPath = "tag"
                root.listQuery = tag
                root.listTag = "tag"
                root.showCategory = false
                root.showList = true
            }
            onResultClicked: (aid) => {
                root.showCategory = false
                root.albumAid = aid
                backend.loadAlbum(aid)
            }
        }

        MoreScreen {
            id: morePane
            visible: root.showMore
            opacity: visible ? 1 : 0
            anchors.fill: parent
            anchors.margins: 32
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
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
                        "week":      ["__week__", "", ""],
                        "favorites": ["__favorites__", "", ""],
                        "history":   ["__history__", "", ""],
                        "tags":      ["标签收藏", "tags_favorite", ""],
                        "notify":    ["__notify__", "", ""],
                        "auth":      ["__login__", "", ""],
                        "profile":   ["__profile__", "", ""],
                        "checkin":   ["__action__", "daily_chk", ""]
                    })[key]
                    if (m && m[1] === "__login__") { root.showMore = false; root.showLogin = true; return }
                    if (m && m[1] === "__profile__") { root.showMore = false; root.showProfile = true; return }
                    if (m && m[1] === "__week__") { root.showMore = false; root.showWeek = true; return }
                    if (m && m[1] === "__history__") { root.showMore = false; root.showHistory = true; return }
                    if (m && m[1] === "__favorites__") { root.showMore = false; root.showFavorites = true; return }
                    if (m && m[1] === "__notify__") { root.showMore = false; root.showNotifications = true; return }
                    if (m && m[1] === "__action__") {
                        root.showMore = false
                        backend.action("checkin", m[1], m[2])
                        root.pageStatus = "已请求：" + m[0]
                        return
                    }
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
            anchors.fill: parent
            anchors.margins: 32
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
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
            anchors.fill: parent
            anchors.margins: 32
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
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

        WeekScreen {
            anchors.fill: parent
            anchors.margins: 32
            id: weekPane
            visible: root.showWeek
            titleColor: root.cText
            bodyColor: root.cTextSecondary
            surfaceColor: root.cSurface2
            accentColor: root.cAccent
            onCloseRequested: root.showWeek = false
            onComicClicked: (aid) => {
                root.albumAid = aid
                backend.loadAlbum(aid)
                root.showWeek = false
            }
        }

        CommentScreen {
            anchors.fill: parent
            anchors.margins: 32
            id: commentPane
            visible: root.showComments
            aid: root.commentAid
            titleColor: root.cText
            bodyColor: root.cTextSecondary
            surfaceColor: root.cSurface2
            accentColor: root.cAccent
            onCloseRequested: root.showComments = false
        }

        ProfileScreen {
            anchors.fill: parent
            anchors.margins: 32
            id: profilePane
            visible: root.showProfile
            titleColor: root.cText
            bodyColor: root.cTextSecondary
            surfaceColor: root.cSurface2
            accentColor: root.cAccent
            onCloseRequested: root.showProfile = false
            onLoginRequested: { root.showProfile = false; root.showLogin = true }
        }
}
