import QtQuick
import "ui/Tokens.js" as Tokens

/* 分隔线：1px，颜色跟随描边色。 */
Rectangle {
    property color lineColor: "#3a3d43"
    height: 1
    color: lineColor
    opacity: 0.6
    implicitHeight: 1
    topPadding: 0
    bottomPadding: 0
}
