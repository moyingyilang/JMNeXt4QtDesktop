import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 通用作品列表屏。
 *
 * 为什么这样做：Kotlin 侧十几个屏（收藏/历史/画师作品/标签/周更/随机推荐/追更…）的**主体都是
 * "一个作品列表"**，差异只在接口与参数。与其每屏写一份四层切片，不如：
 *   JmBackend.loadPaged(tag, path, query) -> pagedReady(tag, titles, ids)  ->  本组件渲染。
 * 这样新增一屏 = 在 MoreScreen 表里加一行（path/query），不再动 C++。
 *
 * 数据形状：JmClient::paged 先按 {list:[...]} 解析，失败再按顶层数组解析，
 * 覆盖 Kotlin 的 PagedList 与 List<ListItem> 两种形态。
 */
ColumnLayout {
    id: screen

    property string title: ""
    property string apiPath: ""
    property string apiQuery: ""
    property string tagId: ""
    property bool includeCovers: true
    /// 分页：page 为当前已加载页（1 起）；query 里的 page= 会被本组件接管
    property int page: 1
    property bool hasMore: true

    property color titleColor: "#e6e6e6"
    property color bodyColor: "#b9bcc2"
    property color surfaceColor: "#26282c"
    property color accentColor: "#5b8def"
    property string pageStatus: ""

    signal closeRequested()
    signal comicClicked(string aid)

    ListModel { id: comicModel }

    RowLayout {
        Layout.fillWidth: true
        Text {
            text: screen.title
            color: screen.titleColor; font.pixelSize: 18; font.bold: true
            Layout.fillWidth: true
        }
        Button {
            text: "刷新"
            onClicked: backend.loadPaged(screen.tagId, screen.apiPath, screen.apiQuery)
        }
    }
    Text {
        text: screen.pageStatus
        color: screen.bodyColor; font.pixelSize: 12
        Layout.fillWidth: true; elide: Text.ElideRight
    }

    ListView {
        id: listView
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: comicModel
        spacing: 2
        visible: comicModel.count > 0
        delegate: ComicCard {
            width: ListView.view.width
            title: model.title
            aid: model.aid
            coverUrl: model.coverUrl
            hoverColor: screen.surfaceColor
            textColor: screen.titleColor
            cardRadius: 6
            onClicked: {
                console.log("QML 列表点击 aid=" + aid)
                screen.comicClicked(aid)
            }
        }
    }

    StateBox {
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: comicModel.count === 0
        kind: screen.pageStatus.indexOf("失败") >= 0 ? "error" : "message"
        title: screen.pageStatus.length > 0 ? screen.pageStatus : "正在载入…"
        description: "点“刷新”重新载入"
        canRetry: true
        textColor: screen.titleColor
        secondaryColor: screen.bodyColor
        onRetry: backend.loadPaged(screen.tagId, screen.apiPath, screen.apiQuery)
    }

    LoadMoreFooter {
        Layout.fillWidth: true
        visible: comicModel.count > 0
        exhausted: !screen.hasMore
        textColor: screen.titleColor
        secondaryColor: screen.bodyColor
        onLoadMore: {
            screen.page += 1
            screen.request(screen.page)
        }
    }

    Button {
        text: "关闭"
        onClicked: screen.closeRequested()
    }

    Connections {
        target: backend
        function onPagedReady(tag, titles, ids) {
            if (tag !== screen.tagId) return
            // 第 1 页替换、后续页追加（与首页的 listReady/listAppended 语义一致）
            if (screen.page <= 1) comicModel.clear()
            for (var i = 0; i < titles.length; ++i)
                comicModel.append({ "title": titles[i], "aid": ids[i], "coverUrl": "" })
            screen.hasMore = titles.length > 0
            console.log("QML 通用列表已填充：" + screen.title + " -> " + comicModel.count + " 条")
            if (screen.includeCovers && comicModel.count > 0) backend.loadCovers(20)
        }
    }

    /// 把 query 里的 page= 去掉，由本组件按 page 拼 —— 否则"加载更多"会一直请求第 1 页
    function baseQuery() {
        var q = screen.apiQuery.replace(/(^|&)page=\d+/, "").replace(/^&/, "")
        if (q.length > 0 && q.charAt(q.length - 1) === "&") q = q.slice(0, -1)
        return q
    }

    function request(pageNo) {
        if (screen.apiPath.length === 0) return
        screen.pageStatus = "正在载入第 " + pageNo + " 页：" + screen.title
        var q = screen.baseQuery()
        q = q.length > 0 ? q + "&page=" + pageNo : "page=" + pageNo
        backend.loadPaged(screen.tagId, screen.apiPath, q)
    }

    function reload() {
        screen.page = 1
        screen.hasMore = true
        comicModel.clear()
        request(1)
    }

    // 实例只创建一次，因此换接口靠属性变化触发重载（否则点第二个入口不会刷新）
    onApiPathChanged: reload()
    Component.onCompleted: reload()
}
