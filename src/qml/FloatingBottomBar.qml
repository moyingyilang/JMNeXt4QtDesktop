import QtQuick

/* FloatingBottomBar —— 对应 Kotlin 版 ui/components/FloatingBottomBar.kt。
   用法：把按钮放进默认属性 children 里。 */
Glass {
    id: bottomBar
    property color textColor: "#e6e6e6"
    property int spacing: 10
    implicitHeight: 52
    radiusMd: 16
    default property alias items: row.data
    Row {
        id: row
        anchors.centerIn: parent
        spacing: bottomBar.spacing
    }
}
