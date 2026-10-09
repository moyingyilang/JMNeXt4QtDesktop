import QtQuick

/* ItemMotion —— 对应 Kotlin 版 ui/components/ItemMotion.kt：
   列表项入场时做轻微的位移 + 淡入（Kotlin 侧是 Modifier 驱动的同款观感）。 */
Item {
    id: motion
    property int index: 0
    property int staggerMs: 28
    property int durationMs: 220
    default property alias content: holder.data
    implicitHeight: holder.implicitHeight
    implicitWidth: holder.implicitWidth
    Item {
        id: holder
        anchors.fill: parent
        opacity: 0
        y: 12
        Component.onCompleted: {
            motion.delay = Math.min(motion.index * motion.staggerMs, 260)
            enter.start()
        }
    }
    property int delay: 0
    SequentialAnimation {
        id: enter
        PauseAnimation { duration: motion.delay }
        ParallelAnimation {
            NumberAnimation { target: holder; property: "opacity"; to: 1; duration: motion.durationMs; easing.type: Easing.OutCubic }
            NumberAnimation { target: holder; property: "y"; to: 0; duration: motion.durationMs; easing.type: Easing.OutCubic }
        }
    }
}
