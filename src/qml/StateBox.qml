import QtQuick
import QtQuick.Controls

/*
 * 状态展示（对应 Kotlin 版 ui/components/StateBox.kt 里的
 * LoadingBox / MessageState / ErrorBox 三个组合函数）。
 *
 * Kotlin 侧是三个函数，QML 侧一个文件的惯例是"一个类型"，所以这里收敛成
 * 一个组件 + `kind` 属性：
 *   kind = "loading"  对应 LoadingBox（转圈 + 可选说明）
 *   kind = "message"  对应 MessageState（标题 + 说明，可选重试）
 *   kind = "error"    对应 ErrorBox（错误信息，可选重试）
 *
 * 重试用 `signal retry()`：由父级连接（QML 里不能像 Compose 那样直接收 lambda）。
 *
 * 颜色由调用方注入（与 ComicCard 同一理由：主题 token 在 Main.qml 里是运行时赋值的属性，
 * 组件内写死会让四套风格切换失效）。
 */
Item {
    id: box

    property string kind: "loading"     // loading | message | error
    property string title: ""
    property string description: ""
    property bool canRetry: false

    property color textColor: "#e6e6e6"
    property color secondaryColor: "#b9bcc2"
    property color accentColor: "#5b8def"

    signal retry()

    implicitWidth: 240
    implicitHeight: 120

    BusyIndicator {
        id: spinner
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 12
        running: box.kind === "loading"
        visible: running
        width: 32
        height: 32
    }

    Text {
        id: titleText
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: spinner.visible ? spinner.bottom : parent.top
        anchors.topMargin: spinner.visible ? 8 : 16
        width: parent.width - 16
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        color: box.kind === "error" ? "#ff7b72" : box.textColor
        font.pixelSize: 13
        font.bold: box.kind !== "loading"
        text: box.kind === "loading"
              ? (box.description.length > 0 ? box.description : "载入中…")
              : box.title
    }

    Text {
        id: descText
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: titleText.bottom
        anchors.topMargin: 6
        width: parent.width - 16
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        color: box.secondaryColor
        font.pixelSize: 12
        visible: box.kind !== "loading" && box.description.length > 0
        text: box.description
    }

    Button {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: descText.visible ? descText.bottom : titleText.bottom
        anchors.topMargin: 10
        visible: box.canRetry
        text: "重试"
        onClicked: box.retry()
    }
}
