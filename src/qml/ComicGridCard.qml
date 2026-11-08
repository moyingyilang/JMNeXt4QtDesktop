import QtQuick
import "ui/Tokens.js" as Tokens

/*
 * 网格卡片 —— 对齐 Kotlin 桌面版的首页封面网格：
 * 上方封面（圆角 8、等比裁切），下方两行标题（超出省略），再一行作者。
 *
 * 与旧的 ComicCard 区别：ComicCard 是「左图右文」的列表行（380 宽），
 * 网格卡片是竖排 160 宽，一屏能放 6 列 —— 这正是 Kotlin 首页的形态。
 */
Item {
    id: card

    property string title: ""
    property string author: ""
    property string aid: ""
    property string coverUrl: ""
    property color titleColor: "#3b3b45"
    property color authorColor: "#8a8a99"
    property color placeholderColor: "#dcdcf5"
    property int coverHeight: 200

    signal clicked()

    implicitWidth: 160
    implicitHeight: coverHeight + 52

    Rectangle {
        id: coverFrame
        width: parent.width
        height: card.coverHeight
        radius: Tokens.radiusLg
        color: card.placeholderColor
        clip: true

        Image {
            anchors.fill: parent
            source: card.coverUrl
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            visible: status === Image.Ready
        }
    }

    Text {
        anchors.top: coverFrame.bottom
        anchors.topMargin: Tokens.space8
        width: parent.width
        text: card.title
        color: card.titleColor
        font.pixelSize: Tokens.fontBody
        wrapMode: Text.Wrap
        maximumLineCount: 2
        elide: Text.ElideRight
    }

    Text {
        anchors.top: coverFrame.bottom
        anchors.topMargin: Tokens.space8 + 34
        width: parent.width
        text: card.author
        color: card.authorColor
        font.pixelSize: Tokens.fontCaption
        elide: Text.ElideRight
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: card.clicked()
    }
}
