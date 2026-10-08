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
