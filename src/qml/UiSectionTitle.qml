import QtQuick
import "ui/Tokens.js" as Tokens

/* 区块标题：统一字号、颜色与上下间距（对应主项目 SectionTitle）。
   页面里不再各写 Text { font.pixelSize: ... }，否则四套材质下层级会漂。 */
Text {
    property color titleColor: "#e6e6e6"
    color: titleColor
    font.pixelSize: Tokens.fontSection
    font.bold: true
    topPadding: Tokens.space4
    bottomPadding: Tokens.space2
}
