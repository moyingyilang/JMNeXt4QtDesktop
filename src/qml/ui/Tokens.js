.pragma library
// UI 令牌（间距 / 圆角 / 字号）——对应主项目 docs/DESIGN-SPEC.md 的 4 的倍数栅格。
//
// 为什么用 JS 库而不是 QML 单例：同目录 singleton 需要显式 import 模块才能解析，
// 之前踩过（取到 undefined）；pragma library 的 JS 文件可以直接 import 目录使用，
// 也不会被实例化，适合纯常量。

var space2 = 2;
var space4 = 4;
var space8 = 8;
var space12 = 12;
var space16 = 16;
var space24 = 24;
var space32 = 32;

var radiusXs = 2;
var radiusSm = 4;
var radiusMd = 6;
var radiusLg = 8;
var radiusXl = 12;

// 四档文字层级（与主项目一致）：页标题 / 区块标题 / 正文 / 说明
var fontTitle = 18;
var fontSection = 14;
var fontBody = 13;
var fontCaption = 11;

// 行高：可点行不低于 44（桌面鼠标）与 48（触控）之间取桌面值；
// 主项目 Android 侧是 48，桌面用 44 更紧凑，但触控区仍按 44 保证可点。
var rowHeight = 44;
var rowHeightTight = 36;
