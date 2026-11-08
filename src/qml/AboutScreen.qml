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

    color: Qt.rgba(about.surfaceColor.r, about.surfaceColor.g, about.surfaceColor.b, 1.0)
    radius: 12

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        PageHeader {
            title: "关于"
            countText: "JMNeXt4QtDesktop"
            description: "桌面端原生重实现（C++ / Qt Quick），界面与功能对齐 Kotlin/Compose 主项目。"
            titleColor: about.titleColor
            bodyColor: about.bodyColor
            accentColor: about.accentColor
            actions: [ { key: "close", label: "关闭", primary: false } ]
            onActionClicked: (key) => { if (key === "close") about.closeRequested() }
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
            text: "已对齐：首页/搜索/分类/周更/收藏/历史/更多/外观/我的/通知/登录/关于；详情与阅读器可用；标签收藏、画师库尚未实现。"
            color: about.accentColor; font.pixelSize: 12; wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }

    }
}
