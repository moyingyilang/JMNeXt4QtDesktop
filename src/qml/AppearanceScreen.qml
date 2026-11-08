import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "ui/Tokens.js" as Tokens

/*
 * 外观页 —— 对齐 Kotlin 桌面版 ui/screens/appearance：
 *   界面风格（五套胶囊）→ 当前说明 → 壁纸模糊 / 面板浓度 / 壁纸压暗 三个滑杆
 *   → 壁纸色板 → 在线壁纸入口。
 *
 * 与旧设置页的区别：旧页只有两个可展开选项行（风格、深浅色），
 * Kotlin 版是「胶囊直选 + 滑杆微调」，滑杆直接作用到玻璃与壁纸。
 */
Rectangle {
    id: appearance

    property color surfaceColor: "#fef7ff"
    property color strokeColor: "#cac4d0"
    property color titleColor: "#1d1b20"
    property color bodyColor: "#49454f"
    property color accentColor: "#6750a4"

    property string styleName: "windowGlass"
    property bool dark: false
    property real blurDp: 5
    property real panelAlpha: 0.12
    property real dim: 0.0

    signal closeRequested()
    signal styleSelected(string key)
    signal blurEdited(real dp)
    signal panelAlphaEdited(real value)
    signal dimEdited(real value)
    signal swatchSelected(string key)

    color: Qt.rgba(surfaceColor.r, surfaceColor.g, surfaceColor.b, 1.0)

    function styleLabel(key) {
        return ({ "windowGlass": "毛玻璃", "translucent": "半透明",
                  "flatBlur": "扁平模糊", "miuix": "Miuix", "material": "Material" })[key] || key
    }

    Flickable {
        anchors.fill: parent
        anchors.margins: 32
        clip: true
        contentHeight: content.height

        Column {
            id: content
            width: parent.width
            spacing: Tokens.space16

            Text {
                text: "外观"
                color: appearance.titleColor
                font.pixelSize: 28
                bottomPadding: Tokens.space8
            }

            UiSectionTitle { text: "界面风格"; titleColor: appearance.titleColor }

            Text {
                text: "换的不只是透明度 —— 圆角与壁纸是否模糊一起换。手机上的实时背景模糊在桌面上拿不到，这里用等效近似。"
                color: appearance.bodyColor
                font.pixelSize: 12
                width: parent.width
                wrapMode: Text.WordWrap
            }

            Row {
                spacing: Tokens.space8

                Repeater {
                    model: [
                        { key: "windowGlass", label: "毛玻璃" },
                        { key: "translucent", label: "半透明" },
                        { key: "flatBlur", label: "扁平模糊" },
                        { key: "miuix", label: "Miuix" },
                        { key: "material", label: "Material" }
                    ]

                    delegate: UiPillButton {
                        text: modelData.label
                        primary: appearance.styleName === modelData.key
                        accentColor: appearance.accentColor
                        onClicked: appearance.styleSelected(modelData.key)
                    }
                }
            }

            Text {
                text: "当前：" + appearance.styleLabel(appearance.styleName)
                      + "（面板透明度 " + Math.round(appearance.panelAlpha * 100) + "%、"
                      + "圆角 " + Tokens.radiusXl + "dp、壁纸模糊 " + appearance.blurDp + "dp）"
                color: appearance.bodyColor
                font.pixelSize: 12
                width: parent.width
            }

            // 三个滑杆：值直接回传，由 Main 作用到玻璃层与壁纸
            Text { text: "壁纸模糊：" + appearance.blurDp + "dp"; color: appearance.titleColor; font.pixelSize: 16 }
            Slider {
                width: parent.width
                from: 0; to: 40; stepSize: 1
                value: appearance.blurDp
                onMoved: appearance.blurEdited(value)
            }

            Text { text: "面板浓度：" + Math.round(appearance.panelAlpha * 100) + "%（在所选风格基础上再调）"; color: appearance.titleColor; font.pixelSize: 16 }
            Slider {
                width: parent.width
                from: 0; to: 1; stepSize: 0.01
                value: appearance.panelAlpha
                onMoved: appearance.panelAlphaEdited(value)
            }

            Text { text: "壁纸压暗：" + Math.round(appearance.dim * 100) + "%（0..100，到 100 壁纸等于纯黑）"; color: appearance.titleColor; font.pixelSize: 16 }
            Slider {
                width: parent.width
                from: 0; to: 1; stepSize: 0.01
                value: appearance.dim
                onMoved: appearance.dimEdited(value)
            }

            UiSectionTitle { text: "壁纸"; titleColor: appearance.titleColor }

            Row {
                spacing: Tokens.space12

                Repeater {
                    model: [
                        { key: "aurora", label: "极光", c1: "#c7d4f5", c2: "#e8d7f0" },
                        { key: "dune", label: "沙丘", c1: "#f2e2c4", c2: "#e8cf9e" },
                        { key: "mint", label: "薄荷", c1: "#d7f0e6", c2: "#bfe3d2" },
                        { key: "ink", label: "水墨", c1: "#eef1f3", c2: "#d6dade" }
                    ]

                    delegate: Column {
                        spacing: Tokens.space4
                        Rectangle {
                            width: 96; height: 60; radius: Tokens.radiusMd
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: modelData.c1 }
                                GradientStop { position: 1.0; color: modelData.c2 }
                            }
                            MouseArea { anchors.fill: parent; onClicked: appearance.swatchSelected(modelData.key) }
                        }
                        Text { text: modelData.label; color: appearance.bodyColor; font.pixelSize: 12 }
                    }
                }
            }

            UiSectionTitle { text: "在线壁纸"; titleColor: appearance.titleColor }

            Flow {
                width: parent.width
                spacing: Tokens.space16

                Repeater {
                    model: ["纯渐变", "Bing 每日", "二次元随机", "全部随机", "自定义地址", "跟随手机壁纸"]
                    delegate: Text {
                        text: modelData
                        color: appearance.accentColor
                        font.pixelSize: 13
                    }
                }
            }

            Text {
                text: "在线壁纸暂未接入（需要额外接口与磁盘缓存）；当前只支持 --wallpaper 指定的本地图片与内置渐变。"
                color: appearance.bodyColor
                font.pixelSize: 11
                width: parent.width
                wrapMode: Text.WordWrap
            }
        }
    }
}
