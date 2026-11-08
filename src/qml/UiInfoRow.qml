import QtQuick
import "ui/Tokens.js" as Tokens

/*
 * 信息行：左侧标签、右侧取值（对应主项目 DetailScreen 里的「作者 / 标签 / 章节」）。
 *
 * 与 UiRow 的区别：UiRow 是可点击的列表项（有 hover、44 高），
 * 这里只展示、不响应点击，高度 28，右侧值可以换行（标签常常很长）。
 */
Item {
    id: info

    property string label: ""
    property string value: ""
    property color labelColor: "#9aa0a8"
    property color valueColor: "#e6e6e6"
    property int labelWidth: 56

    implicitWidth: parent ? parent.width : 0
    implicitHeight: Math.max(labelText.implicitHeight, valueText.implicitHeight) + Tokens.space8

    Text {
        id: labelText
        anchors.left: parent.left
        anchors.top: parent.top
        width: info.labelWidth
        text: info.label
        color: info.labelColor
        font.pixelSize: Tokens.fontCaption
        elide: Text.ElideRight
    }

    Text {
        id: valueText
        anchors.left: labelText.right
        anchors.leftMargin: Tokens.space8
        anchors.right: parent.right
        anchors.top: parent.top
        text: info.value
        color: info.valueColor
        font.pixelSize: Tokens.fontBody
        wrapMode: Text.WordWrap
    }
}
