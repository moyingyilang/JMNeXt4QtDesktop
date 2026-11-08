import QtQuick
import QtQuick.Layouts
import "ui/Tokens.js" as Tokens

/*
 * 列表页头部 —— 对齐 Kotlin 桌面版各列表页的统一开头：
 *   左侧「标题 + 计数」，右侧一排胶囊操作（换一批 / 切换成列表 / 排序…），
 *   下方一行说明文字（可选）。
 *
 * 抽出组件的理由：Kotlin 的 随机本子 / 收藏 / 历史 / 周更 都是这个开头，
 * 此前 Qt 版每屏各写一套标题与按钮，字号与间距都不一致。
 */
ColumnLayout {
    id: header

    property string title: ""
    property string countText: ""
    property string description: ""
    property color titleColor: "#1d1b20"
    property color bodyColor: "#49454f"
    property color accentColor: "#6750a4"
    /** [{ key, label, primary }] */
    property var actions: []

    signal actionClicked(string key)

    Layout.fillWidth: true
    spacing: Tokens.space8

    RowLayout {
        Layout.fillWidth: true
        spacing: Tokens.space12

        Text {
            text: header.title
            color: header.titleColor
            font.pixelSize: 28
            font.bold: true
        }

        Text {
            visible: header.countText.length > 0
            text: header.countText
            color: header.bodyColor
            font.pixelSize: 12
            Layout.alignment: Qt.AlignVCenter
        }

        Item { Layout.fillWidth: true }

        Repeater {
            model: header.actions

            delegate: UiPillButton {
                text: modelData.label
                primary: modelData.primary === true
                accentColor: header.accentColor
                onClicked: header.actionClicked(modelData.key)
            }
        }
    }

    Text {
        Layout.fillWidth: true
        visible: header.description.length > 0
        text: header.description
        color: header.bodyColor
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }
}
