# 移植规格清单（JMNeXt4QtDesktop）

**用途**：这是"一等一重实现"的**判据**。每一项都注明规格来源；实现后逐项勾选，全部勾完才有资格取代 Kotlin 桌面版。
规格来源以主项目 `JMNeXt` 仓库为准（`shared/`、`desktop/`、`docs/STATE.md`、测试）。

> 说明：本清单在写的时候**只依据我确实读过的代码与文档**；标「待核对」的是我知道存在、但还没逐字读过的细节，
> 实现前必须回到主仓库核对，不要凭印象写。

## A. 接口与鉴权（最高优先级，全项目的根）

- [ ] 端点表与主机发现（含主机发现接口的**固定密钥**）—— 来源：`shared/src/main/kotlin/com/jmnext/data/crypto/JmCrypto.kt`
- [ ] 请求头 `Tokenparam: "<秒级时间戳>,<客户端版本>"`、`Token: md5(时间戳 + TOKEN_SEED)` —— 同上
- [ ] **响应体 AES-256-ECB 解密**：密钥 = `md5(时间戳 + TOKEN_SEED)` 的 UTF-8 十六进制字符串（32 字节） —— 同上
- [ ] **例外**：广告接口的密钥不带时间戳（`md5(TOKEN_SEED)`） —— 同上
- [ ] 十六进制小写 MD5（与 npm `md5` 包输出一致） —— 同上
- [ ] 会话/登录：Cookie 与登录态保持 —— 来源：`shared/.../remote/JmSession.kt`（**待核对**：具体端点与字段）
- [ ] **怪癖**：未登录时服务端返回 **HTTP 200 + `{"status":"0","msg":"請先登入"}`** 这类，不能只按状态码判断失败 —— 来源：`docs/STATE.md` 与 `DetailScreen` 注释
- [ ] 错误映射：把服务端 `msg` 映射成用户可读文案（主项目多处如此处理）

## B. 图片管线（最大的性能看点）

- [ ] 切片还原的**几何公式**，与 `JMComic_SRC` 的 `utils/Function.js` 逐字对应：
      `num = md5(aid + page)` 推出的份数；`base = h / num`；`remainder = h % num`；
      第 i 条：源区间 `y = h - base*(i+1) - remainder`，目标起点 `py = base*i`；
      `i == 0` 时把 `remainder` **补进条高**（而不是目标位置），使所有条带正好铺满整页 ——
      来源：`shared/src/main/kotlin/com/jmnext/data/image/ImageUnscramble.kt`（`bands()`）
- [ ] `bands()` 的边界条件：`num <= 1`、`height < num`、`width/height <= 0` 时**返回空列表 = 不需要还原**；
      以及 `srcTop/srcBottom/py/room` 的裁剪逻辑 —— 同上
- [ ] 「是否需要还原」的判定入口 —— 来源：`JmRepository.needsUnscramble`（**待核对**）
- [ ] 解码路径与缓存边界：主项目用 Coil，**磁盘缓存存的是"未还原"的原始字节**，转换结果不落盘
      → 冷启动后第一次看到某页仍要重新解码 + 还原一次。**这是管线本身的边界，不是配置漏了** ——
      来源：`app/src/main/kotlin/com/jmnext/data/image/ScrambleTransformation.kt` 注释
- [ ] 缓存键必须包含页号（`scramble:<aid>:<page>`）：同话不同页的 md5 不同、还原方式也不同 —— 同上
- [ ] **原生实现的机会**：把「解码 + 还原」合并成一次操作，避免"解码成位图 → 拷进数组 → 搬移 → 拷回位图"的两次整页拷贝
      （主项目现在是 Kotlin + Skia，瓶颈在拷贝与内存带宽，不在算力）

## C. 行为语义（用户能感知的部分）

- [ ] 收藏 / 追更 / 点赞（接口返回的是**一句人话**，如「已追踪!」，主项目拿它当权威而**不做乐观翻转**） —— 来源：`app/.../detail/DetailScreen.kt` 注释
- [ ] 屏蔽规则：标签规则 + 关键词（作者）规则；`allowedOnce`（允许一次）；「已读过的标签」收敛流程
      —— 来源：`shared/.../TagBlockResolver.kt`、`app/.../home/HomeScreen.kt`、`RandomListScreen.kt`
