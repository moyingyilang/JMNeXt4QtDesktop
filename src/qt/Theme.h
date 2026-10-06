// 界面主题（QSS）—— 第 (a) 步：先把"Qt 系统默认观感"换掉。
//
// 设计取舍（如实）：
//  - 目前只做**一套深色 + 一套浅色**，变量集中在下面的常量里，方便以后对齐主项目的四套主题；
//  - 不做自绘控件、不引第三方样式库：先用 QSS 把背景/文字/按钮/列表/滚动条统一；
//  - 观感好坏最终由用户看截图决定，这里只保证"不是系统默认的灰按钮"。
#pragma once
#include <QString>

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
)QSS");
}

}  // namespace jmnext::qt
