import QtQuick

/* AmbientBackdrop —— 对应 Kotlin 版 ui/components/AmbientBackdrop.kt。
   Kotlin 侧是「主题色渐变 + 壁纸」两层；这里同样两层：
     1) 底色与两枚光斑（无壁纸时的默认观感）；
     2) 壁纸图片（fillMode: PreserveAspectCrop）+ 压暗层。
   壁纸存在时它才显示，压暗保证上层文字可读；玻璃（GlassBlur）就是模糊这一层。 */
Item {
    id: backdrop
    property color baseColor: "#1e1f22"
    property color glowColor: "#5b8def"
    property real glowAlpha: 0.20

    /** 壁纸地址（file:// 或 qrc:），空则只用渐变。 */
    property url wallpaper: ""
    /** 壁纸压暗程度 0..0.9，越大文字越清楚。 */
    property real wallpaperDim: 0.55

    /** 壁纸预设：极光/沙丘/薄荷/水墨（外观页的四张色板）；空串表示不用 */
    property string wallpaperPreset: ""

    readonly property var presetColors: ({
        "aurora": ["#c7d4f5", "#e8d7f0"],
        "dune":   ["#f2e2c4", "#e8cf9e"],
        "mint":   ["#d7f0e6", "#bfe3d2"],
        "ink":    ["#eef1f3", "#d6dade"]
    })

    Rectangle { anchors.fill: parent; color: backdrop.baseColor }

    // 默认光斑（无壁纸时提供一点层次）
    Rectangle {
        width: parent.width * 0.8; height: parent.height * 0.5
        x: -width * 0.2; y: -height * 0.2
        radius: height / 2
        visible: backdrop.wallpaper.toString() === ""
        color: Qt.rgba(backdrop.glowColor.r, backdrop.glowColor.g, backdrop.glowColor.b, backdrop.glowAlpha * 0.35)
    }

    // 预设渐变壁纸（外观页选色板后生效）：纯 QML 渐变，无需图片资源
    Rectangle {
        anchors.fill: parent
        visible: backdrop.wallpaperPreset !== "" && backdrop.wallpaper.toString() === ""
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: (backdrop.presetColors[backdrop.wallpaperPreset] || ["#eef1f3", "#d6dade"])[0]
            }
            GradientStop {
                position: 1.0
                color: (backdrop.presetColors[backdrop.wallpaperPreset] || ["#eef1f3", "#d6dade"])[1]
            }
        }
    }

    // 壁纸层
    Image {
        id: wallpaperImage
        anchors.fill: parent
        visible: backdrop.wallpaper.toString() !== ""
        source: backdrop.wallpaper
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: true
        smooth: true
    }

    // 压暗层：壁纸越亮，文字越难读，所以默认压 55%
    Rectangle {
        anchors.fill: parent
        visible: backdrop.wallpaper.toString() !== ""
        color: Qt.rgba(0, 0, 0, backdrop.wallpaperDim)
    }
}
