import QtQuick
import "ui/Tokens.js" as Tokens

/*
 * 左侧导航栏 —— 对齐 Kotlin 桌面版（Compose Desktop）的 Shell：
 * 顶部品牌「JMNeXt」，下方是 15 个入口，选中项用强调色浅底 + 圆角高亮。
 *
 * 为什么单独成组件：此前 Qt 版根本没有侧栏（左栏直接放搜索框 + 列表 + FAB），
 * 与 Kotlin 版的第一步就不一样；导航是外壳级的，必须独立出来。
 */
Rectangle {
    id: nav

    property color surfaceColor: "#eeedfa"
    property color textColor: "#3b3b45"
    property color selectedColor: "#dcdcf5"
    property color accentColor: "#2f6fe4"
    property string current: "home"

    signal navigate(string key)
    signal loginRequested()

    implicitWidth: 220
    color: surfaceColor

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: Tokens.space16
        spacing: Tokens.space4

        Text {
            // 品牌：Kotlin 版左上角是纯文字 JMNeXt
            text: "JMNeXt"
            color: nav.accentColor
            font.pixelSize: 20
            font.bold: true
            leftPadding: Tokens.space16
            bottomPadding: Tokens.space12
        }

        Repeater {
            model: [
                { key: "home", label: "首页" },
                { key: "search", label: "搜索" },
                { key: "category", label: "分类" },
                { key: "week", label: "周刊" },
                { key: "random", label: "随机本子" },
                { key: "favorites", label: "收藏" },
                { key: "history", label: "历史" },
                { key: "follow", label: "追更" },
                { key: "tags", label: "标签" },
                { key: "creator", label: "画师与作品库" },
                { key: "notifications", label: "通知" },
                { key: "block", label: "屏蔽设置" },
                { key: "appearance", label: "外观" },
                { key: "about", label: "关于" },
                { key: "profile", label: "我的" }
            ]

            delegate: Rectangle {
                width: nav.width - Tokens.space8 * 2
                height: 34
                x: Tokens.space8
                radius: Tokens.radiusMd
                color: nav.current === modelData.key ? nav.selectedColor : "transparent"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: Tokens.space12
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.label
                    color: nav.current === modelData.key ? nav.accentColor : nav.textColor
                    font.pixelSize: Tokens.fontBody
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: nav.navigate(modelData.key)
                }
            }
        }
    }
}
