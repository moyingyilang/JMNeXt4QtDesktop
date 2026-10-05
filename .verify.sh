cd /data/data/com.termux/files/home/jmc/JMNeXt4QtDesktop || exit 1
cmake --build build -j2 > /tmp/bv5.log 2>&1 || { echo "构建失败："; grep -n -B1 -A4 "error" /tmp/bv5.log | head -20; exit 1; }
echo "构建成功"
ctest --test-dir build 2>&1 | tail -3
echo "=== --help 是否含 --version ==="
env QT_QPA_PLATFORM=offscreen ./build/jmnext4desktop --help 2>/dev/null | grep -c "version" | xargs echo "  含 version 的行数:"
env QT_QPA_PLATFORM=offscreen ./build/jmnext4desktop --version 2>/dev/null | tail -1 | sed 's/^/  --version: /'
