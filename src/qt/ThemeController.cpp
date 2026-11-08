#include "ThemeController.h"

#include "Theme.h"
#include "ThemeTokens.h"

#include <QQmlContext>
#include <QQmlEngine>

namespace jmnext::qt {

QVariantMap themeTokens(const QString& styleIn, bool dark) {
    const QString styleKey = QString::fromUtf8(styleName(styleFromName(styleIn)));
    const auto pal = (styleKey == QLatin1String("translucent")) ? tokens::translucent(dark)
                   : (styleKey == QLatin1String("flatBlur"))    ? tokens::flatBlur(dark)
                   : (styleKey == QLatin1String("miuix"))       ? tokens::miuix(dark)
                   : (styleKey == QLatin1String("material"))    ? tokens::material(dark)
                                                                : tokens::windowGlass(dark);
    const auto rad = (styleKey == QLatin1String("flatBlur")) ? tokens::flatBlurRadius()
                   : (styleKey == QLatin1String("miuix"))    ? tokens::miuixRadius()
                   : (styleKey == QLatin1String("material")) ? tokens::materialRadius()
                                                             : tokens::windowGlassRadius();
    QVariantMap m;
    const auto put = [&m](const char* k, const char* v) {
        m.insert(QString::fromLatin1(k), QString::fromUtf8(v ? v : ""));
    };
    put("accent", pal.accent);
    put("accentHover", pal.accentHover);
    put("accentActive", pal.accentActive);
    put("accentFg", pal.accentFg);
    put("accentSoft", pal.accentSoft);
    put("surfaceMica", pal.surfaceMica);
    put("surface1", pal.surface1);
    put("surface2", pal.surface2);
    put("surface3", pal.surface3);
    put("surfaceSunken", pal.surfaceSunken);
    put("surfaceHover", pal.surfaceHover);
    put("surfaceActive", pal.surfaceActive);
    put("stroke", pal.stroke);
    put("strokeStrong", pal.strokeStrong);
    put("strokeInner", pal.strokeInner);
    put("text", pal.text);
    put("textSecondary", pal.textSecondary);
    put("textTertiary", pal.textTertiary);
    put("textOnAccent", pal.textOnAccent);
    put("error", pal.error);
    m.insert(QStringLiteral("radiusXs"), rad.xs);
    m.insert(QStringLiteral("radiusSm"), rad.sm);
    m.insert(QStringLiteral("radiusMd"), rad.md);
    m.insert(QStringLiteral("radiusLg"), rad.lg);
    put("styleName", styleKey.toUtf8().constData());
    return m;
}

ThemeController::ThemeController(QQmlEngine* engine, QObject* parent)
    : QObject(parent), m_engine(engine) {
    // 必须在这里先推一次：apply() 在"风格与默认值相同"时提前返回，
    // 否则默认的 windowGlass + 深色启动时 themeInit 从未被写入，
    // QML 侧 applyThemeTokens() 读到 undefined，界面一直用 QML 里写死的兜底色。
    push();
}

void ThemeController::apply(const QString& style, bool dark) {
    const QString key = QString::fromUtf8(styleName(styleFromName(style)));
    if (key == m_style && dark == m_dark) return;
    m_style = key;
    m_dark = dark;
    push();
    emit applied();
}

void ThemeController::push() {
    if (!m_engine) return;
    auto* ctx = m_engine->rootContext();
    ctx->setContextProperty(QStringLiteral("themeName"), m_style);
    ctx->setContextProperty(QStringLiteral("darkTheme"), m_dark);
    ctx->setContextProperty(QStringLiteral("themeInit"), themeTokens(m_style, m_dark));
}

void ThemeController::savePreference() const {
    saveStyle(styleFromName(m_style));
    saveDark(m_dark);
}

} // namespace jmnext::qt
