import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 登录 / 注册（对应 Kotlin 版 ui/screens/auth/AuthScreen.kt 的登录部分）。
 *
 * 依据 shared/data/JmRepository.kt：
 *   login(username, password) -> POST "login"（表单），成功取 jwtToken 作为凭证，
 *   之后所有请求自动带 Authorization: Bearer <jwt>。
 *
 * 现状（如实）：登录成功后凭证只在**进程内**生效，未落盘、未加密
 * （Kotlin 侧用 Android Keystore 加密存 SharedPreferences，桌面端待做）。
 * 注册接口本轮未接（Kotlin 侧注册不返回 token，需再登录一次）。
 */
Rectangle {
    id: login

    property color surfaceColor: "#26282c"
    property color strokeColor: "#3a3d43"
    property color titleColor: "#e6e6e6"
    property color bodyColor: "#b9bcc2"
    property color accentColor: "#5b8def"
    property string message: ""

    signal closeRequested()
    signal submit(string username, string password)
    signal logoutRequested()

    color: login.surfaceColor
    border.color: login.strokeColor
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        Text {
            text: "登录"
            color: login.titleColor; font.pixelSize: 18; font.bold: true
            Layout.fillWidth: true
        }
        TextField {
            id: userField
            Layout.fillWidth: true
            placeholderText: "用户名"
        }
        TextField {
            id: passField
            Layout.fillWidth: true
            placeholderText: "密码"
            echoMode: TextInput.Password
            onAccepted: login.submit(userField.text, passField.text)
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Button {
                text: "登录"
                onClicked: login.submit(userField.text, passField.text)
            }
            Button {
                text: "退出登录"
                onClicked: login.logoutRequested()
            }
        }
        Text {
            text: login.message
            color: login.accentColor; font.pixelSize: 12; wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Text {
            text: "说明：凭证当前仅保存在进程内（未落盘/未加密）；收藏、历史、通知等接口需要登录后才可用。"
            color: login.bodyColor; font.pixelSize: 11; wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }

        Button {
            text: "关闭"
            onClicked: login.closeRequested()
        }
    }
}
