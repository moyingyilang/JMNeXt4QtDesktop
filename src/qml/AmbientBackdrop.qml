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

    Rectangle { anchors.fill: parent; color: backdrop.baseColor }

    // 默认光斑（无壁纸时提供一点层次）
    Rectangle {
        width: parent.width * 0.8; height: parent.height * 0.5
        x: -width * 0.2; y: -height * 0.2
        radius: height / 2
        visible: backdrop.wallpaper.toString() === ""
        color: Qt.rgba(backdrop.glowColor.r, backdrop.glowColor.g, backdrop.glowColor.b, backdrop.glowAlpha * 0.35)
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
