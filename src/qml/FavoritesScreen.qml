import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 我的收藏（本地）。
 *
 * 数据来源：backend.favorites()（JmBackend 新增，正在由主线补上），返回 QVariantList，
 * 每项含 aid / title / folder。它与 HistoryScreen 的 history() 是同一路子：读**本地**存储，
 * 不走服务端 watch_list 之类需要登录的接口，所以未登录、断网时也能显示。
 *
 * 写入（收藏 / 取消收藏）仍由 DetailScreen 调 backend.action(...) 完成；
 * 写完 JmBackend 会发 favoritesChanged()，本屏监听后重新加载，保证列表与本地库一致。
 *
 * 「清空」调 backend.clearFavorites()（主线新增，与 history() / clearHistory() 同一形态），
 * 它会清本地表并发出 favoritesChanged()，随后这里重新加载；不自己维护一份缓存。
 *
 * 未实现（如实）：本屏只做列表展示与清空，不支持新建/重命名收藏夹、
 * 也不能在这里逐条取消收藏 —— 那些入口在 DetailScreen，尚未接到本屏。
 */
Rectangle {
    id: favoritesPane

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

    ListModel { id: favoritesModel }

    function reload() {
        favoritesModel.clear()
        const rows = backend.favorites()
        for (let i = 0; i < rows.length; ++i) {
            const row = rows[i]
            favoritesModel.append({
                aid: row.aid,
                // 标题为空时用 aid 兜底，避免列表里出现空白行（与 HistoryScreen 一致）
                title: row.title && row.title.length > 0 ? row.title : ("作品 " + row.aid),
                // 收藏夹名称为空说明这件作品直接收藏在根目录，界面上显示「默认」
                folder: row.folder && row.folder.length > 0 ? row.folder : "默认"
            })
        }
    }

    onVisibleChanged: if (visible) reload()

    Connections {
        target: backend
        // 收藏/取消收藏后主线会发这个信号；只在可见时刷新，避免隐藏页做无谓的读取
        function onFavoritesChanged() { if (favoritesPane.visible) favoritesPane.reload() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Text {
                text: "我的收藏（本地，共 " + favoritesModel.count + " 条）"
                color: favoritesPane.titleColor
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
            }
            Button {
                text: "清空"
                enabled: favoritesModel.count > 0
                onClicked: {
                    backend.clearFavorites()
                    favoritesPane.reload()
                }
            }
            Button {
                text: "关闭"
                onClicked: favoritesPane.closeRequested()
            }
        }

        Text {
            text: favoritesModel.count === 0
                  ? "还没有收藏。打开作品后点收藏，会出现在这里。"
                  : "点击条目可重新打开该作品。"
            color: favoritesPane.bodyColor
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: favoritesModel
            spacing: 2

            delegate: Rectangle {
                width: ListView.view.width
                height: 44
                radius: 6
                color: rowMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.06) : "transparent"

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: favoritesPane.comicClicked(model.aid)
                }

                ColumnLayout {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.right: folderText.left
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2

                    Text {
                        text: model.title
                        color: favoritesPane.titleColor
                        font.pixelSize: 14
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Text {
                        // 第二行放 aid，便于排查「标题对不上是哪个作品」
                        text: "aid " + model.aid
                        color: favoritesPane.bodyColor
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Text {
                    id: folderText
                    anchors.right: parent.right
                    anchors.rightMargin: 76
                    anchors.verticalCenter: parent.verticalCenter
                    text: model.folder
                    color: favoritesPane.bodyColor
                    font.pixelSize: 11
                }

                Button {
                    // 逐条移除：removeFavorite 此前没有调用方（文档核对时发现）
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: 56
                    height: 28
                    text: "移除"
                    enabled: !rowMouse.containsMouse || true
                    onClicked: backend.removeFavorite(model.aid)
                }
            }
        }
    }
}
