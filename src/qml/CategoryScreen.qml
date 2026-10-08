import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 分类（对应 Kotlin 版 ui/screens/category/CategoryScreen.kt）。
 *
 * 数据源依据 shared/data/JmRepository.kt 的说明：**公开分类导航用 hot_tags**
 * （纯字符串数组），而 `categories` 是登录用户的收藏夹分类，不适合做公开导航。
 *
 * 本屏自己收 hotTagsReady（数据链路已由 --hot-tags 自检验证：实测取回 10 个真实标签）。
 * 点标签后的筛选结果（`categories/filter`）**尚未接** —— 点击目前只发信号，
 * 由外层记录；接筛选时要遵守一条协议细节：`c` 为空时**必须省略整个参数**
 * （发 `c=` 会返回错误页而非 JSON，见 JmRepository.kt 注释）。
 */
Rectangle {
    id: category

    property color surfaceColor: "#26282c"
    property color strokeColor: "#3a3d43"
    property color titleColor: "#e6e6e6"
    property color bodyColor: "#b9bcc2"
    property color accentColor: "#5b8def"

    signal closeRequested()
    signal tagClicked(string tag)
    /// 点击筛选结果里的作品（外层负责打开详情）
    signal resultClicked(string aid)

    property string activeTag: ""
    property int resultCount: 0

    color: category.surfaceColor
    border.color: category.strokeColor
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "分类（热门标签）"
                color: category.titleColor; font.pixelSize: 18; font.bold: true
                Layout.fillWidth: true
            }
            Button {
                text: "刷新"
                onClicked: backend.loadHotTags()
            }
        }

        ListModel { id: tagModel }

        // 筛选结果列表（第 53 轮补上渲染；数据链路与处理器此前已验证）
        ListModel { id: resultModel }

        ListView {
            id: resultView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: resultModel
            spacing: 2
            visible: resultModel.count > 0

            delegate: ComicCard {
                width: ListView.view.width
                title: model.title
                aid: model.aid
                coverUrl: model.coverUrl
                hoverColor: "#2f3237"
                textColor: category.titleColor
                cardRadius: 6
                onClicked: {
                    console.log("QML 分类结果点击作品 aid=" + aid)
                    category.resultClicked(aid)
                }
            }
        }

        ListView {
            id: tagView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: tagModel
            spacing: 2
            delegate: Rectangle {
                width: ListView.view.width; height: 32
                color: tagMouse.containsMouse ? "#2f3237" : "transparent"
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left; anchors.leftMargin: 8
                    text: name
                    color: category.titleColor; font.pixelSize: 14
                }
                MouseArea {
                    id: tagMouse
                    anchors.fill: parent; hoverEnabled: true
                    onClicked: {
                        console.log("QML 点击分类标签：" + name)
                        category.activeTag = name
                        category.tagClicked(name)
                        // 真的取筛选结果（数据链路已由 --category-filter 自检验证）
                        backend.categoryFilter(name, 1)
                    }
                }
            }
        }

        Text {
            id: hint
            text: "共 " + tagModel.count + " 个标签"
                  + (category.activeTag.length > 0
                     ? "；分类「" + category.activeTag + "」：" + category.resultCount + " 条" : "")
            color: category.bodyColor; font.pixelSize: 12
            Layout.fillWidth: true
        }

        Button {
            text: "关闭"
            onClicked: category.closeRequested()
        }
    }

    Connections {
        target: backend

        // 分类筛选结果（第 52 轮接通；界面列表下一步再画，先保证数据到位且有日志证据）
        function onCategoryReady(titles, ids) {
            category.resultCount = titles.length
            resultModel.clear()
            for (var k = 0; k < titles.length; ++k)
                resultModel.append({ "title": titles[k], "aid": ids[k], "coverUrl": "" })
            backend.loadCovers(20)
            console.log("QML 分类筛选已收到：" + titles.length + " 条（标签：" + category.activeTag + "）"
                        + (titles.length > 0 ? "，首条：" + titles[0].split("\n")[0] : ""))
        }

        function onHotTagsReady(tags) {
            tagModel.clear()
            for (var i = 0; i < tags.length; ++i)
                tagModel.append({ "name": tags[i] })
            console.log("QML 分类标签已填充：" + tagModel.count + " 个")
        }
    }

    Component.onCompleted: backend.loadHotTags()
}
