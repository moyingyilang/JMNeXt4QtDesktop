#!/bin/bash
# 把 Windows 版打成**单文件安装包**（NSIS）：双击安装到 %LOCALAPPDATA%\JMNeXt，
# 创建开始菜单与桌面快捷方式。Qt 与 MinGW 运行库全部随包。
#
# 用法：scripts/package-windows-exe.sh [输出目录]      默认 dist-win
set -u
J="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$J/dist-win}"
NAME=JMNeXt4QtDesktop
VERSION=$(sed -n 's/.*APP_VERSION\[\] = "\([^"]*\)".*/\1/p' "$J/src/core/Version.h" | head -1)
VERSION=${VERSION:-0.1.1}
STAGE="$J/.win-stage/$NAME"
[ -d "$STAGE" ] || { echo "先运行 scripts/package-windows.sh 生成 stage"; exit 1; }

mkdir -p "$OUT"
NSS="$J/.win-nsis"; rm -rf "$NSS"; mkdir -p "$NSS"

# 直接在生成的脚本里写死绝对路径（makensis 支持 POSIX 路径），避免变量歧义
cat > "$NSS/installer.nsi" <<NSI
Unicode true
Name "$NAME"
OutFile "$OUT/$NAME-$VERSION-windows-x64-setup.exe"
InstallDir "\$LOCALAPPDATA\\$NAME"
RequestExecutionLevel user
SetCompressor /SOLID lzma

Page directory
Page instfiles

Section "Install"
  SetOutPath "\$INSTDIR"
  File /r "$STAGE\\*.*"
  CreateShortCut "\$SMPROGRAMS\\$NAME.lnk" "\$INSTDIR\\jmnext4qml.exe"
  CreateShortCut "\$DESKTOP\\$NAME.lnk" "\$INSTDIR\\jmnext4qml.exe"
  WriteUninstaller "\$INSTDIR\\uninstall.exe"
SectionEnd

Section "Uninstall"
  RMDir /r "\$INSTDIR"
  Delete "\$SMPROGRAMS\\$NAME.lnk"
  Delete "\$DESKTOP\\$NAME.lnk"
SectionEnd
NSI
makensis -V2 "$NSS/installer.nsi" 2>&1 | tail -6
ls -lh "$OUT/$NAME-$VERSION-windows-x64-setup.exe" 2>/dev/null | awk '{print "  "$5"  "$9}'
