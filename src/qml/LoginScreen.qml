import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "ui/Tokens.js" as Tokens

/*
 * 登录 / 注册（对应 Kotlin 版 auth/AuthScreen.kt 的登录部分）。
 *
 * 数据：backend.login(user, pass) -> loginResult(ok, message)
 * 现状：凭证只在进程内生效（未落盘、未加密）；注册接口未接。
 */
Rectangle {
    id: login

    property color surfaceColor: "#fef7ff"
    property color strokeColor: "#cac4d0"
    property color titleColor: "#1d1b20"
    property color bodyColor: "#49454f"
    property color accentColor: "#6750a4"
    property string message: ""

    signal closeRequested()
    signal submit(string username, string password)
    signal logoutRequested()

    color: Qt.rgba(surfaceColor.r, surfaceColor.g, surfaceColor.b, 1.0)
    radius: 12

    Connections {
        target: backend
        function onLoginResult(ok, msg) {
            login.message = (ok ? "登录成功：" : "登录失败：") + msg
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        PageHeader {
            title: "登录"
            countText: "JMNeXt 账号"
            description: "凭证只在本进程内有效；注册接口未接（Kotlin 侧注册后也要再登录一次）。"
            titleColor: login.titleColor
            bodyColor: login.bodyColor
            accentColor: login.accentColor
            actions: [ { key: "close", label: "关闭", primary: false } ]
            onActionClicked: (key) => { if (key === "close") login.closeRequested() }
        }

        // 表单限制宽度：Kotlin 桌面版也是居中窄表单，不是整宽输入框
        ColumnLayout {
            Layout.preferredWidth: 420
            Layout.maximumWidth: 420   // 不设上限会被外层 ColumnLayout 拉满整宽
            Layout.alignment: Qt.AlignLeft
            spacing: 12

            UiSectionTitle { text: "账号"; titleColor: login.titleColor }

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
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                UiPillButton {
                    text: "登录"
                    primary: true
                    accentColor: login.accentColor
                    onClicked: login.submit(userField.text, passField.text)
                }

                UiPillButton {
                    text: "退出登录"
                    primary: false
                    accentColor: login.accentColor
                    onClicked: login.logoutRequested()
                }
            }

            Text {
                Layout.fillWidth: true
                text: login.message
                color: login.message.indexOf("失败") >= 0 ? "#b3261e" : login.bodyColor
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
        }
    }
}
