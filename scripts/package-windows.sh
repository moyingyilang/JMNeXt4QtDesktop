#!/bin/bash
# 打包 Windows x64：交叉编译产物 + Qt 运行库 + QML 模块，输出可双击运行的 zip。
#
# 背景：Qt 6.8 起官方不再提供 MinGW 版 Windows 构建，因此 Windows 包固定在 Qt 6.7.3
# （mingw_64）。宿主工具用同版本 linux_gcc_arm64 包（moc/rcc/qmlcachegen 不能跨平台）。
#
# 用法：scripts/package-windows.sh [输出目录]        默认 dist-win
set -u
J="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$J/dist-win}"
BUILD="${BUILD_DIR:-/root/qt-build-win}"
QT="${QT_WIN:-/root/qt-win/6.7.3/mingw_64}"
MINGW="$(dirname "$(x86_64-w64-mingw32-gcc -print-libgcc-file-name)")"
NAME=JMNeXt4QtDesktop
VERSION=$(sed -n 's/.*APP_VERSION\[\] = "\([^"]*\)".*/\1/p' "$J/src/core/Version.h" | head -1)
VERSION=${VERSION:-0.1.1}

[ -x "$BUILD/jmnext4qml.exe" ] || { echo "没有 $BUILD/jmnext4qml.exe，先交叉编译"; exit 1; }

STAGE="$J/.win-stage"; rm -rf "$STAGE"; mkdir -p "$STAGE/$NAME"
S="$STAGE/$NAME"
mkdir -p "$S/platforms" "$S/styles" "$S/imageformats"

echo "== 可执行文件 =="
cp "$BUILD/jmnext4qml.exe" "$S/"

echo "== Qt DLL =="
for dll in Qt6Core Qt6Gui Qt6Qml Qt6Quick Qt6QuickControls2 Qt6QuickTemplates2 Qt6QuickLayouts Qt6QuickEffects Qt6Network Qt6Widgets; do
    f="$QT/bin/$dll.dll"
    [ -f "$f" ] && cp "$f" "$S/" || echo "  缺少 $dll.dll（若运行时报缺，先确认模块已安装）"
done

echo "== Qt 插件 =="
cp "$QT/plugins/platforms/qwindows.dll" "$S/platforms/" 2>/dev/null
cp "$QT/plugins/styles/qmodernwindowsstyle.dll" "$S/styles/" 2>/dev/null
for img in qjpeg qwebp qgif qico; do
    cp "$QT/plugins/imageformats/$img.dll" "$S/imageformats/" 2>/dev/null
done

echo "== QML 模块（只带界面用到的）=="
for m in QtQuick QtQml; do
    [ -d "$QT/qml/$m" ] || continue
    mkdir -p "$S/qml"
    cp -a "$QT/qml/$m" "$S/qml/"
done

echo "== MinGW 运行库（用 Debian 的 gcc 16 构建，必须带对应 libstdc++）=="
for lib in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
    cp "$MINGW/$lib" "$S/" 2>/dev/null || echo "  缺少 $lib"
done

echo "== D3D / 软件 OpenGL =="
cp "$QT/bin/d3dcompiler_47.dll" "$S/" 2>/dev/null
cp "$QT/bin/opengl32sw.dll" "$S/" 2>/dev/null

echo "== 说明文件 =="
cat > "$S/README.txt" <<'TXT'
JMNeXt 桌面端（Qt 原生实现）—— Windows x64

双击 jmnext4qml.exe 运行。所有依赖（Qt、MinGW 运行库）都在本目录内，无需安装。

命令行参数（可选）：
  --list                 启动即加载首页列表
  --read <aid>           直接打开某作品
  --wallpaper <图片路径>  指定壁纸
  --theme <风格>          windowGlass / translucent / flatBlur / miuix / material
  --shot <png> <毫秒>     渲染截图（调试用）
TXT

mkdir -p "$OUT"
cd "$STAGE" || exit 1
zip -qr "$OUT/$NAME-$VERSION-windows-x64.zip" "$NAME"
echo "== 完成 =="
ls -lh "$OUT/$NAME-$VERSION-windows-x64.zip" | awk '{print "  "$5"  "$9}'
echo "  目录内容: $(du -sh "$S" | cut -f1)"
