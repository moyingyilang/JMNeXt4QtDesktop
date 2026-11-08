#!/bin/bash
# 打包 Qt 桌面端（Linux）：自带 Qt 运行时的便携 tar.gz 与 deb。
#
# 为什么需要它：此前桌面端的交付物只有 JVM 的 fat jar（70–90 MB、要带 JRE），
# 而 Qt 版才是真正的桌面实现。这里把 Qt 运行时一起打包，做到「解压即运行、
# 不依赖系统 Qt」—— 与主项目桌面端「自带运行时」的思路一致，但体积只有几 MB 级。
#
# 用法：scripts/package-linux.sh [输出目录]        默认 dist
#       VERSION=0.2.0 scripts/package-linux.sh
set -u

J="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="${1:-$J/dist}"
BUILD="$J/build-release"
VERSION="${VERSION:-0.1.1}"
ARCH="$(uname -m)"
NAME="JMNeXt4QtDesktop"

QT_LIB=/usr/lib/aarch64-linux-gnu
QT_PLUGINS=$QT_LIB/qt6/plugins
QT_QML=$QT_LIB/qt6/qml

# 版本号单一来源：src/core/Version.h 的 APP_VERSION
APP_VERSION=$(grep -oE '#define APP_VERSION "[^"]+"' "$J/src/core/Version.h" | sed 's/.*"\(.*\)"/\1/')
[ -n "${APP_VERSION:-}" ] && VERSION="$APP_VERSION"

say() { echo "== $*"; }

say "构建（$BUILD）"
cmake -S "$J" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1 || { echo "配置失败"; exit 1; }
cmake --build "$BUILD" -j7 >/dev/null 2>&1 || { echo "编译失败"; exit 1; }

STAGE="$J/.pack-stage"
rm -rf "$STAGE"; mkdir -p "$STAGE/$NAME"/{bin,lib,plugins/{platforms,imageformats},qml}
S="$STAGE/$NAME"

say "拷贝可执行文件与 Qt 插件"
cp "$BUILD/jmnext4qml" "$S/bin/"
cp "$QT_PLUGINS/platforms/libqxcb.so" "$S/plugins/platforms/" 2>/dev/null
cp "$QT_PLUGINS/imageformats/"*.so "$S/plugins/imageformats/" 2>/dev/null
# QML 模块：只带界面实际用到的（整份 9 MB，直接全带，省得日后漏模块）
cp -a "$QT_QML/." "$S/qml/"

say "收集运行时依赖（ldd 递归，排除 glibc/gcc）"
: > /tmp/qtlibs.txt
collect() {
    local f="$1"
    ldd "$f" 2>/dev/null | awk '{for (i=1;i<=NF;i++) if ($i ~ /^\//) print $i}' | while read -r lib; do
        case "$(basename "$lib")" in
            libc.so*|libm.so*|libdl.so*|libpthread.so*|librt.so*|ld-linux*|libstdc++*|libgcc_s*|libresolv*) continue ;;
        esac
        grep -qx "$lib" /tmp/qtlibs.txt && continue
        echo "$lib" >> /tmp/qtlibs.txt
        collect "$lib"
    done
}
collect "$S/bin/jmnext4qml"
for p in "$S/plugins"/*/*.so; do [ -f "$p" ] && collect "$p"; done

n=0
while read -r lib; do
    [ -f "$lib" ] || continue
    cp -L "$lib" "$S/lib/" 2>/dev/null && n=$((n+1))
done < /tmp/qtlibs.txt
say "依赖库 $n 个，合计 $(du -sm "$S/lib" | cut -f1) MB"

say "设置 rpath（\$ORIGIN/../lib）"
patchelf --set-rpath '$ORIGIN/../lib' "$S/bin/jmnext4qml"
for f in "$S"/lib/*.so*; do [ -f "$f" ] && patchelf --set-rpath '$ORIGIN' "$f"; done
for f in "$S"/plugins/*/*.so; do [ -f "$f" ] && patchelf --set-rpath '$ORIGIN/../../lib' "$f"; done

say "启动脚本"
cat > "$S/run.sh" <<'RUN'
#!/bin/sh
# 便携启动：使用自带的 Qt 运行时，不依赖系统安装的 Qt。
HERE=$(cd "$(dirname "$0")" && pwd)
export LD_LIBRARY_PATH="$HERE/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$HERE/plugins"
export QML2_IMPORT_PATH="$HERE/qml"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
exec "$HERE/bin/jmnext4qml" "$@"
RUN
chmod +x "$S/run.sh"

mkdir -p "$OUT_DIR"
say "打 tar.gz"
tar -C "$STAGE" -czf "$OUT_DIR/$NAME-$VERSION-linux-$ARCH.tar.gz" "$NAME"

say "打 deb"
DEB="$STAGE/deb"
mkdir -p "$DEB/DEBIAN" "$DEB/usr/lib/$NAME"
cp -a "$S/." "$DEB/usr/lib/$NAME/"
cat > "$DEB/DEBIAN/control" <<CTRL
Package: jmnext4qtdesktop
Version: $VERSION
Section: net
Priority: optional
Architecture: $ARCH
Maintainer: moyingyilang <3433402317@qq.com>
Depends: libc6
Description: JMNeXt 的 Qt 桌面客户端（自带 Qt 运行时）
 禁漫（JMComic）第三方客户端的原生桌面实现，随包附带 Qt 运行库，无需系统安装 Qt。
CTRL
mkdir -p "$DEB/usr/bin"
ln -sf "/usr/lib/$NAME/run.sh" "$DEB/usr/bin/jmnext4qtdesktop"
dpkg-deb --build --root-owner-group "$DEB" "$OUT_DIR/$NAME-$VERSION-linux-$ARCH.deb" >/dev/null

say "完成"
ls -lh "$OUT_DIR/$NAME-$VERSION-linux-$ARCH".{tar.gz,deb} 2>/dev/null | awk '{print "  "$5"  "$9}'
