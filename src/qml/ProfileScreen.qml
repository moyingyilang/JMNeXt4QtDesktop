import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 我的（对应 Kotlin 版 ui/screens/profile/ProfileScreen.kt）。
 *
 * 结构：Rectangle 根（负责底色与圆角）+ ColumnLayout 内容。
 * 早先根直接是 ColumnLayout，导致面板被布局当成一列、宽度只有内容宽：
 * Layout 类型自己管尺寸，anchors 会被忽略。改成 Rectangle 根后锚定才生效。
 *
 * 数据：settings() 取应用配置（实测 4 条）；登录态来自 login()。
 * 用户资料明细（头像/等级/经验）尚未解析，这里不假装有。
 */
Rectangle {
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

    color: Qt.rgba(surfaceColor.r, surfaceColor.g, surfaceColor.b, 1.0)
    radius: 12

    ListModel { id: configModel }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: "我的"
                color: profile.titleColor
                font.pixelSize: 24
                font.bold: true
                Layout.fillWidth: true
            }
            UiPillButton {
                text: profile.loggedIn ? ("已登录：" + profile.userName) : "去登录"
                primary: !profile.loggedIn
                accentColor: profile.accentColor
                onClicked: profile.loginRequested()
            }
            UiPillButton {
                text: "关闭"
                primary: false
                accentColor: profile.accentColor
                onClicked: profile.closeRequested()
            }
        }

        Text {
            Layout.fillWidth: true
            text: profile.loggedIn
                  ? "登录状态：已登录（凭证仅在本次运行内有效）"
                  : "登录状态：未登录 —— 收藏 / 历史 / 通知 / 签到 / 追更 需要登录"
            color: profile.loggedIn ? profile.accentColor : profile.bodyColor
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            text: profile.pageStatus
            color: profile.bodyColor
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        UiSectionTitle {
            Layout.fillWidth: true
            text: "应用配置"
            titleColor: profile.titleColor
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: configModel
            spacing: 2

            delegate: UiRow {
                width: ListView.view.width
                title: name
                titleColor: profile.titleColor
                subtitleColor: profile.bodyColor
            }
        }
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
            for (var i = 0; i < titles.length; ++i) {
                // 后端标题形如 "图源1\n[]"，空分类要剥掉，否则界面上多一行 "[]"
                var clean = ("" + titles[i]).replace(/\n\s*\[\s*\]\s*$/, "")
                configModel.append({ "name": clean })
            }
            profile.pageStatus = "配置项 " + configModel.count + " 条"
        }
        function onLoginResult(ok, msg) {
            profile.loggedIn = ok
            profile.pageStatus = (ok ? "登录成功：" : "登录失败：") + msg
        }
    }
}
