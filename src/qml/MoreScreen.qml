import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 更多（对应 Kotlin 版 ui/screens/more/MoreListScreen.kt）。
 *
 * 这一屏是**纯静态入口列表**：零数据层成本，不需要 core/JmWorker/JmBackend 任何改动。
 * 每一项点的动作以信号交给外层（哪些已实现、哪些还没做，在界面上如实标注）。
 *
 * 诚实标注：标记为"未实现"的项在 Kotlin 版有、Qt 版没有 —— 这里不隐藏这一点，
 * 点它们只会打日志，不会假装能用。
 */
Rectangle {
    id: more

    property color surfaceColor: "#26282c"
    property color strokeColor: "#3a3d43"
    property color titleColor: "#e6e6e6"
    property color bodyColor: "#b9bcc2"
    property color accentColor: "#5b8def"

    signal closeRequested()
    signal entryClicked(string key)
    /// 已实现项：由外层决定跳到哪一屏（见 Main.qml 的 onEntryClicked）

    color: more.surfaceColor
    border.color: more.strokeColor
    border.width: 1

    // key / 标题 / 是否已在 Qt 侧实现
    ListModel {
        id: entryModel
        ListElement { key: "category"; name: "分类（热门标签）"; ready: true }
        ListElement { key: "about";    name: "关于";             ready: true }
        ListElement { key: "settings"; name: "设置";             ready: true }
        ListElement { key: "random";   name: "随机推荐";         ready: true }
        ListElement { key: "creator";  name: "画师列表";         ready: true }
        ListElement { key: "week";     name: "每周更新（需刊期参数）"; ready: false }
        ListElement { key: "tags";     name: "标签收藏";         ready: false }
        ListElement { key: "favorites";name: "我的收藏";         ready: true }
        ListElement { key: "history";  name: "阅读历史";         ready: true }
        ListElement { key: "profile";  name: "我的";             ready: false }
        ListElement { key: "auth";     name: "登录 / 注册";      ready: true }
        ListElement { key: "notify";   name: "通知";             ready: true }
        ListElement { key: "comments"; name: "评论";             ready: false }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 6

        Text {
            text: "更多"
            color: more.titleColor; font.pixelSize: 18; font.bold: true
            Layout.fillWidth: true
        }
        Text {
            text: "标注「未实现」的项是 Kotlin 版有、Qt 版还没有的（详见 STATUS.md 的完成度审计）"
            color: more.bodyColor; font.pixelSize: 12; wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        ListView {
            id: entryView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: entryModel
            spacing: 2
            delegate: Rectangle {
                width: ListView.view.width; height: 34
                color: entryMouse.containsMouse ? "#2f3237" : "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8; anchors.rightMargin: 8
                    Text {
                        text: name
                        color: ready ? more.titleColor : more.bodyColor
                        font.pixelSize: 14
                        Layout.fillWidth: true
                    }
                    Text {
                        text: ready ? "" : "未实现"
                        color: more.accentColor; font.pixelSize: 11
                    }
                }
                MouseArea {
                    id: entryMouse
                    anchors.fill: parent; hoverEnabled: true
                    onClicked: {
                        console.log("QML 更多：点击 " + key + "（ready=" + ready + "）")
                        more.entryClicked(key)
                    }
                }
            }
        }

        Button {
            text: "关闭"
            onClicked: more.closeRequested()
        }
    }
}
