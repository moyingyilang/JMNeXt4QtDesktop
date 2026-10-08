cd /data/data/com.termux/files/home/jmc/JMNeXt4QtDesktop
export QT_QPA_PLATFORM=offscreen
echo "--- 1) 构建 ---"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/tmp/c.log 2>&1
cmake --build build -j2 >/tmp/b.log 2>&1
echo "  构建退出码=$?"
echo "  错误数=$(grep -cE 'error:' /tmp/b.log)"
echo "--- 2) ctest ---"
ctest --test-dir build 2>&1 | grep -E 'tests passed' | sed 's/^/  /'
echo "--- 3) widget 列表 ---"
timeout 150 ./build/jmnext4desktop --list 2>&1 | tail -2 | sed 's/^/  /'
echo "--- 4) widget 章节 ---"
timeout 150 ./build/jmnext4desktop --chapters 209827 2>&1 | tail -2 | sed 's/^/  /'
echo "--- 5) QML 截图 ---"
timeout 200 ./build/jmnext4qml --shot /data/data/com.termux/files/home/jmc/.work/final-shot.png 9000 2>&1 | grep -E '分类标签已填充|截图成功|加载失败' | tail -2 | sed 's/^/  /'
echo "--- 6) QML 交互自检 ---"
timeout 200 ./build/jmnext4qml --ui-selftest 2>&1 | grep -E 'listAppended' | tail -1 | sed 's/^/  /'
echo "--- 7) 热标签自检 ---"
timeout 150 ./build/jmnext4qml --hot-tags 2>&1 | grep -E '热标签' | head -1 | sed 's/^/  /'
echo "--- 8) 分类筛选自检 ---"
timeout 150 ./build/jmnext4qml --category-filter 女高中生 2>&1 | grep -E '分类筛选' | head -1 | sed 's/^/  /'
