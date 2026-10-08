import QtQuick
import QtQuick.Controls

/*
 * 作品卡片（对应 Kotlin 版 ui/components/ComicCard.kt）。
 *
 * 参数与列表模型一致：`title` / `aid` / `coverUrl`（依据 Main.qml:291 的
 * listModel.append({ "title": …, "aid": …, "coverUrl": "" })）。
 *
 * 主题色**由调用方传入**（hoverColor / textColor / radius），因为主题 token 在
 * Main.qml 里是一组属性、由 themeInit 在运行时赋值（Main.qml:250-255 / 335-340）——
 * 组件里写死颜色会让四套风格切换失效。
 *
 * 动效与既有 delegate 保持一致（P3 阶段已验证的观感）：
 *   悬停背景 120ms 过渡；按压 0.985 弹性（SpringAnimation spring 2.5 / damping 0.35）。
 * 这两条从 Main.qml 的 delegate 原样搬来，避免"换了组件却丢了动效"。
 */
Rectangle {
    id: card

    property string title: ""
    property string aid: ""
    property string coverUrl: ""

    // 主题（由调用方注入，默认值仅作兜底，不参与四套风格切换）
    property color hoverColor: "#2f3237"
    property color textColor: "#e6e6e6"
    property int cardRadius: 6

    signal clicked()

    width: ListView.view ? ListView.view.width : implicitWidth
    height: 74
    color: mouseArea.containsMouse ? card.hoverColor : "transparent"
    radius: card.cardRadius
    scale: mouseArea.pressed ? 0.985 : 1.0

    Behavior on color { ColorAnimation { duration: 120 } }
    Behavior on scale { SpringAnimation { spring: 2.5; damping: 0.35 } }

    Image {
        id: cover
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 8
        width: 44
        height: 58
        source: card.coverUrl.length > 0 ? card.coverUrl : ""
        fillMode: Image.PreserveAspectCrop
        asynchronous: true

        Rectangle {
            anchors.fill: parent
            color: "#2b2d31"
            border.color: "#3a3d43"
            border.width: 1
            visible: cover.status !== Image.Ready
        }
    }

    Text {
        anchors.left: cover.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        text: card.title
        color: card.textColor
        font.pixelSize: 13
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        maximumLineCount: 2
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: card.clicked()
    }
}
