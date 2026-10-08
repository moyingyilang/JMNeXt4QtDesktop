import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * 设置（对应 Kotlin 版 ui/screens/settings/BlockSettingsScreen.kt 与"我的"里的显示项）。
 *
 * 本屏**用后端已有的能力**，不需要新增 C++ 接口：
 *   - 双页模式 -> JmBackend::setTwoPage(bool)
 *   - 屏蔽词   -> JmBackend::setBlockWords(QStringList)
 *
 * 命名注意（第 38 轮踩过）：自定义信号**不能**与属性的自动变化信号同名。
 * 因此这里有 `property bool twoPage` 时，信号必须叫 `twoPageToggled`（而非 `twoPageChanged`）。
 */
Rectangle {
    id: settings

    property color surfaceColor: "#26282c"
    property color strokeColor: "#3a3d43"
    property color titleColor: "#e6e6e6"
    property color bodyColor: "#b9bcc2"
    property color accentColor: "#5b8def"
    property bool twoPage: false
    property string noticeText: ""

    signal closeRequested()
    signal twoPageToggled(bool on)
    signal blockWordsChanged(var words)

    color: settings.surfaceColor
    border.color: settings.strokeColor
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        Text {
            text: "设置"
            color: settings.titleColor; font.pixelSize: 18; font.bold: true
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            CheckBox {
                text: "双页模式"
                checked: settings.twoPage
                onToggled: {
                    settings.twoPage = checked
                    settings.twoPageToggled(checked)
                    settings.noticeText = "双页模式：" + (checked ? "开" : "关")
                }
            }
            Text {
                text: "（阅读器一次显示两页）"
                color: settings.bodyColor; font.pixelSize: 12
                Layout.fillWidth: true
            }
        }

        Text {
            text: "屏蔽词（空格分隔；命中作品名/作者即隐藏）"
            color: settings.bodyColor; font.pixelSize: 12
            Layout.fillWidth: true
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            TextField {
                id: blockField
                Layout.fillWidth: true
                placeholderText: "例如：广告 试看"
            }
            Button {
                text: "应用"
                onClicked: {
                    var raw = blockField.text.trim()
                    var words = raw.length > 0 ? raw.split(/\s+/) : []
                    settings.blockWordsChanged(words)
                    settings.noticeText = "已应用 " + words.length + " 个屏蔽词"
                }
            }
        }

        Text {
            text: settings.noticeText
            color: settings.accentColor; font.pixelSize: 12; wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }

        Button {
            text: "关闭"
            onClicked: settings.closeRequested()
        }
    }
}
