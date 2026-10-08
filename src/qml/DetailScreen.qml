import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 详情视图（从 Main.qml 抽出，对应 Kotlin 版 ui/screens/detail/DetailScreen.kt）。
 *
 * 改写依据（抽前逐行读过原文 80 行）：
 *   root.albumAid/albumName/albumAuthor/albumTags/pageStatus -> 组件属性；
 *   chapterModel（块外的 id）-> 属性传入（模型本身可以直接传）；
 *   backend.openChapterId(cid, 0) -> 信号 chapterClicked(cid)；
 *   root.cSurface1 / root.cStroke 与 7 处硬编码色 -> 组件属性（主题由调用方注入，
 *   写死会让四套风格切换失效）；visible: !root.reading 移到实例上。
 */
Rectangle {
    id: detail

    property string albumAid: ""
    // 以下三项由 onAlbumReady 填充（第 34 轮下沉）：原先在 Main.qml 再注入，属于"两个地方都以为在管"
    property string albumName: ""
    property string albumAuthor: ""
    property string albumTags: ""
    property string pageStatus: ""
    // 章节模型：第 35 轮从 Main.qml 搬入（原先靠属性传进来，但填充它的回调也在这里，属于同一个归属）
    ListModel { id: chapterModel }

    property color surfaceColor: "#26282c"
    property color strokeColor: "#3a3d43"
    property color titleColor: "#e6e6e6"
    property color authorColor: "#b9bcc2"
    property color tagColor: "#9aa0a8"
    property color statusColor: "#8a8f98"
    property color hoverColor: "#26282c"
    property color placeholderColor: "#2b2d31"

    signal chapterClicked(string cid)

    color: detail.surfaceColor
    border.color: detail.strokeColor
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        Image {
            id: detailCover
            Layout.preferredWidth: 120
            Layout.preferredHeight: 160
            fillMode: Image.PreserveAspectCrop
            source: detail.albumAid.length > 0 ? "image://jm/albumcover?" + detail.albumAid : ""
            onStatusChanged: if (status === Image.Ready)
                console.log("QML 详情封面已加载：" + sourceSize.width + "x" + sourceSize.height)
            Rectangle {
                anchors.fill: parent; color: detail.placeholderColor
                border.color: detail.strokeColor; border.width: 1
                visible: parent.status !== Image.Ready
            }
        }
        Text {
            text: detail.albumName.length > 0 ? detail.albumName : "（从左侧选一个作品）"
            color: detail.titleColor; font.pixelSize: 18; wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Text {
            text: detail.albumAuthor
            color: detail.authorColor; font.pixelSize: 13
        }
        Text {
            text: detail.albumTags
            color: detail.tagColor; font.pixelSize: 12; wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Text {
            text: "章节（" + chapterModel.count + "）"
            color: detail.tagColor; font.pixelSize: 12
            Layout.topMargin: 6
        }
        ListView {
            id: chapterView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: chapterModel
            spacing: 2
            delegate: Rectangle {
                width: ListView.view.width; height: 30
                color: chapterMouse.containsMouse ? detail.hoverColor : "transparent"
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left; anchors.leftMargin: 6
                    text: (index + 1) + ". " + name
                    color: detail.titleColor; font.pixelSize: 13
                }
                MouseArea {
                    id: chapterMouse
                    anchors.fill: parent; hoverEnabled: true
                    onClicked: {
                        console.log("QML 点击章节 index=" + index + " id=" + cid)
                        detail.chapterClicked(cid)
                    }
                }
            }
        }
        Text {
            text: detail.pageStatus
            color: detail.statusColor; font.pixelSize: 12
            Layout.fillWidth: true; elide: Text.ElideRight
        }
    }

    // 详情文本的后端回调在此收口（第 34 轮下沉）。
    // albumAid 仍由外层传入 —— 它来自"点击卡片"这一路由动作，不属于本屏逻辑。
    Connections {
        target: backend

        function onAlbumReady(name, author, tags) {
            detail.albumName = name
            detail.albumAuthor = author
            detail.albumTags = "标签：" + tags.join("、")
            console.log("QML 收到详情：" + name + "（标签 " + tags.length + "）")
        }

    // 章节列表填充（第 35 轮下沉）。chapterModel 已随本组件持有，故不改名。
    // albumAid 只用于 autoReadAid 的比较 —— 它仍由外层传入。
    function onChaptersReady(names, ids) {
        chapterModel.clear()
        for (var i = 0; i < names.length; ++i)
            chapterModel.append({ "name": names[i], "cid": ids[i] })
        console.log("QML 章节列表已填充：" + chapterModel.count + " 条")
        if (typeof autoReadAid !== "undefined" && autoReadAid === detail.albumAid && ids.length > 0)
            backend.openChapterId(ids[0], 0)
    }
    }
}
