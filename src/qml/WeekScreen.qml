import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "ui/Tokens.js" as Tokens

/*
 * 每周更新（对应 Kotlin 版 ui/screens/week/WeekScreen.kt 的两级结构）。
 *
 * 数据：JmBackend.loadPaged(tag, path, query) -> pagedReady(tag, titles, ids)
 *   一级 tag "week"        -> GET "week"         刊期列表
 *   二级 tag "weekworks"   -> GET "week/filter"  某刊期的作品
 *
 * 结构：Rectangle 根（底色 + 圆角）→ ColumnLayout → RowLayout（左刊期 / 右作品网格）。
 * 早先根直接是 Column，面板被当成一列、并不满窗；这次按其它屏的统一写法重写。
 */
Rectangle {
    id: week

    property color titleColor: "#1d1b20"
    property color bodyColor: "#49454f"
    property color surfaceColor: "#fef7ff"
    property color accentColor: "#6750a4"
    property string pageStatus: ""
    property string activeIssue: ""
    property string activeIssueName: ""

    signal closeRequested()
    signal comicClicked(string aid)

    color: Qt.rgba(surfaceColor.r, surfaceColor.g, surfaceColor.b, 1.0)
    radius: 12

    ListModel { id: issueModel }
    ListModel { id: workModel }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: "每周更新"
                color: week.titleColor
                font.pixelSize: 24
                font.bold: true
                Layout.fillWidth: true
            }

            Text {
                text: week.activeIssueName.length > 0 ? ("刊期：" + week.activeIssueName) : "（选一个刊期看作品）"
                color: week.bodyColor
                font.pixelSize: 12
            }

            UiPillButton {
                text: "刷新"
                primary: true
                accentColor: week.accentColor
                onClicked: backend.loadPaged("week", "week", "")
            }

            UiPillButton {
                text: "关闭"
                primary: false
                accentColor: week.accentColor
                onClicked: week.closeRequested()
            }
        }

        Text {
            Layout.fillWidth: true
            text: week.pageStatus
            color: week.bodyColor
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            // 一级：刊期
            ListView {
                id: issueView
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                clip: true
                model: issueModel
                spacing: 2

                delegate: UiRow {
                    width: issueView.width
                    tight: true
                    title: model.name
                    titleColor: week.activeIssue === model.cid ? week.accentColor : week.titleColor
                    onClicked: {
                        week.activeIssue = model.cid
                        week.activeIssueName = model.name
                        backend.loadPaged("weekworks", "week/filter",
                                          "id=" + model.cid + "&type=&page=1")
                    }
                }
            }

            // 二级：该刊期作品（网格）
            GridView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: workModel
                cellWidth: 176
                cellHeight: 268

                delegate: ComicGridCard {
                    width: 160
                    title: model.title
                    aid: model.aid
                    coverUrl: model.coverUrl
                    titleColor: week.titleColor
                    authorColor: week.bodyColor
                    onClicked: week.comicClicked(aid)
                }
            }
        }
    }

    Connections {
        target: backend
        function onPagedReady(tag, titles, ids) {
            if (tag === "week") {
                issueModel.clear()
                for (var i = 0; i < titles.length; ++i) {
                    // 后端标题形如 "2026第264期11.06 - 10.30\n[]"，尾部空分类要剥掉
                    var clean = ("" + titles[i]).replace(/\n\s*\[\s*\]\s*$/, "")
                    issueModel.append({ "name": clean, "cid": ids[i] })
                }
                week.pageStatus = "刊期 " + issueModel.count + " 个"
            } else if (tag === "weekworks") {
                workModel.clear()
                for (var j = 0; j < titles.length; ++j) {
                    var cleanW = ("" + titles[j]).replace(/\n\s*\[\s*\]\s*$/, "")
                    workModel.append({ "title": cleanW, "aid": ids[j], "coverUrl": "" })
                }
                week.pageStatus = "该刊期作品 " + workModel.count + " 条"
                if (workModel.count > 0) backend.loadCovers(20)
            }
        }
    }

    onVisibleChanged: if (visible) backend.loadPaged("week", "week", "")
}
