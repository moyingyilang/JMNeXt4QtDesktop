import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "ui/Tokens.js" as Tokens

/*
 * 我的收藏（本地 FavoriteStore）—— 对齐 Kotlin 桌面版列表页的形态：
 * 头部（标题 + 计数 + 清空）+ 封面网格。
 *
 * 数据来自 backend.favorites()（本地 JSON），未登录也有内容；
 * 点卡片发 comicClicked(aid)，右上角「清空」调用 backend.clearFavorites()。
 */
Rectangle {
    id: favoritesPane

    property color surfaceColor: "#fef7ff"
    property color strokeColor: "#cac4d0"
    property color titleColor: "#1d1b20"
    property color bodyColor: "#49454f"
    property color accentColor: "#6750a4"

    signal closeRequested()
    signal comicClicked(string aid)

    color: Qt.rgba(surfaceColor.r, surfaceColor.g, surfaceColor.b, 1.0)
    radius: 12

    ListModel { id: favoritesModel }

    function reload() {
        favoritesModel.clear()
        const rows = backend.favorites()
        for (let i = 0; i < rows.length; ++i) {
            favoritesModel.append({
                aid: rows[i].aid,
                title: rows[i].title && rows[i].title.length > 0 ? rows[i].title : ("作品 " + rows[i].aid),
                folder: rows[i].folder
            })
        }
    }

    onVisibleChanged: if (visible) reload()

    Connections {
        target: backend
        function onFavoritesChanged() { if (favoritesPane.visible) favoritesPane.reload() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        PageHeader {
            title: "我的收藏"
            countText: "共 " + favoritesModel.count + " 条"
            description: favoritesModel.count > 0
                         ? "点击卡片重新打开作品；「移除」按钮逐条取消收藏。"
                         : "还没有收藏。打开作品后点收藏，会出现在这里（本地保存，未登录也可用）。"
            titleColor: favoritesPane.titleColor
            bodyColor: favoritesPane.bodyColor
            accentColor: favoritesPane.accentColor
            actions: [
                { key: "clear", label: "清空", primary: false },
                { key: "close", label: "关闭", primary: false }
            ]
            onActionClicked: (key) => {
                if (key === "clear") backend.clearFavorites()
                else if (key === "close") favoritesPane.closeRequested()
            }
        }

        GridView {
            id: favGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: favoritesModel
            cellWidth: 176
            cellHeight: 268

            delegate: Item {
                width: favGrid.cellWidth - 16
                height: favGrid.cellHeight

                ComicGridCard {
                    width: 160
                    title: model.title
                    aid: model.aid
                    titleColor: favoritesPane.titleColor
                    authorColor: favoritesPane.bodyColor
                    onClicked: favoritesPane.comicClicked(model.aid)
                }

                Button {
                    text: "移除"
                    width: 56
                    height: 28
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.top: parent.top
                    anchors.topMargin: 8
                    onClicked: backend.removeFavorite(model.aid)
                }
            }
        }
    }
}
