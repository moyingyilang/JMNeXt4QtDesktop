cd /data/data/com.termux/files/home/jmc/JMNeXt4QtDesktop || exit 1
echo "=== 1) 从干净构建开始（删除 build 重配）==="
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release > /tmp/h1.log 2>&1 || { echo "配置失败"; tail -8 /tmp/h1.log; exit 1; }
cmake --build build -j2 > /tmp/h2.log 2>&1 || { echo "构建失败："; grep -n -B1 -A4 "error" /tmp/h2.log | head -20; exit 1; }
echo "  构建成功"
echo "=== 2) 全部测试 ==="
ctest --test-dir build 2>&1 | tail -4
echo "=== 3) 真实网络端到端（列表 → 详情 → 章节 → 读一页）==="
timeout 300 env QT_QPA_PLATFORM=offscreen ./build/jmnext4desktop --chapters 209827 2>&1 | grep -E "列表|详情|章节|封面|页" | head -5 | sed 's/^/  /'
echo "=== 4) 界面自检（列表 + 恢复进度）==="
timeout 200 env QT_QPA_PLATFORM=offscreen ./build/jmnext4desktop --list 2>&1 | grep -E "列表已加载|加载更多" | head -2 | sed 's/^/  /'
echo "=== 5) 产物与版本 ==="
env QT_QPA_PLATFORM=offscreen ./build/jmnext4desktop --version 2>/dev/null | tail -1 | sed 's/^/  /'
