#!/bin/sh
# 在容器里跑 JMNeXt4QtDesktop（VNC / 本地显示都行）
#
# 用法：
#   ./run.sh              直接起窗口（用当前 DISPLAY，例如 VNC 的 :1）
#   ./run.sh --list       起窗口并自动加载真实首页列表
#   ./run.sh --chapters 209827   起窗口并打开某作品的章节选择
#
# 前提：容器里已装 qt6-base-dev 与 qt6-image-formats-plugins（后者提供 webp 解码，
# 缺了真实漫画图会全部解不开）；若在 VNC 里，请先设好 DISPLAY。
set -e
DIR=$(cd "$(dirname "$0")" && pwd)
export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/tmp/xdg}
mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR" 2>/dev/null || true
BIN="$DIR/build/jmnext4desktop"
if [ ! -x "$BIN" ]; then
  echo "还没有可执行文件，先构建："
  echo "  cd $DIR && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j2"
  exit 1
fi
echo "DISPLAY=${DISPLAY:-（未设置）}  Qt 平台=${QT_QPA_PLATFORM:-xcb(默认)}"
exec "$BIN" "$@"
