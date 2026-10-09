import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 阅读器（从 Main.qml 抽出，对应 Kotlin 版 ui/screens/reader/ReaderScreen.kt）。
 *
 * 改写依据（抽前逐行读过原文）：`root.pageUrl` / `root.pageStatus` / `root.cBg` /
 * `root.cTextSecondary` → 组件属性；`backend.step(±1)` → 信号 `stepRequested(delta)`；
 * 原先的 `visible: root.reading` 由调用方在实例上设置，组件内部不再管显隐。
 *
 * **一处有意的删减**：原文 `onStatusChanged` 里调用块外的 `geomProbe.restart()`
 * （一个诊断用 Timer，定义在 Main.qml 里，组件内不可见）。本轮去掉了这一行调用，
 * 保留同一处的 `console.log`（绘制尺寸日志仍在）。诊断探针如果需要，应随后续
 * 一轮把它一并搬进组件 —— 已记入 STATUS，不当作"已对齐"。
 */
Rectangle {
    id: reader

    property string pageUrl: ""
    property string pageStatus: ""
    property color bgColor: "#1e1f22"
    property color secondaryColor: "#b9bcc2"

    signal stepRequested(int delta)

    /// 缩放倍率（1.0 原始）；手势与滚轮改它
    property real zoom: 1.0
    readonly property real zoomMin: 1.0
    readonly property real zoomMax: 3.0
    property real dragAccum: 0

    color: reader.bgColor

    // 手势层：左右拖动翻页、滚轮缩放、双击复位（对应 Kotlin 阅读器的 swipe + zoom）
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onPressed: reader.dragAccum = 0
        onPositionChanged: reader.dragAccum = mouseX - pressX
        onReleased: {
            if (Math.abs(reader.dragAccum) > Math.min(width * 0.08, 80)) {
                var d = reader.dragAccum < 0 ? 1 : -1
                console.log("QML 阅读器手势翻页 delta=" + d)
                reader.stepRequested(d)
            }
            reader.dragAccum = 0
        }
        onDoubleClicked: reader.zoom = (reader.zoom > 1.01 ? reader.zoomMin : 2.0)
        onWheel: (wheel) => {
            var next = reader.zoom + (wheel.angleDelta.y > 0 ? 0.15 : -0.15)
            reader.zoom = Math.max(reader.zoomMin, Math.min(reader.zoomMax, next))
        }
    }
    PinchArea {
        anchors.fill: parent
        pinch.minimumScale: reader.zoomMin
        pinch.maximumScale: reader.zoomMax
        onPinchUpdated: (pinch) => { reader.zoom = Math.max(reader.zoomMin, Math.min(reader.zoomMax, pinch.scale)) }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        Image {
            id: pageImage
            Layout.fillWidth: true
            Layout.fillHeight: true
            fillMode: Image.PreserveAspectFit
            source: reader.pageUrl
            // 缩放：滚轮（桌面）与捏合（触屏）都改这个值；双击复位
            scale: reader.zoom
            transformOrigin: Item.Center
            onStatusChanged: if (status === Image.Ready) {
                console.log("QML 阅读页已显示：源 " + sourceSize.width + "x" + sourceSize.height
                            + "，实际绘制 " + Math.round(paintedWidth) + "x" + Math.round(paintedHeight))
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Button { text: "上一页"; onClicked: reader.stepRequested(-1) }
            Button { text: "下一页"; onClicked: reader.stepRequested(1) }
            Text {
                Layout.fillWidth: true
                text: reader.pageStatus
                color: reader.secondaryColor; font.pixelSize: 12
                elide: Text.ElideRight
            }
        }
    }
}