- [ ] **列表接口不下发标签**，因此屏蔽要靠**按 id 异步取详情拿标签**；并发上限 3；共享缓存避免重复请求 —— 同上
- [ ] 「只读可见条目」与「整批读」的取舍：只读可见 ⇒ 没滚到的条目永远没标签 ⇒ **用户要自己翻过去一遍才生效**（主项目踩过，见 issue #3）——
      现做法：**整批读当前一批，可见的排在最前** —— 来源：`RandomListScreen.kt`
- [ ] 首页：`latest` 分页是 **0-indexed**；分区（推荐位）与「最新上架」是两块数据，**两处都要过滤屏蔽** —— 来源：`HomeViewModel.kt`、`HomeScreen.kt`
- [ ] 阅读器：翻页/跳章/进度记录/缩放与手势、预取 —— 来源：`app/.../reader/`、`desktop/.../PagedReader.kt`（**待核对**）
- [ ] 搜索：排序/检索筛选、历史、`hidden` 计数提示、随机推荐 —— 来源：`SearchScreen.kt`（**待核对**）
- [ ] 下载/离线、壁纸、通知（追更未读）、RSS？—— 来源：`SerialNotify.kt`、`WallpaperStore.kt` 等（**待核对范围**）

## D. 更新检查（含两条血泪规则）

- [ ] 版本比较必须**归一化**：去掉 `v` 前缀、忽略变体后缀（`.lite`/`.debug`） —— 来源：`shared/.../UpdateCheck.kt`
- [ ] **`x.x.x.fix(n)` 规则（2026-10-05 新增）**：待修项的修复版命名为 `2.1.7.fix1` 形式；
      **必须把 `fixN` 当作第四段数字比较**（`[2,1,7,1] > [2,1,7]`），否则修复版会被判定为"已是最新"而**永远推不出去** —— 来源：`docs/STATE.md` 第三十三节
- [ ] 附件名归一化要**保留 `fixN`、只丢 `.lite`/`.debug`**（`2.1.7.fix1.lite` → `2.1.7.fix1`） —— 同上
- [ ] **发布页附件名里不含架构**（一个 APK 同时含多 ABI）→ 由**应用自己拼直链**，不要把用户丢到发布页让他自己挑 —— 来源：`docs/STATE.md` 第三十二节、issue #5
- [ ] 桌面端按 `os.name`/`os.arch` 选包，并**把"内含多架构运行时"的统一包排第一** —— 来源：`UpdateCheck.desktopAssetNames`

## E. 工程与交付

- [ ] 版本号规则：`x.x.x`（小功能 +001／大功能 +010）、`x.x.x.fix(n)` 用于待修项
- [ ] 发布矩阵（原生版可**大幅简化**）：Windows x64/arm64、Linux aarch64/x86_64；是否需要"统一包"视体积而定
- [ ] 打包：Windows 用 Qt 的部署工具（`windeployqt`），Linux 用 `linuxdeploy`/手写 tar.gz、deb、rpm、AppImage
- [ ] **验收脚本**：主项目用 `verify-apk.sh` 断言产物（APK 包名/版本/manifest/资源）；
      原生版应对每个平台产物有一份**断言式验收**（不能只看"文件存在、命令退出码为 0"——主项目为此吃过发布坏包的亏）
- [ ] 日志与诊断：崩溃/启动失败**必须留下可读日志**（主项目的 exe 曾因静默无输出而无法定位）

## F. 对齐验证（"一等一"的客观判据）

- [ ] 移植主项目 **56 个共享层测试**的语义（`shared/src/test`）：算法类（切片几何、版本比较、屏蔽匹配）**逐条对齐**
- [ ] 关键 UI 行为做**人工对照清单**（同一作品、同一账号、两台机器对比：列表顺序、屏蔽结果、阅读器交互）
- [ ] 声明对齐基线：本仓库 README 里写明"对应 JMNeXt <版本> 的功能集"

## 当前状态

- 本清单为**初稿**：A/B/D 三节依据我确实读过的代码与文档；C/E 的部分条目与 B 的两处标了「待核对」。
- 下一步（实现前）：把「待核对」逐条回主仓库读实，并补齐 C 节（阅读器、搜索、下载、壁纸、通知的真实范围）。

## G. 第一轮核对结果（补 A–F 中「待核对」项）

依据：直接读主仓库 `app/`、`desktop/`、`shared/` 的源码。**已读实的写结论，没读实的保持「待核对」**。

### G1. 阅读器（原 C 节待核对项）

- **翻页模型**：Android 用 `HorizontalPager` + `rememberPagerState`（**横向分页**，非纵向连续滚动）——
  来源：`app/.../reader/ReaderScreen.kt`（第 33、34 行 import）
