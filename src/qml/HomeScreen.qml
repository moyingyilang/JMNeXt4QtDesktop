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
    property color hoverColor: "#2f3237"
    property color textColor: "#e6e6e6"
    property color secondaryColor: "#b9bcc2"
    property int cardRadius: 6

    signal cardClicked(string aid)
    signal retryClicked()
    signal moreClicked()

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
}
