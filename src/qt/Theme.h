// 界面主题（QSS）—— 第 (a) 步：先把"Qt 系统默认观感"换掉。
//
// 设计取舍（如实）：
//  - 目前只做**一套深色 + 一套浅色**，变量集中在下面的常量里，方便以后对齐主项目的四套主题；
//  - 不做自绘控件、不引第三方样式库：先用 QSS 把背景/文字/按钮/列表/滚动条统一；
//  - 观感好坏最终由用户看截图决定，这里只保证"不是系统默认的灰按钮"。
#pragma once
#include <QString>
#include "qt/ThemeTokens.h"

namespace jmnext::qt {

/// 深色主题（默认）
inline QString darkThemeQss() {
    return QStringLiteral(R"QSS(
QWidget        { background: #1e1f22; color: #e6e6e6; font-size: 13px; }
QMainWindow    { background: #1e1f22; }
QLabel         { background: transparent; }
QLabel#cover   { border: 1px solid #3a3d43; border-radius: 4px; }
QLabel#viewer  { border: 1px solid #3a3d43; border-radius: 4px; background: #141517; }
QPushButton    { background: #2b2d31; color: #e6e6e6; border: 1px solid #3a3d43;
                 border-radius: 6px; padding: 5px 10px; }
QPushButton:hover   { background: #35373c; }
QPushButton:pressed { background: #26282c; }
QLineEdit      { background: #141517; border: 1px solid #3a3d43; border-radius: 6px;
                 padding: 4px 8px; selection-background-color: #4c8bf5; }
QLineEdit:focus { border: 1px solid #4c8bf5; }
QListWidget    { background: #141517; border: 1px solid #3a3d43; border-radius: 6px;
                 outline: none; padding: 4px; }
QListWidget::item          { padding: 6px 4px; border-radius: 4px; }
QListWidget::item:selected  { background: #2f5fa8; color: #ffffff; }
QListWidget::item:hover     { background: #26282c; }
QPlainTextEdit { background: #141517; border: 1px solid #3a3d43; border-radius: 6px;
                 color: #b9bcc2; font-family: monospace; }
QScrollArea    { border: none; }
QScrollBar:vertical   { background: #1e1f22; width: 10px; margin: 0; }
QScrollBar::handle:vertical   { background: #3a3d43; border-radius: 5px; min-height: 24px; }
QScrollBar::handle:vertical:hover { background: #4a4d55; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; }
QComboBox      { background: #2b2d31; border: 1px solid #3a3d43; border-radius: 6px; padding: 4px 8px; }
QLabel#pageInfo { color: #9aa0a8; padding: 2px 0; font-size: 12px; }
QLabel#albumInfo { color: #d6d9de; font-size: 13px; padding: 2px 0; }
)QSS");
}

/// 浅色主题（--light 时使用）
inline QString lightThemeQss() {
    return QStringLiteral(R"QSS(
QWidget        { background: #f5f6f8; color: #1d1f23; font-size: 13px; }
QMainWindow    { background: #f5f6f8; }
QLabel         { background: transparent; }
QLabel#cover   { border: 1px solid #c9ccd2; border-radius: 4px; }
QLabel#viewer  { border: 1px solid #c9ccd2; border-radius: 4px; background: #ffffff; }
QPushButton    { background: #ffffff; color: #1d1f23; border: 1px solid #c9ccd2;
                 border-radius: 6px; padding: 5px 10px; }
QPushButton:hover   { background: #eceef1; }
QPushButton:pressed { background: #e0e3e8; }
QLineEdit      { background: #ffffff; border: 1px solid #c9ccd2; border-radius: 6px; padding: 4px 8px; }
QLineEdit:focus { border: 1px solid #2f6fd0; }
QListWidget    { background: #ffffff; border: 1px solid #c9ccd2; border-radius: 6px;
                 outline: none; padding: 4px; }
QListWidget::item          { padding: 6px 4px; border-radius: 4px; }
QListWidget::item:selected  { background: #cfe0fb; color: #14305c; }
QListWidget::item:hover     { background: #eceef1; }
QPlainTextEdit { background: #ffffff; border: 1px solid #c9ccd2; border-radius: 6px;
                 color: #4a4d55; font-family: monospace; }
QScrollArea    { border: none; }
QScrollBar:vertical   { background: #f5f6f8; width: 10px; }
QScrollBar::handle:vertical   { background: #c9ccd2; border-radius: 5px; min-height: 24px; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; }
QComboBox      { background: #ffffff; border: 1px solid #c9ccd2; border-radius: 6px; padding: 4px 8px; }
QLabel#pageInfo { color: #5a5f68; padding: 2px 0; font-size: 12px; }
QLabel#albumInfo { color: #2a2d33; font-size: 13px; padding: 2px 0; }
)QSS");
}


// ---------------------------------------------------------------------------
// 具名风格（对应主项目的 ThemeStyle）：配色与圆角来自**生成**的 ThemeTokens.h，
// 数值原样搬运、不自创。这里用"基础 QSS + 风格覆盖块"的加法结构：
// 覆盖块写在最后，QSS 里后出现的规则优先，因此不会丢掉已有的 #cover/#viewer/#pageInfo/#albumInfo 等规则。
//
// 做不到的部分（如实）：模糊（FlatBlur/Translucent 的高斯模糊与背景透明）、颗粒（Acrylic noise）、
// 行高、壁纸压暗/背光 —— QSS 没有这些能力，需 QML 或自绘合成。
// Material 未接入：主项目里它直接用 Material You 3 的颜色角色，本表没有它的配色。
// ---------------------------------------------------------------------------
enum class Style { WindowGlass, Translucent, FlatBlur, Miuix };

inline const char* styleName(Style s) {
    switch (s) {
        case Style::Translucent: return "translucent";
        case Style::FlatBlur:    return "flatBlur";
        case Style::Miuix:       return "miuix";
        case Style::WindowGlass: break;
    }
    return "windowGlass";
}

/// 认不出来就用 WindowGlass —— 与主项目一致（默认不该换掉老用户的界面）。
inline Style styleFromName(const QString& name) {
    const QString n = name.trimmed().toLower();
    if (n == QStringLiteral("translucent")) return Style::Translucent;
    if (n == QStringLiteral("flatblur"))    return Style::FlatBlur;
    if (n == QStringLiteral("miuix"))       return Style::Miuix;
    return Style::WindowGlass;
}

/// 风格覆盖块：把该风格的表面/描边/文字/强调色与圆角套到基础 QSS 之上。
inline QString styleOverrideQss(Style style, bool dark) {
    const tokens::Palette p = (style == Style::Translucent) ? tokens::translucent(dark)
                            : (style == Style::FlatBlur)    ? tokens::flatBlur(dark)
                            : (style == Style::Miuix)       ? tokens::miuix(dark)
                                                            : tokens::windowGlass(dark);
    tokens::Radius r{2, 4, 6, 8, 12};
    switch (style) {
        case Style::FlatBlur: r = tokens::flatBlurRadius(); break;
        case Style::Miuix:    r = tokens::miuixRadius();    break;
        case Style::Translucent:
        case Style::WindowGlass:
            // translucent 在主项目里没有独立 spec 块（继承基座），故沿用 windowGlass 的圆角。
            r = tokens::windowGlassRadius(); break;
    }
    auto s = [](const char* v) { return QString::fromLatin1(v); };
    QString q;
    q += QStringLiteral("QWidget { background: %1; color: %2; }\n").arg(s(p.surface1), s(p.text));
    q += QStringLiteral("QMainWindow { background: %1; }\n").arg(s(p.surface1));
    q += QStringLiteral("QPushButton { background: %1; color: %2; border: 1px solid %3;"
                        " border-radius: %4px; padding: 5px 10px; }\n")
             .arg(s(p.surface2), s(p.text), s(p.stroke)).arg(r.md);
    q += QStringLiteral("QPushButton:hover { background: %1; }\n").arg(s(p.surfaceHover));
    q += QStringLiteral("QPushButton:pressed { background: %1; color: %2; }\n")
             .arg(s(p.surfaceActive), s(p.textSecondary));
    q += QStringLiteral("QLineEdit { background: %1; color: %2; border: 1px solid %3;"
                        " border-radius: %4px; padding: 4px 8px; }\n")
             .arg(s(p.surfaceSunken), s(p.text), s(p.stroke)).arg(r.md);
    q += QStringLiteral("QLineEdit:focus { border: 1px solid %1; }\n").arg(s(p.accent));
    q += QStringLiteral("QListWidget { background: %1; color: %2; border: 1px solid %3;"
                        " border-radius: %4px; padding: 4px; }\n")
             .arg(s(p.surfaceSunken), s(p.text), s(p.stroke)).arg(r.md);
    q += QStringLiteral("QListWidget::item:selected { background: %1; color: %2; }\n")
             .arg(s(p.accentSoft), s(p.text));
    q += QStringLiteral("QListWidget::item:hover { background: %1; }\n").arg(s(p.surfaceHover));
    q += QStringLiteral("QLabel#cover { border: 1px solid %1; border-radius: %2px; }\n")
             .arg(s(p.stroke)).arg(r.sm);
    q += QStringLiteral("QLabel#viewer { border: 1px solid %1; border-radius: %2px; background: %3; }\n")
             .arg(s(p.stroke)).arg(r.sm).arg(s(p.surfaceSunken));
    q += QStringLiteral("QLabel#pageInfo { color: %1; }\n").arg(s(p.textTertiary));
    q += QStringLiteral("QLabel#albumInfo { color: %1; }\n").arg(s(p.textSecondary));
    q += QStringLiteral("QPlainTextEdit { background: %1; color: %2; border: 1px solid %3;"
                        " border-radius: %4px; }\n")
             .arg(s(p.surfaceSunken), s(p.textTertiary), s(p.stroke)).arg(r.md);
    q += QStringLiteral("QScrollBar::handle:vertical { background: %1; border-radius: 5px; }\n").arg(s(p.stroke));
    q += QStringLiteral("QComboBox { background: %1; color: %2; border: 1px solid %3;"
                        " border-radius: %4px; padding: 4px 8px; }\n")
             .arg(s(p.surface2), s(p.text), s(p.stroke)).arg(r.md);
    q += QStringLiteral("QSplitter::handle { background: %1; }\n").arg(s(p.stroke));
    return q;
}

/// 最终样式表：基础（深/浅）+ 所选风格的覆盖块。
inline QString themeQss(Style style, bool dark) {
    return (dark ? darkThemeQss() : lightThemeQss()) + styleOverrideQss(style, dark);
}
}  // namespace jmnext::qt
