import QtQuick

/*
 * Glass —— 对应 Kotlin 版 ui/components/Glass.kt。
 *
 * 两条实现路径，按运行时能力二选一：
 *   - Qt 6.5+（有 QtQuick.Effects）：加载 GlassBlur.qml，抓取 backdrop 做真背景模糊；
 *   - Qt 6.4（容器里的版本）：保持「半透明底色 + 1px 描边 + 圆角」的近似实现。
 *
 * 选择由 C++ 侧暴露的 hasMultiEffect 决定（main_qml.cpp 按 QT_VERSION 编译期判定），
 * 因此 6.4 的构建不会去加载不存在的 QtQuick.Effects 模块。
 *
 * 公共属性（tint / tintAlpha / radiusMd / strokeColor）保持与原实现一致，
 * 各页面无需改动；想启用模糊时把 backdrop 指到壁纸层即可。
 */
Item {
    id: glass

    property color tint: "#26282c"
    property color strokeColor: "#3a3d43"
    property int radiusMd: 8
    property real tintAlpha: 0.72

    /** 要模糊的背景（壁纸层）。为 null 时用全局 AppBackdrop（Main.qml 启动时登记）。 */
    property Item backdrop: null

    /** 实际使用的背景：显式指定的优先，否则用全局登记的那一层。 */
    readonly property Item effectiveBackdrop: backdrop !== null ? backdrop : AppBackdrop.item

    /** 最大模糊半径（像素），对应主项目玻璃质量的 12dp / 8dp。 */
    property real blurRadiusPx: 24

    Loader {
        anchors.fill: parent
        // hasMultiEffect 来自 C++ 上下文属性；没有 backdrop 时不值得抓图，直接回退
        source: (typeof hasMultiEffect !== "undefined" && hasMultiEffect && glass.effectiveBackdrop)
                ? "GlassBlur.qml"
                : ""
        onLoaded: {
            item.tint = Qt.binding(function () { return glass.tint; });
            item.strokeColor = Qt.binding(function () { return glass.strokeColor; });
            item.radiusMd = Qt.binding(function () { return glass.radiusMd; });
            item.tintAlpha = Qt.binding(function () { return glass.tintAlpha; });
            item.backdrop = Qt.binding(function () { return glass.effectiveBackdrop; });
            item.blurRadiusPx = Qt.binding(function () { return glass.blurRadiusPx; });
        }
    }

    // 回退实现：无 backdrop 或不支持 MultiEffect 时可见
    Rectangle {
        anchors.fill: parent
        visible: !(typeof hasMultiEffect !== "undefined" && hasMultiEffect && glass.effectiveBackdrop)
        color: Qt.rgba(glass.tint.r, glass.tint.g, glass.tint.b, glass.tintAlpha)
        border.color: glass.strokeColor
        border.width: 1
        radius: glass.radiusMd
    }
}
