import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 每周更新（对应 Kotlin 版 ui/screens/week/WeekScreen.kt 的两级结构）。
 *
 * 依据 shared/data/JmRepository.kt：
 *   weekIssues()            -> GET "week"          返回 {categories:[{id,title,time}], type:[...]}
 *   weekList(issueId,type,page) -> GET "week/filter" 参数 id / type / page（PagedList）
 * 实测：week -> 260 条刊期（显示名取 time）；week/filter id=261&type=&page=1 -> 20 条作品（type 传空可用）。
 *
 * 数据走通用通路 JmBackend.loadPaged(tag,...) -> pagedReady(tag,...)：
 *   一级 tag "week"，二级 tag "weekworks"。
 */
ColumnLayout {
    id: week

    property color titleColor: "#e6e6e6"
    property color bodyColor: "#b9bcc2"
    property color surfaceColor: "#26282c"
    property color accentColor: "#5b8def"
    property string pageStatus: ""
    property string activeIssue: ""
    property string activeIssueName: ""

    signal closeRequested()
    signal comicClicked(string aid)

    ListModel { id: issueModel }
    ListModel { id: workModel }

    RowLayout {
        Layout.fillWidth: true
        Text {
            text: "每周更新"
            color: week.titleColor; font.pixelSize: 18; font.bold: true
            Layout.fillWidth: true
        }
        Text {
            text: week.activeIssueName.length > 0 ? ("刊期：" + week.activeIssueName) : "（选一个刊期看作品）"
            color: week.bodyColor; font.pixelSize: 12
        }
        Button {
            text: "刷新"
            onClicked: backend.loadPaged("week", "week", "")
        }
    }

    Text {
        text: week.pageStatus
        color: week.bodyColor; font.pixelSize: 12
        Layout.fillWidth: true; elide: Text.ElideRight
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 10

        // 一级：刊期
        ListView {
            id: issueView
            Layout.preferredWidth: 210
            Layout.fillHeight: true
            clip: true
            model: issueModel
            spacing: 2
            delegate: Rectangle {
                width: ListView.view.width; height: 30
                color: issueMouse.containsMouse ? week.surfaceColor : "transparent"
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left; anchors.leftMargin: 6
                    text: name
                    color: week.titleColor; font.pixelSize: 13; elide: Text.ElideRight
                    width: parent.width - 12
                }
                MouseArea {
                    id: issueMouse
                    anchors.fill: parent; hoverEnabled: true
                    onClicked: {
                        week.activeIssue = cid
                        week.activeIssueName = name
                        workModel.clear()
                        console.log("QML 周更：选刊期 " + cid)
                        backend.loadPaged("weekworks", "week/filter", "id=" + cid + "&type=&page=1")
                    }
                }
            }
        }

        // 二级：该刊期作品
        ListView {
            id: workView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: workModel
            spacing: 2
            delegate: ComicCard {
                width: ListView.view.width
                title: model.title
                aid: model.aid
                coverUrl: model.coverUrl
                hoverColor: week.surfaceColor
                textColor: week.titleColor
                cardRadius: 6
                onClicked: week.comicClicked(aid)
            }
        }
    }

    Button {
        text: "关闭"
        onClicked: week.closeRequested()
    }

    Connections {
        target: backend
        function onPagedReady(tag, titles, ids) {
            if (tag === "week") {
                issueModel.clear()
                for (var i = 0; i < titles.length; ++i)
                    issueModel.append({ "name": titles[i], "cid": ids[i] })
                week.pageStatus = "刊期 " + issueModel.count + " 个"
            } else if (tag === "weekworks") {
                workModel.clear()
                for (var j = 0; j < titles.length; ++j)
                    workModel.append({ "title": titles[j], "aid": ids[j], "coverUrl": "" })
                week.pageStatus = "该刊期作品 " + workModel.count + " 条"
                if (workModel.count > 0) backend.loadCovers(20)
            }
        }
    }

    Component.onCompleted: backend.loadPaged("week", "week", "")
}
