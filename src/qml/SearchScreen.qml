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

    property string orderKey: ""
    property color titleColor: "#e6e6e6"
    property color accentColor: "#5b8def"

    signal searchRequested(string word)
    signal searchOrderedRequested(string word, string order)
    /// 打开"关于"屏（第 37 轮新增，零数据层成本的入口）
    /// 导航按钮行是否显示：侧栏上线后由 Main 传 false（Kotlin 桌面版把入口放在侧栏）
    property bool showNavButtons: true

    signal aboutRequested()
    /// 打开设置屏（第 39 轮）
    signal settingsRequested()
    /// 打开分类屏（第 49 轮）
    signal categoryRequested()
    /// 打开"更多"屏（第 58 轮）
    signal moreRequested()

    // 导航行：四个入口单独一行。原来它们和搜索框挤在同一行，左栏只有 380px，
    // 结果「更多」被裁成「更」——拆成两行后每行都不溢出。
    RowLayout {
        visible: search.showNavButtons
        Layout.fillWidth: true
        Layout.leftMargin: 8; Layout.rightMargin: 8; Layout.topMargin: 8
        spacing: 8

        Button {
            text: "关于"
            onClicked: search.aboutRequested()
        }
        Button {
            text: "设置"
            onClicked: search.settingsRequested()
        }
        Button {
            text: "分类"
            onClicked: search.categoryRequested()
        }
        Button {
            text: "更多"
            onClicked: search.moreRequested()
        }
        Item { Layout.fillWidth: true }
    }

    // 搜索行：输入关键词后回车或点按钮，结果直接进同一个列表（后端 search -> listReady）
    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 8; Layout.rightMargin: 8
        spacing: 8

        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "搜索作品…"
            text: typeof initialSearch !== "undefined" ? initialSearch : ""
            onAccepted: search.searchRequested(text)
        }
        Button {
            text: "搜索"
            onClicked: {
                if (search.orderKey === "")
                    search.searchRequested(searchField.text)
                else
                    search.searchOrderedRequested(searchField.text, search.orderKey)
            }
        }
    }

    // 排序：对应主项目 SearchFilters.Order。空串表示「最新」（接口省略 o 参数）。
    ExpandableOptionRow {
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        title: "排序"
        titleColor: search.titleColor
        valueColor: search.secondaryColor
        accentColor: search.accentColor
        current: search.orderKey
        options: [
            { value: "", label: "最新" },
            { value: "mv", label: "最多点阅" },
            { value: "mp", label: "最多图片" },
            { value: "tf", label: "最多爱心" },
            { value: "old", label: "最旧" }
        ]
        onSelected: function (v) {
            search.orderKey = v
            if (searchField.text.length > 0)
                search.searchOrderedRequested(searchField.text, v)
        }
    }

    Text {
        text: "作品列表（" + search.listModel.count + " 条）"
        color: search.secondaryColor; font.pixelSize: 12
        Layout.leftMargin: 12; Layout.topMargin: 8
    }
}
