import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 阅读历史（对应主项目 profile 里的 DailyHistorySection 与本地 HistoryStore）。
 *
 * 数据来源是**本地**（core/HistoryStore.h）：打开作品时写入，断网也在。
 * 此前「更多 → 阅读历史」走的是服务端 watch_list 接口（需登录），
 * 这里改为本地优先，未登录也能用 —— 与主项目「本地是唯一数据源」的结论一致。
 */
Rectangle {
    id: historyPane

    property color surfaceColor: "#26282c"
    property color strokeColor: "#3a3d43"
    property color titleColor: "#e6e6e6"
    property color bodyColor: "#b9bcc2"
    property color accentColor: "#5b8def"

    signal closeRequested()
    signal comicClicked(string aid)

    color: Qt.rgba(surfaceColor.r, surfaceColor.g, surfaceColor.b, 1.0)
    border.color: strokeColor
    border.width: 1

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

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Text {
                text: "阅读历史（本地，共 " + historyModel.count + " 条）"
                color: historyPane.titleColor
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
            }
            Button {
                text: "清空"
                enabled: historyModel.count > 0
                onClicked: {
                    backend.clearHistory()
                    historyPane.reload()
                }
            }
            Button {
                text: "关闭"
                onClicked: historyPane.closeRequested()
            }
        }

        Text {
            text: historyModel.count === 0
                  ? "还没有记录。打开任意作品后，这里会出现它。"
                  : "点击条目可重新打开该作品。"
            color: historyPane.bodyColor
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: historyModel
            spacing: 2

            delegate: UiRow {
                width: ListView.view.width
                title: model.title
                subtitle: "aid " + model.aid
                titleColor: historyPane.titleColor
                subtitleColor: historyPane.bodyColor
                onClicked: historyPane.comicClicked(model.aid)
            }
        }
    }
}
