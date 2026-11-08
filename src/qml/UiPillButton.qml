import QtQuick
import "ui/Tokens.js" as Tokens

/*
 * 胶囊按钮 —— 对齐 Kotlin 桌面版的 Material3 Button：
 * 主按钮填充蓝色、次按钮浅底、禁用为灰底；圆角取高度一半（胶囊形）。
 *
 * 此前 Qt 版用的是 Control 默认 Button（白底方角、8px 圆角），
 * 与 Kotlin 桌面的胶囊 + 填充风格完全不同，这是最显眼的差异之一。
 */
Rectangle {
    id: pill

    property string text: ""
    property bool primary: true
    property bool buttonEnabled: true
    property color accentColor: "#2f6fe4"
    property color textColor: "#ffffff"
    property color tonalColor: "#e2e1f6"
    property color disabledColor: "#e6e5ee"

    signal clicked()

    implicitWidth: label.implicitWidth + Tokens.space24 * 2
    implicitHeight: 40
    radius: height / 2
    color: !pill.buttonEnabled ? pill.disabledColor
           : (pill.primary ? pill.accentColor : pill.tonalColor)

    Text {
        id: label
        anchors.centerIn: parent
        text: pill.text
        color: pill.primary ? pill.textColor : "#2b2b33"
        font.pixelSize: Tokens.fontBody
        opacity: pill.buttonEnabled ? 1.0 : 0.5
    }

    MouseArea {
        anchors.fill: parent
        enabled: pill.buttonEnabled
        cursorShape: pill.buttonEnabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: pill.clicked()
    }
}
