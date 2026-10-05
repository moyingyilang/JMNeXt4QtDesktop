#!/bin/sh
# 打 Linux 包（tar.gz 与 deb）—— 骨架阶段的第一个测试版打包。
#
# 设计取舍（如实）：
#  - **不捆绑 Qt**：捆绑要 linuxdeploy/AppImage 那一套，留到正式发版再做。
#    现在打成"可执行文件 + 说明 + run.sh"，依赖由系统包提供（deb 里声明 Depends）。
#  - deb 的依赖名按 Debian 12 实测填写（用 dpkg -S 查出来的，不是猜的）。
set -e
DIR=$(cd "$(dirname "$0")" && pwd)
VERSION=${VERSION:-0.1.0}
ARCH=$(dpkg --print-architecture)
OUT="$DIR/dist"
STAGE="$DIR/dist/stage"
NAME="JMNeXt4QtDesktop-$VERSION-linux-$ARCH"

echo "== 1) 构建 Release =="
cmake -S "$DIR" -B "$DIR/build" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$DIR/build" -j2 >/dev/null
echo "  完成：$DIR/build/jmnext4desktop"

echo "== 2) 准备暂存目录 =="
rm -rf "$STAGE"
mkdir -p "$STAGE/usr/bin" "$STAGE/usr/share/applications" "$OUT"
cp "$DIR/build/jmnext4desktop" "$STAGE/usr/bin/jmnext4desktop"
cat > "$STAGE/usr/share/applications/jmnext4desktop.desktop" <<'DESK'
[Desktop Entry]
Type=Application
Name=JMNeXt4QtDesktop
Comment=JMComic desktop client (native C++/Qt, early stage)
Exec=jmnext4desktop
Terminal=false
Categories=Graphics;Viewer;
DESK
cat > "$STAGE/README-测试版.txt" <<'README'
JMNeXt4QtDesktop —— 测试版（早期阶段）

这是什么：JMNeXt 的桌面端**原生重实现**（C++/Qt）。主仓库的 Kotlin/Compose 桌面版仍是稳定线，
本项目在对齐完成前请**优先使用主项目的发布包**。

运行前请确保系统有 Qt 6 与图片格式插件（Debian 12）：
  apt-get install -y libqt6widgets6 libqt6gui6 libqt6network6 qt6-image-formats-plugins

其中 qt6-image-formats-plugins **不能省**：真实漫画图是 WebP，缺了它图片会全部打不开。

用法：
  jmnext4desktop             起窗口（默认自动加载真实首页列表）
  jmnext4desktop --help      见程序内说明（--list / --chapters <aid> / --zoom <aid> / --screenshot <png>）

已验证 / 未验证：
  已验证：主机发现、首页列表（80 条 + 封面）、详情（标签/章节）、章节选择、阅读器翻页、
          图片缓存（内存+磁盘）、屏蔽规则、键盘与滚轮翻页、缩放
  未验证：账号登录与需登录接口；Windows 端；macOS；本包在其它发行版上的表现
README

echo "== 3) 打 tar.gz =="
tar -czf "$OUT/$NAME.tar.gz" -C "$DIR/dist" stage
echo "  $OUT/$NAME.tar.gz  $(stat -c %s "$OUT/$NAME.tar.gz") 字节"

echo "== 4) 打 deb =="
DEB="$OUT/${NAME}.deb"
SIZE=$(du -sk "$STAGE" | cut -f1)
mkdir -p "$STAGE/DEBIAN"
cat > "$STAGE/DEBIAN/control" <<CTRL
Package: jmnext4desktop
Version: $VERSION
Section: graphics
Priority: optional
Architecture: $ARCH
Depends: libqt6widgets6, libqt6gui6, libqt6network6, qt6-image-formats-plugins
Installed-Size: $SIZE
Maintainer: moyingyilang
Description: JMNeXt desktop client, native C++/Qt reimplementation (early stage)
 Early-stage native desktop client. Prefer the main JMNeXt release packages
 until feature parity is reached. WebP image support requires
 qt6-image-formats-plugins, otherwise real manga pages will not open.
CTRL
dpkg-deb --build --root-owner-group "$STAGE" "$DEB" >/dev/null
echo "  $DEB  $(stat -c %s "$DEB") 字节"

echo "== 5) 产物清单 =="
ls -l "$OUT" | sed 's/^/  /'
