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
                        category.tagClicked(name)
                    }
                }
            }
        }

        Text {
            id: hint
            text: "共 " + tagModel.count + " 个标签"
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

        function onHotTagsReady(tags) {
            tagModel.clear()
            for (var i = 0; i < tags.length; ++i)
                tagModel.append({ "name": tags[i] })
            console.log("QML 分类标签已填充：" + tagModel.count + " 个")
        }
    }

    Component.onCompleted: backend.loadHotTags()
}
