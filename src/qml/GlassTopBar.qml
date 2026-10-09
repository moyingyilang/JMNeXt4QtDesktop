import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/* GlassTopBar —— 对应 Kotlin 版 ui/components/GlassTopBar.kt（标题 + 返回/动作区）。 */
Glass {
    id: topBar
    property string title: ""
    property color titleColor: "#e6e6e6"
    property bool canBack: true
    signal backClicked()
    implicitHeight: 44
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8; anchors.rightMargin: 8
        spacing: 6
        Button {
            text: "返回"
            visible: topBar.canBack
            onClicked: topBar.backClicked()
        }
        Text {
            text: topBar.title
            color: topBar.titleColor; font.pixelSize: 16; font.bold: true
            Layout.fillWidth: true; elide: Text.ElideRight
        }
        Row { id: actionsSlot; spacing: 6 }
        default property alias actions: actionsSlot.data
    }
}
