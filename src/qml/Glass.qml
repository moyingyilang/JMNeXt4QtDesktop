import QtQuick

/*
 * Glass —— 对应 Kotlin 版 ui/components/Glass.kt。
 * 平台限制：Qt 6.4 无 MultiEffect（无法做真实背景模糊），因此用**半透明底色 + 细描边 + 圆角**
 * 近似；这与 Kotlin 侧"压低透明度 + 描边"的视觉意图一致，但不等于真模糊。已如实记录。
 */
Rectangle {
    id: glass
    property color tint: "#26282c"
    property color strokeColor: "#3a3d43"
    property int radiusMd: 8
    property real tintAlpha: 0.72
    color: Qt.rgba(tint.r, tint.g, tint.b, tintAlpha)
    border.color: strokeColor
    border.width: 1
    radius: radiusMd
}
