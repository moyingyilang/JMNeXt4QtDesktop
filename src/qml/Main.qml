import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// P0 最小窗口：只验证 Qt Quick 在容器里能起、能显示、能拿到 C++ 侧传进来的信息。
// 真正的五屏 1:1 实现在 P2；主题/效果在 P3。
ApplicationWindow {
    id: root
    width: 900; height: 600
    visible: true
    title: "JMNeXt4QtDesktop (QML) " + appVersion
    color: "#1e1f22"

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 12
        Text {
            text: "JMNeXt4QtDesktop — QML 前端（P0 骨架）"
            color: "#e6e6e6"; font.pixelSize: 22
        }
        Text {
            text: "版本：" + appVersion + "    渲染后端：" + renderBackend
            color: "#9aa0a8"; font.pixelSize: 14
        }
        Text {
            text: "当前风格：" + themeName + "    深浅：" + (darkTheme ? "深色" : "浅色")
            color: "#9aa0a8"; font.pixelSize: 14
        }
        Rectangle {
            Layout.preferredWidth: 220; Layout.preferredHeight: 40
            radius: 6; color: "#2b2d31"; border.color: "#3a3d43"; border.width: 1
            Text { anchors.centerIn: parent; text: "（后续在此放五屏）"; color: "#e6e6e6" }
        }
    }
}
