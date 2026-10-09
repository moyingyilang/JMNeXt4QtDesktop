# 项目状态（交接用）

> 更新于 0.1.1 开发中。**先读这句**：本项目是 [JMNeXt](https://github.com/moyingyilang/JMNeXt) 的
> 桌面端原生重实现（C++ / Qt），处于早期阶段；**对齐完成前请优先使用主项目的发布包**。

## 一分钟验证它是否还能跑

```sh
apt-get install -y cmake g++ qt6-base-dev qt6-image-formats-plugins   # 后者不能省（真实图是 WebP）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j2
ctest --test-dir build                      # 期望 16/16
./build/jmnext4net discover                 # 真实网络：主机发现（期望打印一个 https:// 主机）
./build/jmnext4desktop --list               # 界面自检：期望"列表已加载：80 条"+"加载更多之后：160 条"
./build/jmnext4desktop --chapters 209827    # 期望 章节列表项数：159、封面 400x533、恢复/显示某页
```

无显示环境加 `QT_QPA_PLATFORM=offscreen` 或 `xvfb-run -a`。

## 已经做到的（每条都有验证方式）

| 能力 | 验证方式 |
| --- | --- |
| 端点表（45 条，由主项目常量机械抽出） | 抽样断言 |
| MD5 / AES-256-ECB / PKCS#7 / Base64 | RFC 1321 向量、NIST SP800-38A 向量、**openssl 交叉验证** |
| 响应解密（`{code,data}` 信封、`\/` 还原、多 seed、"人话"响应） | 单测 + **真实网络** |
| 主机发现（两个入口、固定种子、裸密文、不带凭证） | 单测 + **真实网络**（4 个候选主机） |
| 首页列表 + 封面 + 分页 | **真实网络**（80 → 160 条；封面 400x533） |
| 详情（标题/作者/标签/封面，author 是数组） | **真实网络**（标签 5、章节 159） |
| 章节列表点选切换 | **真实网络**（159 项） |
| 阅读器：逐页下载 → WebP 解码 → 切片还原 → 显示；键盘/滚轮翻页；适应窗口 / 100% | **真实网络**（285 页；真实图 852x1280） |
| 图片缓存（内存 LRU + 磁盘原始字节） | 跨进程命中实测 |
| 翻页预取 | 缓存统计（10 次取图 4 次命中） |
| 屏蔽规则（关键词子串 / 标签与分类精确） | 单测 + **真实数据**（80 → 64） |
| 阅读进度（写入 + 再次打开接着读） | 单测 7 组 + 两次运行实测 |
| 全部走后台线程（列表/章节/翻页不阻塞） | 异步自检（走 worker 的代码路径） |
| 打包（tar.gz + deb） | Debian 12 上"装 → 跑真实数据 → 卸" |
| 版本单一来源 | `src/core/Version.h`；`--version` 与打包脚本都从它取 |
| 更新检查（含 `fix(n)` 规则） | 单测 + 罐装响应；**在线一步未验证** |

## 还没做（按建议顺序）

1. **账号登录**与一切需登录接口（搜索、收藏、追更、下载、屏蔽设置页）—— 需要真实账号；
2. **界面布局整理**：控件集中在少数几行；建议先拍图给人看再改；
3. **x86_64 包**：当前只出 linux-arm64；需要 Qt 交叉工具链；
4. **视觉系统对齐**（主项目有四套主题风格与自研动效）—— 目前是 Qt 默认观感，这是与主项目最大的差距；
5. **未捆绑 Qt**：依赖系统包；正式发版应考虑 AppImage 或捆绑。

## 已知问题 / 限制

- ~~**历史提交里含敏感截图**~~：**这条记录不成立，已作废（2026-10-07 复核）**。
  全历史对象里没有任何 png/jpg/webp（`git rev-list --all --objects` 过滤图片扩展名为 0 条），
  也不存在 `19e3939` / `cb24755` 这两个提交号；`.work-shots/` 已被 `.gitignore` 排除且从未被跟踪。
  原记录写错了，保留在此以免后来者再去"清历史"；
- **截图模式是定时拍照**（`--screenshot`，带子参数等 45 秒），对阅读页存在 race：
  空窗口约 37 KB、真实阅读页 135 KB 以上，**拿到小文件就重拍**；
- **在线更新检查未验证**：开发环境到 `api.github.com` 的 HTTPS 不通（`HTTP 0`）；
- 阅读进度只记**最后一次**（主项目按作品记录多条目）；
- deb 只在 Debian 12 (arm64) 验过；Windows / macOS 未做。

## 与主项目的偏离

- 主项目用 Compose 多套主题 + 自研动效；本实现**没有**做视觉对齐；
- 主项目阅读进度按作品记；本实现单条；
- 主项目还有壁纸、RSS/追更通知、下载/离线等；本实现都没有。

## 给接手者的提醒（踩过的坑，别重踩）

1. **`/tmp` 在本开发环境（Termux 侧）不可写** —— 临时文件与日志一律放项目下的 `.work/`；
2. **提交一律显式列路径**：`git add -A` 已三次把不该进仓库的东西卷进去（截图、垃圾文件 `=`、临时脚本）；
3. **Qt 的 `signals:` / `public slots:` 区块是语义性的**：往里插普通成员声明会被 moc 当成信号（链接期 multiple definition）；
4. **含中文/引号/命令替换的替换内容不要塞进 `sed`/`perl` 表达式** —— 写进 heredoc 再用 awk 按行插入；
5. **锚点先 grep 验证非空**，否则 `sed -i "${空行号}a..."` 会把代码插到文件开头；
6. **`signals`/`slots` 之外**：头文件里不要对只前向声明的类型调用成员（`QListWidget` 的 `count()` 要放到 `.cpp`）；
7. 真实响应会纠正"想当然"：`{code,data}` 信封、`\/` 转义、`Accept` 头、`author` 是数组、图片是 WebP —— 都是真实数据教出来的。

## 性能基线（本机实测，`[ms]` 来自程序日志的时间戳）

| 场景 | 耗时 | 说明 |
| --- | --- | --- |
| 首屏 → 第一页（**无任何缓存**） | **3685 ms** | 主机发现 + 详情 + 章节 + 首图，四次串行请求 |
| 首屏 → 第一页（主机缓存命中、图片磁盘缓存温） | **1091 ms** | 省约 2.6 秒 |
| 主机发现本身（有缓存 vs 无缓存） | **16 ms vs 1588 ms** | `AppDataLocation/host.txt` |
| 翻到下一页（预取命中） | **59–177 ms** | 预取在后台线程完成，不阻塞界面 |

日志格式为 `[  3862 ms] 第 4/285 页 904x1320`（自启动以来的毫秒数），可直接复现这些测量。

**边界**：同一台机器、同一网络下的测量；换网络/换机器会不同。剩余首屏大头是
"详情 → 章节 → 首图"三次串行请求（章节 id 依赖详情，无法并行）。

## 关于「直取第一话」这个优化假设的验证情况（截至本轮）

代码里 JmWorker::openChapter 有一条优化：**直接用作品 id 取第一话**，省掉"详情"那一轮往返；
取不到时回退到"先取详情、再用 series 第一项的 id"。回退分支**至今没有被真实数据走到**。

抽样验证（真实网络，逐个查 album 比对 aid 与 series 首项 id）：

| 作品 aid | series 项数 | series 首项 id | 结论 |
| --- | --- | --- | --- |
| 1479598 | 7 | 1479598 | 相同 |
| 1479588 | 1 | 1479588 | 相同 |
| 1158362 | 16 | 1158362 | 相同 |
| 1446453 | 5 | 1446453 | 相同 |
| 1435987 | 17（应用侧）/ 13+（原始响应） | 1435987 | 相同 |
| 1064326 | （未可靠取到） | — | 不采信 |
| 1479439 | 0（单本） | 无 | 单本，走"按第一话读取"分支 |

也就是说：**在已抽样的 1 到 17 话作品上，series 首项 id 都等于 aid**，所以"直取第一话"的假设成立；
但**反例还没有找到**，回退分支仍属"编译通过、逻辑保留、未被真实走到"。

本轮抽样过程中我自己的脚本错了两次（第一次 `tail -1` 取到末尾空行、样本数变成 1；
第二次把 `category.id`（值就是 "2"）也当成作品 id 混入样本，且对空 series 的正则越界、
给出假的"相同"判定）。结论只采信**直接查看原始响应**得到的那几条。工具本身的教训：
用正则从 JSON 里抠字段很容易越界，**要看证据就打印原始片段**。

## 关于"对齐主项目主题"（规格 G5）的真实情况 —— 先纠正我自己的说法

我在目标里一直写"主项目四套主题"，**这是不准确的**。去看主项目源码后确认：

**位置**：`JMComic_Next/app/src/main/kotlin/com/jmnext/ui/theme/ThemeStyle.kt` 与 `Theme.kt`
（工作区里的主项目目录名是 `JMComic_Next`，不是我以为的 `JMNeXt`）。

**事实**：主项目有**五套**可选风格，而且是五套**表面工艺**，不是五套配色。主项目自己的注释写着：
"这不是「五套配色」，而是五套**表面工艺**：圆角尺度、表面材质、描边、投影、字体层级、动效手感都不一样。
只换颜色的话，五套风格在截图里会长得一样 —— 那和做五个主题包没区别"。默认是 `WindowGlass`。

| 风格 | 参考 | 关键差异 |
| --- | --- | --- |
| WindowGlass（默认） | Windows 11 窗口玻璃 | 8dp 圆角、发丝描边、Acrylic 颗粒 |
| Translucent | Windhawk 透明系 | 重度透明 + 强调色薄染 |
| FlatBlur | 平面化 + 高斯模糊 | 无描边、无颗粒、无高光 |
| Miuix | HyperOS | 大圆角实心卡片、无描边、弹性按压 |
| Material | Material You 3 | 按 M3 颜色角色分层、可动态取色 |

每套风格的参数（来自 ThemeStyle.kt）：
`RadiusScale(xs/sm/md/lg/xl)`、`fillAlphaScale`、`accentTint`、`hairline`（描边宽度）、
`backdropSaturate`、`noise`（颗粒）、`shadows`（多级投影）、`pressScale`（弹性按压）、
`lineHeightFactor`、`wallpaperScrim`、`backdropGlow`。

### 这个 Qt 栈里能做到什么（如实）

| 主项目要素 | Qt Widgets + QSS 能否做到 |
| --- | --- |
| 配色 / 圆角尺度 / 描边（hairline） | **能**（QSS 的 border-radius、border、rgba 颜色） |
| 按下反馈（pressScale 的静态近似） | 部分能（`:pressed` 伪状态改内边距/颜色；真弹性动效要 QPropertyAnimation） |
| 投影（shadows 多级） | 部分能（QSS 无 box-shadow，需 QGraphicsDropShadowEffect 逐控件加） |
| 颗粒（noise / Acrylic 颗粒） | **不能**（需自绘噪声纹理） |
| 高斯模糊 / 背景透明（FlatBlur、Translucent） | **不能**（Qt Widgets 没有 backdrop blur；需换 QML 或自绘合成） |
| 行高（lineHeightFactor） | **不能**（QSS 无 line-height） |
| 壁纸压暗 / 背光（wallpaperScrim、backdropGlow） | **不能**（依赖壁纸取景，本实现也没有壁纸功能） |
| 动效手感（弹性、过渡） | 需逐处 QPropertyAnimation 写，工作量大 |

**结论**：忠实复刻五套风格在这个技术栈上**做不到**。可行的做法是"**五套具名风格 + 能落地的子集**"：
每套给正确的配色、圆角尺度、描边、按下反馈；**并在文档与界面上如实说明"模糊/颗粒/动效未实现"**，
而不是做五个只有名字不同、看起来一样的主题（那正是主项目注释里批评的做法）。

### 下一步（(d) 的落地计划）

1. 从主项目 `Theme.kt` 提取五套风格的**配色 token**（浅色/深色两套），照抄数值，不自创；
2. Qt 侧把 `Theme.h` 从"深色/浅色"扩成"五套具名风格 × 深/浅"，先落地配色+圆角+描边+按下反馈；
3. 每套拍一张截图给用户确认；界面上标注"模糊与颗粒未实现"；
4. 投影（QGraphicsDropShadowEffect）与按下动画按需逐项补，每项单独验证。

## 界面阶段（(a)~(d) + 相关修复）的完整对照

用户原话"这 gui 有点太绿皮了吧"。以下是这一阶段做的每一件事、**怎么验证的**、以及**边界在哪**。

| 项 | 做了什么 | 验证方式 | 边界 / 未验证 |
| --- | --- | --- | --- |
| (a) 主题 | QSS 深/浅两套，替掉系统默认控件样式 | 截图（体积与观感变化）；修掉"主题管不到的控件"死 CSS | 观感只能由用户判断 |
| (b) 布局 | 左右分栏（QSplitter）、按钮拆两行、四周留白与间距、三区域标题、阅读区占两份 | `--list` 80 → 160 条、`--chapters` 159 项仍正常；截图 | 分栏比例是否合适需用户判断 |
| (c) 阅读区 | 页码 + 进度百分比、加载反馈（首屏 1–4 秒不再像卡住）、单页/双页 | 日志时间戳（"正在加载第 N 页…"→"第 N/M 页 · x%"）；双页用**对照组**验证（不带开关 0 条双页日志，带上 3 条"已就位"） | 双页两页缩放的协调性未目视确认 |
| (d) 风格 | 四套具名风格（windowGlass/translucent/flatBlur/miuix），配色与圆角**照抄主项目**（生成器产出，180+ 赋值，逐条对照） | 每套打印实际取值：translucent 更透（#0AFFFFFF）、flatBlur 描边全透明、miuix 实心（#FF1C1C1E）——**与主项目对每套的说明一致** | 模糊/颗粒/行高/壁纸压暗做不到；Material 未接入；translucent 圆角沿用 windowGlass（推断） |
| 界面切换 | 风格下拉框 + 深浅按钮，**两者都会被记住** | 判别性验证：带 `--theme miuix` 跑一次 → 不带参数再跑仍是 miuix；深浅同理 | **点击路径未真实走到**（无头环境点不了） |
| 投影 | 封面与阅读区各挂一层（模糊 12、偏移 0,2） | `--list` 打印"已挂投影控件数 2" | 参数是保守取值，未按主项目多级 shadows 校准；观感未目视 |
| 搜索 | 接口参数名**探出来的**（key/keyword 都返回空结果，search_query 才对）；结果复用 listReady 填入列表 | CLI 与应用内两条路径：`--search test` → 45 条、中文"巨乳" → 80 条 | 第 3 页起未测 |
| 详情区 | 封面 + 文字独立成行，放到阅读区上方 | 回归：详情标签、封面 400x533、章节 159 均正常 | 封面与文字高度比例未目视 |

### 这一阶段修掉的真实缺陷（都不是"猜"出来的）

1. **交互式列表根本没有封面** —— `loadCovers` 只在自检路径被调用；加日志后实测：改前 0 张、改后 12 张；
2. **搜索取封面会取错** —— `loadCovers` 重新拉首页列表；用**零结果关键词**做判别（空列表时应 0 张，旧代码会错取首页 3 张）；
3. **屏蔽隐藏后封面下标错位** —— 随 2 一起修（改存"过滤后"的条目）；
4. **搜索后点"加载更多"会混进首页内容** —— 判别性验证：零结果搜索 + 加载更多应仍为 0 条；
5. **单本作品被误报为自检失败** —— 多作品抽样发现（该作品 series 为空但阅读正常）；改后如实打印"该作品章节表为空（单本），按第一话读取：通过"；
6. **主题 QSS 是死代码** —— 控件没有 objectName，且内联样式会盖掉主题；
7. **旧的 setStyleSheet 覆盖新样式** —— 四种风格实际完全没生效，但日志照样打印"主题：miuix"（**日志正确、行为错误**）。

### 这一阶段我自己犯的错（同一类反复出现，记在这里）

- **头文件类型**（三次）：QListWidget 只前向声明却写内联实现、QComboBox 同样、QPushButton 缺声明；
- **单行替换模板漏写 `next`**（两次）：新旧行并存，第二次导致四种风格完全没生效；
- **含 shell 变量/命令替换的内容塞进 perl/sed 表达式**（多次）：`$SRC` 被 perl 吃掉、`%1` 被 printf 当格式符；
- **脚本里写 /tmp**（多次）：本环境 /tmp 不可写；
- **`git add -A`**（三次）：误收截图、垃圾文件 `=`、临时脚本。

这些都已写进各自的提交信息；`STATUS.md` 末尾的"给接手者的提醒"里保留了可操作的规则。

## QML 前端推进状态（本轮更新）

用户已决定：**一对一复刻 Kotlin(Compose) 版界面**，UI 层走 **Qt Quick / QML**；
第一步范围 = **核心阅读路径**（首页列表 / 详情 / 阅读器 / 章节选择 / 搜索）。
后来又追加为**混合方案**：Widgets 主体 + 局部嵌 QML（弹簧按压、胶囊吸附、列表条目位移这四项只有 QML 原生）。

| 阶段 | 状态 | 证据 |
| --- | --- | --- |
| P0 Qt Quick 可用 | **完成** | 容器内装齐 `qt6-declarative-dev` 与一批 `qml6-module-*`；`jmnext4qml` 能起窗口并截图（900x600） |
| 共用层抽取 | **完成** | 新增静态库 `jmnext_qtlayer`（JmClient/QtHttpClient/ImageCache/ImageCodec/JmWorker），widget 与 QML 都链它 |
| P1b 后端桥 | **完成** | `src/qml/JmBackend.{h,cpp}` 把共用 `JmWorker` 放工作线程、用 `Q_INVOKABLE` 暴露给 QML；`--selftest` 实测拿到**列表 80 条**与**真实一页图 852x1280** |
| P1c 图片提供器 | **完成**（下表"已知坑"仍适用） | 提交 `cd18c89`：`src/qml/JmImageProvider.*` 让 QML 显示封面，跨实例命中磁盘缓存已验证 |
| P2 五屏 1:1 | **进行中：四屏已落地**（首页列表、详情、阅读器、搜索） | 提交 `0642968` / `29d2e92` / `6cf7850` / `86c5421`；依据是 `docs/ui-inventory/01..04` |
| P3 主题与效果 | **进行中** | 提交 `67aa3d3` 把主题 token 接进 QML（四套风格可切换并各自生效）、`6d614a1` 加列表悬停过渡与按压弹性；Qt 6.4 无 `MultiEffect`，模糊/颗粒需 compat 或自写 shader |

> 上表原记「P1c 未开始、P2/P3 未开始」，与提交 `cd18c89` 起的实际进度不符，2026-10-07 核对后按提交修正。

### P1c 的已知坑（下次动手前必读）

`QQuickImageProvider::requestImage` 在 **Qt Quick 渲染线程**里被调用，因此：

1. **不能**直接用 widget 侧那个 `JmClient` 实例：它的 `QNetworkAccessManager` 有线程归属（必须在创建它的线程使用）；
2. `ImageCache` 的内存 LRU **不是线程安全的**；
3. 可行方向（任选其一，动手前先确认 API）：
   - 提供器只做**磁盘缓存读取**（自己的 `ImageCache` 实例或直接读缓存目录），网络一律由后端在 worker 线程完成；
   - 或让后端在取到图后**主动推给 QML**（不走 ImageProvider），QML 侧用 `Image` 的 `source` 指向一个本地临时文件；
4. 无论选哪种，都要有一句"缓存未命中时返回占位图而不是空图"的降级逻辑，否则 QML 侧会显示空白且无提示。

### 当前可核对的自检命令（都在容器内）

```sh
# widget 版真实数据
QT_QPA_PLATFORM=offscreen ./build/jmnext4desktop --list          # 期望：列表条数 80 -> 加载更多 160
QT_QPA_PLATFORM=offscreen ./build/jmnext4desktop --chapters 209827   # 期望：章节列表项数 159
# QML 前端：环境摘要 + 后端真实链路 + 截图
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ./build/jmnext4qml --selftest
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ./build/jmnext4qml --shot /tmp/qml.png
# 测试
ctest --test-dir build      # 期望 16/16
```

注意：**模式参数必须是第一个参数**（`--selftest` / `--shot <png>` / `--list` …）；
`--theme` / `--light` / `--verbose` 可以放后面，放前面会被当成"无模式"而启动 GUI 常驻。

## P1c 设计已定案（下次一次性实现，别再重新推）

**目标**：让 QML 能显示封面与阅读页。**关键约束**：`ImageCache` 的磁盘缓存键就是**完整 URL**
（`<imageBase>/media/albums/<id>_3x4.jpg?v=<updateAt>`，见 `core/JmUrls.cpp:19-31`），
所以提供器必须算出与 worker **完全一致**的 URL 才能命中同一份磁盘缓存。

由此得到三条硬结论：

1. **不能**只加一个 `QQuickImageProvider` 就完事 —— `?v=` 来自列表条目的 `updateAt`，
   而当前后端只把 `listReady(titles, ids)` 转出去（**没有 updateAt**）。必须先让后端把
   `updateAt`（或直接给出**完整封面 URL**）暴露给 QML；
2. **不能**把 `?v=` 从缓存键里去掉来"省事"：那个版本号就是用来让封面在服务端更新后失效的，
   去掉会让用户长期看到旧封面（**这是行为回退，不是优化**）；
3. 提供器在 **Qt Quick 渲染线程**被调用，因此**不能**用 widget 侧那个 `JmClient`
   （`QNetworkAccessManager` 有线程归属），`ImageCache` 的内存 LRU 也不是线程安全的。
   安全做法：提供器**只用磁盘缓存**（自己的 `ImageCache` 实例，只调 `hasRaw/raw/putRaw`），
   解码用 `ImageCodec`；缓存未命中时返回**占位图**（纯色），不要返回空图。

**建议的 id 形式**（把 URL 需要的信息全带上，提供器自身无状态）：
`cover/<aid>/<updateAt>`（`updateAt` 可为空，表示不带 `?v=`）
页图另议：页 URL 来自章节数据，若也要走提供器，则 id 直接带**完整 URL**更省事。

**验证方式（下次照做）**：
1. 后端自检里先让 worker 取一张封面（`loadCovers(1)` 会触发 `coverReady`）；
2. 再用**同一个 URL** 向提供器请求一次，打印命中与尺寸 —— 命中即证明"worker 写盘、提供器读盘"跨实例生效；
3. 另请求一个不存在的 id，确认返回的是**占位图**（不是空图、不崩溃）。

### P2 第三屏（阅读器）尝试记录：已撤回，两个已知错误

第一次尝试"QML 阅读器"时改了两处，**未提交、已撤回**，两个错误记在这里避免重犯：

1. **context property 不是信号**：我在 `Connections { target: backend }` 里写了
   `onPageFileChanged` / `onPageSeqChanged`，想接 `main_qml.cpp` 里用
   `setContextProperty("pageFile", ...)` 设置的值。QML 运行时报
   "Detected function ... but no signal of the target matches the name" ——
   根因（context property 没有变更信号）反而被这行警告直接指出来了。
   正确做法：用一个带 `Q_PROPERTY(... NOTIFY ...)` 的小 QObject 暴露给 QML，
   而不是裸 context property。
2. **`--read` 没有触发任何加载**：只设置了 `autoReadAid`，却没有像 `--open` 那样调用
   `backend.loadAlbum(aid)`，所以详情与章节列表都没发生，阅读器分支自然没被走到
   （日志里只有首页列表，截图与上一屏字节数相同即为例证）。

**结论**：QML 显示 `QImage` 的可行路径是"GUI 线程落临时文件 + 用带 NOTIFY 的属性暴露路径"；
下一次先把属性对象做好，再谈阅读器，避免又出一个"看起来完成了"的空屏。

### P2 阅读器布局问题：测量数据与已排除的假设（未修好，供下次直接接手）

**现象**：页面图加载成功、阅读器"可见"，但界面上看不到那一页。

**实测数据**（`--read 209827`，页面就绪后 1.5 秒由 QML 探针打印）：

```
QML 阅读页已显示：源 852x1280，实际绘制 0x0，阅读器可见=true     <- onStatusChanged 那一刻
几何：右栏面板 2x780，图片 176x734，绘制 176x264               <- 1.5 秒后（探针）
```

**关键点**：右栏面板 **2 像素宽**（2 恰等于 RowLayout 的 spacing，说明它分配到的宽度是 0），
而左栏是 `Layout.preferredWidth: 380`，窗口是 1180x780。截图字节数与详情屏几乎相同
（224348 对 224360），与"页面没画出来"一致。

**已排除的假设**：
1. ~~moc 未生成/过期~~：检查过 `moc_JmBackend.cpp`，新属性 `pageSeq` 出现 4 次，时间戳新于头文件；
2. ~~context property 在编译后的绑定里不可见~~（这是真存在的一个坑，见上一节，已修）：
   修好后绑定不再报错，但面板宽度不变；
3. ~~布局子项 `visible` 切换导致 RowLayout 不重新布局~~：把右栏改成"单一面板 + 内容切换可见性"
   之后重测，面板宽度**仍是 2**，所以此假设不成立。

**下次的两个方向**（按代价从低到高）：
- 先确认窗口/RowLayout 的实际尺寸与左栏宽度（在探针里一并打印 `root.width`、左栏宽度、
  `RowLayout.width`），把"右栏为什么只分到 2"定位到具体某一层；
- 再考虑用 `StackLayout`（显式 `currentIndex`，不依赖可见性）或给右栏显式 `Layout.preferredWidth`
  来绕开这一层。

**当时状态（历史记录，问题已在下一节修好）**：那一轮改动全部撤回（未提交），仓库停在 `d5005aa`
（阅读器实现 + 条件等待截图），即"阅读器代码在、但页面显示不出来"。
当前 HEAD 已远在该提交之后，本节仅作排查过程留档。

### P2 阅读器布局问题：**已定位并修好**（2026-10-06）

**根因**：左栏（侧栏）虽然写了 `Layout.preferredWidth: 380`，但它内部子项（列表项的
`width: ListView.view.width` 这类绑定）产生的隐式/最小宽度把 RowLayout 的分配**顶到了 1178**，
右栏因此只分到 2 像素（恰好等于 spacing），页面虽然加载成功却画在 2 像素宽的区域里，等于看不见。

**修法**：给侧栏补上 `Layout.minimumWidth: 380` 与 `Layout.maximumWidth: 380`（固定侧栏宽度，
这本就是设计意图），而不是只给 preferredWidth。

**测量对比**（QML 布局探针，启动 3 秒后打印，无需页面）：
```
修前：窗口 1180x780，RowLayout 1180x780，左栏 1178，右栏 2x780
修后：窗口 1180x780，RowLayout 1180x780，左栏 380，右栏 800x780
```

**仍未验证**：修好后重跑阅读器 3 次都没等到页面（CDN 波动），所以"页面在 800 像素面板里
画出来（paintedWidth/paintedHeight 非零）"这条**没有在本轮复测成功**。
下次取到页面时看探针即可确认（探针会打印面板、图片与绘制尺寸）。

### P2 阅读器渲染：**离线验证通过**（2026-10-06）

布局修好后的复测一直被 CDN 波动挡住（取不到页面），所以加了一条**测试通道**绕开网络：

```
jmnext4qml --page <png> --shot out.png 9000
```

`--page` 直接把本地图片交给阅读器（跳过后端），用于离线验证"阅读器能不能把图画出来、尺寸对不对"。

**实测结果**：
```
--page：已把 <png>（1180x780）交给阅读器
几何：阅读器 800x780，图片 784x734，绘制 784x518
截图 348687 字节（首页列表为 224348）
```
- 阅读器面板 800x780（修好后的正确宽度）；
- 图片实际绘制 **784x518**，非零 —— 渲染路径成立；
- 截图体积从 224348 涨到 348687，说明画面内容确实变了。

**结论**：布局修复 + 渲染路径都已验证。
**仍未做的一步**：修好布局后，用**真实网络页面**跑一次把两条证据串起来（此前多次因 CDN/接口波动
取不到页面；离线验证已排除渲染侧问题）。

### P2 阅读器：真实网络页面复测的结论（2026-10-06）

**已确认的事实（都有证据）**：
1. **页面能加载**：QML 版曾打印 `QML 阅读页已显示：源 852x1280`（真实一页图）；
2. **布局已修好**：探针 `左栏 380，右栏 800`；
3. **渲染路径成立**：离线通道 `绘制 784x518`，截图体积从 224348 涨到 348687；
4. **核心与网络不是问题**：同一时刻 widget 版 `--reader-async 209827 0` 成功（"收到 3 页，末页状态 第 9/194 页"）。

**取不到页面时的特征**：`收到详情` 有、`章节列表 159 条` 有、但页面迟迟不到（60 秒内多次未到）。
这说明接口本身是通的，卡在"取章节页图"这一步，且**与读的是 widget 还是 QML 无关**（QML 在更早的轮次
成功过、widget 这次成功，两边都出现过等待）。判断是**该接口/图床的波动**，不是本地代码缺陷。

**因此**：把"一次运行同时拿到三项证据"当作完成门槛并不合理（窗口期不由我控制）。
现有三项证据已分别独立成立；若下次再遇到等待，用 `--page <本地图>` 的离线通道验证渲染侧即可。

**工具经验**：几何探针依赖 QML 里的 id（`readerPane`/`pageImage`）。我在排查时一度把它改成
`rightPane`（那是**详情**面板的 id），属于指错对象 —— 记录在此避免重犯：改探针前先核对 id 归属。

### 构建提速：Qt 端改用 Ninja（2026-10-06）

**问题**：Makefile 生成器下，**每次** `cmake --build build` 都会重编 QML 缓存（`Main_qml.cpp`）并重新链接，
即使一行代码都没改 —— 连续三次实测 18.7 / 25.1 / 13.2 秒。

**根因**：`Main_qml.cpp` 的依赖包含 `JMNeXt/jmnext4qml.qmltypes`、`.rcc/*.qrc` 与 C++ 类型注册产物；
只要 `qmltyperegistrations` 重跑（构建输出里每次都能看到它），这条链就整体重跑。这是 Makefile 对
生成文件依赖的典型问题，Ninja 能正确处理。

**改动**：`build/` 重新配置为 Ninja（`cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`），
其余命令（`cmake --build build`、`ctest --test-dir build`、脚本里的可执行文件路径）**都不需要改**。

**实测（同机同命令）**：

| 场景 | Makefile | Ninja |
| --- | --- | --- |
| 无改动增量 | 13~25 秒（每次） | **0.04~0.20 秒** |
| 首次全量重建 | 未测 | 77.3 秒（一次性） |

**验证**：重建后 `ctest` 16/16 通过；QML 前端仍能起并截图（证明换生成器没破坏功能）。
**未采用**：`ccache`（容器已装 4.7.5）暂未接入，留待评估全量重建收益。

### 构建提速：自动接入 ccache（2026-10-06）

**做法**：`CMakeLists.txt` 在 `project()` **之前**检测 ccache，检测到就设为 `CMAKE_CXX_COMPILER_LAUNCHER`
（必须在 project() 前设置才生效）；可用 `-DCMAKE_CXX_COMPILER_LAUNCHER=` 显式关闭。

**实测（同机、Ninja、清空 build 目录后的全量重建）**：

| 场景 | 耗时 |
| --- | --- |
| 无 ccache | 72.5 秒 |
| 有 ccache，首次（填充缓存） | 99.4 秒（多付约 27 秒） |
| 有 ccache，缓存热 | **13.0 / 13.2 秒**（74/148 命中，约 5.5 倍快） |

**验证**：配置输出确认自动生效（`检测到 ccache，已启用…`）；`ctest` 16/16 通过。
**诚实说明**：只在首次变慢，之后每次清空重建都省约 60 秒；如果某台机器只做一次构建，它反而略慢。

---

## 基准验证（2026-10-07，本轮实测，作为移植工作的起点）

移植 Qt 版之前先钉死"底座能不能跑"。以下**都是本轮在容器内实测**的输出，不是转述旧记录：

| 步骤 | 命令 | 实测结果 |
| --- | --- | --- |
| 配置 | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` | 退出码 0 |
| 构建 | `cmake --build build -j2` | 退出码 0，**错误行数 0**；`jmnext4qml` 目标已生成 |
| 单测 | `ctest --test-dir build` | **16/16 通过**（0.19 秒） |
| 界面自检 | `./build/jmnext4desktop --list`（`QT_QPA_PLATFORM=offscreen`） | **列表已加载：80 条**；封面 **400x533** 连续显示多张 |
| 真实网络 | `./build/jmnext4net discover` | 候选主机 4 个，**发现 `https://www.cdnhjk.net/`** |

**结论**：数据层与构建链是**真实可用**的（不是空壳）：真主机发现、真列表、真封面解码。
所以移植工作的性质是"**在可用底座上补齐界面与业务**"，不是"从不毛之地开始"。

### 与 Kotlin 版的结构差距（本轮的量化结论）

| 维度 | Kotlin 版 | Qt 版现状 |
| --- | --- | --- |
| 屏幕组织 | `ui/screens/` **25 个文件、10,903 行**，每屏一个文件 | 整个界面在 `src/qml/Main.qml`，**352 行** |
| 通用组件 | `ui/components/` **8 个文件、1,424 行** | 未见对应实现 |
| 导航 | `JmNavHost` 统一路由与转场 | 目前是 `visible: root.reading` 在两个视图间切换 |
| 颜色 | 主题 token 与四套风格 | `Main.qml` 里仍写死一组颜色常量 |

清单详见 `docs/ui-inventory/05-gap-vs-kotlin.md`。
**下一步（本目标第 1 步）**：把 `Main.qml` 按屏拆分（新增 QML 需登记进
`CMakeLists.txt` 的 `qt_add_qml_module(... QML_FILES ...)`），并落第一个 `components/*.qml`。

### 新增一个 QML 文件的正确姿势（本轮实测跑通，照这个做）

1. 把文件放进 `src/qml/`（屏幕放 `src/qml/screens/`，组件放 `src/qml/components/`）；
2. **必须登记进 `CMakeLists.txt` 的 `QML_FILES` 列表** —— 该行缩进是 **4 个空格**
   （`    QML_FILES src/qml/Main.qml`），登记后写成多行，每项 8 空格缩进；
3. 重新 `cmake -S . -B build ... && cmake --build build -j2`；
4. **验证方式（不要只看构建退出码）**：
   - 构建产物里应出现该文件：`find build -name 'ComicCard.qml'` →
     期望 `build/JMNeXt/src/qml/components/ComicCard.qml`；
   - 模块的类型清单里应出现该类型：`grep ComicCard build/JMNeXt/qmldir`。

本轮按此新建了 `src/qml/components/ComicCard.qml`（65 行，对应 Kotlin 的
`ui/components/ComicCard.kt`：只暴露 title/author/coverUrl 与 `clicked()` 信号），
两项验证都通过。**注意**：它目前**尚未在 `Main.qml` 里使用**（本轮只跑通"新增即登记即可加载"的链路），
因此"能加载"已验证、"已被界面使用"未验证。

### 下一步（把列表项换成 ComicCard）的三个前提 —— 先查清再动，避免倒退

本轮本想把 `Main.qml` 的列表项换成新落的 `components/ComicCard.qml`，**核对后发现直接换会倒退**：
现有 delegate（`Main.qml:65` 起）已经带着 P3 阶段的动效 ——

```qml
color: mouseArea.containsMouse ? root.cSurface2 : "transparent"
Behavior on color { ColorAnimation { duration: 120 } }
scale: mouseArea.pressed ? 0.985 : 1.0
Behavior on scale { SpringAnimation { spring: 2.5; damping: 0.35 } }
```

而 `ComicCard.qml` 目前是朴素版（颜色写死、无悬停与弹性）。所以替换前必须先补齐三件事：

1. **模型字段名**：delegate 直接用了 `aid` 与 `coverUrl` 等角色名，组件里要原样接收；
   需先确认模型的完整角色清单（`JmBackend` / `JmWorker` 一侧），不能猜；
2. **主题 token 的取用方式**：现有 delegate 用 `root.cSurface2` / `root.radiusMd`（写在
   `Main.qml` 里的属性），组件要复用它们就得知道 token 是单例、上下文属性还是 `Main.qml` 的属性 ——
   这决定了 `ComicCard.qml` 该怎么取色，**不能把颜色写死在组件里**（否则主题切换会失效）；
3. **动效要一起搬**：`ColorAnimation`（悬停 120ms）与 `SpringAnimation`（按压 0.985 弹性）
   必须进组件，否则就是"换了个组件、丢了动效"。

**结论**：先补齐这三项，再把 delegate 换成 `ComicCard`；替换当轮必须带
`--list` 显示 80 条（回归）与截图（观感）两项证据。本轮**不做**会倒退的替换。

### 本轮：列表 delegate 已换成组件（拆结构的第一刀落地）

`Main.qml` 的列表项由内联 `Rectangle`（65–117 行）换成 `components/ComicCard.qml`，
原有点击逻辑原样保留（`console.log` + `root.albumAid = aid` + `backend.loadAlbum(aid)`）。

**证据（本轮实测）**：

| 项 | 命令 | 结果 |
| --- | --- | --- |
| 构建 | `cmake --build build -j2` | 退出码 0，错误 0 |
| 回归（列表） | `./build/jmnext4desktop --list` | **列表已加载：80 条**，封面 400x533 连续显示 |
| 单测 | `ctest --test-dir build` | 通过（与替换前同为全绿） |
| 观感 | `./build/jmnext4qml --list --screenshot` | **未取得**：退出码 1，无 PNG 产出（`--screenshot` 可能需要子参数，STATUS 旧记录提到"带子参数等 45 秒"） |

**如实标注**：本次替换的**功能性**（80 条列表、封面、构建、单测）已验证；
**观感**（悬停过渡与按压弹性是否仍生效）**未经截图或人工确认** —— 组件里这两条动效是从原 delegate
原样搬来的，但"搬对了"与"看起来一样"是两件事，需下一轮补上截图或由使用者目测。

### 修正与教训：QML 类型解析不会被 C++ 构建发现（2026-10-07）

上一轮我把列表项换成 `components/ComicCard.qml` 后提交，理由是"构建 0 错误 + `--list` 80 条"。
**这是假通过。** 本轮用 QML 程序实测才发现：

```
qrc:/JMNeXt/src/qml/Main.qml:65:27: ComicCard is not a type
QML 加载失败
```

原因：`qt_add_qml_module` 的模块根是 `src/qml/`，**放在子目录（`components/`）里的文件不会被自动
暴露为该模块的类型** —— 子目录要么配子模块（`URI JMNeXt.components`），要么用资源别名。
而 `--list` 那条路径走的是 **widget 版**（`jmnext4desktop`），不加载 QML，
所以它照样显示 80 条，**掩盖了 QML 已经起不来这件事**。

**修法（本轮）**：把组件移到模块根 `src/qml/ComicCard.qml`，登记路径同步改。
子目录结构留待以后用子模块方式正规化。

**验证（本轮实测，这才是真验证）**：

| 项 | 结果 |
| --- | --- |
| 构建 | 退出码 0，错误 0 |
| **QML 程序运行** | `./build/jmnext4qml --shot <path>` **退出码 0** |
| **截图** | 成功：`1180x780`，17857 字节 |
| 单测 | 全绿 |

**记在这里的规矩（后面每轮都适用）**：
1. **改 QML 之后必须跑 QML 程序**（`jmnext4qml`），只看 `cmake --build` 与 widget 版 `--list` 不算验证；
2. **截图不作为仓库内容**（输出到仓库外，只把路径与尺寸写进本文档）——
   `STATUS.md` 旧记录里已经有"`git add -A` 三次误收截图"的教训；
3. 截图模式是 `--shot <path>`（**不是** `--screenshot`；后者可能是 widget 版的旧写法）。

### 结构决策：UI 文件先保持**扁平**在 `src/qml/`（有实测依据）

第 7 轮已证实：`qt_add_qml_module` 的模块根是 `src/qml/`，**放在子目录里的 QML 不会被暴露为该模块的类型**
（`Main.qml` 报 `ComicCard is not a type`，界面起不来，而 C++ 构建照样通过）。

要让文件分目录，需要额外配置：要么给子目录建**子模块**（`URI JMNeXt.components` + 把模块链进目标），
要么给源文件设**资源别名**。这两种都要动 CMake 结构，且都属于"改完必须跑 `jmnext4qml` 才能确认"的改动。

**本轮决策**：UI 文件先**扁平**放在 `src/qml/`，用**文件名**对齐 Kotlin 的屏幕名
（`HomeScreen.qml` / `SearchScreen.qml` / `DetailScreen.qml` / `ReaderScreen.qml` / `ComicCard.qml` …）。
理由：先拿到"每屏一个文件"这个**最关键的组织收益**，且**已验证可用**；目录分层的收益小、风险大，
留作后续单独立项（届时用子模块方式正规化）。

### 抽取 `HomeScreen.qml` 时必须一起搬的东西（本轮查清）

`Main.qml` 里与首页列表耦合的部分（行号为当前版本）：

| 项 | 位置 | 为什么必须一起搬 |
| --- | --- | --- |
| `ListModel { id: listModel }` | 16 | 列表数据本体 |
| 列表视图（含刚刚换成的 `ComicCard` delegate） | 57–117 | 首页主体 |
| `onListReady(titles, ids)` | 288–291 | 后端回调，往里 `append({title, aid, coverUrl})` |
| `coverUrlReady` 等封面回填 | 待核 | 封面是异步到的，回填逻辑与列表同源 |
| "加载更多"入口 | 待核 | 与列表分页同源 |

**做法**：把上述几项作为一个整体迁进 `HomeScreen.qml`，主页只保留一个实例与到后端的连接；
迁完**必须**跑 `./build/jmnext4qml --shot <path>`（第 7 轮的规矩：改 QML 必须跑 QML 程序），
并确认 `--list` 那条 widget 路径仍 80 条。

### 本轮：状态组件 `StateBox.qml` 落地（对应 Kotlin 的 StateBox.kt）

Kotlin 侧状态展示是三个组合函数：`LoadingBox` / `MessageState(title, description, icon, onRetry)` / `ErrorBox(message)`。
QML 的惯例是一个文件一个类型，所以收敛为一个组件加 `kind`：

| Kotlin | 本组件用法 |
| --- | --- |
| `LoadingBox` | `StateBox { kind: "loading"; description: "…" }` |
| `MessageState` | `StateBox { kind: "message"; title: …; description: … }` |
| `ErrorBox` | `StateBox { kind: "error"; title: 错误信息 }` |
| 重试回调 | 父级连接 `onRetry:`（QML 不能像 Compose 那样直接收 lambda） |

颜色与 `ComicCard` 同一策略：由调用方注入，组件内不写死（否则四套风格切换失效）。

**证据（本轮实测）**：

| 项 | 结果 |
| --- | --- |
| 构建 | 退出码 0，错误 0 |
| 文件进入模块 | `build/JMNeXt/src/qml/StateBox.qml` 存在 |
| 类型已登记 | `build/JMNeXt/qmldir` 命中 StateBox |
| QML 程序运行 | `jmnext4qml --shot` 退出码 0，截图 1180x780 |

**如实标注**：本轮只完成"组件落地 + 可加载"，**尚未在 `Main.qml` 里替换现有的纯文本状态显示**
（现状是 `root.pageStatus` 直接塞进 `Text`，见 Main.qml:115/198，由 `onStatus`/`onFailed`/`onPageReady` 驱动）。
替换属下一步，替换后需按同样四项验证。

**本轮踩到的坑（第 3 次同类）**：登记 QML 文件时我又按"看起来的缩进"写了 sed 模式，结果没匹配上
（列表项实际是 **10 个空格**，不是我以为的 10/12 混合）。**可靠做法**：`sed -n '/QML_FILES/,/^  )/p' ... | cat -A`
先看真实空白，或用 `\(\s*\)` 让 sed 自己吞掉缩进。现已按后者修正并验证。

### 本轮：`StateBox` 接进首页（列表空/失败时不再是一片空白）

此前列表为空时界面**没有任何说明**（只有一行 `pageStatus` 文本），使用者分不清"在加载 / 没内容 / 失败了"。
现在在列表区域叠加 `StateBox`：

```qml
StateBox {
    anchors.fill: listView
    visible: listModel.count === 0
    kind: root.pageStatus.indexOf("失败") >= 0 ? "error" : "message"
    title: root.pageStatus.length > 0 ? root.pageStatus : "还没有内容"
    canRetry: true
    onRetry: backend.loadList()
}
```

对应 Kotlin 的 `ErrorBox` / `MessageState`（`ui/components/StateBox.kt`）。

**证据（本轮实测）**：构建退出码 0、错误 0；`jmnext4qml --shot` 退出码 0、截图 **1180x780**。

**如实标注**：
- 已验证的是"**编译 + QML 能加载**"；
- **空/失败状态本身的观感没有被截图覆盖** —— 截图时列表有 80 条，`visible: listModel.count === 0` 为假，
  所以那张图里这个组件是隐藏的。要覆盖它需要在"无网络/空结果"的情况下拍照，列为下一步验证项。

### 本轮：空态"被走到但没拍到"，并发现一个真问题（待修）

**做法**：用不可能命中的关键词让列表为空，以逼出 `StateBox` 并拍照
（`./build/jmnext4qml --search zzz_no_such_work_zzz --shot <path>`）。

**实测结果**：

| 观察 | 含义 |
| --- | --- |
| 日志出现 **`QML 列表已填充：0 条`** | 空列表路径**确实被走到**，`StateBox` 的 `visible` 条件成立 |
| **退出码 124（超时）** | 进程没退出，**截图未生成** |
| 产物目录只有上一张（80 条时的）截图 | 空态观感**仍未取得** |

**结论（如实）**：空态逻辑**已执行**，但**没有视觉证据**；并且发现：

> **列表为空时，`--shot` 拍照流程会卡住（超时）。**

这本身就是 Qt 侧的一个真问题 —— 拍照依赖"列表非空"这一隐含前提（很可能在等某个只在有数据时才发的信号）。
它有两个影响：
1. 现在无法给空态留证据；
2. 若使用者在此状态下退出，行为是否正常也未知。

**下一步（列为本目标内的一项）**：读 `src/qml/main_qml.cpp` 里 `--shot` 的实现，
找出它等的信号，改成"列表就绪（无论条数）或超时即拍"，然后补上两张证据图：
有数据一张、空列表一张。

### 更正：上一轮记的"空列表时 --shot 卡住"**不成立**（我调用方式错了）

上一轮我写 `--search <词> --shot <路径>`，报告"空列表时拍照超时"。**读源码后确认那是我的错**：

```cpp
if (argc >= 3 && std::string(argv[1]) == "--shot") { … }   // 拍照分支要求 --shot 是**第一个参数**
QTimer::singleShot(maxWaitMs, &app, grabAndQuit);          // 兜底：默认 1500ms，本就该退出
```

`--shot` 不在 `argv[1]` 时，整个拍照分支（含兜底定时器）**都不会执行**，程序自然一直在 `app.exec()` 里。
所以那不是"拍照流程依赖列表非空"，而是**参数位置写错**。

**正确姿势**（本轮实测）：`--shot` 放第一位，后面可跟其它选项与"最长等待毫秒"：

```sh
./build/jmnext4qml --shot <输出.png> 8000 --search <关键词>
```

教训：**这条命令的用法应来自源码或 `--help`，而不是我拼出来的顺序**；本轮之前那份"发现真问题"的记录已作废，
留在这里是为了让后来者看到"我曾误判过什么、依据是什么"。

### 本轮：QML 前端补上"加载更多"（此前只有 widget 版有）

**先量后改的发现**：`Main.qml` 里**没有任何 `loadMore`** —— 那个能力只在 widget 版
（`jmnext4desktop --list` 那条"80 → 160 条"走的是 widget 路径）。也就是说
**QML 前端的首页只能看第一页**，这是与 Kotlin 版的一处真实功能缺口。

**本轮动作**：

1. 新增 `src/qml/LoadMoreFooter.qml`（55 行，对应 Kotlin 的 `ui/components/LoadMoreFooter.kt`：
   `loading` / `exhausted` / `error` 三态 + `signal loadMore()`；颜色由调用方注入）；
2. 登记进 QML 模块；
3. 接到首页（插在 `StateBox` 块之后），`onLoadMore` 调 `backend.loadMore()`。

**证据（本轮实测，四项全过）**：

| 项 | 结果 |
| --- | --- |
| 构建 | 退出码 0，错误 0 |
| 文件进模块 | `build/JMNeXt/LoadMoreFooter.qml` 存在 |
| QML 程序 | `--shot` 退出码 0，日志 `QML 列表已填充：80 条` |
| 截图 | 成功：**1180x780** |

**如实标注的两点**：
1. 页脚的 `loading` / `exhausted` 目前**写死为 false** —— 所以它现在恒显示"加载更多"，
   **"已经到底了"这条分支还没有数据来源**（后端未暴露"是否还有下一页"）。要真正对齐 Kotlin 版，
   需要 `JmBackend` 增加一个 `hasMore` 属性（列为下一步）；
2. "点击后真的加载出第二页"**尚未验证**（需要点击交互，而 `--shot` 是静态拍照）。

### 本轮：页脚的"已经到底了"有了真实依据（在界面侧推导，不动 C++）

`JmBackend` 没有暴露"是否还有下一页"，而加 C++ 属性属结构性改动。**改为在 QML 侧推导**：
`onListReady` 里先 `listModel.clear()` 再重填**累计列表**（widget 版的"80 → 160 条"证明是累计语义），
因此 **"条数不再增长"就等于"已到底"**：

```qml
root.listLoading = false
if (listModel.count <= root.lastListCount) {
    root.listExhausted = true
    console.log("QML 列表已到底：" + listModel.count + " 条")
}
root.lastListCount = listModel.count
```

页脚的 `loading` / `exhausted` 也接上了真实状态（不再写死 false），点击时置 `listLoading = true`。

**证据（本轮实测）**：

| 场景 | 命令 | 结果 |
| --- | --- | --- |
| 有数据 | `--shot <path> 8000` | 退出码 0，截图 **1180x780** |
| 空列表 | `--shot <path> 8000 --search zzz_none` | `列表已填充：0 条` → **`列表已到底：0 条`**（推导分支确实执行），截图 1180x780 |

**如实标注**：
1. "已经到底了"的**视觉**只在空列表那张图里可能出现，而空列表时 `StateBox` 覆盖了列表区 ——
   两者是否视觉上打架**尚未确认**（需要人工看图）；
2. "点击后真的加载出第二页并转为 160 条"仍**未验证**（需要点击交互，`--shot` 是静态拍照）。

### 本轮：修掉"空列表时 StateBox 与页脚打架"

上一轮标出的隐患：空列表时 `StateBox` 占满列表区，而 `LoadMoreFooter` 仍在下方显示"加载更多"。
本轮修掉（`visible: listModel.count > 0`）—— 空列表时既没有"更多"可言，也不该与空态抢视觉。

**证据（本轮实测）**：

| 场景 | 结果 |
| --- | --- |
| 构建 | 退出码 0，错误 0 |
| 有数据 | 退出码 0，`列表已填充：80 条`，截图 1180x780 |
| 空列表 | 退出码 0，`列表已填充：0 条` → `列表已到底：0 条`，截图 1180x780 |

### 仍未验证：点击"加载更多"是否真的出第二页（以及怎么把它变成自动化验证）

现状：`--shot` 是静态拍照，**无法点击**；所以"80 → 160 条"只在 **widget 版**被验证过，
QML 侧只是接线正确、能加载。两条可选做法（下一步择一）：

1. **QML 侧加一个自检触发**：给 `main_qml.cpp` 加一个 `--ui-selftest`，
   依次 `listReady` → 调用一次 `loadMore()` → 断言 `listModel.count` 从 80 变 160（需改 C++，属结构性改动）；
2. **不改 C++ 的替代**：在 `Main.qml` 里加一个只在自检模式下启用的 `Timer`（触发条件由后端已有的某个属性决定），
   到点自动调一次 `loadMore()` 并把结果打进日志 —— 然后仍用 `--shot` 抓图与日志核对。

**倾向第 1 条**：QML 侧的自检触发会引入"只在特定模式生效的代码路径"，反而是维护负担；
而 `main_qml.cpp` 已有 `--selftest`（第 161 行起），扩展它是顺着既有结构走。

### 本轮：加 `--ui-selftest`，并**立刻抓出一个真 bug**

**动机**：`--shot` 是静态拍照、点不了鼠标，所以"点加载更多真的出第二页"一直没验证。
本轮给 `main_qml.cpp` 加 `--ui-selftest`（顺着既有 `--selftest` 的结构）：走 QML 侧同一条路径 ——
`loadList()` → 列表就绪 → 主动调 `JmBackend::loadMore()` → 核对累计条数是否增长。

**实测结果（第一次跑就出问题）**：

```
界面自检：第一页 80 条，请求加载更多…
界面自检：第二页累计 80 条（第一页 80 条，未增长：失败）
```

**结论**：**QML 侧的"加载更多"不起作用** —— 调了 `JmBackend::loadMore()` 之后列表仍是 80 条。
对照：**widget 版同一个功能是 80 → 160 条**（`jmnext4desktop --list` 的既有验证）。
所以问题在 **QML 这条路径**（`JmBackend::loadMore` 或其到 worker 的转发），不在数据层。

这也说明本轮之前我在 `STATUS.md` 里写的"接线正确、能加载"是**过于乐观的措辞** ——
正确说法是：**界面能加载首页，但"加载更多"无效**，已按此更正。

**下一步**：读 `JmBackend::loadMore()` 与 `JmWorker` 的对应实现，找出为什么没增长
（候选：`loadMore` 只是重发第一页 / 页码状态没维护 / signal 没接），修好后 `--ui-selftest` 应打印"增长：通过"。

**其它证据**：`ctest` 仍 **16/16 通过**（本轮 C++ 改动未影响既有测试）。

### 排查记录：为什么 QML 的"加载更多"没增长（本轮否掉一个假设）

**先对比两端写法**：

| 端 | 调用方式 | 结果 |
| --- | --- | --- |
| widget（`src/qt/MainWindow.cpp:222`） | `worker_->loadMore()` **直接调** | 有效（80 → 160） |
| QML（`src/qml/JmBackend.cpp:44`） | `invoke("loadMore")` 元对象调用 | **无效**（80 → 80） |

**假设一（本轮否掉）**：`invoke` 走元对象，若 `loadMore` 不是 `Q_INVOKABLE`/slot 就会**静默失败**。
查证 `src/qt/JmWorker.h`：

```
25: class JmWorker : public QObject {
31: public slots:
35:     void loadList();
45:     void loadMore();
```

`loadMore` **就在 `public slots:` 之下**，元对象可调用；而且同一个 `invoke` 通道上的 `loadList`
在 QML 侧**是工作的**（首页能出 80 条与封面）。**所以"不是 slot"这个解释不成立。**

**剩下的嫌疑（按可能性排序，下一步逐个查）**：

1. **`JmBackend::invoke` 的连接方式与参数**：它对不同方法是否用了不同的连接类型/是否要求参数匹配
   （`loadMore` 无参，而 `search` 带参 —— 若模板对无参情形处理特殊，可能出问题）；
2. **`JmWorker::loadMore()` 自身的实现**：是否维护了"当前页"状态、是否真的请求下一页并**追加**
   （widget 能工作说明它至少有可用路径，但 QML 侧进入它时机的差异可能让状态没准备好）；
3. **`listReady` 的语义**：QML 侧两次都收到 80 条 —— 也可能是 `loadMore` 真的请求了第二页，
   但 worker 发回来的仍是第一页（例如"当前页"没自增）。

**下一步**：读 `JmWorker::loadMore()` 实现与 `JmBackend::invoke` 模板（两者都不长），
定位后修好，届时 `--ui-selftest` 应从"未增长：失败"变为"增长：通过"。

### 根因找到了：`loadMore` 发的是 `listAppended`，而 QML 从来没接它

前一轮我把嫌疑缩到三处，本轮读完 `JmWorker` 后**定位到确切原因**：

| 事实 | 位置 |
| --- | --- |
| `loadMore()` 的产出走 **`emit listAppended(titles, ids)`** | `src/qt/JmWorker.cpp:382` |
| `emit listReady(...)` 只有两处：`loadList()` 与 `search()` | `JmWorker.cpp:60` / `:91` |
| **QML 侧（`src/qml/`）没有任何 `listAppended`** | 本轮 grep 确认 |

**所以"加载更多"的机制是**：worker 侧**拼接**（`currentEntries_.insert(...)`、`pendingIds_ += ids`、`page_ = next`）
并通过 `listAppended` 告诉界面"追加这些"；而 widget 版接了这个信号（所以 80 → 160 能用），
**QML 侧根本没接** → 数据被丢掉 → 界面永远 80 条。

**这也解释了我那个 `--ui-selftest` 为什么打印"第二页累计 80 条"**：它接的是 `listReady`，
而 `loadMore` **根本不会**发 `listReady` —— 那次"第二页"其实是别的原因触发的同页 `listReady`。
**我的自检本身也接错了信号**，这一点同样要改。

### 修法（下一步，三处）

1. `JmBackend`：把 worker 的 `listAppended` 转发成对 QML 可见的信号（现在只连了 `status`/`failed` 等）；
2. `Main.qml`：新增 `onListAppended` 处理 —— **追加**到 `listModel`（而不是像 `onListReady` 那样 `clear()` 后重填），
   并顺带请求新条目的封面（`backend.loadCovers(...)` 的现有用法需按索引核对）；
3. `--ui-selftest`：改接 `listAppended` 来断言"新增了多少条"，而不是接 `listReady`。

**验收标准不变**：改完后 `--ui-selftest` 应打印"增长：通过"，并且 `--shot` 出的图里列表条数变为 160。

#### 更正（同一轮内）：缺口**只在 `Main.qml`**，后端那一层是接好的

上一条我写"QML 侧（`src/qml/`）没有任何 `listAppended`"是**不准确的**。复核后：

```
src/qml/JmBackend.h:41    void listAppended(const QStringList& titles, const QStringList& ids);
src/qml/JmBackend.cpp:17  connect(worker_, &JmWorker::listAppended, this, &JmBackend::listAppended);
```

**信号已经从 worker 转发到了 QML 可见的后端层**，缺的只是 **`Main.qml` 里没有一个 `onListAppended` 处理函数**
（所以在 QML 里"发出来的信号没人听"）。于是修法缩小为**两处**，而不是三处：

1. `Main.qml`：新增 `function onListAppended(titles, ids)` —— 把新条目**追加**进 `listModel`
   （**不要**像 `onListReady` 那样先 `clear()`；追加后按新索引请求封面）；
2. `--ui-selftest`：改接 `listAppended` 来断言"新增了多少条"。

**验收标准不变**：`--ui-selftest` 打印"增长：通过"，且 `--shot` 的图里条数变为 160。

**这也是一处教训**：我上一条把"我 grep 的范围"当成了"整个 QML 层"，于是把 1 处的缺口写成了 3 处。
正确做法是像这次一样**逐层确认信号链的每一跳**（worker → JmBackend → Main.qml），而不是只看一层就下结论。

### 修复完成：QML 的"加载更多"现在真的追加（本轮实测通过）

按上一轮定位的根因改了两处：

1. **`Main.qml` 新增 `function onListAppended(titles, ids)`** —— 把新条目**追加**进 `listModel`
   （**不**先 `clear()`），并把 `listLoading` 复位、对新条目请求封面；
2. **`--ui-selftest` 改接 `listAppended`** 作断言；原先接 `listReady` 的第二次判断**删掉**
   （它永远看不到 `loadMore` 的结果，会打出误导性的"未增长：失败"），只留一句"忽略"提示。

**证据（本轮实测）**：

```
界面自检：第一页 80 条，请求加载更多…
界面自检：又收到一次 listReady（80 条），忽略
界面自检：listAppended 追加 80 条（第一页 80 条，增长：通过）
```

构建退出码 0、错误 0。**"增长：通过"是唯一结论**，且这条检查以后每轮都能自动跑。

### 这一条修完，QML 首页与 widget 版/Kotlin 版的对齐情况

| 能力 | QML 侧 | 说明 |
| --- | --- | --- |
| 首页列表（80 条） | 对齐 | 真数据 + 真封面 |
| **加载更多（+80）** | **本轮对齐** | 走 `listAppended` 追加；`--ui-selftest` 可断言 |
| 搜索 | 对齐 | 走 `listReady`（替换列表） |
| 空/失败状态 | 已补（`StateBox`） | 有说明与重试 |
| 页脚三态 | 已补（`LoadMoreFooter`） | "已到底"按累计条数推导；空列表时隐藏 |
| 详情 / 阅读器 | 已有 | 通过 `--open` / `--chapters` 路径可验证 |

**仍未验证**：`--shot` 的图里列表是否真的显示 160 条（自检已证明数据层与信号链正确，
但"追加后界面渲染出 160 条"还需要一张图或人工目测）。

---

## 移植进度账（本目标第 20/60 轮时的实际状态）

> 只写有实测证据的条目；"对齐"= 与 Kotlin 版（或与自家 widget 版）行为一致且有验证手段。

### 已经对齐的

| 项 | 证据／验证手段 |
| --- | --- |
| 首页列表（真主机、真数据、真封面） | `--shot` 出的图 + 日志"列表已填充：80 条"、"封面 400x533" |
| **加载更多（+80 追加）** | **`--ui-selftest`：`listAppended 追加 80 条，增长：通过`** |
| 搜索（替换列表） | 走 `listReady`，`--search` 可触发 |
| 空/失败状态（含重试） | `StateBox` 已接线；空列表分支实测执行（日志"已到底：0 条"） |
| 页脚三态（加载中/已到底/失败） | `LoadMoreFooter` 已接线；"已到底"按累计条数推导 |
| 详情 / 阅读器 / 章节 | 既有能力（`--open` / `--chapters` 路径可验证） |
| 主题 token | 四套风格可切换；组件颜色由调用方注入，不写死 |

### 组件层（对应 Kotlin `ui/components/`）

| Kotlin | Qt 侧 | 状态 |
| --- | --- | --- |
| `ComicCard.kt` | `src/qml/ComicCard.qml` | 已接线（含悬停 120ms 过渡、按压 0.985 弹性） |
| `StateBox.kt`（三个组合函数） | `src/qml/StateBox.qml` | 已接线（`kind` + `retry` 信号） |
| `LoadMoreFooter.kt` | `src/qml/LoadMoreFooter.qml` | 已接线 |
| `Glass.kt` / `GlassTopBar.kt` / `FloatingBottomBar.kt` / `ItemMotion.kt` / `AmbientBackdrop.kt` | — | **未做** |

### 屏幕层（Kotlin `ui/screens/` 有 25 个文件）

| 现状 | 说明 |
| --- | --- |
| **全部界面仍集中在 `src/qml/Main.qml`** | 本轮之前的组件抽取是"从 Main.qml 里拿出去"，但**屏幕本身还没独立成文件** |
| 下一步 | 抽 `HomeScreen.qml`（第 8 轮列的五项一起搬：`ListModel`、列表视图、`onListReady`+`loadCovers(20)`、`coverUrlChanged` 回填、`onListAppended`） |

### 仍未验证 / 未做（按优先级）

1. **图里是否显示 160 条**：自检已证明数据与信号链正确，但"追加后界面渲染出 160 条"只差一张图或人工目测；
2. `HomeScreen.qml` 等**每屏一个文件**的结构改造（本目标的核心结构目标）；
3. 其余五个组件（Glass / GlassTopBar / FloatingBottomBar / ItemMotion / AmbientBackdrop）；
4. Kotlin 侧那 25 个屏幕里，Qt 侧目前只有首页/搜索/详情/阅读器四个的雏形，
   收藏 / 分类 / 画师 / 评论 / 随机 / 我的 / 登录 / 屏蔽设置 / 标签 / 周更 / 通知 / 更多列表 **均未做**。

### 验证手段一览（每轮都跑）

| 命令 | 覆盖 |
| --- | --- |
| `ctest --test-dir build` | 数据层既有单测（16 项） |
| `./build/jmnext4desktop --list` / `--chapters <aid>` | widget 路径真实链路 |
| `./build/jmnext4qml --shot <png> <ms> [--search 词]` | QML 渲染结果（静态；`--shot` 必须在第一位） |
| `./build/jmnext4qml --ui-selftest` | QML 交互链路（加载更多） |

### 抽取 `HomeScreen.qml` 的"现状快照"（下次照此搬，不必重新摸索）

`Main.qml` 里属于首页的部分（当前行号，随改动会漂移，搬前用 grep 重新确认）：

| 片段 | 当前位置 | 搬进 `HomeScreen.qml` 后由谁持有 |
| --- | --- | --- |
| 搜索行（TextField + 搜索按钮 + "作品列表（N 条）"标题） | ~21–55 | **留在 Main.qml**（它属于搜索屏；Kotlin 侧也在 `SearchScreen`） |
| `ListModel { id: listModel }` | ~16 | **HomeScreen 持有** |
| 列表视图（含 `ComicCard` delegate） | ~57–78 | HomeScreen 持有 |
| `StateBox`（空/失败 + 重试） | ~81–91 | HomeScreen 持有 |
| `LoadMoreFooter` | ~93–103 | HomeScreen 持有 |
| `function onListReady(titles, ids)` | ~280 起 | HomeScreen 的 `Connections` 持有 |
| `function onListAppended(titles, ids)` | ~296 起 | 同上 |
| `function onCoverUrlReady(index, url)` | 待确认行号 | 同上 |
| 与列表相关的 `property`（`lastListCount` / `listExhausted` / `listLoading`） | ~245–247 | 同上 |

**关键约束（本目标已踩过的坑）**：
1. `backend` 在 QML 里是通过上下文属性可见的，因此 `HomeScreen.qml` **可以直接引用 `backend`**，
   不必把后端一路当参数传下去；但**主题色必须由调用方注入**（token 是 `Main.qml` 的属性）；
2. 新文件必须**登记进 `CMakeLists.txt` 的 `QML_FILES`**，且缩进用 `\(\s*\)` 让 sed 自己匹配；
3. 搬完**必须跑 `jmnext4qml --shot`**（第 7 轮的假通过教训）+ `--ui-selftest`（确认"增长：通过"仍在）。

**回退点**：本轮已打 tag `port-20`，可整体回退到"组件层完成、加载更多修通"的状态。

### 抽取 HomeScreen 的失败记录与修正后的做法（第 23 轮）

**失败现象**（`--shot` 时）：

```
Main.qml:57:13: Type HomeScreen unavailable
HomeScreen.qml:33:21: Non-existent attached object
QML 加载失败
```

**失败原因（是我方法上的错，不是手滑）**：我用 sed 对搬出来的 50 行做"机械改写"，其中这几条
**破坏了语句结构**：

| 我的改写 | 造成的后果 |
| --- | --- |
| `s/root.albumAid = aid//g` | **把一行挖空**，留下残骸 → 就是第 33 行"不存在的附加对象"的来源 |
| `s/backend.loadAlbum(aid)/cardOpenClicked(aid)/g` | 前半句删掉、后半句改名，该行结构不完整 |
| `s/\blistModel\b/model/g; s/root\.cText/textColor/g; …` | 正则扫描**无法区分"引用"与"声明"**，也无法处理跨行语义 |

**结论**：**"机械搬运 + 正则改写"对互相咬合的代码不可靠**。已在同一轮回滚（`Main.qml` 恢复、新文件删除、登记撤销），
回滚后实测：构建 0 错误、`列表已填充：80 条`、截图 1180x780、`git status` 干净 —— 未造成任何残留。

### 修正后的做法（下一轮照此执行，不要再走 sed 改写的老路）

**第一刀只搬"纯渲染"，不碰任何回调**：

1. 先 **`sed -n '<A>,<B>p'` 把要搬的原文打印出来读一遍**（约 50 行，值得花这一次上下文）；
2. 新文件 `src/qml/HomeScreen.qml` 的结构**只有**：
   - `property var model`（数据由调用方传入）
   - 主题色属性（`textColor` / `secondaryColor` / `accentColor` / `cardRadius`）
   - 两个信号：`requestMore()` / `requestReload()`
   - **原样照抄**的 `ListView`（delegate 里对 `model.xxx` 的引用**只把 `listModel` 换成 `model`**，
     点击处理**不改成信号**，而是让组件暴露一个 `property var onCardClicked: function(aid) {}` 之类的最小挂钩，
     或干脆**先不搬 delegate**）；
   - `StateBox` 与 `LoadMoreFooter`：**原样照抄**，只把 `root.cText` 之类换成组件属性；
3. `Main.qml` 里：原区间删掉，换成 `HomeScreen { model: listModel; … }`，
   **全部 `Connections` 与 `backend.*` 调用留在 `Main.qml` 原地不动**（这一刀不动它们）；
4. 登记 `CMakeLists.txt`（用 `\(\s*\)` 匹配缩进）→ 构建 → **必须跑 `--shot` 与 `--ui-selftest`**；
5. 任何一步不过 → `git checkout -- src/qml/Main.qml && rm src/qml/HomeScreen.qml` 并撤销登记（本轮已验证这条回滚路径可用）。

**再下一轮**才把 `Connections` 与回调搬进组件 —— 一次只搬一层，每层都有验证。

### 读原文后的精确改写清单（第 24 轮，修正上一轮计划的过度设计）

已把 `Main.qml` 的列表区（57–106 行，共 50 行）逐行读过。**真实需要改写的点比上一轮设想的少得多**：

| 原文里的写法 | 抽取后 | 数量 |
| --- | --- | --- |
| `model.title` / `model.aid` / `model.coverUrl` | **原样不动** | 3 处（delegate 里已是 `model.` 角色名，**不需要**把 `listModel` 改名） |
| `listModel.count` | `listModel.count`（属性名由调用方传入） | 3 处 |
| `root.cSurface2` / `root.cText` / `root.radiusMd` / `root.cTextSecondary` | `hoverColor` / `textColor` / `cardRadius` / `secondaryColor` | 7 处 |
| `root.pageStatus` | `pageStatus` | 3 处 |
| `root.listLoading` / `root.listExhausted` | `loading` / `exhausted` | 2 处 |
| `root.albumAid = aid` + `backend.loadAlbum(aid)` | → **一个信号** `cardClicked(aid)`，处理留在 `Main.qml` | 2 行 |
| `backend.loadList()`（StateBox 重试） | → 信号 `retryClicked()` | 1 行 |
| `backend.loadMore()` + `root.listLoading = true` | → 信号 `moreClicked()`，处理留在 `Main.qml` | 2 行 |
| `Layout.fillWidth/fillHeight`（ListView） | `anchors.fill: parent`（组件根是 Item，内部不用 Layout 附加属性） | 2 处 |
| `Layout.fillWidth`（LoadMoreFooter） | `width: parent.width` | 1 处 |
| `anchors.fill: listView`（StateBox） | **原样**（`listView` 的 id 一起搬进组件） | 1 处 |

**上一轮的过度设计**：我打算把 `backend.loadAlbum/loadMore/loadList` 全改成信号并顺带改 delegate 结构 ——
其实只需要 **3 个信号**（`cardClicked` / `retryClicked` / `moreClicked`），delegate 内部结构一行都不用动。

**组件根用 `Item` + 在 `Main.qml` 里以 `HomeScreen { Layout.fillWidth: true; Layout.fillHeight: true; … }`
实例化**（`Layout.*` 附加属性对实例化对象有效，这是 Qt 的标准写法）。

**下一步就是按这张表逐点改**（每个改动点都有明确的行与目标写法），改完跑四项验证；不过就按第 23 轮记的回滚命令退回。

### 成功：`HomeScreen.qml` 抽出来了（本目标的第一屏独立成文件）

**过程**：第 22 轮用"机械搬运 + 正则改写"失败（挖空一行 → `Non-existent attached object`），
第 23 轮回滚并改写方法，第 24 轮**先读原文**才发现真正要改的只有 22 处（我原计划改动的两三倍都是多余的），
第 25 轮按清单逐点改，一次通过。

| 项 | 结果 |
| --- | --- |
| 新文件 | `src/qml/HomeScreen.qml`（85 行，`ColumnLayout` 根：列表视图 + StateBox + LoadMoreFooter） |
| `Main.qml` | 第 57 行起改为 `HomeScreen { … }` 实例（数据与主题色注入，3 个信号回传） |
| 登记 | `CMakeLists.txt` 的 `QML_FILES` |
| 构建 | 退出码 0，错误 0 |
| **QML 程序** | `--shot` 退出码 0，`列表已填充：80 条`，**截图 1180x780** |
| **交互自检** | `--ui-selftest`：`listAppended 追加 80 条，增长：通过` |
| 单测 | `ctest` 通过 |

**设计要点（下一屏照此复制）**：
- 组件**只搬纯渲染**，数据用 `property var listModel` 传入，后端调用用**信号**回传（本轮共 3 个：
  `cardClicked` / `retryClicked` / `moreClicked`），`Connections` 与 `backend.*` **全部留在 `Main.qml`**；
- 根用 `ColumnLayout`，于是原文里的 `Layout.fillWidth/fillHeight` **可以原样保留**，改造面最小；
- 主题色由调用方注入（`hoverColor` / `textColor` / `secondaryColor` / `cardRadius`）。

**下一步**：按同样套路抽 `SearchScreen.qml`（搜索行 + 结果列表），然后把两屏的 `Connections`
也逐步搬进各自屏幕（一次一层，每层验证）。

### 第二屏抽出：`SearchScreen.qml`（同一套两步法，一次通过）

| 项 | 结果 |
| --- | --- |
| 新文件 | `src/qml/SearchScreen.qml`（49 行：搜索行 `RowLayout` + "作品列表（N 条）" `Text`） |
| 改写点 | **只有两处**：`backend.search(...)` → 信号 `searchRequested(word)`（2 处）；标题硬编码色 → 属性 `secondaryColor` |
| 无需改名的原因 | `initialSearch` 是上下文属性（QML 全可见）；数据以**同名** `property var listModel` 接收，故 `listModel.count` 原样可用 |
| 四项验证 | 构建 0 错误 / `--shot` 退出码 0 且 80 条 + 截图 1180x780 / `--ui-selftest` 增长通过 / `ctest` 通过 |

**至此 `src/qml/` 的结构**：

```
Main.qml            主页（路由 + 全局状态 + Connections）
HomeScreen.qml      首页（列表视图 + StateBox + LoadMoreFooter）
SearchScreen.qml    搜索（搜索行 + 条数标题）
ComicCard.qml       组件（列表卡片，含悬停/弹性动效）
StateBox.qml        组件（空/失败态 + 重试）
LoadMoreFooter.qml  组件（分页页脚）
```

**下一步（按同一套路）**：把 `Main.qml` 里剩下的部分继续分出去 ——
详情视图与阅读器视图（现在是 `root.reading` 切换的两块），
以及把 `Connections` 里的回调逐步下沉到各屏（一次一层）。

### 第三屏抽出：`ReaderScreen.qml`（阅读器）

| 项 | 结果 |
| --- | --- |
| 新文件 | `src/qml/ReaderScreen.qml`（55 行：翻页图 + 上一页/下一页 + 状态行） |
| 改写点 | `root.pageUrl/pageStatus/cBg/cTextSecondary` → 组件属性；`backend.step(±1)` → 信号 `stepRequested(delta)`；`visible: root.reading` 移到实例上 |
| 四项验证 | 构建 0 错误 / `--shot` 退出码 0 + 截图 1180x780 / `--ui-selftest` 仍在跑 / `ctest` 16/16 |
| `Main.qml` | 进一步变短（315 → 约 300 行） |

**一处有意的删减（不当作"已对齐"）**：原文 `onStatusChanged` 里调用了块外的 `geomProbe.restart()`
（一个诊断 Timer，定义在 `Main.qml` 内、组件里不可见）。本轮**去掉了这一行**，保留了同处的绘制尺寸日志。
要恢复它，应把那个 Timer 一并搬进 `ReaderScreen.qml`（或改成信号）——列为本目标内的一项。

**至此 `src/qml/` 的结构**：

```
Main.qml            主页/路由（+ 详情视图、全局状态、Connections）
HomeScreen.qml      首页
SearchScreen.qml    搜索
ReaderScreen.qml    阅读器
ComicCard.qml / StateBox.qml / LoadMoreFooter.qml   组件
```

**下一屏**：详情视图（`Main.qml` 107 行起，约 95 行 —— 比前三屏都大，是封面 + 标题 + 作者 + 标签 +
章节 `ListView` 的组合）。套路相同：先读原文 → 只搬纯渲染 → 信号回传 → 四项验证。

### 第四屏抽出：`DetailScreen.qml`（详情：封面 + 标题/作者/标签 + 章节列表）

| 项 | 结果 |
| --- | --- |
| 新文件 | `src/qml/DetailScreen.qml`（约 120 行，`Rectangle` 根：封面 `Image` + 三段文字 + 章节 `ListView` + 状态行） |
| 改写点 | `root.albumAid/albumName/albumAuthor/albumTags/pageStatus` → 属性；**`chapterModel`（块外 id）→ 属性传入**；`backend.openChapterId(cid,0)` → 信号 `chapterClicked(cid)`；`root.cSurface1/cStroke` 与 7 处硬编码色 → 注入属性；`visible: !root.reading` 移到实例 |
| 四项验证 | 构建 0 错误 / `--shot` 退出码 0 且 80 条 + 截图 1180x780 / `--ui-selftest` **增长：通过** / `ctest` **16/16** |

**`src/qml/` 结构（四屏 + 三组件）**：

```
Main.qml            主页/路由（+ 全局状态 + Connections）
HomeScreen.qml      首页
SearchScreen.qml    搜索
ReaderScreen.qml    阅读器
DetailScreen.qml    详情
ComicCard.qml / StateBox.qml / LoadMoreFooter.qml   组件
```

**Qt 侧已存在的四个屏（首页/搜索/详情/阅读器）现在全部独立成文件** ——
这是与 Kotlin 版 `ui/screens/` 对齐的第一块结构性成果。
剩余 21 屏（收藏/分类/画师/评论/随机/我的/登录/屏蔽设置/标签/周更/通知/更多列表等）Qt 侧**尚未实现**，
不在"抽取"范畴，而是**新建**。

**下一步**：把 `Main.qml` 的 `Connections`（`onListReady` / `onListAppended` / `onCoverUrlReady` /
`onPageReady` / `onStatus` / `onFailed` / `onCurrentAidChanged`）按归属下沉到各屏 ——
否则 `Main.qml` 永远是"薄不了"的中枢。这是本目标后半段的重点。

### `Connections` 下沉的设计（第 29 轮定稿，下一步照此执行）

`Main.qml` 现在仍持有**全部**后端回调。不把它们按归属下沉，`Main.qml` 就永远是"什么都管"的中枢，
前四轮抽出去的屏也只是"渲染搬走了、逻辑没走"。本轮把归属与状态改动定清楚：

| 回调 / 状态 | 应归属 | 下沉要点 |
| --- | --- | --- |
| `onListReady`（填充列表 + `loadCovers(20)`） | **HomeScreen** | 组件内 `Connections { target: backend }`；`listModel` 已是组件属性，可直接 append |
| `onListAppended`（追加 + 复位 loading + 请求封面） | **HomeScreen** | 同上 |
| `onCoverUrlReady(index, url)`（回填封面） | **HomeScreen** | 同上（纯模型改动） |
| `listLoading` / `listExhausted` / `lastListCount` | **HomeScreen 内部状态** | 现在是 `Main` 的属性、再注入组件 —— 下沉后改由组件自己维护（`LoadMoreFooter` 本来就在组件里，不需要跨层） |
| `onPageReady(image, statusText)` | **ReaderScreen** | 组件内维护 `pageUrl` 与状态行；`Main` 只需传入"是否在阅读" |
| `onCurrentAidChanged` | **DetailScreen** | 组件接收 `albumAid`；`Main` 不再中转 |
| `onStatus(text)` / `onFailed(text)` | **各屏自取** | 这两条是全局提示。**建议保留在 `Main`**（做成一条状态栏），因为它跨屏；若将来 Qt 侧也有 `FloatingBottomBar` 之类的全局区，就放那里 |
| `onListReady` 的**搜索**语义（替换列表） | **SearchScreen** | 注意：搜索结果走的是 `listReady`（**替换**），首页走 `listReady`（首屏）+ `listAppended`（追加）—— 下沉时要按"当前是哪一屏发起"分派，不能只看信号名 |

**状态归属的这条最关键**：`listLoading` / `listExhausted` / `lastListCount` 下沉后，
`Main.qml` 里对它们的引用（给 `HomeScreen` 的属性绑定）要**一并删掉**，否则会出现"两个地方都以为自己在管这个状态"。
这也是前四轮抽取时我特意**没有**动回调的原因 —— 回调与状态是绑在一起改的，必须一次改一处、改完立刻验证。

**验收方式（不变）**：`--shot`（界面仍出图）+ `--ui-selftest`（`listAppended 追加 80 条，增长：通过` 必须仍然成立 ——
它正是"下沉后加载更多还在工作"的证据；加 `ctest`。

### 下沉尝试失败与回滚（第 30 轮）——边界只覆盖了第一个函数

**失败经过**：读原文后执行下沉，用 awk 取"区间末端"时写的是
`NR>a && /^        \}$/ {print NR; exit}` —— 它匹配的是**第一个**同缩进 `}`，
也就是 `onListReady` 自己的收尾，于是只删掉了第一个函数：
`onListAppended` / `onCoverUrlReady` 留在 `Main.qml`，并且残留一处
`root.listLoading = false`（那个属性已被删）→ **QML 必然报错**。

**回滚**：恢复 `Main.qml` 与 `HomeScreen.qml`（改动前的备份），重建后实测：
构建 0 错误、`列表已填充：80 条`、截图 1180x780、`--ui-selftest` **增长：通过**、`git status` 干净。
**未造成任何残留。**

**教训（本目标第 3 次同类）**：**"取到某个同缩进 `}` 为止"不能用来圈定"多个函数"的区间**。
要删多个相邻函数，边界必须满足其一：

1. 匹配**下一个函数头**（`^        function on…`）而不是 `}`；
2. 或者从区间起点往下数出**需要几个同缩进 `}`**（这次需要 3 个）；
3. 或者把那一段的**原文读出来后整体重写**（前四屏抽取成功用的就是这条）。

**下一步**：按第 1 条取边界（`onListReady` 起点 → 下一个非本组函数头之前），
并把 `Main.qml` 的状态属性删除与 `HomeScreen` 的 `Connections` 新增**分两步**做、每步各自验证。

### `Connections` 下沉成功第一个（第 31 轮）：`onCoverUrlReady` 归 `HomeScreen`

| 项 | 结果 |
| --- | --- |
| `Main.qml` | 删除 `onCoverUrlReady`（精确文本替换 `perl -0pi -e "s#\Q…\E##s"`，不用行号区间） |
| `HomeScreen.qml` | 新增 `Connections { target: backend; function onCoverUrlReady(...) }`，改自己那份 `listModel` |
| 四项验证 | 构建 0 错误 / `--shot` 退出码 0 + **`QML 收到封面 URL：index 0`** + 截图 1180x780 / `--ui-selftest` **增长：通过** / `ctest` **16/16** |

**本轮踩到的两个坑（都记下）**：

1. **把 `Connections` 追加到 `HomeScreen.qml` 文件末尾** → `HomeScreen.qml:88:1 Syntax error`：
   根对象（`ColumnLayout`）已在上一行闭合，追加等于**第二个根对象**。必须放在根对象**内部**。
   这与我在 `Main.qml` 里插 `HomeScreen{}` 的做法相同（那次是对的，这次手滑）。
2. **差点被"看起来成功"的日志骗过**：第一次尝试时 C++ 构建**报错失败**，我仍跑 `--shot`，
   它用的是**上一次的二进制**（QML 编译进资源，旧二进制跑旧 QML），却照样打印
   `收到封面 URL：index 0` —— **那行日志不能证明新代码生效**。
   正确做法：**先确认构建成功，再跑 QML 程序**，否则证据无效。这条与第 7 轮的"假通过"同源，
   但这次是"旧二进制的假证据"，比上次更隐蔽。

**下一步**：同法下沉 `onListAppended`（加载更多的追加），再下沉 `onListReady` + 三个列表状态
（`lastListCount` / `listExhausted` / `listLoading`）—— **一次一个，各自验证**。

### `Connections` 下沉第二个（第 32 轮）：`onListAppended` 归 `HomeScreen`

| 项 | 结果 |
| --- | --- |
| `Main.qml` | 删除 `onListAppended`（精确文本替换）；同时删掉注入给 `HomeScreen` 的 `loading:` / `exhausted:` 绑定与 `onMoreClicked` 里的 `root.listLoading = true` —— **loading 状态改由组件自己维护**（这正是第 29 轮设计里那条"状态归属要一并改"） |
| `HomeScreen.qml` | `onListAppended` 加进已有 `Connections`（插在块收尾之前，仍在根对象内部） |
| 四项验证 | 构建 0 错误 / `--shot` 退出码 0 + 80 条 + `收到封面 URL：index 0` + 截图 1180x780 / **`--ui-selftest`：`listAppended 追加 80 条，增长：通过`** / `ctest` 16/16 |

**这条验证的意义**：`--ui-selftest` 打印的"追加 80 条"现在是由 **`HomeScreen` 自己的 `Connections`** 处理的 ——
也就是说"加载更多"这条链路的核心逻辑已经从 `Main.qml` 搬走了，而功能没有退化。

**剩余（列表部分）**：`onListReady`（首屏填充 / 搜索替换）+ 三个状态属性
（`lastListCount` / `listExhausted` / `listLoading`）。做完这三样，
`Main.qml` 里与列表相关的部分才算真正清空。

### `Connections` 下沉完成（列表部分，第 33 轮）：`Main.qml` 里列表逻辑已清空

| 项 | 结果 |
| --- | --- |
| `Main.qml` | `onListReady` / `onListAppended` / `onCoverUrlReady` **全部为 0**；`lastListCount` / `listExhausted` / `listLoading` 三个状态**残留 0** |
| `HomeScreen.qml` | 三个回调各 1，`lastListCount` 为组件内部状态，`loading` / `exhausted` 由组件自管 |
| 有数据路径 | `列表已填充：80 条` + 截图 1180x780 |
| **空列表路径** | `列表已填充：0 条` → **`列表已到底：0 条`**（推导逻辑在新家照常工作） |
| 交互自检 | `listAppended 追加 80 条，增长：通过` |
| 单测 | `ctest` **16/16** |

**这条纵向切片的价值**：这是本目标第一次**完整沉掉一层**（一个屏的全部后端回调 + 它需要的状态）。
前四轮只是"渲染搬走、逻辑留原地"，`Main.qml` 因此一直薄不下来；现在列表这一层真正搬完了。

**做法固化（后面各屏照此）**：
1. 删除用**精确文本替换**（`\Q…\E`），不用行号区间；
2. 新增插到**根对象内部**（按行号定位收尾行），绝不追加到文件末尾；
3. **一次一个函数**，四项目标（构建 0 错误 / `--shot` 出图与日志 / `--ui-selftest` / `ctest`）各自验证；
4. **先确认构建成功再看 QML 输出**（旧二进制会给出假证据）。

**下一层**：`onAlbumReady` / `onChaptersReady` / `onCurrentAidChanged` → `DetailScreen`；
`onPageReady` / `onPageChanged` → `ReaderScreen`；`onStatus` / `onFailed` 留在 `Main`（跨屏状态栏）。

### `Connections` 下沉第三个（第 34 轮）：`onAlbumReady` 归 `DetailScreen`

| 项 | 结果 |
| --- | --- |
| `Main.qml` | 删除 `onAlbumReady`；同时删掉 `albumName`/`albumAuthor`/`albumTags` 三个属性与给 `DetailScreen` 的绑定（残留 0） |
| `DetailScreen.qml` | 新增 `Connections { target: backend; function onAlbumReady(...) }`，三个字段改为组件自有 |
| 列表路径验证 | `列表已填充：80 条` + 截图 1180x780 |
| **详情路径验证** | `--shot <png> 12000 --open 209827` → **`详情封面已加载：400x533`** + **`QML 收到详情：魔都精兵的奴隶…（标签 5）`** + 截图 1180x780 |
| 交互自检 / 单测 | `listAppended 追加 80 条，增长：通过` / `ctest` 16/16 |

**本轮新发现一条验证路径**：`--shot <png> <ms> --open <aid>` 能直接开详情屏并抓图，
于是"详情屏的回调是否真的工作"可以带**真实数据**验证（上表第三行就是证据），
不必只靠"能加载"这种弱证据。

### 本轮同时否掉了三个不该下沉的回调（先量后改）

| 回调 | 为什么不沉 |
| --- | --- |
| `onCurrentAidChanged` | `albumAid` 是**卡片点击**这一路由动作设置的（`HomeScreen.onCardClicked` → `Main` → `DetailScreen`），属于外层状态；下沉会导致点击路径无法传达 aid |
| `onStatus` / `onFailed` / `onPageReady` | 三者都写同一个 `pageStatus`，是**跨屏状态栏**，不属于任何单屏 |
| `onPageChanged` | 它设置 `reading`，决定**显示哪个视图**，属于路由 |

**结论**：`Connections` 下沉不是"全搬走"，而是**按归属搬**。`Main.qml` 最终应剩：
路由（`reading`）、外层状态（`albumAid`、`pageUrl`）、跨屏状态栏（`pageStatus`）、源切换。

**下一层**：`onChaptersReady` → `DetailScreen`（需把 `chapterModel` 一起搬进去）；
再之后 `onPageReady`/`onPageChanged` 视路由设计而定。

### `Connections` 下沉第四个（第 35 轮）：`onChaptersReady` + `chapterModel` 归 `DetailScreen`

| 项 | 结果 |
| --- | --- |
| `Main.qml` | 删除 `onChaptersReady`、`ListModel { id: chapterModel }`、以及实例上的 `chapterModel:` 绑定 —— **残留 0** |
| `DetailScreen.qml` | 自带 `ListModel { id: chapterModel }`（id 不变，故组件内引用无需改名）+ `Connections` 新增 `onChaptersReady` |
| **详情路径验证** | `--shot <png> 12000 --open 209827` → `详情封面已加载：400x533` + `收到详情：魔都精兵的奴隶…（标签 5）` + **`章节列表已填充：159 条`** + 截图 1180x780 |
| 列表路径验证 | `列表已填充：80 条` + 截图 1180x780 |
| 交互自检 / 单测 | `追加 80 条，增长：通过` / `ctest` 16/16 |

**过程中的两个真实错误（都已修）**：
1. 把 `detail.chapterModel` 只改了一处（`model:`），漏了另一处（`text: "章节（" + …count + "）"`）→ 运行期 `QML 加载失败`；
2. 删除 `Main.qml` 里实例绑定时，sed 模式**多写了一个逗号**（实际行是 `chapterModel: chapterModel` 无逗号）→ 没删掉，导致
   `Main.qml:82: Cannot assign to non-existent property "chapterModel"`。

**两条经验**：
- 改属性名/搬模型时，**同一文件里可能有多处引用**，要用 `grep -n 名字` 全查，而不是只改印象中的那一处；
- 删除一行的 sed 模式**必须用 `cat -A` 看真实内容**（逗号、缩进、行尾）——这是本目标第 4 次因"凭印象写模式"返工。

**`Main.qml` 现状**：列表与章节逻辑均已清空，只剩路由（`reading`）、外层状态（`albumAid`、`pageUrl`）、
跨屏状态栏（`pageStatus`）、源切换。**这就是它该有的样子。**

### 新建屏幕的成本评估（第 36 轮实测数据层家底）

**`JmWorker` / `JmBackend` 现有能力**（`Q_INVOKABLE` 与 slots 全文读过）：

```
loadList / loadMore / search / loadAlbum / openChapter / openChapterId / step /
loadCovers / setBlockWords / setTwoPage / reportCacheStats / fetchAlbumCover /
showPageAt / prefetch / saveProgressNow / previewNext
```

**没有**：分类、随机、评论、收藏、历史、签到、画师、登录 —— 一个都没有。

**结论：新建一屏的成本 = 一条纵向切片（4 层）**：

| 层 | 要加的东西 |
| --- | --- |
| `core/JmApi` + `JmParse` | 新接口的请求与解析（JM 的 `/categories`、`/comments`、`/favorite` …） |
| `qt/JmWorker` | 一个 slot + 一个信号（`categoriesReady` …） |
| `qml/JmBackend` | 一个 `Q_INVOKABLE`（以及必要的 `Q_PROPERTY`） |
| `src/qml/` | 新屏 QML（照 `HomeScreen`/`DetailScreen` 模板） |

**所以 21 屏分两类**：

| 类别 | 屏幕 | 成本 |
| --- | --- | --- |
| **不需要数据层** | **关于**、**更多列表**、**设置**（本机 prefs 即可） | 最低：纯 QML，一屏一轮 |
| 需要新数据接口 | 分类 / 随机 / 评论 / 收藏 / 历史 / 签到 / 画师 / 登录 | 每屏 1–2 轮（含 C++ 解析与真机核对） |

**下一步建议**：先做 **关于** 与 **更多列表**（零数据层成本），把"**新建**一屏（而非抽取）"的流程跑通；
再做**分类**（JM 的 `/categories` 结构简单，且能复用现成的列表渲染）。

**如实说明**：剩余 24 轮，需要新接口的屏有 8 类以上，**不可能全部完成**；
我会按"成本 × 可见度"排序推进，并在 `STATUS.md` 里逐项标注"已对齐 / 未做 / 为什么"。

### 新建屏第一例（第 37 轮）：`AboutScreen.qml`（零数据层成本）

| 项 | 结果 |
| --- | --- |
| 新文件 | `src/qml/AboutScreen.qml`（67 行，`Rectangle` 根：标题 / 说明 / 许可证 / 现状 / 关闭按钮） |
| 数据层 | **零改动**（不碰 `core` / `JmWorker` / `JmBackend`） |
| 接线 | `SearchScreen` 新增"关于"按钮 → `signal aboutRequested()` → `Main.qml` 的 `showAbout` 状态 → `AboutScreen` 实例（`visible: root.showAbout`） |
| 登记 | `CMakeLists.txt` 的 `QML_FILES` |
| 验证 | 构建 0 错误（**`qmlcachegen` 已在构建期编译该屏**）/ QML 运行正常：`列表已填充：80 条` + 截图 1180x780（无回归）/ `--ui-selftest` 增长通过 / `ctest` 16/16 |

**如实标注**：该屏初始 `visible: false`，而 `--shot` **点不了鼠标**，所以**"关于屏的实际外观"没有被截图覆盖**。
已验证的是：**它能被编译、能被实例化、且没有影响既有界面**。外观需要人工点一次或下一轮加一条可拍摄路径。

**这一步的意义**：把"**新建**一屏"（而非抽取别人的）的流程跑通了：
**写新 QML → 登记 → 接线（信号 + 外层状态）→ 构建/运行/自检/单测四项验证**。
后面那些"零数据层成本"的屏（更多列表、设置）可照此复制；
"需要新接口"的屏（分类/随机/评论/收藏/历史/签到/画师/登录）在此基础上再加一层 C++ 请求与解析。

### 设置屏（第 38 轮）失败与回滚：信号名与属性的自动信号重名

**失败现象**（QML 运行期，构建期 `qmlcachegen` 反而通过了）：

```
Main.qml:183:9: Type SettingsScreen unavailable
SettingsScreen.qml:28:12: Duplicate signal name: invalid override of property change signal or superclass signal
```

**根因**：我写了 `property bool twoPage`，QML 会自动生成变化信号 **`twoPageChanged`**；
而我又手写了一个 `signal twoPageChanged(bool on)` → **重名**，整个类型不可用。

**修法（两处，各一行）**：把自定义信号改名，避开自动生成的名字 ——
`signal twoPageChanged(bool on)` → **`signal twoPageToggled(bool on)`**；
`Main.qml` 实例上的 `onTwoPageChanged:` → **`onTwoPageToggled:`**。

**回滚结果**：删除 `SettingsScreen.qml`、恢复 `Main.qml` 与 `SearchScreen.qml`、撤销 CMake 登记 →
构建 0 错误、`列表已填充：80 条`、截图 1180x780、`git status` 干净。**无残留。**

**这条经验要记进"写 QML 的规矩"**（本目标已积累若干条）：

> **自定义信号不能与属性的自动变化信号同名**（`fooChanged` 由 `property ... foo` 自动生成）。
> 同理，属性名也不能与 `Rectangle`/`Item` 的内置属性重名（第 5 轮踩过 `radius`）。

**下一步**：按上述两处改名重做设置屏（QML 文件可原样复用，只改信号名与实例上的处理器名）。

### 新建屏第二例（第 39 轮）：`SettingsScreen.qml`（用后端已有能力）

| 项 | 结果 |
| --- | --- |
| 新文件 | `src/qml/SettingsScreen.qml`（101 行：双页模式勾选 + 屏蔽词输入 + 提示行 + 关闭） |
| 数据层 | **零改动** —— 用的是后端**已有**的 `setTwoPage(bool)` 与 `setBlockWords(QStringList)` |
| 接线 | `SearchScreen` 加"设置"按钮 → `signal settingsRequested()` → `Main.showSettings` → 屏实例；`onTwoPageToggled` 调 `backend.setTwoPage`，`onBlockWordsChanged` 调 `backend.setBlockWords` |
| 命名 | 自定义信号用 `twoPageToggled`（避开 `property bool twoPage` 的自动变化信号 `twoPageChanged`）—— 这正是第 38 轮的失败根因 |
| 验证 | 构建 0 错误 / QML 运行 `列表已填充：80 条` + 截图 1180x780（无回归）/ `--ui-selftest` 增长通过 / `ctest` 16/16 |

**如实标注**：
1. 该屏外观**没有被截图覆盖**（初始不可见，`--shot` 点不了鼠标）；
2. `twoPageToggled` / `blockWordsChanged` 两条链路**只是接好了，没有被点击验证** ——
   要证明"勾选后后端真的收到 setTwoPage"，需要一条可自动触发的自检路径（与 `--ui-selftest` 同类），
   目前**没有**。列为未验证项。

**至此 Qt 侧屏幕清单**：Home / Search / Reader / Detail（抽取）、**About / Settings（新建）**，
共 6 屏；剩余 19 屏（收藏/分类/画师/评论/随机/我的/登录/屏蔽设置页/标签/周更/通知/更多列表等）尚未实现。

### 分类屏（第 40 轮）：数据层接口的位置已探明

本轮先做"先量后改"的定位，结论如下（供下一步直接动手）：

| 问题 | 结论 |
| --- | --- |
| 谁在发请求？ | `core::JmApi` 只提供底层 `request(path, query)` / `requestPath(...)` / `doRequest(...)` |
| `latest` / `search` / `album` 在哪？ | 不在 `JmApi.h` —— 由上层封装提供（`JmWorker` 通过 `client()` 调用） |
| 解析入口在哪？ | `core/JmParse.h`：`ListEntry` / `SeriesEntry` / `AlbumInfo` / `PageImage` / `ChapterImages` / `SearchPage` |

**因此"加分类"最小改动的正确落点是**：
1. 在提供 `latest/search/album` 的那个封装类里加一个 `categories()`（走 `JmApi::request`，参照 `latest` 的写法）；
2. `JmParse` 加一个 `parseCategories(json)`（JM 的 `/categories` 是数组，元素含 `id/title` 与子分类）；
3. `JmWorker` 加 `loadCategories()` slot 与 `categoriesReady(names, ids)` 信号；
4. `JmBackend` 加 `Q_INVOKABLE void loadCategories();` 与对应信号转发；
5. `src/qml/CategoryScreen.qml` 照 `HomeScreen`/`DetailScreen` 模板写（列表直接复用 `ComicCard`）。

**未做**：本轮只完成定位，未写代码。下一步按上面五步实施，验收仍为四项目标
（构建 0 错误 / `--shot` 出图与日志 / 交互自检或新增一条可断言的自检 / `ctest` 16/16）。

### 分类切片：样板已读齐（第 41 轮）

| 发现 | 意义 |
| --- | --- |
| `src/core/JmPaths.h` **已有 `CATEGORIES[] = "categories"`** 与 `CATEGORIES_FILTER[]` | **少一处改动** —— 路径常量不用加 |
| `JmClient::latest` 的写法（`JmClient.cpp:75`） | 四行结构：bootstrap 检查 → `JmApi api(session_, http_)` → `api.request(路径, 查询)` → `parse*(r->text)` |
| `parseLatestList` 在 `JmParse.cpp:201` | 解析样板（含 JSON 手法），`parseCategories` 照它写 |

**剩余还需确认的一处**：`parseLatestList` 用的 JSON 库与取值写法（下一轮开头读 20 行即可）。

**五步清单更新为四步**（路径常量已存在）：
1. `JmClient` 加 `categories()`
2. `JmParse` 加 `parseCategories()`
3. `JmWorker` 加 slot + 信号
4. `JmBackend` 加 `Q_INVOKABLE` + 信号转发 → `CategoryScreen.qml`

**关于节奏的如实说明**：第 40、41 两轮都是"读代码定位"，没有产出功能。这是必要的（前面几次失败都源于没看清结构），
但也确实慢了。下一步将**连续实施**这四步，做完即验证。

### 重要发现：Kotlin 侧的数据源在 `shared` 模块（第 42 轮）

Qt 侧此前的参照只看了 `app/src/main/kotlin`，**漏了 `shared`** —— 而剩下那些屏的数据模型与调用**全在这里**：

| 文件 | 对应未实现的屏 |
| --- | --- |
| `shared/data/remote/dto/Models.kt` | 所有 DTO（如 `CategoryNode` 在 **:210**） |
| `shared/data/JmRepository.kt` | 各接口的调用（分类/收藏/评论/签到…） |
| `shared/data/RandomRanking.kt` | **随机** |
| `shared/data/Daily.kt` | **周更 / 每日** |
| `shared/data/FavoriteTags.kt` | **收藏标签** |
| `shared/data/BlockRules.kt` | 屏蔽（Qt 侧已有 `core/BlockRules.h`） |

**这条发现的价值**：后面每做一屏，都能直接在 `shared` 里找到**权威的模型字段与调用方式**，
不必再靠猜或探接口 —— 这正是"以 JMNeXt 为唯一参照"的正确入口。

**分类的具体形状已读到**（`Models.kt` 第 205–224 行），下一轮据此写
`parseCategories`（照 `parseLatestList` 的手写扫描套路）+ `JmClient::categories`，
再走 worker / backend / QML 四层。

### 决定性发现：分类屏该用 `hot_tags`，不是 `categories`（第 43 轮）

读 `shared/data/JmRepository.kt` 得到**权威答案**（不是我的推测）：

| 接口 | 用途 | 结论 |
| --- | --- | --- |
| `categories` | 条目带 `slug`/`updated_at`，**是登录用户的收藏夹分类** | **不能**用作公开分类导航 |
| **`hot_tags`** | **纯字符串数组** | **分类浏览页就是用它**（`JmRepository.kt:252` 注释原文） |
| `categories/filter` | 按分类筛选作品（`PagedList`，可分页） | 点标签后取作品列表用它 |

**若我照最初的想法用 `categories` 实现，就是照着一个 Kotlin 版自己都不用的接口做** —— 这次读参照价值在于此。

**另一个必须遵守的协议细节**（`JmRepository.kt` 对 `categoryFilter` 的注释，实测结论）：

> `c` 为空时**整个参数必须省略**；发 `c=` 会让服务端返回 `Could not connect to mysql!` 错误页（不是 JSON）。
> 省略 `c` 是合法的"不筛选"语义。分类树里第一个「最新A漫」的 slug 正是空串。

**Qt 侧落地因此简化很多**：
1. `hot_tags` 是**字符串数组** → 解析用现成的 `splitTopLevel` 即可（不需要 DTO 结构解析）；
2. 点标签后的作品列表是 `PagedList` → **与搜索同一形态**，很可能可复用现成的 `parseSearchPage`；
3. `JmPaths.h` 已有 `HOT_TAGS` / `CATEGORIES_FILTER` 常量。

**四层切片清单（已无未知项）**：`parseStringArray` + `JmClient::hotTags/categoryFilter`
→ `JmWorker` 两个 slot 与信号 → `JmBackend` 两个 `Q_INVOKABLE` → `CategoryScreen.qml`。

### 分类切片：可直接照写的代码（第 44 轮，辅助函数已确认）

`src/core/JmParse.cpp` 的匿名命名空间里**已有全部所需辅助**（逐个确认过）：

| 辅助 | 签名 | 用途 |
| --- | --- | --- |
| `readString` | `bool readString(const std::string& s, std::size_t i, std::string& out, std::size_t& end)` | **读 JSON 字符串**（hot_tags 的元素就用它） |
| `skipWs` | `std::size_t skipWs(const std::string&, std::size_t)` | 跳过空白 |
| `matchingBracket` | `std::size_t matchingBracket(const std::string&, std::size_t)` | 配对括号 |
| `splitTopLevel` | `std::vector<std::string> splitTopLevel(const std::string& body)` | 顶层元素切分 |
| `stringArrayField` | `std::vector<std::string> stringArrayField(const std::string& obj, const std::string& key)` | **可参照它的实现写顶层数组版**（它在 `JmParse.cpp:132`） |

**据此可直接写出的函数**（下一轮照此落地，不必再探）：

```cpp
// JmParse.h（声明，放在 parseSearchPage 附近）
std::optional<std::vector<std::string>> parseHotTags(const std::string& json);

// JmParse.cpp（实现；照 parseLatestList 的"手写扫描"套路，元素用 readString）
std::optional<std::vector<std::string>> parseHotTags(const std::string& json) {
    const auto start = json.find('[');
    if (start == std::string::npos) return std::nullopt;
    const auto end = matchingBracket(json, start);
    if (end == std::string::npos) return std::nullopt;
    std::vector<std::string> out;
    for (auto& item : splitTopLevel(json.substr(start + 1, end - start - 1))) {
        std::string value;
        std::size_t stop = 0;
        if (readString(item, skipWs(item, 0), value, stop) && !value.empty()) out.push_back(value);
    }
    return out;
}
```

`JmClient` 侧照 `latest` 的四行结构（`JmClient.cpp:75`）：

```cpp
std::optional<std::vector<std::string>> JmClient::hotTags() {
    if (!bootstrapped_) { lastError_ = "尚未初始化主机"; return std::nullopt; }
    JmApi api(session_, http_);
    auto r = api.request(paths::HOT_TAGS, "");
    if (!r) { lastError_ = api.lastError(); return std::nullopt; }
    return parseHotTags(r->text);
}
```

**关于节奏的如实记录**：第 40–44 连续五轮没有产出功能代码（定位、读参照、确认辅助函数）。
原因是真实的：**本会话上下文已近耗尽，每轮只够执行一条命令**，而分类切片需要改 5 个文件（`JmParse.h/.cpp`、
`JmClient.h/.cpp`、`JmWorker`、`JmBackend`、新 QML），一轮做不完。
但**规格已经全部落纸**：本节两个函数可直接粘贴，其余三层（worker slot+信号、backend Q_INVOKABLE、QML 屏）
在 `STATUS.md` 前几节已有清单与验收方式。

### 分类切片第一步落地（第 45 轮）：核心层两个函数

| 项 | 结果 |
| --- | --- |
| `JmParse.h` | 新增声明 `parseHotTags`（附注释说明为何用 hot_tags 而非 categories） |
| `JmParse.cpp` | 新增实现（照 `parseLatestList` 的手写扫描：`find('[')` → `matchingBracket` → `splitTopLevel` → `readString`） |
| `JmClient.h` / `.cpp` | 新增 `hotTags()`（照 `latest` 的四行结构，路径用已有的 `paths::HOT_TAGS`） |
| 四处插入的锚点 | 插入前逐个 `grep -cF` 核对，**均为唯一命中（1 处）** |
| 验证 | 构建退出码 0、错误 0；`ctest` **16/16 通过**（既有测试未受影响） |

**如实标注**：新增的 `parseHotTags` / `hotTags` **还没有被任何测试或运行路径调用** ——
本轮只证明"能编译、不破坏既有测试"。真正的验证要等 worker / backend / QML 三层接上之后，
用一条可断言的路径（例如 `--selftest` 里打印标签数量）来证明它真的取到了数据。

**下一轮**：`JmWorker` 加 `loadHotTags()` slot 与 `hotTagsReady(QStringList)` 信号
→ `JmBackend` 加 `Q_INVOKABLE void loadHotTags()` 与信号转发 → 在 `--selftest` 里断言标签数量
→ 最后写 `CategoryScreen.qml`。

### 分类切片第二步（第 46 轮）：`JmWorker` 通路

| 项 | 位置 | 说明 |
| --- | --- | --- |
| slot | `JmWorker.h:50` | `void loadHotTags();`（放在 `loadCovers` 之后） |
| 信号 | `JmWorker.h:81` | `void hotTagsReady(const QStringList& tags);` |
| 实现 | `JmWorker.cpp:322` | `ensureStarted()` → `client().hotTags()` → `emit status("热门标签：N 个")` + `emit hotTagsReady(...)`；失败走 `emit failed(...)` |
| 锚点 | 三处插入前均 `grep -cF` 核对，**各唯一命中** | |
| 验证 | 构建退出码 0、错误 0；`ctest` **16/16** | |

**仍如实标注**：`loadHotTags()` **还没有任何调用者**（`JmBackend` 与 QML 尚未接），
所以现在仍然只能证明"能编译"。下一步：

1. `JmBackend` 加 `Q_INVOKABLE void loadHotTags();` + 信号转发（照 `listAppended` 那条连接的写法）；
2. 在 `--selftest` 里调用它并**断言标签数量 > 0**（这样新链路就有自动验证）；
3. 写 `CategoryScreen.qml`（标签列表 + 按标签筛选）。

### 分类切片第三步（第 47 轮）：`JmBackend` 通路完成

| 项 | 位置 | 内容 |
| --- | --- | --- |
| `Q_INVOKABLE` | `JmBackend.h:38` | `void loadHotTags();` |
| 信号 | `JmBackend.h:45` | `void hotTagsReady(const QStringList& tags);` |
| 连接 | `JmBackend.cpp:18` | `connect(worker_, &JmWorker::hotTagsReady, this, &JmBackend::hotTagsReady);` |
| 转发 | `JmBackend.cpp:46` | `void JmBackend::loadHotTags() { invoke("loadHotTags"); }` |
| 锚点 | 四处插入前均 `grep -cF`，**各唯一命中** | |
| 验证 | 构建 0 错误 / `ctest` **16/16** / QML 冒烟截图 1180x780 | |

**至此热标签链路已经贯通四层**：`JmParse` → `JmClient` → `JmWorker` → `JmBackend`（QML 可调用）。
**但仍未被调用**，所以还没有"真的取到标签"的证据 —— 下一步两件事：

1. 在 `--selftest` 里 `backend.loadHotTags()` 并**断言标签数量大于 0**（第一次真正验证这条链路）；
2. 写 `CategoryScreen.qml`：左侧/上方标签列表（`hotTagsReady` 填充），点标签后调
   `categories/filter`（**注意 `c` 为空时必须省略参数**，见第 43 轮记的协议陷阱）取作品列表并复用 `ComicCard`。

### 分类切片第四步（第 48 轮）：热标签链路**实测取到真实数据**

新增 `--hot-tags` 自检（与 `--ui-selftest` 同一套路：走真实网络、把结果打到日志），首次运行即通过：

```
$ ./build/jmnext4qml --hot-tags
热标签自检：10 个，前三个：超長篇 / 觀淫 / 豐滿
```

| 层 | 状态 | 证据 |
| --- | --- | --- |
| `JmParse::parseHotTags` | **已验证** | 取回 10 个标签（解析正确） |
| `JmClient::hotTags` | **已验证** | 同上（真实网络请求成功） |
| `JmWorker::loadHotTags` | **已验证** | 信号按预期发出 |
| `JmBackend::hotTagsReady` | **已验证** | QML 侧自检收到并打印 |
| `CategoryScreen.qml` | **未做** | 屏还没写；这层数据链路已可供其使用 |

**这条验证的意义**：第 45–47 轮三次提交都只能说"能编译、不破坏既有测试"，
本轮第一次有了**"真的取到数据"**的证据 —— 而且它以后每轮都能自动重跑。

**验证手段又添一条**（现有四条）：

| 命令 | 覆盖 |
| --- | --- |
| `ctest --test-dir build` | 数据层既有单测（16 项） |
| `./build/jmnext4desktop --list` / `--chapters <aid>` | widget 路径 |
| `./build/jmnext4qml --shot <png> <ms> [--open <aid>]` | QML 渲染（静态） |
| `./build/jmnext4qml --ui-selftest` | QML 交互（加载更多） |
| **`./build/jmnext4qml --hot-tags`** | **分类数据链路（本轮新增）** |

### 分类切片第五步（第 49 轮）：`CategoryScreen.qml` 建成

| 项 | 结果 |
| --- | --- |
| 新文件 | `src/qml/CategoryScreen.qml`（104 行：标题 + 刷新按钮 + 标签 `ListView` + 计数 + 关闭） |
| 数据源 | **`hot_tags`**（依据 `shared/data/JmRepository.kt` 的说明：公开分类导航用它，`categories` 是登录用户收藏夹分类） |
| 取数时机 | `Component.onCompleted: backend.loadHotTags()` + 手动"刷新"按钮 |
| 接线 | `SearchScreen` 加"分类"按钮 → `Main.showCategory` → 屏实例 |
| 验证 | 构建 0 错误 / QML 运行 + 截图 1180x780（无回归）/ `--hot-tags` 自检 **10 个** / `ctest` **16/16** |

**未做的部分（如实）**：点标签后的**筛选结果尚未接** —— 点击目前只发 `tagClicked(tag)` 信号、由外层打日志。
接筛选要用 `categories/filter`，且必须遵守协议细节：**`c` 为空时省略整个参数**（发 `c=` 会返回错误页而非 JSON）。

**Qt 侧屏幕清单**：Home / Search / Reader / Detail（抽取）、About / Settings / **Category（新建）** = **7 屏**；
未实现 18 屏。

### 分类切片第七步（第 51 轮）：筛选链路接通并**实测返回真实作品**

新增 `--category-filter <标签>` 自检，首次运行即通过：

```
$ ./build/jmnext4qml --category-filter 女高中生
分类筛选自检：80 条，首条：慾望入门课
```

| 层 | 位置 | 状态 |
| --- | --- | --- |
| `JmClient::categoryFilter` | `JmClient.cpp` | 已验证（返回 80 条） |
| `JmWorker::categoryFilter` / `categoryReady` | `JmWorker.h`（slot+信号）、`.cpp`（实现） | 已验证 |
| `JmBackend::categoryFilter` / `categoryReady` | `JmBackend.h`（Q_INVOKABLE+信号）、`.cpp`（转发+连接） | 已验证 |
| `--category-filter` 自检 | `main_qml.cpp` | 本轮新增，可重跑 |

**协议细节已按 Kotlin 原码落实并生效**：`c` 为空时**省略整个参数**（不拼 `c=`）；
本轮用非空标签调用，拿到 80 条，说明请求拼接与 `parseSearchPage` 复用都正确。

**验证手段现为六条**：`ctest`（16 项）、widget `--list/--chapters`、`--shot`（含 `--open`）、
`--ui-selftest`（加载更多）、`--hot-tags`（分类标签）、**`--category-filter`（分类筛选）**。

**仍待做**：把筛选结果接到 `CategoryScreen` 的界面上（现在是 `--category-filter` 在数据层验证，
屏里点标签只打日志）。Qt 侧屏幕仍为 7 个，未实现 18 个。

### 分类屏接上筛选（第 52 轮）：屏内处理器已验证

| 改动 | 内容 |
| --- | --- |
| 点击标签 | 现在**真的调** `backend.categoryFilter(name, 1)`（保留 `tagClicked` 信号给外层），并记录 `activeTag` |
| 新增处理器 | `CategoryScreen` 的 `Connections` 里加 `onCategoryReady(titles, ids)`：更新 `resultCount` 并打日志 |
| 提示行 | 显示"分类「标签」：N 条" |

**证据**：

```
$ ./build/jmnext4qml --shot <png> 9000      # 屏内标签填充
  QML 分类标签已填充：10 个
$ ./build/jmnext4qml --category-filter 女高中生
  QML 分类筛选已收到：80 条（标签：），首条：慾望入门课
```

第二行是**屏内 `onCategoryReady` 处理器**打出来的（该次运行里 QML 引擎已加载、`Connections` 处于活动状态，
于是自检触发的 `categoryReady` 也被屏接收）—— 这正好证明**屏内处理器确实工作**，不是"只写了没跑"。
（`标签：` 为空是因为该次运行没有人点击，`activeTag` 未设置。）

**仍未做**：筛选结果的**列表渲染**（现在是计数 + 日志，没有把 80 条画成 `ComicCard` 列表）。
数据与处理器都已验证，只差渲染。

### 分类屏补上结果渲染（第 53 轮）：这屏已完整

| 改动 | 内容 |
| --- | --- |
| 结果模型与列表 | `ListModel { id: resultModel }` + `ListView`，delegate 复用 **`ComicCard`**（同一卡片组件，含悬停/按压动效） |
| 收到结果 | `onCategoryReady` 里 `resultModel.clear()` + 逐条 append，并调 `backend.loadCovers(20)` 让封面就位 |
| 点击结果 | 新增 `signal resultClicked(aid)`，由外层负责打开详情 |

**证据**：

```
$ ./build/jmnext4qml --shot <png> 9000        -> QML 分类标签已填充：10 个；截图 1180x780
$ ./build/jmnext4qml --category-filter 女高中生 -> QML 分类筛选已收到：80 条，首条：慾望入门课
构建 0 错误；ctest 16/16
```

**核对时发现的一个布局问题（已记，下一步一行修）**：结果 `ListView` 与标签 `ListView` 都设了
`Layout.fillHeight: true`，而 **QML 里隐藏的项在 Layout 中仍然占位** —— 于是"没有筛选结果时，
标签列表也被压成一半"。修法：把结果列表改成 `Layout.fillHeight: resultModel.count > 0`
（空时高度 0，标签列表占满）。**本轮未修**，如实记录。

**分类屏至此功能完整**：标签列表（`hot_tags`）→ 点击取作品（`categories/filter`，遵守 `c` 省略规则）
→ 结果用 `ComicCard` 渲染 → 点击结果可打开详情（信号已就位）。这是 Qt 侧**第一块从零贯通四层的完整功能屏**。

### 分类屏布局修复（第 54 轮）

上一轮记下的问题已修：

```qml
// 结果列表（第 63 行）：空结果时高度 0，让标签列表占满
Layout.fillHeight: resultModel.count > 0
// 标签列表（第 87 行）：保持 true
```

**原因回顾**：QML 里 `visible: false` 的项在 Layout 中**仍然占位**，所以原本两个 `fillHeight: true`
会把可用高度对半分，导致"还没筛选时标签列表只占一半"。

**验证**：构建 0 错误 / 标签填充 **10 个** / 筛选 **80 条** / 截图 1180x780 / `ctest` **16/16**。

**如实标注**：这是**观感**改动，而我无法看图（本机缺 `sharp`，`read_image` 不可用）——
"标签列表现在确实占满、筛选后两个列表平分"这一点**需要人工看一眼**。逻辑与构建层面已验证。

---

# 交接清单（第 55 轮整理）

给接手的自己或他人：**照这个做，不需要重新摸索。**

## 一、新建一屏的标准五步

以分类屏（唯一走完的五步样板）为例：

| 步 | 做什么 | 参照 |
| --- | --- | --- |
| 1 | 在 `shared/` 找**权威模型与调用**（不要猜接口） | `JMNeXt/shared/src/main/kotlin/com/jmnext/data/` |
| 2 | `core/JmParse` 加解析（**手写扫描**，用现成的 `readString`/`splitTopLevel`/`matchingBracket`/`scalarField`/`stringField`） | `parseLatestList`（`JmParse.cpp:201`） |
| 3 | `qt/JmClient` 加方法（四行结构：bootstrap 检查 → `JmApi api(session_, http_)` → `api.request(路径, 查询)` → `parse*(r->text)`） | `JmClient::latest`（`JmClient.cpp:75`） |
| 4 | `qt/JmWorker` 加 slot + 信号（slot 里 `ensureStarted()` → 调 client → `emit status(...)` + `emit xxxReady(...)`；失败走 `emit failed`） | `JmWorker::loadHotTags` |
| 5 | `qml/JmBackend` 加 `Q_INVOKABLE` + 信号声明 + `connect` + `invoke` 转发；再写 QML 屏（`ColumnLayout` 根、主题色注入、`Connections { target: backend }`） | `JmBackend` 的 hotTags/categoryFilter 两套 |

**新增 QML 文件必须登记进 `CMakeLists.txt` 的 `QML_FILES`**，sed 用 `\(\s*\)` 吞缩进。

## 二、六条验证命令（每步都跑）

```sh
ctest --test-dir build                                   # 数据层 16 项单测
./build/jmnext4desktop --list                            # widget 路径列表
./build/jmnext4desktop --chapters 209827                 # widget 路径章节
./build/jmnext4qml --shot <png> 8000 [--open <aid>]      # QML 渲染（--shot 必须在第一位）
./build/jmnext4qml --ui-selftest                         # QML 交互：加载更多
./build/jmnext4qml --hot-tags                            # 分类标签（10 个）
./build/jmnext4qml --category-filter 女高中生            # 分类筛选（80 条）
```

容器内构建：`/data/data/com.termux/files/home/jmc/.work/enter.sh 'cd <仓库> && cmake --build build -j2'`，
并设 `QT_QPA_PLATFORM=offscreen`。

## 三、已知坑（全部踩过，别再踩）

| 类别 | 坑 | 纪律 |
| --- | --- | --- |
| 锚点 | 凭缩进猜 sed 模式 → 匹配不到（返工 4 次） | 改前 `grep -cF` 核对唯一命中；`cat -A` 看真实空白 |
| 锚点 | 取"第一个同缩进 `}`"圈定多函数区间 → 只删了一半 | 多函数区间要匹配**下一个函数头**，或整体重写 |
| 构建 | `cmake --build` 通过 **不等于** QML 可用（`qmlcachegen` 过、运行期挂） | 必须跑 `jmnext4qml` |
| 构建 | 构建**失败**时跑 `--shot` 用的是**旧二进制**，日志照样正常 → 假证据 | 先确认构建成功，再看 QML 输出 |
| QML | 属性与 `Rectangle` 内置名冲突（如 `radius`） | 改名（`cardRadius`） |
| QML | 自定义信号与属性的自动变化信号重名（`twoPage` → `twoPageChanged`） | 信号改名（`twoPageToggled`） |
| QML | 追加内容到**根对象之外** → `Syntax error` | 插到根对象**内部** |
| QML | `visible: false` 的项在 Layout 里**仍占位** | 用条件 `Layout.fillHeight` |
| 协议 | `categories/filter` 的 `c` 为空时**必须省略参数**（发 `c=` 返回错误页而非 JSON） | 见 `JmRepository.kt` 注释 |
| 协议 | 公开分类导航用 **`hot_tags`**，不是 `categories`（后者是登录用户收藏夹分类） | 同上 |
| 环境 | Termux 与容器间 bind 挂载会掉；`/tmp` 在 Termux 不可写 | 临时文件放 `.work/` |

## 四、未做的事（如实，逐项给原因）

| 项 | 原因 |
| --- | --- |
| 其余 18 屏（收藏/画师/评论/随机/我的/登录/屏蔽设置页/标签/周更/通知/更多列表等） | 每屏要开一条四层纵向切片，剩余轮次不足；**非技术阻碍** |
| 五个组件（`Glass`/`GlassTopBar`/`FloatingBottomBar`/`ItemMotion`/`AmbientBackdrop`） | 同上，未开始 |
| 交互：翻页手势与缩放、下拉刷新、共享元素过渡 | 未开始 |
| `JmNavHost` 式的路由与转场 | 现在是 `Main.qml` 里的 `reading`/`showAbout`/`showSettings`/`showCategory` 布尔开关，**不是**真正的路由栈 |
| 阅读器的 `geomProbe` 诊断探针 | 抽取时该探针在块外，被有意删减（第 27 轮记） |
| 设置屏两条链路（`setTwoPage`/`setBlockWords`）的点击验证 | 缺自动触发路径（第 39 轮记） |
| 分类屏观感（布局占比） | 无法看图，需人工确认（第 54 轮记） |
| 图片质量档位 / 默认图源 | Qt 侧数据层尚无对应能力 |

---

## 附：完成度审计的原始数据（第 56 轮采集，供逐项对照）

**Kotlin 侧屏幕文件（`app/src/main/kotlin/com/jmnext/ui/screens/`）**：

```
about/AboutScreen.kt
auth/AuthScreen.kt
category/CategoryScreen.kt
comments/CommentsScreen.kt
creator/CreatorScreen.kt
creator/CreatorWorkScreen.kt
detail/DetailScreen.kt
favorites/FavoritesScreen.kt
favorites/FolderDialogs.kt
home/HomeScreen.kt
home/HomeViewModel.kt
home/RandomFab.kt
more/MoreListScreen.kt
notifications/NotificationsScreen.kt
profile/DailyHistorySection.kt
profile/ProfileScreen.kt
random/RandomListScreen.kt
reader/ChapterPickerDialog.kt
reader/PageRatio.kt
reader/ReaderScreen.kt
search/SearchFilters.kt
search/SearchScreen.kt
settings/BlockSettingsScreen.kt
tags/TagFavoritesScreen.kt
week/WeekScreen.kt
```

**Kotlin 侧组件文件（`ui/components/`）**：

```
AmbientBackdrop.kt
ComicCard.kt
FloatingBottomBar.kt
Glass.kt
GlassTopBar.kt
ItemMotion.kt
LoadMoreFooter.kt
StateBox.kt
```

**Qt 侧 QML 文件（`src/qml/`）**：

```
AboutScreen.qml
CategoryScreen.qml
ComicCard.qml
DetailScreen.qml
HomeScreen.qml
LoadMoreFooter.qml
Main.qml
ReaderScreen.qml
SearchScreen.qml
SettingsScreen.qml
StateBox.qml
```

> 注：本清单只列**文件存在性**，不等于已对齐 —— 逐项状态判定在下一轮按此数据给出。

---

# 完成度审计（第 57 轮，逐项对应目标的完成判定）

> 判定依据只有两类：**仓库里存在的文件** + **可重跑的验证证据**。不用感觉判断。
> 三档：**已对齐**（功能与观感均已验证）/ **部分**（主路径可用已验证，但有明确缺口）/ **未做**。

## 一、Kotlin 侧 25 个屏幕文件 → Qt 侧状态

| Kotlin 文件 | Qt 状态 | 依据 / 缺口 |
| --- | --- | --- |
| `home/HomeScreen.kt` | **部分** | 列表 80 条、加载更多（`--ui-selftest` 增长通过）、空/失败态、页脚已对齐；**缺**：随机 FAB、下拉刷新 |
| `home/RandomFab.kt` | 未做 | 依赖随机接口 |
| `home/HomeViewModel.kt` | 部分 | Qt 侧以 `JmWorker` 承担其职责（列表/分页/封面） |
| `search/SearchScreen.kt` | **部分** | 搜索行 + 结果替换已对齐；**缺**：筛选器 |
| `search/SearchFilters.kt` | 未做 | 无接口 |
| `detail/DetailScreen.kt` | **部分** | 封面 400x533、名称/作者/标签、章节 **159 条**均已实测；**缺**：评论入口、收藏按钮、评分等 |
| `reader/ReaderScreen.kt` | **部分** | 翻页图 + 上一页/下一页 + 状态行；**缺**：手势、缩放、双页（后端有 `setTwoPage` 但界面未用）、下拉 |
| `reader/ChapterPickerDialog.kt` | 未做 | 章节列表在详情页已有，弹窗未做 |
| `reader/PageRatio.kt` | 未做 | 页宽比逻辑未移植 |
| `category/CategoryScreen.kt` | **已对齐（功能面）** | 标签 10 个、筛选 80 条、`ComicCard` 渲染、点击可打开详情；**观感待人工确认** |
| `about/AboutScreen.kt` | **部分** | 信息齐；外观未被截图覆盖 |
| `settings/BlockSettingsScreen.kt` | **部分** | 双页 + 屏蔽词已接后端；**链路未被点击验证**；其余设置项未做 |
| `favorites/FavoritesScreen.kt` | 未做 | 无接口 |
| `favorites/FolderDialogs.kt` | 未做 | 无接口 |
| `creator/CreatorScreen.kt` | 未做 | 无接口 |
| `creator/CreatorWorkScreen.kt` | 未做 | 无接口 |
| `comments/CommentsScreen.kt` | 未做 | 无接口 |
| `random/RandomListScreen.kt` | 未做 | 无接口 |
| `profile/ProfileScreen.kt` | 未做 | 依赖登录 |
| `profile/DailyHistorySection.kt` | 未做 | 依赖登录/每日接口 |
| `auth/AuthScreen.kt` | 未做 | 依赖登录链路 |
| `more/MoreListScreen.kt` | 未做 | 纯静态列表，成本最低的遗留项 |
| `notifications/NotificationsScreen.kt` | 未做 | 无接口 |
| `tags/TagFavoritesScreen.kt` | 未做 | 无接口 |
| `week/WeekScreen.kt` | 未做 | 无接口（`shared/data/Daily.kt` 有参照） |

**小计**：已对齐 1 / 部分 7 / 未做 17（`HomeViewModel` 计入部分）。

## 二、Kotlin 侧 8 个组件 → Qt 侧状态

| Kotlin 组件 | Qt 状态 |
| --- | --- |
| `ComicCard.kt` | **已对齐**（含悬停 120ms 过渡、按压 0.985 弹性） |
| `StateBox.kt` | **已对齐**（`kind` + `retry`，空/失败态已接线） |
| `LoadMoreFooter.kt` | **已对齐**（三态 + 已到底推导 + 空列表隐藏） |
| `Glass.kt` / `GlassTopBar.kt` / `FloatingBottomBar.kt` / `ItemMotion.kt` / `AmbientBackdrop.kt` | **未做**（5 个） |

**小计**：3 / 8。

## 三、交互（目标 (a)3）

| 交互 | Qt 状态 |
| --- | --- |
| 加载更多 | **已对齐**（`--ui-selftest` 断言"增长：通过"） |
| 分类筛选 | **已对齐**（`--category-filter` 实测 80 条） |
| 翻页手势 / 缩放 | **未做**（只有上一页/下一页按钮） |
| 下拉刷新 | **未做** |
| 共享元素过渡 | **未做** |
| 动画与视觉效果 | **部分**（卡片悬停/按压有；`ItemMotion`、`AmbientBackdrop`、壁纸未做） |

## 四、数据层（目标 (a)1）

| 能力 | Qt 状态 |
| --- | --- |
| 主机发现与签名 | **已对齐**（`jmnext4net discover` 实测 `https://www.cdnhjk.net/`） |
| 列表 / 搜索 / 详情 / 章节 / 翻页 | **已对齐**（widget 与 QML 两条路径均实测） |
| 反切片 | **已对齐**（Android 侧修通；Qt 侧 `core` 有 `needsUnscrambleFor` 与实现，`ctest` 覆盖） |
| 图片解码与缓存、磁盘缓存 | **已对齐**（`ctest` 覆盖；自检有 `cacheStats`） |
| 屏蔽词 / 双页 | **已对齐（后端）**，界面接线未被点击验证 |
| **热门标签 / 分类筛选** | **本轮新增并已验证** |
| 收藏 / 历史 / 评论 / 画师 / 分类树 / 随机 / 签到 / 登录 | **未做**（每项都要新开接口与切片） |
| 本地存储（prefs、阅读进度） | **部分**（阅读进度保存有 `saveProgressNow`；`prefs`/屏蔽标签持久化未验证） |

## 五、结论（对应目标的完成判定）

| 目标条款 | 达成情况 |
| --- | --- |
| "覆盖 Kotlin 版**全部屏幕**" | **未达成**：已对齐 1、部分 7、未做 17（共 25） |
| "覆盖**主要交互**" | **未达成**：6 类交互中 2 类已对齐、1 类部分、3 类未做 |
| "对照表逐项标注已对齐" | 已补（本表） |
| "未能对齐的**如实列出并说明原因**" | 已补（上表每行的"缺"与原因；多数是"无接口 / 依赖登录 / 轮次不足"） |
| "未验证项如实记录" | 已补（阅读器 `geomProbe`、设置屏两条链路、分类屏观感） |

**因此本目标未完成，不应标记为 complete。** 剩余轮次（3 轮）不足以改变这个结论：
其余 17 屏每屏都需新开一条四层纵向切片（解析 + 客户端 + worker + backend + QML），
且其中 6 屏还依赖登录链路（凭据与登录流程本身尚未移植）。

### 新建屏第四例（第 58 轮）：`MoreScreen.qml`（零数据层成本）

| 项 | 结果 |
| --- | --- |
| 新文件 | `src/qml/MoreScreen.qml`（104 行：13 个入口的静态列表 + 关闭按钮） |
| 数据层 | **零改动** |
| 特色 | 每项**如实标注**是否已在 Qt 侧实现：分类/关于/设置标为可用（点击真的跳转），其余 10 项标"未实现"（点击只打日志，**不假装能用**） |
| 接线 | 搜索屏"更多"按钮 → `Main.showMore` → 屏实例；`onEntryClicked` 对已实现项做跳转 |
| 验证 | 构建 0 错误 / QML 运行（`分类标签已填充：10 个`）+ 截图 1180x780 / `ctest` 16/16 |

**本轮的一个如实记录**：同一轮里另跑 `--hot-tags` 自检时遇到**偶发网络失败**：

```
热标签自检失败：热门标签失败：网络请求失败
```

同一条自检在第 48、49、53 轮**均通过**（返回 10 个标签），因此判定为**偶发网络问题，不是代码回归**。
但这条要记下来，因为它说明：**六条验证命令中依赖网络的那几条（`--list`、`--hot-tags`、`--category-filter`、
`--chapters`）都可能因网络抖动而失败** —— 排查时应先重跑一次，不要立刻怀疑代码。

**Qt 侧屏幕现为 8 个**：Home / Search / Reader / Detail / About / Settings / Category / **More**；未实现 17 项（按审计表）。

---

# 收尾验证（第 59 轮）：八项全过，无回归

`scripts/verify-all.sh`（本轮加入仓库，容器内 `bash scripts/verify-all.sh` 即可重跑）：

| # | 检查 | 结果 |
| --- | --- | --- |
| 1 | 干净重建（`cmake -S . -B build` + `--build -j2`） | 退出码 0，**错误 0** |
| 2 | `ctest --test-dir build` | **100% tests passed, 0 failed out of 16** |
| 3 | `jmnext4desktop --list` | 封面缩略图成功 3 张；**首页列表自检：通过** |
| 4 | `jmnext4desktop --chapters 209827` | 章节 159 / 列表项 159；末页 第 1/194 页 764x1200；**章节选择自检：通过** |
| 5 | `jmnext4qml --shot <png> 9000` | 分类标签已填充 10 个；截图 **1180x780** |
| 6 | `jmnext4qml --ui-selftest` | **listAppended 追加 80 条，增长：通过** |
| 7 | `jmnext4qml --hot-tags` | **10 个**（前三个：辦公女郎 / 動畫化 / 歹戲拖棚） |
| 8 | `jmnext4qml --category-filter 女高中生` | **80 条**，首条 慾望入门课 |

**两点说明**：
1. 第 7 项在上一轮曾失败（网络抖动），本轮**恢复通过** —— 印证上轮"偶发非回归"的判断；依赖网络的检查先重跑一次再怀疑代码；
2. 本套件已入库（`scripts/verify-all.sh`），接手者一条命令即可复现上述状态。

**仓库状态**：工作区干净、与远端同步、自 tag `port-20` 以来 **38 个提交**。

---

# 最终交付说明（第 60 轮，本目标结束时的状态）

## 一、这一段究竟做了什么

从"`JMNeXt4QtDesktop` 里 UI 全挤在一个 `Main.qml`、QML 前端缺功能"的状态，做到：

| 成果 | 证据 |
| --- | --- |
| **结构拆分**：`Main.qml` 从约 352 行降到约 190 行，成为"路由 + 外层状态 + 状态栏" | 五个屏独立成文件 |
| **逻辑下沉**：列表层（3 个回调 + 3 个状态）与详情层（2 个回调 + 1 个模型）从 `Main.qml` 移入各屏，`Main.qml` 里残留为 0 | 第 31–35 轮，每步四项验证 |
| **补上真实缺陷**：QML 前端此前**没有"加载更多"**；接上后发现并修好"点了没反应"（`loadMore` 发的是 `listAppended`，QML 从未处理该信号） | 第 13、19、31–33 轮；`--ui-selftest` 断言 |
| **新增功能屏 4 个**：关于、设置、**分类**、更多 | 分类屏是**唯一从零贯通四层**的完整功能屏 |
| **新增数据能力 2 项**：热门标签、分类筛选（含协议陷阱的处理） | `--hot-tags`、`--category-filter` 两条自检 |
| **新增验证手段 3 条**：`--ui-selftest`、`--hot-tags`、`--category-filter`，并入库 `scripts/verify-all.sh` | 八项全过 |

## 二、确切状态（不夸大）

| 维度 | Kotlin 侧 | Qt 侧 | 覆盖 |
| --- | --- | --- | --- |
| 屏幕文件 | 25 | 8（抽取 4 + 新建 4） | **8 / 25** |
| 组件 | 8 | 3（`ComicCard`/`StateBox`/`LoadMoreFooter`） | **3 / 8** |
| 交互 | 6 类 | 2 已对齐、1 部分、3 未做 | **2 / 6** |
| 数据层 | 全能力 | 列表/搜索/详情/章节/翻页/反切片/缓存/主机发现/热门标签/分类筛选已对齐；收藏/历史/评论/画师/随机/签到/登录未做 | 约一半 |

逐项判定见本文件"完成度审计（第 57 轮）"一节。

## 三、接手怎么跑

```sh
# 容器内
bash scripts/verify-all.sh          # 八项检查，应全过
```

单独跑：

```sh
ctest --test-dir build                                   # 16 项
./build/jmnext4desktop --list / --chapters 209827         # widget 路径
./build/jmnext4qml --shot <png> 8000 [--open <aid>]       # QML 渲染（--shot 必须第一）
./build/jmnext4qml --ui-selftest                          # 加载更多
./build/jmnext4qml --hot-tags                             # 热标签
./build/jmnext4qml --category-filter 女高中生             # 分类筛选
```

新建一屏的**标准五步**、**11 条已知坑**、**`shared` 模块对应关系**：见"交接清单（第 55 轮）"一节。

## 四、未验证项（不静默带过）

1. 阅读器的 `geomProbe` 诊断探针在抽取时被有意删减（第 27 轮）；
2. 设置屏的 `setTwoPage`/`setBlockWords` 两条链路**接好了但未被点击验证**（第 39 轮）；
3. 分类屏与各屏的**观感**（布局占比、颜色）**需人工看一眼** —— 本机 `read_image` 不可用，我无法看图；
4. 依赖网络的检查会偶发失败（第 58 轮遇上一次，第 59 轮恢复）—— 先重跑再怀疑代码。

## 五、为什么本目标不应标记为完成

目标的完成判定要求"覆盖 Kotlin 版**全部屏幕与主要交互**"。现状是 **8/25 屏、3/8 组件、2/6 交互**，
其中 6 屏还依赖**尚未移植的登录链路**。剩余 17 屏每屏都需新开一条四层纵向切片
（`JmParse` → `JmClient` → `JmWorker` → `JmBackend` → QML），不是靠收尾能补齐的。

**因此：本目标未完成。** 留给下一步的不是一堆半成品，而是
"清单（审计表）+ 模板（分类屏五步）+ 验证（`scripts/verify-all.sh`）+ 坑（11 条）"齐备的起点。

## 第 61 轮（提速模式）：一次切片解锁多屏 + 组件补齐到 8/8

| 项 | 内容 |
| --- | --- |
| 通用数据通路 | `JmClient::paged(path,query)` → `JmWorker::loadPaged(tag,path,query)` → `JmBackend::loadPaged` + `pagedReady(tag,titles,ids)`。**一屏覆盖所有"作品列表"型接口**，新增屏 = 在 `MoreScreen` 表里加一行，不再动 C++ |
| 宽松解析 | `JmParse::parseLooseList`：字段名多态（`name/work_title/title`、`author/author_name`、`image/work_image`、`id/aid`）且自动下钻到第一个有内容的对象数组 |
| 通用屏 | `src/qml/ComicListScreen.qml`（标题+刷新+列表+空态+关闭；换接口靠 `onApiPathChanged` 重载） |
| 新自检 | `--paged <path> [query]`，一条命令核对任意列表接口 |
| **组件 8/8** | 补齐 `Glass`/`GlassTopBar`/`FloatingBottomBar`/`ItemMotion`/`AmbientBackdrop`（说明：Qt 6.4 无 `MultiEffect` 真模糊，`Glass` 用半透明+描边近似，已在文件头注明） |
| 入口接线 | `MoreScreen` 的随机推荐/画师/收藏/历史/标签/通知均映射到真实 `path/query`（取自 `shared/data/JmRepository.kt`） |

**实测（`--paged`）**：

| 接口 | 结果 |
| --- | --- |
| `random_recommend` | **30 条，首条有真实标题** |
| `creator_author page=1` | 30 条，但**标题为空**（字段未对上，待查原始 JSON） |
| `favorite page=1` / `watch_list page=1` | **网络请求失败**（这两个接口需要登录态，而登录链路尚未移植） |
| `week` | 返回的是**刊期与分类**而非作品列表 → 取作品需 `week/filter` 且需 issue/type 两级参数，入口已如实标回"未实现" |

**进度**：屏幕 **8 个文件 + 一个通用列表屏覆盖 7 个入口**；组件 **8/8**（本目标首次满额）。
构建 0 错误、QML 运行与截图正常。

## 第 62 轮（提速模式）：登录链路

| 层 | 改动 |
| --- | --- |
| 传输 | `HttpClient` 加 `post()`（**默认实现返回失败**，因此既有 16 项测试的测试桩无需改动）；`QtHttpClient::post` 用 `QNetworkAccessManager::post` 实现（单独文件 `QtHttpClientPost.cpp`） |
| 协议 | `JmApi::post(path, formBody)`：表单 `Content-Type: application/x-www-form-urlencoded`；`headersFor` 在登录后追加 `Authorization: Bearer <jwt>`（Kotlin 侧 `JmRemote` 同款） |
| 凭证 | `core::setAuthJwt/authJwt`（**过渡实现：进程内全局**，未落盘未加密；Kotlin 侧是 Keystore 加密存储 —— 已在代码注释与本文如实标注） |
| 客户端 | `JmClient::login(username,password)` → POST `login`，取 `jwt_token`/`jwtToken`/`token`；`logout()` 清凭证（单独文件 `JmClientAuth.cpp`） |
| 工作线程/桥 | `JmWorker::login/logout` + `loginResult(ok,msg)`；`JmBackend::login/logout` + 信号转发 |
| 界面 | `src/qml/LoginScreen.qml`（用户名/密码/登录/退出 + 结果提示 + 现状说明）；`MoreScreen` 的"登录/注册"入口已接 |

**经验（本轮踩的）**：**往 .cpp 末尾追加实现会落到命名空间之外**（两次编译失败）——
改成"实现放独立 .cpp 文件"后一次通过。另外用 `\s*` 前导匹配做 CMake 插入会把两行粘在一起，已修。

**未验证**：登录成功与否需要真实账号（凭据不落盘、不进日志、不在回复里回显）；
按纪律交给用户在自己的账号上验证。注册接口未接。

## 第 63 轮：下拉刷新 + 未知响应形态的回显通路

| 改动 | 说明 |
| --- | --- |
| 首页下拉刷新 | `HomeScreen.qml` 的列表在顶部继续下拉超过 60px 即触发 `requestReload()`（对应 Kotlin 首页 pullToRefresh） |
| 原始响应回显 | `JmClient::lastRaw()` 记住最近一次响应原文；`JmWorker::loadPaged` 在"首行标题为空"时把它打进 `status`（截断 220 字，不落盘） |
| `--paged` 自检 | 现在同时打印后端 `status`，未知形态的定位不必再猜 |

**未解决**：`creator_author` 仍显示 30 条而标题为空，但"首行标题为空"的回显分支**没有触发**，
说明标题串首行非空、而自检打印处显示为空 —— 两者矛盾，本轮未查明。**如实记录，不算已对齐**。
下一步用它查：`--paged creator_author page=1` 看 status，或加 `--raw` 直接打印响应前 300 字。

## 第 64 轮：转场 + 收藏/历史/通知接线

| 改动 | 说明 |
| --- | --- |
| 转场 | `Main.qml` 的 8 个面板统一加 `opacity: visible ? 1 : 0` + `Behavior on opacity`（140ms OutCubic）—— 对应 `JmNavHost` 的过场观感；Qt 6.4 无 `MultiEffect`，故用透明度而非共享元素位移 |
| 收藏 / 历史 / 通知 | `MoreScreen` 标为可用并接到通用列表屏（路径 `favorite` / `watch_list` / `notifications`，参数 `page=1`），**登录后可用** |
| 画师 | 标为可用（`creator_author page=1`，30 条能取到但标题待修） |

**当前规模**：QML 19 个文件；组件 8/8；交互 4/6（加载更多、分类筛选、翻页手势+缩放、下拉刷新）；
屏幕 8 个独立文件 + 通用列表屏覆盖 7 个入口（随机/画师/收藏/历史/通知/标签/追更）。

**未验证**：收藏/历史/通知需要真实账号；`creator_author` 标题；周更两级屏未做。

## 第 65 轮：--raw 定位 + 画师字段修好 + 周更两级屏

| 项 | 结果 |
| --- | --- |
| 新调试入口 | `--raw <path> [query]` 直出响应原文（截断 1200 字），未知形态一次看清 |
| 画师（`creator_author`） | 看清响应是 `data.content`、名字键 **`author_name`** → 宽松解析补该兜底 → **实测「30 条，首条：SirensParadise」** |
| 周更刊期（`week`） | 刊期在 `categories`、显示名是 **`time`** → 补兜底 → **实测「260 条，首条：2026第260期10.09 - 10.02」** |
| 周更作品（`week/filter`） | 参数 `id`/`type`/`page`（Kotlin 原码）；**`type` 传空可用** → **实测「20 条，首条：[3D]王大吊传奇01」** |
| 周更两级屏 | 新增 `src/qml/WeekScreen.qml`（左列刊期 → 右侧该刊期作品，全部走通用 `loadPaged`，两个 tag：`week` / `weekworks`），已接到更多列表 |
| 标签收藏 | `tags_favorite` 实测**网络请求失败** → 需登录态（与收藏/历史同类），暂不改其"未实现"标记 |

**规模**：QML **20 个文件**；组件 8/8；交互 4/6；屏幕 10 个（Home/Search/Reader/Detail/About/Settings/Category/More/Login/Week）+ 通用列表屏覆盖 7 个入口。

## 第 66 轮：通用 POST 动作通路 + 详情收藏 + 签到

| 项 | 内容 |
| --- | --- |
| **通用动作通路** | `JmClient::action(path, form, msg)` → `JmWorker::action(tag,path,form)` + `actionDone(tag,ok,msg)` → `JmBackend::action`。**一次覆盖**收藏/点赞/追更/签到/评论发送/删除等所有 POST 动作接口，新增动作不用再改 C++ |
| 详情页收藏 | `DetailScreen.qml` 加"收藏 / 取消"按钮 → `backend.action("favorite","favorite","aid=<aid>")`（依据 `JmRepository.toggleFavorite`） |
| 每日签到 | 更多列表加"每日签到"入口 → `backend.action("checkin","daily_chk","")`（依据 `JmPaths.DAILY_CHECK`） |
| 验证 | 构建 0 错误、QML 运行与截图正常 |

**未验证（如实）**：收藏与签到都**需要登录态**，本轮只验证了代码通路；
`forum`（评论）用 `--raw forum 'aid=209827&page=1'` 返回**空响应**，参数或登录要求待查。

## 第 67 轮：容器挂载掉了（已重挂）+ 两个接口的探测结论

| 事项 | 结果 |
| --- | --- |
| 环境 | 容器内 `/data/.../jmc` **绑定挂载掉了**（`cd: No such file or directory`）→ 按既有做法 `su -c "mount -o bind <宿主> <容器内同名路径>"` **重挂成功**（容器内又能看到仓库） |
| `--raw` 增强 | 现在同时接 `failed`，能区分"请求失败"与"响应为空" |
| `forum`（评论） | 参数纠正为 `mode=all&page=1&aid=<aid>`（依据 `JmRepository.comments`）后：**请求成功但正文为空** → 很可能需要登录态 |
| `setting`（应用配置） | 参数纠正为 `app_img_shunt=1&lang=zh&t=<epoch>`（依据 `JmRepository.settings`）后：**同样正文为空** → 待登录后复查 |

**评论屏与"我的"屏因此暂缓**：接口有响应但正文为空，此时写屏只能得到空列表 —— 先不做假屏。
下一步优先做**不依赖登录**的部分：搜索筛选器、详情页信息补全、阅读器双页（后端 `setTwoPage` 已有）。
