import QtQuick
import QtQuick.Effects

/*
 * GlassBlur —— 真·背景模糊的玻璃（Qt 6.5+，依赖 QtQuick.Effects 的 MultiEffect）。
 *
 * 由 Glass.qml 在 hasMultiEffect 为真时通过 Loader 加载；Qt 6.4 上这个文件不参与构建。
 *
 * 为什么需要 backdrop：Qt 的 MultiEffect 只能模糊**它自己的 source**，不能模糊"背后的任意内容"。
 * 因此这里用 ShaderEffectSource 主动抓取 backdrop（本实现里是壁纸层）与自身矩形重叠的那块，
 * 再交给 MultiEffect 做高斯模糊 + 饱和度补偿，最后叠半透明底色与 1px 描边。
 *
 * 与主项目 Kotlin 版的对应关系（ui/components/Glass.kt）：
 *   - 模糊半径：blurMax 对应 GlassQuality 的 12dp / 8dp；
 *   - 表面色：tint + tintAlpha（实心材质不会走到这里，它们 alpha 为 1）；
 *   - 饱和度补偿 1.5：主项目用 HazeColorEffect 的 saturation，这里用 MultiEffect.saturation。
 */
Item {
    id: root

    /** 要模糊的对象（通常是壁纸层）。为 null 时退化为纯色，不报错。 */
    property Item backdrop: null

    property color tint: "#26282c"
    property color strokeColor: "#3a3d43"
    property int radiusMd: 8
    property real tintAlpha: 0.72

    /** 最大模糊半径（像素）。对应主项目「玻璃质量」的 12dp / 8dp。 */
    property real blurRadiusPx: 24
    /** 饱和度补偿，主项目取 1.5。 */
    property real saturation: 1.5

    clip: true

    Component.onCompleted: console.log("GlassBlur 已加载：真背景模糊生效（blurMax=" + root.blurRadiusPx + "px）")

    // 抓取 backdrop 与本控件重叠的区域。映射到 backdrop 坐标系后再取矩形，
    // 这样滚动时模糊内容跟着动，而不是抓一张静止快照。
    ShaderEffectSource {
        id: backdropSource
        anchors.fill: parent
        visible: false
        live: true
        recursive: true
        sourceItem: root.backdrop
        sourceRect: {
            if (!root.backdrop)
                return Qt.rect(0, 0, 0, 0);
            const p = root.mapToItem(root.backdrop, 0, 0);
            return Qt.rect(p.x, p.y, root.width, root.height);
        }
    }

    MultiEffect {
        anchors.fill: parent
        source: backdropSource
        visible: root.backdrop !== null
        blurEnabled: true
        blur: 1.0
        blurMax: root.blurRadiusPx
        saturation: root.saturation
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(root.tint.r, root.tint.g, root.tint.b, root.tintAlpha)
        radius: root.radiusMd
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: root.radiusMd
        border.color: root.strokeColor
        border.width: 1
    }
}
