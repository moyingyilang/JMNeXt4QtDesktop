import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 评论（对应 Kotlin 版 ui/screens/comments/CommentsScreen.kt）。
 *
 * 依据 shared/data/JmRepository.kt 的 comments(aid, page, mode)：
 *   GET "forum"，参数 mode / page / aid（详情页固定 mode=all）。
 * 实测（登录后）：`--login` 的探测显示 forum 返回 2 条，首条「康娜卡姆依」。
 *
 * 数据走通用通路 loadPaged(tag,...)：tag 用 "comments"。
 * 评论行的字段名与作品列表不同，故这里直接显示原始行（后续按 DTO 细化）。
 */
ColumnLayout {
    id: comments

    property color titleColor: "#e6e6e6"
    property color bodyColor: "#b9bcc2"
    property color surfaceColor: "#26282c"
    property color accentColor: "#5b8def"
    property string pageStatus: ""
    property string aid: ""

    signal closeRequested()

    ListModel { id: commentModel }

    RowLayout {
        Layout.fillWidth: true
        Text {
            text: comments.aid.length > 0 ? ("评论（作品 " + comments.aid + "）") : "评论"
            color: comments.titleColor; font.pixelSize: 18; font.bold: true
            Layout.fillWidth: true
        }
        Button {
            text: "刷新"
            onClicked: comments.reload()
        }
    }
    Text {
        text: comments.pageStatus
        color: comments.bodyColor; font.pixelSize: 12
        Layout.fillWidth: true; elide: Text.ElideRight
    }

    ListView {
        id: commentView
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: commentModel
        spacing: 4
        delegate: Rectangle {
            width: ListView.view.width
            height: Math.max(38, lineText.implicitHeight + 16)
            color: index % 2 === 0 ? "transparent" : comments.surfaceColor
            Text {
                id: lineText
                anchors.fill: parent
                anchors.margins: 8
                text: name
                color: comments.titleColor; font.pixelSize: 13
                wrapMode: Text.WordWrap; elide: Text.ElideRight
            }
        }
    }

    Button {
        text: "关闭"
        onClicked: comments.closeRequested()
    }

    function reload() {
        if (comments.aid.length === 0) { comments.pageStatus = "未选择作品"; return }
        commentModel.clear()
        comments.pageStatus = "正在载入评论…"
        backend.loadPaged("comments", "forum", "mode=all&page=1&aid=" + comments.aid)
    }

    onAidChanged: reload()
    Component.onCompleted: reload()

    Connections {
        target: backend
        function onPagedReady(tag, titles, ids) {
            if (tag !== "comments") return
            commentModel.clear()
            for (var i = 0; i < titles.length; ++i)
                commentModel.append({ "name": titles[i] })
            comments.pageStatus = "评论 " + commentModel.count + " 条"
        }
    }
}
