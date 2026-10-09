import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 首页（从 Main.qml 抽出的第一屏，对应 Kotlin 版 ui/screens/home/HomeScreen.kt）。
 *
 * 本轮只搬**纯渲染**三块：列表视图 + StateBox + LoadMoreFooter。
 * 数据（ListModel）与后端回调仍由调用方持有，通过属性传入、用信号回传 ——
 * 这样这一刀不触碰 Main.qml 里那些互相咬合的 Connections 逻辑。
 *
 * 改写依据：抽取前逐行读过原文（Main.qml 57–106 行），真正要改的只有：
 *   root.c* / root.radiusMd -> 组件属性；root.pageStatus/listLoading/listExhausted -> 组件属性；
 *   backend.loadAlbum/loadList/loadMore 与 root.albumAid -> 三个信号；
 *   delegate 里的 model.title/aid/coverUrl 与 StateBox 的 anchors.fill: listView **原样不动**。
 *
 * 根用 ColumnLayout：这样原文里的 Layout.fillWidth/fillHeight 可以原样保留。
 */
ColumnLayout {
    id: home

    property var listModel
    property string pageStatus: ""
    property bool loading: false
    property bool exhausted: false

    // 主题色由调用方注入（token 是 Main.qml 的运行时属性，写死会让四套风格切换失效）
    // 列表分页状态：全部由本组件维护（原在 Main.qml，第 31–33 轮下沉）
    property int lastListCount: 0

    property color hoverColor: "#2f3237"
    property color textColor: "#e6e6e6"
    property color secondaryColor: "#b9bcc2"
    property int cardRadius: 6

    signal cardClicked(string aid)
    signal retryClicked()
    signal moreClicked()
    /// 随机作品的入口（对应 Kotlin 版 home/RandomFab.kt）
    signal randomClicked()

    ListView {
        id: listView
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: home.listModel
        spacing: 2

        delegate: ComicCard {
            title: model.title
            aid: model.aid
            coverUrl: model.coverUrl
            hoverColor: home.hoverColor
            textColor: home.textColor
            cardRadius: home.cardRadius
            onClicked: {
                console.log("QML 点击作品 aid=" + aid)
                home.cardClicked(aid)
            }
        }
    }

    // 空/失败状态：列表为空时给出明确说明与重试入口（对应 Kotlin 的 ErrorBox / MessageState）
    StateBox {
        anchors.fill: listView
        visible: home.listModel.count === 0
        kind: home.pageStatus.indexOf("失败") >= 0 ? "error" : "message"
        title: home.pageStatus.length > 0 ? home.pageStatus : "还没有内容"
        description: "点“重试”重新载入列表"
        canRetry: true
        textColor: home.textColor
        secondaryColor: home.secondaryColor
        onRetry: home.retryClicked()
    }

    LoadMoreFooter {
        // 列表为空时不显示页脚：此时 StateBox 已占满列表区，再显示"加载更多"既无意义也会打架
        visible: home.listModel.count > 0
        Layout.fillWidth: true
        loading: home.loading
        exhausted: home.exhausted
        textColor: home.textColor
        secondaryColor: home.secondaryColor
        onLoadMore: {
            console.log("QML 请求加载更多")
            home.moreClicked()
        }
    }

    // 封面 URL 回填：数据在本组件手里，所以这条回调归它管（第 31 轮下沉的第一个函数）。
    // 注意：必须放在根对象（ColumnLayout）**内部** —— 追加到文件末尾会变成第二个根对象，QML 直接语法报错。
    Connections {
        target: backend

        function onCoverUrlReady(index, url) {
            if (index >= 0 && index < home.listModel.count) {
                home.listModel.setProperty(index, "coverUrl", url)
                if (index === 0) console.log("QML 收到封面 URL：index 0")
            }
        }

    // 加载更多的结果走 listAppended（worker 侧已拼接好，这里只做"追加"）——
    // 注意**不能**像 onListReady 那样先 clear()，否则会把已有列表清掉。
    // loading 状态自本组件维护（原先在 Main.qml，下沉后不再由外层注入）。
    function onListAppended(titles, ids) {
        for (var j = 0; j < titles.length; ++j)
            home.listModel.append({ "title": titles[j], "aid": ids[j], "coverUrl": "" })
        home.loading = false
        console.log("QML 追加 " + titles.length + " 条，当前共 " + home.listModel.count + " 条")
        backend.loadCovers(20)
    }

    // 首屏填充（首页列表）与搜索结果（替换语义）都走这条：两种情况下都是"清空后重填"。
    // 注意与 onListAppended 的区别：那条是**追加**（加载更多），这条是**替换**。
    function onListReady(titles, ids) {
        home.listModel.clear()
        for (var i = 0; i < titles.length; ++i)
            home.listModel.append({ "title": titles[i], "aid": ids[i], "coverUrl": "" })
        console.log("QML 列表已填充：" + home.listModel.count + " 条")
        home.loading = false
        if (home.listModel.count <= home.lastListCount) {
            home.exhausted = true
            console.log("QML 列表已到底：" + home.listModel.count + " 条")
        }
        home.lastListCount = home.listModel.count
        backend.loadCovers(20)          // 让前 20 条的封面 URL 与图片就位
    }
    }

    // 随机 FAB（对应 Kotlin 版 home/RandomFab.kt）：悬浮在列表右下角
    FloatingBottomBar {
        Layout.alignment: Qt.AlignRight
        Layout.rightMargin: 12
        Layout.bottomMargin: 6
        visible: home.listModel.count > 0
        tint: home.hoverColor
        strokeColor: home.secondaryColor
        Button {
            text: "随机"
            onClicked: {
                console.log("QML 首页：随机 FAB")
                home.randomClicked()
            }
        }
    }
}
