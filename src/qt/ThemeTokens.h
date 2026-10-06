// 本文件由 tools/gen-theme-tokens.sh 生成，请勿手改。
// 数值来源：主项目 JMComic_Next 的 Tokens.kt / Palettes.kt（原样搬运，未做转换）。
// Material 风格不在此表内：它直接用 Material You 3 的颜色角色（主项目也是这么做的）。
#pragma once

namespace jmnext::qt::tokens {

struct Palette {
    const char* accent;
    const char* accentHover;
    const char* accentActive;
    const char* accentFg;
    const char* accentSoft;
    const char* accentGlow;
    const char* surfaceMica;
    const char* surfaceSunken;
    const char* surfaceHover;
    const char* surfaceActive;
    const char* stroke;
    const char* strokeStrong;
    const char* strokeInner;
    const char* text;
    const char* textSecondary;
    const char* textTertiary;
    const char* textOnAccent;
    const char* error;
    const char* errorFg;
    const char* tintWarm;
    const char* tintCool;
};

inline Palette windowGlass(bool dark) {
    static const Palette lightPal = [] {
        Palette p{};
        p.accent = "#FF0F6CBD";
        p.accentHover = "#FF115EA3";
        p.accentActive = "#FF0C3B5E";
        p.accentFg = "#FFFFFFFF";
        p.accentSoft = "#1A0F6CBD";
        p.accentGlow = "#470F6CBD";
        p.surfaceMica = "#B8F6F7FA";
        p.surfaceSunken = "#090F172A";
        p.surfaceHover = "#0D0F172A";
        p.surfaceActive = "#140F172A";
        p.stroke = "#170F172A";
        p.strokeStrong = "#290F172A";
        p.strokeInner = "#BFDFFFFF";
        p.text = "#FF16181D";
        p.textSecondary = "#FF4A4F5A";
        p.textTertiary = "#FF767C88";
        p.textOnAccent = "#FFFFFFFF";
        p.error = "#FFB3261E";
        p.errorFg = "#FFFFFFFF";
        p.tintWarm = "#33F78736";
        p.tintCool = "#33367DF7";
        return p;
    }();
    static const Palette darkPal = [] {
        Palette p{};
        p.accent = "#FF60CDFF";
        p.accentHover = "#FF7FD8FF";
        p.accentActive = "#FF9AE2FF";
        p.accentFg = "#FF04263A";
        p.accentSoft = "#2460CDFF";
        p.accentGlow = "#5760CDFF";
        p.surfaceMica = "#B816181E";
        p.surfaceSunken = "#3D000000";
        p.surfaceHover = "#14FFFFFF";
        p.surfaceActive = "#21FFFFFF";
        p.stroke = "#1AFFFFFF";
        p.strokeStrong = "#2EFFFFFF";
        p.strokeInner = "#1FFFFFFF";
        p.text = "#FFF3F4F7";
        p.textSecondary = "#B8FFFFFF";
        p.textTertiary = "#80FFFFFF";
        p.textOnAccent = "#FF04263A";
        p.error = "#FFFFB4AB";
        p.errorFg = "#FF690005";
        p.tintWarm = "#24F78736";
        p.tintCool = "#2E367DF7";
        return p;
    }();
    return dark ? darkPal : lightPal;
}

inline Palette translucent(bool dark) {
    static const Palette lightPal = [] {
        Palette p{};
        p.accent = "#FF0F6CBD";
        p.accentHover = "#FF115EA3";
        p.accentActive = "#FF0C3B5E";
        p.accentFg = "#FFFFFFFF";
        p.accentSoft = "#1A0F6CBD";
        p.accentGlow = "#470F6CBD";
        p.surfaceMica = "#8CF6F7FA";
        p.surfaceSunken = "#0B0F172A";
        p.surfaceHover = "#100F172A";
        p.surfaceActive = "#1A0F172A";
        p.stroke = "#2E0F172A";
        p.strokeStrong = "#420F172A";
        p.strokeInner = "#D9FFFFFF";
        p.text = "#FF16181D";
        p.textSecondary = "#FF4A4F5A";
        p.textTertiary = "#FF767C88";
        p.textOnAccent = "#FFFFFFFF";
        p.error = "#FFB3261E";
        p.errorFg = "#FFFFFFFF";
        p.tintWarm = "#40F78736";
        p.tintCool = "#40367DF7";
        return p;
    }();
    static const Palette darkPal = [] {
        Palette p{};
        p.accent = "#FF60CDFF";
        p.accentHover = "#FF7FD8FF";
        p.accentActive = "#FF9AE2FF";
        p.accentFg = "#FF04263A";
        p.accentSoft = "#2460CDFF";
        p.accentGlow = "#5760CDFF";
        p.surfaceMica = "#99101218";
        p.surfaceSunken = "#4D000000";
        p.surfaceHover = "#1AFFFFFF";
        p.surfaceActive = "#29FFFFFF";
        p.stroke = "#24FFFFFF";
        p.strokeStrong = "#3DFFFFFF";
        p.strokeInner = "#2EFFFFFF";
        p.text = "#FFF3F4F7";
        p.textSecondary = "#B8FFFFFF";
        p.textTertiary = "#80FFFFFF";
        p.textOnAccent = "#FF04263A";
        p.error = "#FFFFB4AB";
        p.errorFg = "#FF690005";
        p.tintWarm = "#2EF78736";
        p.tintCool = "#3D367DF7";
        return p;
    }();
    return dark ? darkPal : lightPal;
}

inline Palette flatBlur(bool dark) {
    static const Palette lightPal = [] {
        Palette p{};
        p.accent = "#FF0F6CBD";
        p.accentHover = "#FF115EA3";
        p.accentActive = "#FF0C3B5E";
        p.accentFg = "#FFFFFFFF";
        p.accentSoft = "#1A0F6CBD";
        p.accentGlow = "#470F6CBD";
        p.surfaceMica = "#CCF7F8FA";
        p.surfaceSunken = "#090F172A";
        p.surfaceHover = "#0D0F172A";
        p.surfaceActive = "#140F172A";
        p.stroke = "#00000000";
        p.strokeStrong = "#140F172A";
        p.strokeInner = "#00000000";
        p.text = "#FF16181D";
        p.textSecondary = "#FF4A4F5A";
        p.textTertiary = "#FF767C88";
        p.textOnAccent = "#FFFFFFFF";
        p.error = "#FFB3261E";
        p.errorFg = "#FFFFFFFF";
        p.tintWarm = "#00000000";
        p.tintCool = "#00000000";
        return p;
    }();
    static const Palette darkPal = [] {
        Palette p{};
        p.accent = "#FF60CDFF";
        p.accentHover = "#FF7FD8FF";
        p.accentActive = "#FF9AE2FF";
        p.accentFg = "#FF04263A";
        p.accentSoft = "#2460CDFF";
        p.accentGlow = "#5760CDFF";
        p.surfaceMica = "#CC14161C";
        p.surfaceSunken = "#4D000000";
        p.surfaceHover = "#14FFFFFF";
        p.surfaceActive = "#21FFFFFF";
        p.stroke = "#00000000";
        p.strokeStrong = "#1FFFFFFF";
        p.strokeInner = "#00000000";
        p.text = "#FFF3F4F7";
        p.textSecondary = "#B8FFFFFF";
        p.textTertiary = "#80FFFFFF";
        p.textOnAccent = "#FF04263A";
        p.error = "#FFFFB4AB";
        p.errorFg = "#FF690005";
        p.tintWarm = "#00000000";
        p.tintCool = "#00000000";
        return p;
    }();
    return dark ? darkPal : lightPal;
}

inline Palette miuix(bool dark) {
    static const Palette lightPal = [] {
        Palette p{};
        p.accent = "#FF3482FF";
        p.accentHover = "#FF2168E0";
        p.accentActive = "#FF0F4FBD";
        p.accentFg = "#FFFFFFFF";
        p.accentSoft = "#1F3482FF";
        p.accentGlow = "#4D3482FF";
        p.surfaceMica = "#FFF2F3F5";
        p.surfaceSunken = "#0F000000";
        p.surfaceHover = "#0A000000";
        p.surfaceActive = "#14000000";
        p.stroke = "#00000000";
        p.strokeStrong = "#14000000";
        p.strokeInner = "#00000000";
        p.text = "#FF0D0D0D";
        p.textSecondary = "#FF666666";
        p.textTertiary = "#FF999999";
        p.textOnAccent = "#FFFFFFFF";
        p.error = "#FFB3261E";
        p.errorFg = "#FFFFFFFF";
        p.tintWarm = "#00000000";
        p.tintCool = "#00000000";
        return p;
    }();
    static const Palette darkPal = [] {
        Palette p{};
        p.accent = "#FF4C93FF";
        p.accentHover = "#FF6BA6FF";
        p.accentActive = "#FF8AB9FF";
        p.accentFg = "#FF06203F";
        p.accentSoft = "#294C93FF";
        p.accentGlow = "#5C4C93FF";
        p.surfaceMica = "#FF000000";
        p.surfaceSunken = "#59000000";
        p.surfaceHover = "#14FFFFFF";
        p.surfaceActive = "#21FFFFFF";
        p.stroke = "#00000000";
        p.strokeStrong = "#1FFFFFFF";
        p.strokeInner = "#00000000";
        p.text = "#FFFFFFFF";
        p.textSecondary = "#FFB3B3B3";
        p.textTertiary = "#FF808080";
        p.textOnAccent = "#FF04263A";
        p.error = "#FFFFB4AB";
        p.errorFg = "#FF690005";
        p.tintWarm = "#00000000";
        p.tintCool = "#00000000";
        return p;
    }();
    return dark ? darkPal : lightPal;
}

struct Radius { int xs, sm, md, lg, xl; };

inline Radius windowGlassRadius() { return Radius{2,4,6,8,12}; }
inline Radius flatBlurRadius() { return Radius{8,12,16,20,28}; }
inline Radius miuixRadius() { return Radius{4,8,12,16,24}; }

}  // namespace jmnext::qt::tokens
