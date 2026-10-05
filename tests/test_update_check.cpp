// 版本比较与附件名。判据来自主项目 UpdateCheckTest（7 条）**加上**新定的 fix(n) 规则（4 条）。
#include "core/UpdateCheck.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

using namespace jmnext::core;

static int failures = 0;
static void eqStr(const std::string& got, const std::string& want, const char* what) {
    if (got != want) {
        std::printf("FAIL: %s\n  实际 %s\n  期望 %s\n", what, got.c_str(), want.c_str());
        ++failures;
    }
}
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static std::string partsToStr(const std::string& raw) {
    auto p = versionParts(raw);
    if (!p) return "(null)";
    std::string s;
    for (int x : *p) { if (!s.empty()) s += "."; s += std::to_string(x); }
    return s;
}

int main() {
    // ---------- 主项目 UpdateCheckTest 的语义 ----------
    eqStr(partsToStr("1.8.0.lite"), "1.8.0", "变体后缀被忽略（lite）");
    eqStr(partsToStr("1.8.0.litedebug"), "1.8.0", "变体后缀被忽略（litedebug）");
    eqStr(partsToStr("1.8.0.debug"), "1.8.0", "变体后缀被忽略（debug）");
    eqStr(partsToStr("v1.8.0"), "1.8.0", "v 前缀被忽略");
    check(!isNewer("v1.8.0", "1.8.0.lite"), "带后缀的当前版本与干净 tag 相等 → 不算更新");
    check(!isNewer("v1.8.0", "1.8.0.debug"), "同上（debug）");
    check(isNewer("v1.8.1", "1.8.0.lite"), "逐段比较：1.8.1 > 1.8.0");
    check(!isNewer("v1.8.0", "1.8.1"), "更旧的远端不算更新");
    check(isNewer("v1.10.0", "1.9.9"), "按数字比较而不是字符串（10 > 9）");

    // ---------- 新规则：x.x.x.fix(n)（主项目 2026-10-05 定，写在 docs/STATE.md 第三十三节）----------
    eqStr(partsToStr("2.1.7.fix1"), "2.1.7.1", "fix1 作为第四段");
    eqStr(partsToStr("2.1.7.fix1.lite"), "2.1.7.1", "fix 与变体后缀可以共存（fix 保留、lite 丢弃）");
    check(isNewer("v2.1.7.fix1", "2.1.7"), "修复版必须被认为比原版新（否则永远推不出去）");
    check(isNewer("v2.1.7.fix2", "2.1.7.fix1"), "fix2 > fix1");
    check(!isNewer("v2.1.7", "2.1.7.fix1"), "原版不比修复版新");

    // ---------- 附件名与直链 ----------
    eqStr(cleanVersion("2.1.7.fix1.lite"), "2.1.7.fix1", "cleanVersion 保留 fixN、丢弃 lite");
    eqStr(cleanVersion("v2.1.7.lite"), "2.1.7", "cleanVersion 丢弃变体后缀");
    eqStr(tagOf("2.1.7.fix1"), "v2.1.7.fix1", "tag 带 v 前缀");
    eqStr(androidAssetName("v2.1.7", false), "Android-full-2.1.7.apk", "Android full 附件名");
    eqStr(androidAssetName("2.1.7.lite", true), "Android-lite-2.1.7.apk", "Android lite 附件名（本机带后缀也要归一化）");
    eqStr(androidAssetName("2.1.7.fix1", false), "Android-full-2.1.7.fix1.apk", "修复版的附件名也要保留 fixN");

    {
        auto win = desktopAssetNames("v2.1.7", "Windows 10", "amd64");
        check(!win.empty() && win[0] == "Windows-universal-2.1.7.exe", "桌面候选里统一包排第一（Windows）");
        check(std::find(win.begin(), win.end(), "Windows-x64-2.1.7.zip") != win.end(), "Windows x64 ZIP 在候选里");
        auto winArm = desktopAssetNames("v2.1.7", "Windows 11", "aarch64");
        check(std::find(winArm.begin(), winArm.end(), "Windows-arm64-2.1.7.zip") != winArm.end(),
              "Windows arm64 ZIP 在候选里");
        auto lin = desktopAssetNames("v2.1.7", "Linux", "aarch64");
        check(!lin.empty() && lin[0] == "Linux-universal-2.1.7.tar.gz", "桌面候选里统一包排第一（Linux）");
        check(std::find(lin.begin(), lin.end(), "Linux-aarch64-2.1.7.tar.gz") != lin.end(), "Linux aarch64 tar.gz 在候选里");
    }
    eqStr(assetUrl("v2.1.7", "Android-full-2.1.7.apk"),
          "https://github.com/moyingyilang/JMNeXt/releases/download/v2.1.7/Android-full-2.1.7.apk",
          "附件直链");
    check(assetUrl("2.1.7", "x").find("/download/v2.1.7/") != std::string::npos, "tag 少 v 前缀时自动补上");

    if (failures == 0) std::printf("全部通过：版本比较（含 fix(n) 规则）与附件名/直链\n");
    return failures == 0 ? 0 : 1;
}
