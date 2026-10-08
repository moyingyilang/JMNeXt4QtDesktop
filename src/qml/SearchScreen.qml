import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 搜索屏（从 Main.qml 抽出，对应 Kotlin 版 ui/screens/search/SearchScreen.kt）。
 *
 * 本轮只搬"搜索行 + 条数标题"这两块纯渲染；后端调用（search）用信号回传，
 * 仍留在 Main.qml —— 与 HomeScreen 的抽法一致（一次只搬一层，每层验证）。
 *
 * 改写依据（抽前逐行读过原文）：真正要改的只有两处 ——
 *   1. `backend.search(...)` -> 信号 `searchRequested(word)`（2 处）；
 *   2. 标题的硬编码颜色 -> 组件属性 `secondaryColor`。
 *   `initialSearch` 是上下文属性（QML 全可见），**原样不动**；
 *   数据也以同名属性 `listModel` 接收，因此 `listModel.count` **无需改名**。
 */
ColumnLayout {
    id: search

    property var listModel
    property color secondaryColor: "#9aa0a8"

    signal searchRequested(string word)
    /// 打开"关于"屏（第 37 轮新增，零数据层成本的入口）
    signal aboutRequested()

    // 搜索行：输入关键词后回车或点按钮，结果直接进同一个列表（后端 search -> listReady）
    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 8; Layout.rightMargin: 8; Layout.topMargin: 8
        spacing: 6

        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "搜索作品…"
            text: typeof initialSearch !== "undefined" ? initialSearch : ""
            onAccepted: search.searchRequested(text)
        }
        Button {
            text: "搜索"
            onClicked: search.searchRequested(searchField.text)
        }
        Button {
            text: "关于"
            onClicked: search.aboutRequested()
        }
    }

    Text {
        text: "作品列表（" + search.listModel.count + " 条）"
        color: search.secondaryColor; font.pixelSize: 12
        Layout.leftMargin: 10; Layout.topMargin: 8
    }
}
