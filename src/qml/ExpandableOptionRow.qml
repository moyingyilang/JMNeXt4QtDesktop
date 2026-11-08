import QtQuick

/*
 * 可展开选项行 —— 对应主项目 components/ExpandableOptionRow.kt。
 *
 * 为什么不用横排按钮：选项一多就挤成两三行，而且靠标记表示选中的做法既不清晰也占地方
 * （主项目已因此改掉）。这里改成「一行显示当前值 + 箭头，点开是竖向列表」。
 *
 * 动效与主项目一致：箭头弹性旋转、列表弹性展开（Qt Quick 用 Behavior + NumberAnimation，
 * 参数取 spring 的近似值：时长 220ms、Easing.OutBack 带轻微过冲）。
 */
Item {
    id: row

    property string title: ""
    /** [{ value, label }]，value 用于回调 */
    property var options: []
    property var current: undefined
    property color titleColor: "#e6e6e6"
    property color valueColor: "#9aa0a8"
    property color accentColor: "#5b8def"
    property color borderColor: "#3a3d43"

    signal selected(var value)

    implicitWidth: parent ? parent.width : 0
    implicitHeight: column.implicitHeight

    property bool expanded: false

    function currentLabel() {
        for (var i = 0; i < row.options.length; ++i)
            if (row.options[i].value === row.current) return row.options[i].label
        return ""
    }

    Column {
        id: column
        width: row.width

        // 头部：标题 + 当前值 + 箭头
        Rectangle {
            width: column.width
            height: 40
            color: mouse.containsMouse ? Qt.rgba(1, 1, 1, 0.04) : "transparent"
            radius: 6

            MouseArea {
                id: mouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: row.expanded = !row.expanded
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: row.title
                color: row.titleColor
                font.pixelSize: 14
            }

            Text {
                anchors.right: arrow.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: row.currentLabel()
                color: row.valueColor
                font.pixelSize: 12
            }

            Text {
                id: arrow
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: "▾"
                color: row.valueColor
                font.pixelSize: 12
                rotation: row.expanded ? 180 : 0
                Behavior on rotation {
                    NumberAnimation { duration: 220; easing.type: Easing.OutBack }
                }
            }
        }

        // 展开区：高度弹性过渡
        Item {
            width: column.width
            height: row.expanded ? listColumn.implicitHeight : 0
            clip: true
            Behavior on height {
                NumberAnimation { duration: 220; easing.type: Easing.OutBack }
            }

            Column {
                id: listColumn
                width: parent.width

                Repeater {
                    model: row.options
                    delegate: Rectangle {
                        width: listColumn.width
                        height: 36
                        radius: 6
                        color: itemMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.06) : "transparent"

                        MouseArea {
                            id: itemMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                row.selected(modelData.value)
                                row.expanded = false
                            }
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 16
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.label
                            // 选中态只用颜色与字重，不加圆点或对勾（与主项目一致）
                            color: modelData.value === row.current ? row.accentColor : row.titleColor
                            font.pixelSize: 13
                            font.bold: modelData.value === row.current
                        }
                    }
                }
            }
        }
    }
}
