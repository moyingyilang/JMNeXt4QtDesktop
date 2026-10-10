import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 我的（对应 Kotlin 版 ui/screens/profile/ProfileScreen.kt）。
 *
 * 依据 shared/data/JmRepository.kt：
 *   settings() -> GET "setting"，参数 app_img_shunt / lang / t（实测返回 4 条，首条「图源1」）
 *   login()    -> 登录后用户信息（uid/username/coin/level/album_favorites…）
 *
 * 现状（如实）：本屏显示**登录态**与**应用配置**两类信息；用户资料明细（头像/等级/经验）
 * 需要把登录返回体解析成成员模型，尚未做 —— 这里不假装有。
 */
ColumnLayout {
    id: profile

    property color titleColor: "#e6e6e6"
    property color bodyColor: "#b9bcc2"
    property color surfaceColor: "#26282c"
    property color accentColor: "#5b8def"
    property string pageStatus: ""
    property bool loggedIn: false
    property string userName: ""

    signal closeRequested()
    signal loginRequested()

    ListModel { id: configModel }

    RowLayout {
        Layout.fillWidth: true
        Text {
            text: "我的"
            color: profile.titleColor; font.pixelSize: 18; font.bold: true
            Layout.fillWidth: true
        }
        Button {
            text: profile.loggedIn ? "已登录：" + profile.userName : "去登录"
            onClicked: profile.loginRequested()
        }
    }

    Text {
        text: profile.loggedIn
              ? "登录状态：已登录（凭证仅在本次运行内有效）"
              : "登录状态：未登录 —— 收藏 / 历史 / 通知 / 签到 / 追更 需要登录"
        color: profile.loggedIn ? profile.accentColor : profile.bodyColor
        font.pixelSize: 12; wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    Text {
        text: profile.pageStatus
        color: profile.bodyColor; font.pixelSize: 12
        Layout.fillWidth: true; elide: Text.ElideRight
    }

    Text {
        text: "应用配置"
        color: profile.bodyColor; font.pixelSize: 13
    }
    ListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: configModel
        spacing: 2
        delegate: Rectangle {
            width: ListView.view.width; height: 28
            color: "transparent"
            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left; anchors.leftMargin: 6
                text: name
                color: profile.titleColor; font.pixelSize: 13; elide: Text.ElideRight
                width: parent.width - 12
            }
        }
    }

    Button {
        text: "关闭"
        onClicked: profile.closeRequested()
    }

    function reload() {
        configModel.clear()
        profile.pageStatus = "正在载入配置…"
        backend.loadPaged("settings", "setting", "app_img_shunt=1&lang=zh&t=" + Math.floor(Date.now() / 1000))
    }

    onVisibleChanged: if (visible) reload()

    Connections {
        target: backend
        function onPagedReady(tag, titles, ids) {
            if (tag !== "settings") return
            configModel.clear()
            for (var i = 0; i < titles.length; ++i)
                configModel.append({ "name": titles[i] })
            profile.pageStatus = "配置项 " + configModel.count + " 条"
        }
        function onLoginResult(ok, msg) {
            profile.loggedIn = ok
            profile.pageStatus = (ok ? "登录成功：" : "登录失败：") + msg
        }
    }
}
