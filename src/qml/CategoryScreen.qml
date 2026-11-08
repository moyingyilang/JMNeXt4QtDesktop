import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "ui/Tokens.js" as Tokens

/*
 * 分类（热门标签）—— 对齐 Kotlin 桌面版的列表页形态：
 * 头部（标题 + 计数 + 刷新/关闭）+ 标签胶囊云 + 该标签下的作品网格。
 *
 * 数据：backend.loadHotTags() -> hotTagsReady(tags)
 *       backend.categoryFilter(tag, page) -> categoryReady(titles, ids)
 */
Rectangle {
    id: category

    property color titleColor: "#1d1b20"
    property color bodyColor: "#49454f"
    property color surfaceColor: "#fef7ff"
    property color strokeColor: "#cac4d0"
    property color accentColor: "#6750a4"

    property string activeTag: ""
    property string pageStatus: ""

    signal closeRequested()
    signal comicClicked(string aid)

    color: Qt.rgba(surfaceColor.r, surfaceColor.g, surfaceColor.b, 1.0)
    radius: 12

    ListModel { id: tagModel }
    ListModel { id: resultModel }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        PageHeader {
            title: "分类"
            countText: category.activeTag.length > 0
                       ? ("标签：" + category.activeTag + "，作品 " + resultModel.count + " 条")
                       : (tagModel.count + " 个热门标签")
            description: "点标签筛选作品；热门标签来自接口，每次进入刷新。"
            titleColor: category.titleColor
            bodyColor: category.bodyColor
            accentColor: category.accentColor
            actions: [
                { key: "refresh", label: "刷新", primary: true },
                { key: "close", label: "关闭", primary: false }
            ]
            onActionClicked: (key) => {
                if (key === "refresh") backend.loadHotTags()
                else if (key === "close") category.closeRequested()
            }
        }

        // 标签云（最多两行高度，可滚动）
        Flickable {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(tagFlow.height + 8, 120)
            clip: true
            contentWidth: width
            contentHeight: tagFlow.height

            Flow {
                id: tagFlow
                width: parent.width
                spacing: 8

                Repeater {
                    model: tagModel

                    delegate: UiChip {
                        text: model.name
                        selected: category.activeTag === model.name
                        textColor: category.titleColor
                        accentColor: category.accentColor
                        borderColor: category.strokeColor
                        onClicked: {
                            category.activeTag = model.name
                            category.pageStatus = "正在筛选：" + model.name
                            backend.categoryFilter(model.name, 1)
                        }
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: category.pageStatus.length > 0
            text: category.pageStatus
            color: category.bodyColor
            font.pixelSize: 12
        }

        GridView {
            id: resultGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: resultModel
            cellWidth: 176
            cellHeight: 268

            delegate: ComicGridCard {
                width: 160
                title: model.title
                aid: model.aid
                titleColor: category.titleColor
                authorColor: category.bodyColor
                onClicked: category.comicClicked(model.aid)
            }
        }
    }

    Connections {
        target: backend
        function onFailed(text) {
            // 分类页自己接失败：否则用户只看到「0 个热门标签」，不知道是解密失败
            if (text.indexOf("热门标签") >= 0) category.pageStatus = "失败：" + text
        }
        function onHotTagsReady(tags) {
            tagModel.clear()
            for (var i = 0; i < tags.length; ++i)
                tagModel.append({ "name": tags[i] })
            category.pageStatus = "热门标签 " + tagModel.count + " 个"
        }
        function onCategoryReady(titles, ids) {
            resultModel.clear()
            for (var j = 0; j < titles.length; ++j)
                resultModel.append({ "title": titles[j], "aid": ids[j] })
            category.pageStatus = "筛选结果 " + resultModel.count + " 条"
        }
    }

    onVisibleChanged: if (visible) backend.loadHotTags()

}
