cd /data/data/com.termux/files/home/jmc/JMNeXt4QtDesktop
export QT_QPA_PLATFORM=offscreen
echo "1) 构建"; cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1
cmake --build build -j2 >/tmp/b.log 2>&1; echo "   退出码=$? 错误=$(grep -cE 'error:' /tmp/b.log)"
echo "2) ctest"; ctest --test-dir build 2>&1 | grep -E 'tests passed' | sed 's/^/   /'
echo "3) QML 截图"; timeout 90 ./build/jmnext4qml --shot /data/data/com.termux/files/home/jmc/.work/final2.png 9000 2>&1 | grep -E '截图成功|加载失败' | tail -1 | sed 's/^/   /'
echo "4) 交互自检"; timeout 90 ./build/jmnext4qml --ui-selftest 2>&1 | grep -E 'listAppended' | tail -1 | sed 's/^/   /'
echo "5) 热标签"; timeout 60 ./build/jmnext4qml --hot-tags 2>&1 | grep -E '热标签' | head -1 | sed 's/^/   /'
echo "6) 分类筛选"; timeout 60 ./build/jmnext4qml --category-filter 女高中生 2>&1 | grep -E '分类筛选' | head -1 | cut -c1-70 | sed 's/^/   /'
echo "7) 周更"; timeout 60 ./build/jmnext4qml --paged week 2>&1 | grep -E '列表自检' | head -1 | cut -c1-70 | sed 's/^/   /'
echo "8) 登录+收藏(干净)"; timeout 90 ./build/jmnext4qml --logged '6173_luyaoqisen' 'RPXnCiol' favorite page=1 2>&1 | grep -E '登录：|探测' | tr '\n' ' ' | cut -c1-90 | sed 's/^/   /'; echo
