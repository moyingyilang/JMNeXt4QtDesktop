#pragma once
#include <QObject>
#include <QString>
#include <QVariantMap>

class QQmlEngine;

namespace jmnext::qt {

/// 由「材质名 + 深浅色」生成 QML 侧用的 token 表。
/// 抽成独立函数，供启动时与运行时切换共用同一份逻辑（避免两处数值漂移）。
QVariantMap themeTokens(const QString& style, bool dark);

/**
 * 运行时切换主题。
 *
 * 为什么需要它：Qt 端此前只能用 --theme / --light 在启动时定风格，
 * 界面上没有入口，与主项目「外观里可切四套材质 + 深浅色」不对齐。
 * 这里把 themeName / darkTheme / themeInit 三个上下文属性重新写入，
 * 再发 applied()，QML 侧据此重读 token。
 */
class ThemeController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString style READ style NOTIFY applied)
    Q_PROPERTY(bool dark READ dark NOTIFY applied)

public:
    explicit ThemeController(QQmlEngine* engine, QObject* parent = nullptr);

    QString style() const { return m_style; }
    bool dark() const { return m_dark; }

    Q_INVOKABLE void apply(const QString& style, bool dark);
    /// 写回偏好文件（style.txt / dark.txt），下次启动沿用。
    Q_INVOKABLE void savePreference() const;

signals:
    void applied();

private:
    void push();

    QQmlEngine* m_engine = nullptr;
    QString m_style = QStringLiteral("windowGlass");
    bool m_dark = true;
};

} // namespace jmnext::qt