- **换章时的重建语义**：以**章节 id 作 key**，换话时整体重建 —— 这样 LazyColumn 的滚动位置与 Pager 的页码一起归零。
  注释里专门写了这件事（"换话时整体重建"），说明这是**有意行为**，移植时不要"优化"成保留滚动位置 —— 同上（第 332 行附近）
- **进度记录**：`ReadProgressStore`（基于 `SharedPrefsKeyValueStore`）记录阅读进度；`recordProgress(payload)` —— 第 252、298 行
- **相邻章节**：`neighbour(offset)` 取上一话/下一话 —— 第 258 行
- **图片尺寸**：`PageRatio.kt` 负责按页比例决定显示尺寸（83 行） —— 移植时要一并搬

### G2. 是否有"离线下载"功能

**核对结论：看起来没有独立的下载/离线模块。** 搜 `download|下载|离线|offline` 命中的文件基本都是注释或 URL 字样
（`ImageBytes.kt` 是取字节、`JmImage.kt`/`ScrambleTransformation.kt` 是图片管线）。
**待确认**：详情页某处注释提过"追更 / 下载 / 标签收藏的结果提示"，需回去确认那是指"下载图片到缓存"还是真有离线下载入口。

### G3. 追更与通知

- 有 `SerialNotify.kt`（数据层）与 `NotificationsScreen.kt`（界面）—— 未读数是**服务端**给的，不是本地算的（见 `SerialNotify.kt` 注释）
- 通知入口在首页（`HomeScreen.kt` 里出现相关字样）

### G4. 搜索（原 C 节待核对项）

- 已确认有：排序/检索两组筛选（`SearchFilters.Order` / `SearchFilters.Type`）、搜索历史（可清空）、
  "换一批"随机推荐、以及"被屏蔽规则挡掉了 N 条"的提示条 —— 来源：`app/.../search/SearchScreen.kt`
- 具体端点与字段仍**待核对**

### G5. 视觉与动效系统（"一等一"里最容易被低估的一块）★

主项目的界面不只是"颜色 + 布局"，它有一整套**设计系统与动效规范**，移植时必须一并重建，否则观感一定不像：

