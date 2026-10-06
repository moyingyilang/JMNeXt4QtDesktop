import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// P2 第一屏：首页列表（标题 + 封面 + 点击进入阅读）。
// 数据与图片都来自共用后端：backend（JmBackend）负责数据，image://jm/ 由 JmImageProvider 负责图片。
// 说明：这里的布局是"结构级"复刻的起点，P3 才会按主项目五套表面工艺细化观感。
ApplicationWindow {
    id: root
    width: 1180; height: 780
    visible: true
    title: "JMNeXt4QtDesktop (QML) " + appVersion
    color: darkTheme ? "#1e1f22" : "#f5f6f8"

    // 列表数据：标题、aid、封面 URL（URL 由后端在取封面时给出，见 coverUrlReady）
    ListModel { id: listModel }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 左栏：作品列表
        ColumnLayout {
            Layout.preferredWidth: 380
            Layout.fillHeight: true
            spacing: 6

            Text {
                text: "作品列表（" + listModel.count + " 条）"
                color: "#9aa0a8"; font.pixelSize: 12
                Layout.leftMargin: 10; Layout.topMargin: 8
            }

            ListView {
                id: listView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: listModel
                spacing: 2

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 74
                    color: mouseArea.containsMouse ? "#26282c" : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 8

                        // 封面：后端给出 URL 前显示占位（image 源为空时不请求）
                        Image {
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 64
                            fillMode: Image.PreserveAspectCrop
                            source: coverUrl.length > 0
                                    ? "image://jm/cover?" + encodeURIComponent(coverUrl)
                                    : ""
                            // 加载失败或尚未就绪时，用纯色底代替空白
                            Rectangle {
                                anchors.fill: parent
                                color: "#2b2d31"; border.color: "#3a3d43"; border.width: 1
                                visible: parent.status !== Image.Ready
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: title
                            color: "#e6e6e6"
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            console.log("QML 点击作品 aid=" + aid)
                            root.albumAid = aid
                            backend.loadAlbum(aid)
                        }
                    }
                }
            }
        }

        // 右栏：阅读器（有页面后覆盖详情视图）
        Rectangle {
            visible: root.reading
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: darkTheme ? "#101113" : "#f0f1f3"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Image {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    fillMode: Image.PreserveAspectFit
                    source: root.pageUrl
                    onStatusChanged: if (status === Image.Ready)
                        console.log("QML 阅读页已显示：源 " + sourceSize.width + "x" + sourceSize.height
                                    + "，实际绘制 " + Math.round(paintedWidth) + "x" + Math.round(paintedHeight)
                                    + "，阅读器可见=" + root.reading)
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Button { text: "上一页"; onClicked: backend.step(-1) }
                    Button { text: "下一页"; onClicked: backend.step(1) }
                    Text {
                        Layout.fillWidth: true
                        text: root.pageStatus
                        color: "#b9bcc2"; font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }
            }
        }

        // 右栏：详情视图（封面图待扩展图片提供器后补上；本轮先文字 + 章节列表）
        Rectangle {
            visible: !root.reading
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: darkTheme ? "#141517" : "#ffffff"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 8

                Image {
                    id: detailCover
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 160
                    fillMode: Image.PreserveAspectCrop
                    source: root.albumAid.length > 0 ? "image://jm/albumcover?" + root.albumAid : ""
                    onStatusChanged: if (status === Image.Ready)
                        console.log("QML 详情封面已加载：" + sourceSize.width + "x" + sourceSize.height)
                    Rectangle {
                        anchors.fill: parent; color: "#2b2d31"
                        border.color: "#3a3d43"; border.width: 1
                        visible: parent.status !== Image.Ready
                    }
                }
                Text {
                    text: root.albumName.length > 0 ? root.albumName : "（从左侧选一个作品）"
                    color: "#e6e6e6"; font.pixelSize: 18; wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Text {
                    text: root.albumAuthor
                    color: "#b9bcc2"; font.pixelSize: 13
                }
                Text {
                    text: root.albumTags
                    color: "#9aa0a8"; font.pixelSize: 12; wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Text {
                    text: "章节（" + chapterModel.count + "）"
                    color: "#9aa0a8"; font.pixelSize: 12
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
                        color: chapterMouse.containsMouse ? "#26282c" : "transparent"
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left; anchors.leftMargin: 6
                            text: (index + 1) + ". " + name
                            color: "#e6e6e6"; font.pixelSize: 13
                        }
                        MouseArea {
                            id: chapterMouse
                            anchors.fill: parent; hoverEnabled: true
                            onClicked: {
                                console.log("QML 点击章节 index=" + index + " id=" + cid)
                                backend.openChapterId(cid, 0)
                            }
                        }
                    }
                }
                Text {
                    text: pageStatus
                    color: "#8a8f98"; font.pixelSize: 12
                    Layout.fillWidth: true; elide: Text.ElideRight
                }
            }
        }
    }

    // 状态行（后端 status 信号）
    property string pageStatus: ""
    // 阅读器状态：**不在绑定里直接读 context property**（编译后的 QML 绑定看不到它们，会当 null），
    // 改由 Connections 的 onPageChanged 计算后赋给根属性，绑定只读根属性。
    property bool reading: false
    property string pageUrl: ""
    property string albumName: ""
    property string albumAuthor: ""
    property string albumTags: ""
    property string albumAid: ""
    ListModel { id: chapterModel }

    Connections {
        target: backend

        function onListReady(titles, ids) {
            listModel.clear()
            for (var i = 0; i < titles.length; ++i)
                listModel.append({ "title": titles[i], "aid": ids[i], "coverUrl": "" })
            console.log("QML 列表已填充：" + listModel.count + " 条")
            backend.loadCovers(20)          // 让前 20 条的封面 URL 与图片就位
        }

        function onCoverUrlReady(index, url) {
            if (index >= 0 && index < listModel.count) {
                listModel.setProperty(index, "coverUrl", url)
                if (index === 0) console.log("QML 收到封面 URL：index 0")
            }
        }

        function onAlbumReady(name, author, tags) {
            root.albumName = name; root.albumAuthor = author
            root.albumTags = "标签：" + tags.join("、")
            console.log("QML 收到详情：" + name + "（标签 " + tags.length + "）")
        }

        function onChaptersReady(names, ids) {
            chapterModel.clear()
            for (var i = 0; i < names.length; ++i)
                chapterModel.append({ "name": names[i], "cid": ids[i] })
            console.log("QML 章节列表已填充：" + chapterModel.count + " 条")
            if (typeof autoReadAid !== "undefined" && autoReadAid === root.albumAid && ids.length > 0)
                backend.openChapterId(ids[0], 0)
        }

        function onCurrentAidChanged() { root.albumAid = backend.currentAid }

        function onStatus(text) { root.pageStatus = text }
        function onFailed(text) { root.pageStatus = "失败：" + text }
        function onPageReady(image, statusText) { root.pageStatus = statusText }

        function onPageChanged() {
            root.reading = backend.pageSeq > 0
            root.pageUrl = root.reading
                    ? "file://" + backend.pagePath + "?v=" + backend.pageSeq
                    : ""
        }
    }

    Component.onCompleted: {
        console.log("QML 首页列表：开始加载")
        backend.loadList()
    }
}
