import QtQuick
import "ui/Tokens.js" as Tokens

/* 标签胶囊：分类标签、搜索建议等处复用。选中态只用强调色描边 + 文字，不加快捷标记。 */
Rectangle {
    id: chip

    property string text: ""
    property color textColor: "#e6e6e6"
    property color accentColor: "#5b8def"
    property color borderColor: "#3a3d43"
    property bool selected: false
    property bool interactive: true

    signal clicked()

    implicitWidth: label.implicitWidth + Tokens.space16 * 2
    implicitHeight: 28
    radius: 14
    color: selected ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.18) : "transparent"
    border.width: 1
    border.color: selected ? accentColor : borderColor

    Text {
        id: label
        anchors.centerIn: parent
        text: chip.text
        color: chip.selected ? chip.accentColor : chip.textColor
        font.pixelSize: Tokens.fontCaption
    }

    MouseArea {
        anchors.fill: parent
        enabled: chip.interactive
        onClicked: chip.clicked()
    }
}
