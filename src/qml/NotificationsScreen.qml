import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 通知（对应 Kotlin 版的消息中心）。
 *
 * 数据来源：backend.fetch("notify", "notifications", "")。
 * 路径与参数取自 Main.qml 里 notify 项（notifications，无额外 query），与 Kotlin 版一致；
 * 该接口需要登录，未登录时服务端会返回失败。
 *
 * fetch 的结果不是同步返回，而是通过 backend.textReady(tag, ok, text) 异步回传，
 * 所以这里用 Connections 按 tag === "notify" 收取；textReady 是通用回传信号，
 * 「更多」页的未读数也走它，必须按 tag 过滤，否则会把别页的响应误当成通知。
 *
 * 现状（临时展示，后续再解析）：这一版只把原始 JSON 文本原样显示，
 * 先确认「能不能取到」，不解析字段结构（未读数、通知条目等留给主线后续做模型）。
 * 解析完成之前不假装已经有列表 —— 注释与界面都如实说明这一点。
 */
Rectangle {
    id: notifyPane

    property color surfaceColor: "#26282c"
    property color strokeColor: "#3a3d43"
    property color titleColor: "#e6e6e6"
    property color bodyColor: "#b9bcc2"
    property color accentColor: "#5b8def"

    signal closeRequested()

    // 请求是否在途（区分「正在请求」与「请求失败」）
    property bool pending: false
    // 最近一次 textReady 的 ok
    property bool fetchOk: false
    /** 原始响应是否展开（默认折叠：错误态下先把结论说清楚，原文按需看） */
    property bool rawOpen: false
    // 服务端回传的原始文本；失败时也留着原样显示，便于排查
    property string rawText: ""

    color: Qt.rgba(surfaceColor.r, surfaceColor.g, surfaceColor.b, 1.0)
    border.color: strokeColor
    border.width: 1

    function refresh() {
        pending = true
        fetchOk = false
        rawText = ""
        backend.fetch("notify", "notifications", "")
    }

    onVisibleChanged: if (visible) refresh()

    Connections {
        target: backend
        function onTextReady(tag, ok, text) {
            if (tag !== "notify") return
            notifyPane.pending = false
            notifyPane.fetchOk = ok
            // ok 为 false 时也把 text 存下来：错误详情往往就写在响应体里
            notifyPane.rawText = text ? text : ""
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Text {
                text: "通知"
                color: notifyPane.titleColor
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
            }
            Button {
                text: "刷新"
                onClicked: notifyPane.refresh()
            }
            Button {
                text: "关闭"
                onClicked: notifyPane.closeRequested()
            }
        }

        // 结论区：失败用统一的 UiEmptyState（标题 + 说明 + 重试），成功时只留一行说明
        UiEmptyState {
            visible: !notifyPane.pending && !notifyPane.fetchOk
            Layout.fillWidth: true
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
            text: notifyPane.pending ? "正在请求通知…" : "没有通知。"
            color: notifyPane.bodyColor
            font.pixelSize: 12
            Layout.fillWidth: true
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
            // 可滚动文本区：只读 TextArea 放进 ScrollView，长 JSON 也能翻
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Qt.rgba(0, 0, 0, 0.18)
            border.color: notifyPane.strokeColor
            border.width: 1
            radius: 6
            clip: true

            ScrollView {
                anchors.fill: parent
                anchors.margins: 8
                clip: true

                TextArea {
                    id: rawArea
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextArea.Wrap
                    // 按纯文本显示，不把 JSON 里的 < > 之类当成富文本/HTML
                    textFormat: TextEdit.PlainText
                    color: notifyPane.bodyColor
                    font.pixelSize: 12
                    text: notifyPane.rawText
                    // 去掉控件默认的白色背景，跟随本屏面板配色
                    background: Rectangle { color: "transparent" }
                }
            }
        }

        Text {
            text: "接口：GET notifications（需登录）。原始文本便于核对字段，解析成列表是后续工作。"
            color: notifyPane.bodyColor
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
