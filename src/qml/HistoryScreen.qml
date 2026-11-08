import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "ui/Tokens.js" as Tokens

/*
 * 阅读历史（本地 HistoryStore）—— 与收藏页同一套列表页形态：
 * 头部（标题 + 计数 + 清空/关闭）+ 封面网格。
 *
 * 历史在打开作品时写入（见 Main 的 onAlbumReady），断网也在；
 * 封面用图片提供器的 albumcover 兜底，命中磁盘缓存时秒出。
 */
Rectangle {
    id: historyPane

    property color surfaceColor: "#fef7ff"
    property color strokeColor: "#cac4d0"
    property color titleColor: "#1d1b20"
    property color bodyColor: "#49454f"
    property color accentColor: "#6750a4"

    signal closeRequested()
    signal comicClicked(string aid)

    color: Qt.rgba(surfaceColor.r, surfaceColor.g, surfaceColor.b, 1.0)
    radius: 12

    ListModel { id: historyModel }

    function reload() {
        historyModel.clear()
        const rows = backend.history()
        for (let i = 0; i < rows.length; ++i) {
            historyModel.append({
                aid: rows[i].aid,
                title: rows[i].title && rows[i].title.length > 0 ? rows[i].title : ("作品 " + rows[i].aid)
            })
        }
    }

    onVisibleChanged: if (visible) reload()

    Connections {
        target: backend
        function onHistoryChanged() { if (historyPane.visible) historyPane.reload() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        PageHeader {
            title: "阅读历史"
            countText: "共 " + historyModel.count + " 条"
            description: historyModel.count > 0
                         ? "点击卡片重新打开作品。"
                         : "还没有记录。打开任意作品后，这里会出现它。"
            titleColor: historyPane.titleColor
            bodyColor: historyPane.bodyColor
            accentColor: historyPane.accentColor
            actions: [
                { key: "clear", label: "清空", primary: false },
                { key: "close", label: "关闭", primary: false }
            ]
            onActionClicked: (key) => {
                if (key === "clear") backend.clearHistory()
                else if (key === "close") historyPane.closeRequested()
            }
        }

        GridView {
            id: historyGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: historyModel
            cellWidth: 176
            cellHeight: 268

            delegate: ComicGridCard {
                width: 160
                title: model.title
                aid: model.aid
                titleColor: historyPane.titleColor
                authorColor: historyPane.bodyColor
                onClicked: historyPane.comicClicked(model.aid)
            }
        }
    }
}
