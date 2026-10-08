import QtQuick
import QtQuick.Controls

/*
 * 分页页脚（对应 Kotlin 版 ui/components/LoadMoreFooter.kt）。
 *
 * Kotlin 侧参数：loading / error / exhausted / onLoadMore / onRetry。
 * QML 侧同构：属性 + `signal loadMore()`（父级连接；QML 不能像 Compose 那样直接收 lambda）。
 *
 * 为什么要有它：Qt 的 QML 前端此前**没有加载更多**（只有 widget 版有），
 * 首页列表只能看到第一页 —— 这是与 Kotlin 版的一处真实功能缺口。
 *
 * 颜色由调用方注入（与其它组件同一理由：主题 token 是运行时赋值的属性）。
 */
Item {
    id: footer

    property bool loading: false
    property bool exhausted: false
    property string error: ""

    property color textColor: "#e6e6e6"
    property color secondaryColor: "#b9bcc2"
    property color accentColor: "#5b8def"

    signal loadMore()

    implicitHeight: 44
    implicitWidth: 200

    BusyIndicator {
        id: spinner
        anchors.centerIn: parent
        width: 22
        height: 22
        running: footer.loading
        visible: running
    }

    Text {
        anchors.centerIn: parent
        visible: !footer.loading
        color: footer.error.length > 0 ? "#ff7b72"
             : (footer.exhausted ? footer.secondaryColor : footer.accentColor)
        font.pixelSize: 12
        text: footer.error.length > 0 ? ("加载失败：" + footer.error)
            : (footer.exhausted ? "已经到底了" : "加载更多")
    }

    MouseArea {
        anchors.fill: parent
        enabled: !footer.loading && !footer.exhausted
        onClicked: footer.loadMore()
    }
}
