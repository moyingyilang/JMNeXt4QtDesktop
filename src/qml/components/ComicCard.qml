import QtQuick
import QtQuick.Controls

/*
 * 作品卡片（对应 Kotlin 版 ui/components/ComicCard.kt）。
 *
 * 移植说明：Kotlin 版对外参数是 (comic, onClick, modifier)，这里同样只暴露
 * "展示一部作品 + 被点击"这三件事，不掺入任何业务：
 *   title / author / coverUrl 三个属性 + clicked() 信号。
 *
 * 观感按 Main.qml 里既有的列表项 1:1 复刻（74px 高、封面 PreserveAspectCrop、
 * 标题单行省略、封面未就绪时显示占位边框），因为那一版已经过真机截图核对。
 */
Rectangle {
    id: card

    property string title: ""
    property string author: ""
    property string coverUrl: ""

    signal clicked()

    width: ListView.view ? ListView.view.width : implicitWidth
    height: 74
    color: "transparent"

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
        text: card.author.length > 0 ? card.title + "\n" + card.author : card.title
        color: "#e6e6e6"
        font.pixelSize: 13
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        maximumLineCount: 2
    }

    MouseArea {
        anchors.fill: parent
        onClicked: card.clicked()
    }
}
