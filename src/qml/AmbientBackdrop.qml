import QtQuick

/* AmbientBackdrop —— 对应 Kotlin 版 ui/components/AmbientBackdrop.kt。
   Kotlin 侧用主题色叠一层径向渐变；这里用两个 RadialGradient 光斑模拟同一观感。 */
Item {
    id: backdrop
    property color baseColor: "#1e1f22"
    property color glowColor: "#5b8def"
    property real glowAlpha: 0.20
    Rectangle { anchors.fill: parent; color: backdrop.baseColor }
    Rectangle {
        width: parent.width * 0.9; height: width; radius: width / 2
        x: -width * 0.25; y: -height * 0.35
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(backdrop.glowColor.r, backdrop.glowColor.g, backdrop.glowColor.b, backdrop.glowAlpha) }
            GradientStop { position: 1.0; color: "transparent" }
        }
        visible: false
    }
    // 用一个柔和的椭圆代替（Gradient 在小部件上更稳）
    Rectangle {
        width: parent.width * 0.8; height: parent.height * 0.5
        x: -width * 0.2; y: -height * 0.2
        radius: height / 2
        color: Qt.rgba(backdrop.glowColor.r, backdrop.glowColor.g, backdrop.glowColor.b, backdrop.glowAlpha * 0.35)
    }
}
