import QtQuick

/* ItemMotion —— 对应 Kotlin 版 ui/components/ItemMotion.kt：
   列表项入场时做轻微的位移 + 淡入（Kotlin 侧是 Modifier 驱动的同款观感）。

   高度必须由**内容**决定：此前写 implicitHeight: holder.implicitHeight，
   而 holder 是普通 Item，子项不会贡献 implicitHeight，于是 delegate 高度算成 0，
   列表里 80 条全部叠在同一个位置（截图里表现为文字糊成一团）。
   现在用 holder.childrenRect.height，并让 holder 的高度跟内容走。 */
Item {
    id: motion
    property int index: 0
    property int staggerMs: 28
    property int durationMs: 220
    default property alias content: holder.data

    implicitWidth: holder.childrenRect.width
    implicitHeight: holder.childrenRect.height

    Item {
        id: holder
        width: motion.width
        height: childrenRect.height
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
