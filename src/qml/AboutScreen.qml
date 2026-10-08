import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 关于（对应 Kotlin 版 ui/screens/about/AboutScreen.kt）。
 *
 * 本屏是"新建屏"（不是从 Main.qml 抽取）的第一例，也是**零数据层成本**的一类：
 * 它只展示静态信息，不需要 core / JmWorker / JmBackend 任何改动。
 *
 * 主题色由调用方注入（与其它屏一致）。
 */
Rectangle {
    id: about

    property color surfaceColor: "#26282c"
    property color strokeColor: "#3a3d43"
    property color titleColor: "#e6e6e6"
    property color bodyColor: "#b9bcc2"
    property color accentColor: "#5b8def"

    signal closeRequested()

    color: about.surfaceColor
    border.color: about.strokeColor
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        Text {
            text: "关于 JMNeXt（Qt 版）"
            color: about.titleColor; font.pixelSize: 18; font.bold: true
            Layout.fillWidth: true
        }

        Text {
            text: "桌面端原生重实现（C++ / Qt Quick），界面与功能对齐 Kotlin/Compose 主项目。"
            color: about.bodyColor; font.pixelSize: 13; wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Text {
            text: "许可证：AGPL-3.0（与主项目一致）"
            color: about.bodyColor; font.pixelSize: 13
        }
        Text {
            text: "协议与界面结构来自同一作者的主项目 JMNeXt；未使用第三方参照实现的代码。"
            color: about.bodyColor; font.pixelSize: 12; wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Text {
            text: "当前处于移植过程中：首页/搜索/详情/阅读器已可用，其余屏尚未实现（详见 STATUS.md）。"
            color: about.accentColor; font.pixelSize: 12; wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }

        Button {
            text: "关闭"
            onClicked: about.closeRequested()
        }
    }
}
