import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "ui/Tokens.js" as Tokens

/*
 * 更多（对应 Kotlin 版 more/MoreListScreen.kt）—— 纯入口列表，无数据层成本。
 *
 * 结构：Rectangle 根 + PageHeader（关闭）+ UiRow 列表。
 * 未实现项在行尾如实标出「未实现」，不假装能用。
 */
Rectangle {
    id: more

    property color surfaceColor: "#fef7ff"
    property color strokeColor: "#cac4d0"
    property color titleColor: "#1d1b20"
    property color bodyColor: "#49454f"
    property color accentColor: "#6750a4"

    /** 通知未读数（-1 表示未知） */
    property int unreadCount: -1

    signal closeRequested()
    signal entryClicked(string key)

    color: Qt.rgba(surfaceColor.r, surfaceColor.g, surfaceColor.b, 1.0)
    radius: 12

    function refreshUnread() {
        backend.fetch("unread", "notifications/unreadCount", "")
    }

    onVisibleChanged: if (visible) refreshUnread()

    Connections {
        target: backend
        function onTextReady(tag, ok, text) {
            if (tag !== "unread") return
            if (!ok) { more.unreadCount = -1; return }
            // 响应形如 {"site_notice":1,"comic_follow":1}：把里面的数字加起来
            var total = 0
            var re = /:\s*(\d+)/g
            var m
            while ((m = re.exec(text)) !== null) total += parseInt(m[1], 10)
            more.unreadCount = total
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        PageHeader {
            title: "更多"
            countText: more.unreadCount >= 0 ? ("未读通知 " + more.unreadCount) : ""
            description: "标「未实现」的项是 Kotlin 版有、Qt 版还没有的；点击只记日志，不假装能用。"
            titleColor: more.titleColor
            bodyColor: more.bodyColor
            accentColor: more.accentColor
            actions: [ { key: "close", label: "关闭", primary: false } ]
            onActionClicked: (key) => { if (key === "close") more.closeRequested() }
        }

        ListView {
            id: entryView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2
            model: ListModel {
                ListElement { key: "category"; name: "分类（热门标签）"; ready: true }
                ListElement { key: "about";    name: "关于";             ready: true }
                ListElement { key: "settings"; name: "设置";             ready: true }
                ListElement { key: "random";   name: "随机推荐";         ready: true }
                ListElement { key: "creator";  name: "画师列表";         ready: true }
                ListElement { key: "week";     name: "每周更新";         ready: true }
                ListElement { key: "tags";     name: "标签收藏";         ready: false }
                ListElement { key: "favorites";name: "我的收藏";         ready: true }
                ListElement { key: "history";  name: "阅读历史";         ready: true }
                ListElement { key: "profile";  name: "我的";             ready: true }
                ListElement { key: "auth";     name: "登录 / 注册";      ready: true }
                ListElement { key: "checkin";  name: "每日签到";         ready: true }
                ListElement { key: "notify";   name: "通知";             ready: true }
            }

            delegate: UiRow {
                width: entryView.width
                title: name
                titleColor: ready ? more.titleColor : more.bodyColor
                onClicked: {
                    console.log("QML 更多：点击 " + key + "（ready=" + ready + "）")
                    more.entryClicked(key)
                }

                Text {
                    visible: !ready
                    text: "未实现"
                    color: more.accentColor
                    font.pixelSize: 11
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
}
