pragma Singleton
import QtQuick

/*
 * AppBackdrop —— 全局背景引用（单例）。
 *
 * 为什么需要它：玻璃要做到真背景模糊，必须知道"背后那一层"是谁（本实现里是 Main.qml 的
 * AmbientBackdrop）。但 Glass 在几十个页面里被使用，逐个传 backdrop 既啰嗦又容易漏。
 * 因此启动时由 Main.qml 把背景项登记到这里，Glass 在自身未显式指定 backdrop 时回退到它。
 *
 * 为 null 时（例如无背景的独立窗口）Glass 自动走半透明回退，不会报错。
 */
QtObject {
    property Item item: null
}
