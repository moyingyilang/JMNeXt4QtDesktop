import QtQuick
import QtQuick.Controls
import "ui/Tokens.js" as Tokens

/* 空/失败态：一句标题 + 一句说明 + 可选动作按钮（对应主项目 StateBox / ErrorBox）。 */
Column {
    id: empty

    property string title: ""
    property string description: ""
    property string actionText: ""
    property color titleColor: "#e6e6e6"
    property color bodyColor: "#9aa0a8"
    property color accentColor: "#5b8def"

    signal action()

    spacing: Tokens.space8
    width: parent ? parent.width : 0
    topPadding: Tokens.space24

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        text: empty.title
        color: empty.titleColor
        font.pixelSize: Tokens.fontSection
        horizontalAlignment: Text.AlignHCenter
    }

    Text {
        visible: empty.description.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        text: empty.description
        color: empty.bodyColor
        font.pixelSize: Tokens.fontCaption
        horizontalAlignment: Text.AlignHCenter
        width: parent.width
    }

    Button {
        visible: empty.actionText.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        text: empty.actionText
        onClicked: empty.action()
    }
}