| 层 | 主项目位置 | 移植要求 |
| --- | --- | --- |
| 调色板 | `app/.../ui/theme/Palettes.kt` | 直接对照取色，不要自己"调一版更好看的" |
| 形状 | `Shapes.kt`、`Tokens.kt` | 圆角/描边/字距等令牌逐项对齐 |
| **主题风格**（4 套） | `ThemeStyle.kt`（**420 行**） | WindowGlass / Translucent / Miuix / Material 四种表面工艺；**这是最大的移植面** |
| 主题装配 | `Theme.kt`（324 行） | 颜色/形状/字体的组合方式 |
| 动效令牌 | `desktop/.../Motion.kt` | 时长（QUICK 120 / NORMAL 220 / PAGE 280 / IMAGE 260 ms）与四条缓动（Standard/Enter/Exit） |
| 共享元素 | `desktop/.../SharedElement.kt` | 封面→详情页的共享元素过渡 |
| 交互反馈 | `desktop/.../Interactions.kt` | 悬停/按下/**键盘焦点描边**（这条是本轮新加的，见主项目 docs/STATE.md） |

**两条硬约定**（主项目用血泪换来的，移植时必须遵守）：

1. **数值一律走令牌，不硬编码** —— 时长、缓动、圆角、间距都从 `Motion`/`Tokens` 取；
2. **只动 `alpha` 与 `transform`，不要动布局** —— 避免每帧重新布局（主项目 `docs/MOTION.md` 的约定）。

**CJK 文本**：主项目为此专门做了"显式选择含中日韩字形的系统字体族"的修复（Windows 上默认字体不含汉字，
会显示成方框或被 fallback 到日文字形）——来源：`desktop/.../BlogTheme.kt` 的 `cjkFontFamily()`。
Qt 侧同样要注意：**不要用 `QFont` 默认族直接画中文**，要显式给 CJK 族并确认回退链。

### G6. 仍未核对（实现前必须回主仓库读实）

- [ ] 接口端点表与登录/会话细节（`JmSession.kt`）
- [ ] `needsUnscramble` 的判定条件（`JmRepository`）
- [ ] 搜索与分类的具体端点、分页参数、字段名
- [ ] 收藏夹（多收藏夹/目录）的接口与语义
- [ ] 壁纸（`WallpaperStore.kt` 328 行）与桌面端的远程壁纸（`WallpaperRemote.kt`）
- [ ] 屏蔽设置页的完整语义（`BlockSettingsScreen.kt`，含"允许一次"的撤销）
- [ ] 主题风格的 4 套具体视觉参数（`ThemeStyle.kt` 420 行需逐项抄写）

### G7. 第二轮核对：剩下的「仍未核对」项已定位到文件（部分读实）

| 待办 | 状态 | 位置与结论 |
| --- | --- | --- |
| **接口端点表** | **已定位** ★ | `shared/.../data/remote/JmPaths.kt` —— **这就是移植最该先抄的东西**：所有路径常量集中在此 |
| 主机发现 | **已定位** | `shared/.../data/remote/JmHostDiscovery.kt`（对应 `JmCrypto` 里那个固定密钥） |
| 会话层 | **已读实** | `shared/.../data/remote/JmSession.kt` 很薄：`token`/`tokenParam`（都由 `JmCrypto` 派生）、`refresh()` 重算时间戳、`useHost(base)` 换主机、`apiUrl(path)`/`imageUrl(path)` 拼 URL —— **移植时照抄这个结构即可，不要自己设计** |
| `needsUnscramble` | **已读实** | `JmRepository.needsUnscramble(imageUrl, aid, scrambleId)` → 直接委托 `JmCrypto.needsUnscramble(...)`。注意参数里有 **`scrambleId`**（不是只靠 aid/page），移植时别丢 |
| 收藏夹（多目录） | **已定位** | `FavoriteTags.kt`（本地缓存统计）、`JmPaths.kt`（接口路径）、`dto/AuthModels.kt`（模型）—— 具体语义**待细读** |
| 壁纸 | 规模已知 | `WallpaperStore.kt` **328 行**（Android）、`desktop/.../WallpaperRemote.kt` **209 行** —— 功能不小，移植要专门排期 |
| 追更通知 | 规模已知 | `SerialNotify.kt` **138 行**；未读数来自服务端 |
| 四套主题参数 | 待抄 | `ThemeStyle.kt` 420 行需逐项抄成 Qt 侧的一份等价配置 |

**给实现者的最短路径建议**（基于这两轮核对）：

1. 先抄 `JmPaths.kt` 端点表 + `JmSession.kt` 结构 + `JmCrypto.kt`（Token/AES/MD5/`needsUnscramble`）—— 这三件通了就能拿到数据；
2. 再抄 `ImageUnscramble.bands()` 的几何 + 解码，做出"能看图"的最小阅读器；
3. 然后按 `ThemeStyle.kt`/`Tokens.kt`/`Motion.kt` 重建外观与动效；
4. 最后补收藏夹、壁纸、通知、屏蔽设置、更新检查。

**验收**：第 1、2 步完成后，用主项目 `shared/src/test` 里对应的算法测试（切片几何、版本比较、屏蔽匹配）作为**逐条对齐的判据**。

---

# 进度对照（截至 0.1.1 开发中）

把 A–F 各节与当前实现逐条对照。**已做的都注明验证方式**；没做的明确列出，不含糊。

## 已完成并有证据

| 规格条目 | 实现 | 证据 |
| --- | --- | --- |
| A 端点表 | `src/core/JmPaths.h`（45 条，由 Kotlin 常量表机械抽出） | 抽样断言 + 与上游条数一致 |
| A Token / Tokenparam / AES-256-ECB / MD5 | `JmCrypto` / `AesEcb` / `Md5` / `Base64` | RFC 1321 向量、NIST SP800-38A 向量、**openssl 交叉验证** |
| A 响应解密（含多 seed、"人话"响应） | `decryptApiData` | 单测 + **真实网络**（真实列表 24766 字节） |
| A **响应是 `{code,data}` 信封** | `JmApi::extractEnvelope`（含 `\/` 还原） | 真实网络确认 + 单测 |
| A 主机发现（两个入口、固定种子、裸密文） | `HostDiscovery` | **真实网络**（4 个候选主机）+ 单测 |
| A 未登录怪癖、错误映射 | 仅做了"失败不静默"（`lastError`） | 部分；未做完整文案映射 |
| B 切片还原几何 + 像素还原 | `ImageUnscramble` / `UnscrambleApply` | 不重不漏铺满整页 + 逐行独立复算 + 大量抽查 |
| B 解码路径（**WebP**） | Qt `QImage` + `qt6-image-formats-plugins` | **真实图片**（852x1280）解码成功；缺插件时失败（已记录） |
| B 缓存边界（磁盘存未还原字节） | `ImageCache` | 跨进程命中实测 |
| C 详情（标签、章节表） | `parseAlbum` + 界面详情区 | **真实网络**（标签 5 个、章节 159 个） |
| C 章节选择 | 左侧章节列表 | **真实网络**（列表项数 159） |
| C 阅读器（翻页、缩放） | `MainWindow` + worker | **真实网络**（285 页；键盘/滚轮/空格；适应窗口/100%） |
| C 列表（真实字段、分页、封面） | `parseLatestList` + `coverUrl` | **真实网络**（80 条 → 160 条；封面 400x533） |
| C 屏蔽规则 | `BlockRules` + 列表过滤 | 单测 + **真实数据**（屏蔽 Chinese 隐藏 16 条，80→64） |
| C 阅读进度 | `ReadProgress` + 界面接线 | 单测 7 组 + 两次运行的恢复实测 |
| D 更新检查（含 fix(n)） | `core/UpdateCheck` + `jmnext4net update` | 单测 + 罐装响应（`0.1.1` vs `v0.1.1.fix1` 判为新版本）；**在线一步未验证** |
| E 打包（tar.gz + deb） | `package-linux.sh` | Debian 12 上"装 → 跑真实数据 → 卸" |
| E 版本命名规则 | `src/core/Version.h` 单一来源 | `--version` 与打包脚本都从它取 |
| F 以主项目测试为判据 | 核心 14 组测试 | `ctest` 14/14 |

## 尚未做（按重要性）

1. **账号登录**与一切需登录接口（搜索、收藏、追更、下载、屏蔽设置页）—— 需要真实账号才能验证；
2. **详情页/搜索页完整界面**：目前详情是一行标签 + 封面，搜索页完全没有；
3. **界面布局整理**：控件集中在少数几行，观感待用户确认；
4. **x86_64 / Windows / macOS 包**：当前只出 linux-arm64；x86_64 需要 Qt 交叉工具链；
5. **未捆绑 Qt**：依赖系统包；正式发版应考虑 AppImage 或捆绑；
6. 四套主题风格（规格 G5）的视觉对齐 —— 完全没开始，目前是 Qt 默认观感。

## 与主项目的偏离（如实）

- 主项目用 Compose 的多套主题与自研动效；本实现目前**没有**做视觉系统对齐（规格 G5 是最大缺口）；
- 主项目的阅读进度是"按作品记录"，本实现只记**最后一次**（单条），多作品续读尚未支持。

## 下一步的精确定法：左右分栏（QSplitter）

现状：所有区域纵向堆叠在一个 `QVBoxLayout`（`outer`）里 —— 所以界面"像表单而不像阅读器"。

改法（约 10 处路由改动，需一次做完，不要中途停）：

1. 在 `MainWindow::setupUi()` 里，`auto* outer = new QVBoxLayout(central);` **之后**加：
   ```cpp
   auto* rootRow   = new QHBoxLayout(central);
   auto* leftPanel  = new QWidget();  auto* leftCol  = new QVBoxLayout(leftPanel);
   auto* rightPanel = new QWidget();  auto* rightCol = new QVBoxLayout(rightPanel);
   auto* split = new QSplitter(Qt::Horizontal);
   split->addWidget(leftPanel); split->addWidget(rightPanel);
   split->setStretchFactor(0, 0); split->setStretchFactor(1, 1);   // 左侧窄、右侧主区域
   leftCol->setContentsMargins(0,0,0,0);  rightCol->setContentsMargins(0,0,0,0);
   ```
2. 把所有属于**左侧**的 `outer->add…` 改为 `leftCol->add…`：
   「① 作品列表」标题块、`listView_`、`listRow`（含加载/更多/屏蔽输入）、「② 章节」标题块、`chapterList_`；
3. 其余 `outer->add…` 改为 `rightCol->add…`：
   「③ 阅读区」标题块、`views`、`pageLabel_`、`form`、`buttons`、`logView_`；
4. 最后 `setCentralWidget(split)`（替换 `central`），并删掉不再使用的 `outer`/`central` 组合方式；
5. 编译 + `ctest` 15/15 + 拍截图对比。

**注意**：`#include <QSplitter>` 要加；`outer->setContentsMargins/setSpacing` 的调用要挪到 `leftCol`/`rightCol` 上（否则留白失效）。
