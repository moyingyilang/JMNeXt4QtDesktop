import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "ui/Tokens.js" as Tokens

/*
 * 通知 —— 对齐 Kotlin 桌面版的列表页形态（PageHeader + 内容区）。
 *
 * 现状：接口 GET notifications 需要登录；未登录时返回「解密失败」。
 * 因此这里不假装有列表：结论区说明原因，原始响应折叠展示，便于核对字段。
 * 解析成通知列表是后续工作（主项目有完整的通知模型）。
 */
Rectangle {
    id: notifyPane

    property color surfaceColor: "#fef7ff"
    property color strokeColor: "#cac4d0"
    property color titleColor: "#1d1b20"
    property color bodyColor: "#49454f"
    property color accentColor: "#6750a4"

    property bool pending: false
    property bool fetchOk: false
    property string rawText: ""
    property bool rawOpen: false

    signal closeRequested()

    color: Qt.rgba(surfaceColor.r, surfaceColor.g, surfaceColor.b, 1.0)
    radius: 12

    function reload() {
        notifyPane.pending = true
        notifyPane.rawText = ""
        backend.fetch("notify", "notifications", "")
    }

    onVisibleChanged: if (visible) reload()

    Connections {
        target: backend
        function onTextReady(tag, ok, text) {
            if (tag !== "notify") return
            notifyPane.pending = false
            notifyPane.fetchOk = ok
            notifyPane.rawText = text
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        PageHeader {
            title: "通知"
            countText: notifyPane.pending ? "请求中…" : (notifyPane.fetchOk ? "已取到响应" : "未取到")
            description: "接口 GET notifications 需要登录；原始响应可在下方展开核对。"
            titleColor: notifyPane.titleColor
            bodyColor: notifyPane.bodyColor
            accentColor: notifyPane.accentColor
            actions: [
                { key: "refresh", label: "刷新", primary: true },
                { key: "close", label: "关闭", primary: false }
            ]
            onActionClicked: (key) => {
                if (key === "refresh") notifyPane.reload()
                else if (key === "close") notifyPane.closeRequested()
            }
        }

        UiEmptyState {
            visible: !notifyPane.pending && !notifyPane.fetchOk
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "没取到通知"
            description: "这个接口需要登录；也可能是服务端解密失败。原文可在下方展开核对。"
            actionText: "重试"
            titleColor: notifyPane.titleColor
            bodyColor: notifyPane.bodyColor
            accentColor: notifyPane.accentColor
            onAction: notifyPane.reload()
        }

        Text {
            visible: notifyPane.pending || (notifyPane.fetchOk && notifyPane.rawText.length === 0)
            Layout.fillWidth: true
            text: notifyPane.pending ? "正在请求通知…" : "没有通知。"
            color: notifyPane.bodyColor
            font.pixelSize: 12
        }

        UiRow {
            Layout.fillWidth: true
            visible: notifyPane.rawText.length > 0
            title: "原始响应"
            subtitle: notifyPane.rawOpen ? "点击折叠" : "点击展开（临时展示，后续解析成列表）"
            titleColor: notifyPane.titleColor
            subtitleColor: notifyPane.bodyColor
            onClicked: notifyPane.rawOpen = !notifyPane.rawOpen
        }

        Rectangle {
            visible: notifyPane.rawOpen
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Qt.rgba(0, 0, 0, 0.04)
            border.color: notifyPane.strokeColor
            border.width: 1
            radius: 8
            clip: true

            ScrollView {
                anchors.fill: parent
                anchors.margins: 8
                clip: true

                TextArea {
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextArea.Wrap
                    textFormat: TextEdit.PlainText
                    color: notifyPane.bodyColor
                    font.pixelSize: 12
                    text: notifyPane.rawText
                    background: Rectangle { color: "transparent" }
                }
            }
        }
    }
}
