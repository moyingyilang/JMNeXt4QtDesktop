import QtQuick
import "ui/Tokens.js" as Tokens

/*
 * 通用列表行：左侧主文案 + 可选副文案，右侧留一个插槽（开关/箭头/数值）。
 * 主项目里这类行出现十几次，桌面端此前每处都是一段 RowLayout + 两个 Text，
 * 字号与行高各写各的（32/36/40/44 混用）。统一后只有 44 与 36 两档。
 */
Rectangle {
    id: row

    property string title: ""
    property string subtitle: ""
    property color titleColor: "#e6e6e6"
    property color subtitleColor: "#9aa0a8"
    property bool hoverable: true
    property bool tight: false
    property int cornerRadius: Tokens.radiusMd
    default property alias trailing: slot.data

    signal clicked()

    width: parent ? parent.width : 0
    height: row.tight ? Tokens.rowHeightTight : Tokens.rowHeight
    radius: row.cornerRadius
    color: row.hoverable && mouse.containsMouse ? Qt.rgba(1, 1, 1, 0.05) : "transparent"

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: row.clicked()
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: Tokens.space12
        anchors.right: slot.left
        anchors.rightMargin: Tokens.space8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 0

        Text {
            text: row.title
            color: row.titleColor
            font.pixelSize: Tokens.fontBody
            elide: Text.ElideRight
            width: parent.width
        }

        Text {
            visible: row.subtitle.length > 0
            text: row.subtitle
            color: row.subtitleColor
            font.pixelSize: Tokens.fontCaption
            elide: Text.ElideRight
            width: parent.width
        }
    }

    Row {
        id: slot
        anchors.right: parent.right
        anchors.rightMargin: Tokens.space12
        anchors.verticalCenter: parent.verticalCenter
    }
}
