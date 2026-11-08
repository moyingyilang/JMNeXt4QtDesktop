import QtQuick

/*
 * Glass —— 对应 Kotlin 版 ui/components/Glass.kt。
 *
 * 材质分治（与主项目 2.2.2 同一套判定）：
 *   - windowGlass / translucent / flatBlur → 半透明，可用真背景模糊（Qt 6.5+）；
 *   - **miuix → 不透明，禁止模糊**（主项目里它就是实心材质，透光会把层次糊掉）。
 *
 * 两条渲染路径，按运行时能力二选一：
 *   - Qt 6.5+（有 QtQuick.Effects）：加载 GlassBlur.qml，抓背景层做真背景模糊；
 *   - Qt 6.4（容器里的版本）：半透明底色 + 1px 描边。
 *
 * 背景怎么拿到：Main.qml 的根窗口上有 `appBackdrop` 属性（启动时指向 AmbientBackdrop），
 * 这里通过附加属性 `Window.window` 取到窗口再读它。
 * **不用 QML 单例**：同目录的 singleton 需要显式 import 模块才能解析，
 * 之前那一版在运行期表现为 `AppBackdrop` 未定义、`Component.onCompleted` 静默失败。
 */
Item {
    id: glass

    /** 当前材质名：windowGlass / translucent / flatBlur / miuix。 */
    readonly property string material:
        (typeof themeName !== "undefined" && themeName) ? String(themeName) : "windowGlass"

    /** 实心材质：不透明、不模糊、不采样背景。 */
    readonly property bool opaqueMaterial: material === "miuix"

    property color tint: "#26282c"
    property color strokeColor: "#3a3d43"
    property int radiusMd: 8

    /** 表面不透明度。默认 -1 表示按材质自动取（见 autoAlpha）。 */
    property real tintAlpha: -1

    /** 各材质的默认表面不透明度（与主项目接近）。 */
    readonly property real autoAlpha: {
        if (opaqueMaterial) return 1.0;
        switch (material) {
        case "windowGlass": return 0.40;   // 主项目：0.40 表面色 + 饱和度补偿
        case "translucent": return 0.62;
        case "flatBlur":    return 0.55;
        default:            return 0.72;
        }
    }

    readonly property real effectiveAlpha: tintAlpha >= 0 ? Math.min(tintAlpha, 1.0)
                                                          : autoAlpha

    /** 要模糊的背景。为 null 时用窗口上的 appBackdrop（Main.qml 启动时登记）。 */
    property Item backdrop: null
    readonly property Item effectiveBackdrop:
        backdrop !== null ? backdrop
                          : (Window.window ? Window.window.appBackdrop : null)

    /** 最大模糊半径（像素）：按材质档位取，实心材质为 0。 */
    property real blurRadiusPx: -1
    readonly property real effectiveBlurPx: {
        if (opaqueMaterial) return 0;
        if (blurRadiusPx >= 0) return blurRadiusPx;
        switch (material) {
        case "windowGlass": return 24;
        case "translucent": return 20;
        case "flatBlur":    return 14;
        default:            return 16;
        }
    }

    /** 只有「支持 MultiEffect」且「非实心材质」且「有背景」时才做真模糊。 */
    readonly property bool usesBlur:
        (typeof hasMultiEffect !== "undefined" && hasMultiEffect)
        && !(typeof glassForceOff !== "undefined" && glassForceOff)
        && !opaqueMaterial && effectiveBackdrop !== null

    Component.onCompleted: {
        const win = Window.window;
        if (win && typeof glassDebug !== "undefined" && glassDebug && win.glassDebugCount < 4) {
            win.glassDebugCount += 1;
            console.log("玻璃[" + glass.material + "] 不透明=" + glass.opaqueMaterial
                        + " 模糊=" + glass.usesBlur + " alpha=" + glass.effectiveAlpha
                        + " blurPx=" + glass.effectiveBlurPx);
        }
    }

    Loader {
        anchors.fill: parent
        source: glass.usesBlur ? "GlassBlur.qml" : ""
        onLoaded: {
            item.tint = Qt.binding(function () { return glass.tint; });
            item.strokeColor = Qt.binding(function () { return glass.strokeColor; });
            item.radiusMd = Qt.binding(function () { return glass.radiusMd; });
            item.tintAlpha = Qt.binding(function () { return glass.effectiveAlpha; });
            item.backdrop = Qt.binding(function () { return glass.effectiveBackdrop; });
            item.blurRadiusPx = Qt.binding(function () { return glass.effectiveBlurPx; });
        }
    }

    // 回退实现：不支持 MultiEffect、实心材质、或没有背景时使用
    Rectangle {
        anchors.fill: parent
        visible: !glass.usesBlur
        color: Qt.rgba(glass.tint.r, glass.tint.g, glass.tint.b, glass.effectiveAlpha)
        border.color: glass.strokeColor
        border.width: 1
        radius: glass.radiusMd
    }
}
