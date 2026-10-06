# JMComic_Next 桌面端专属模块界面清点（04-desktop-module）

清点对象：`/data/data/com.termux/files/home/jmc/JMComic_Next/desktop/src/main/kotlin/com/jmnext/desktop/`

口径说明：

- 只写源码里**实际存在**的东西，全部给 `文件:行号`；没找到的直接写"未找到"，不做推测。
- 数值一律照抄源码字面量（例如 `rMd = 12.dp`），不换算、不概括。
- 行号口径：Composable 函数行数 = 其 `fun` 首行到该函数闭合 `}`（顶层缩进的 `}`），用 awk 实测，不手数。
- 路径简写：下文 `X.kt:NN` 一律指 `desktop/src/main/kotlin/com/jmnext/desktop/X.kt:NN`。
- 本文档只覆盖桌面端模块；Android 端（`app/`）与共享层（`shared/`）只在做对照时引用。

---

## 0. 总数

| 计数口径 | 数量 |
|---|---|
| 目录内 `.kt` 文件 | 49 |
| 目录总行数（`wc -l` 实测） | 8703 |
| 可路由屏幕 Composable | 20 |
| 其中独立实现 | 19（`FavoriteScreen` 被 `favorites` 与 `tracking` 两条路由共用） |
| 有实现但无任何调用方的屏幕 | 1（`TrackingScreen`） |
| 不可达的占位页壳 | 1（`PageShell`） |
| `App()` 的 `when` 分支数（含兜底） | 21 |
| 左侧常驻导航项 | 15 |
| `BlogTheme.kt` 独立颜色 token | 20（浅 10 + 深 10） |
| `BlogTheme.kt` 独立圆角 token | 3 |

---

## 1. 桌面端 UI 框架

### 1.1 入口链路

| 环节 | 位置 | 内容 |
|---|---|---|
| JVM 入口 | `Main.kt:88` | `fun main()` |
| 日志先初始化 | `Main.kt:91` | `Log.init()`（tee 到 `~/jmnext.log`，`Log.kt:29-37`） |
| 渲染后端默认 | `Main.kt:97-100` | 非 `JMCOMIC_RENDER=GL` 时 `System.setProperty("skiko.renderApi", "SOFTWARE")` |
| 版本与构建时间 | `Main.kt:103-107` | 读 codeSource 的 lastModified，`Log.line("启动", ...)` |
| 应用外壳 | `Main.kt:117-128` | `application { Window(...) { BlogTheme { App() } } }` |
| 主题包裹 | `Main.kt:126` | `BlogTheme { App() }` |
| 根 Composable | `Main.kt:309` | `private fun App()` |

`App()` 的骨架（自上而下）：

```
Row(fillMaxWidth)                                  Main.kt:344
 ├─ SideNav(selected, unreadNotifications, onSelect) Main.kt:345
 └─ Column(weight(1f).fillMaxSize())                 Main.kt:350
     ├─ [可选] 连载提醒降级提示 Row                   Main.kt:352-365
     ├─ 自绘顶栏 Row                                  Main.kt:366-393
     └─ Box(weight(1f))                              Main.kt:395
         ├─ NoticeHost()  瞬时反馈覆盖层              Main.kt:396
         ├─ [可选] 退出登录确认 AlertDialog           Main.kt:398-417
         └─ AnimatedContent(targetState = screen)     Main.kt:418
             └─ SharedPageHost(this@AnimatedContent)  Main.kt:428
                 └─ when (val s = target) { ... }     Main.kt:429-518
```

分发方式：**手写 `when` 对 `Screen` 密封接口做穷尽分支**（`Main.kt:429`），套在 `AnimatedContent` 里做页面切换动效（`Main.kt:418-427`），不是 Navigation Compose，也没有路由表/回退栈数据结构。

页面切换动效（`Main.kt:421-425`）：

- 进场：`fadeIn(tween(280, Motion.Enter))` + `slideInHorizontally(tween(280, Motion.Enter)) { it / 14 }`
- 退场：`fadeOut(tween(120, Motion.Exit))`
- `label = "route"`

路由字符串的两种动态形态：

- `"more/<id>?title=<URLEncoder.encode(title)>"`，构造于 `Main.kt:434`，解析于 `Main.kt:498-511`
- `"comments:<aid>"`，构造于 `Main.kt:444` / `Main.kt:460`，解析于 `Main.kt:479-483`（用 `removePrefix("comments:")`）

### 1.2 `Screen` 密封接口

定义于 `Main.kt:300-306`，`private sealed interface Screen`，共 5 个成员：

| 成员 | 类型 | 字段 | 对应界面 |
|---|---|---|---|
| `Home` | `data object` | 无 | 首页 `HomeScreen` |
| `Detail` | `data class` | `id: String`, `title: String` | 作品详情 `DetailScreen` |
| `Reader` | `data class` | `comicId: String`, `chapterId: String`, `chapterIds: List<String> = emptyList()` | 阅读页 `ReaderScreen` |
| `Login` | `data object` | 无 | 登录/注册/找回 `LoginScreen` |
| `Page` | `data class` | `route: String` | 其余全部页面（按 route 字符串再分派） |

`Page.route` 的取值集合（源码实际用到的字符串，全部列出）：`random`、`week`、`about`、`category`、`profile`、`notifications`、`creator`、`comments:<aid>`、`block`、`appearance`、`tags`、`search`、`favorites`、`history`、`tracking`、`more/<id>?title=<...>`（共 15 条固定 route + 2 条带参数 route）。

`Screen` 是 `private`：`Main.kt:300` 明确写 `private sealed interface Screen`，模块外不可见；没有保存到 SavedState、没有跨进程序列化、没有深链接（未找到）。

`navSelection` 的计算（`Main.kt:314-317`）：只有 `Screen.Page` 的 route 会传给 `SideNav`，其余（Home/Detail/Reader/Login）一律当成 `"home"`，因此详情页与阅读页打开时左侧导航高亮停在"首页"。

### 1.3 `App()` 的 21 个分派分支

| 序号 | 匹配 | 目标 Composable | 行号 |
|---|---|---|---|
| 1 | `Screen.Home` | `HomeScreen(onOpen, onOpenSection, onOpenRandom)` | `Main.kt:430-438` |
| 2 | `Screen.Detail` | `DetailScreen(...)` | `Main.kt:440-452` |
| 3 | `Screen.Reader` | `ReaderScreen(...)` | `Main.kt:454-466` |
| 4 | `Screen.Login` | `LoginScreen(repository, onDone)` | `Main.kt:468` |
| 5 | `Page("random")` | `RandomScreen` | `Main.kt:470` |
| 6 | `Page("week")` | `WeekScreen` | `Main.kt:471` |
| 7 | `Page("about")` | `AboutScreen` | `Main.kt:473` |
| 8 | `Page("category")` | `CategoryScreen` | `Main.kt:474` |
| 9 | `Page("profile")` | `ProfileScreen` | `Main.kt:475` |
| 10 | `Page("notifications")` | `NotificationScreen` | `Main.kt:476` |
| 11 | `Page("creator")` | `CreatorScreen` | `Main.kt:477` |
| 12 | `Page("comments:*")` | `CommentsScreen(aid, onBack)` | `Main.kt:479-483` |
| 13 | `Page("block")` | `BlockScreen` | `Main.kt:484` |
| 14 | `Page("appearance")` | `AppearanceScreen()` | `Main.kt:485` |
| 15 | `Page("tags")` | `TagsScreen(onSearch = { screen = Screen.Page("search") })` | `Main.kt:486-489` |
| 16 | `Page("search")` | `SearchScreen` | `Main.kt:490` |
| 17 | `Page("favorites")` | `FavoriteScreen` | `Main.kt:491` |
| 18 | `Page("history")` | `HistoryScreen` | `Main.kt:492` |
| 19 | `Page("tracking")` | `FavoriteScreen(initialTab = "tracking")` | `Main.kt:495` |
| 20 | `Page("more/<id>")` | `WeeklyUpdateScreen`（id == `JmRepository.WEEKLY_SECTION_ID`）或 `MoreListScreen` | `Main.kt:498-512` |
| 21 | `Page(<其它>)` | `PageShell(title, planned = PAGE_PLANS[route] ?: "（待补）")` | `Main.kt:514-517` |

注意第 16 分支：`TagsScreen` 的 `onSearch` 回调把标签搜索路由到 `Page("search")`，但**没有把标签名带过去**（`Main.kt:486-489` 里只切页，不传 query），与 `DetailScreen.onOpenTag` / `CategoryScreen.onSearch` 两个出口的语义不一致（见 §8）。

### 1.4 左侧常驻导航：15 项、顺序、图标

定义于 `SideNav.kt:25-41`（`val NAV_ITEMS: List<Pair<String, String>>`，`route to 标题`）：

| 序号 | route | 标题 | 图标 | 图标实现 | 桌面目标界面 |
|---|---|---|---|---|---|
| 1 | `home` | 首页 | 无 | 纯文字 | `HomeScreen`（`Main.kt:430`） |
| 2 | `search` | 搜索 | 无 | 纯文字 | `SearchScreen`（`Main.kt:490`） |
| 3 | `category` | 分类 | 无 | 纯文字 | `CategoryScreen`（`Main.kt:474`） |
| 4 | `week` | 周刊 | 无 | 纯文字 | `WeekScreen`（`Main.kt:471`） |
| 5 | `random` | 随机本子 | 无 | 纯文字 | `RandomScreen`（`Main.kt:470`） |
| 6 | `favorites` | 收藏 | 无 | 纯文字 | `FavoriteScreen`（`Main.kt:491`） |
| 7 | `history` | 历史 | 无 | 纯文字 | `HistoryScreen`（`Main.kt:492`） |
| 8 | `tracking` | 追更 | 无 | 纯文字 | `FavoriteScreen(initialTab = "tracking")`（`Main.kt:495`） |
| 9 | `tags` | 标签 | 无 | 纯文字 | `TagsScreen`（`Main.kt:486`） |
| 10 | `creator` | 画师与作品库 | 无 | 纯文字 | `CreatorScreen`（`Main.kt:477`） |
| 11 | `notifications` | 通知 | 无 | 纯文字（未读时标题后追加 `（N）`） | `NotificationScreen`（`Main.kt:476`） |
| 12 | `block` | 屏蔽设置 | 无 | 纯文字 | `BlockScreen`（`Main.kt:484`） |
| 13 | `appearance` | 外观 | 无 | 纯文字 | `AppearanceScreen`（`Main.kt:485`） |
| 14 | `about` | 关于 | 无 | 纯文字 | `AboutScreen`（`Main.kt:473`） |
| 15 | `profile` | 我的 | 无 | 纯文字 | `ProfileScreen`（`Main.kt:475`） |

关于"图标是什么"的实证结论：**15 项全部没有图标**。理由与替代做法在源码里写明了两处：

- `ChapterPickerDialog.kt:38`：「桌面端未引入 material-icons 依赖」
- `Main.kt:534`：「桌面端**未引入 material-icons 依赖**（见 `ChapterPickerDialog` 的说明与 `PageRail` 的做法），所以按钮上写字而不是画骰子图标」

`desktop/build.gradle.kts` 的依赖只有 `compose.desktop.currentOs`、`compose.material3` 与数据层的 5 个包（retrofit / okhttp / kotlinx-serialization / coroutines），没有 `material-icons-*`。全目录 `grep "Icons\."` 零命中，唯一叫 `icon` 的是 AWT 托盘图标位图（`SerialReminder.kt:89`）。

导航项渲染细节（`SideNav.kt:44-82`）：

| 项 | 数值/行为 | 行号 |
|---|---|---|
| 侧栏宽度 | `Modifier.width(176.dp)` | `SideNav.kt:47` |
| 侧栏高度 | `fillMaxHeight()` | `SideNav.kt:48` |
| 侧栏背景 | `MaterialTheme.colorScheme.surface` | `SideNav.kt:49` |
| 侧栏内边距 | `padding(horizontal = 10.dp, vertical = 12.dp)` | `SideNav.kt:50` |
| 项间距 | `Arrangement.spacedBy(2.dp)` | `SideNav.kt:51` |
| 侧栏顶部品牌字 | `Text("JMNeXt")`，`titleMedium`，`primary` 色，`padding(horizontal = 10.dp, vertical = 8.dp)` | `SideNav.kt:53-58` |
| 每项文字 | `title`，`bodyMedium` | `SideNav.kt:69` |
| 选中字色 | `colorScheme.primary`；未选中 `onSurface` | `SideNav.kt:70` |
| 选中背景 | `colorScheme.primaryContainer` + 圆角 `RoundedCornerShape(8.dp)`；未选中背景是 `colorScheme.surface` | `SideNav.kt:73-77` |
| 每项内边距 | `padding(horizontal = 10.dp, vertical = 8.dp)` | `SideNav.kt:79` |
| 未读通知角标 | 标题拼成 `title + "（" + N + "）"`，`N > 99` 时写 `99+`；`N <= 0` 不显示 | `SideNav.kt:64-68` |
| 角标数据来源 | `App()` 里 `LaunchedEffect(authState.loggedIn, screen)` 调 `repository.notificationsUnread().total`，未登录强制 0 | `Main.kt:328-335` |

**折叠行为：未找到。** `SideNav` 没有折叠参数、没有折叠按钮、没有宽度动画；宽度是硬编码 `176.dp`（`SideNav.kt:47`），`App()` 里也没有把任何"折叠/展开"状态传进去（`Main.kt:345-348` 只传 `selected` / `unreadNotifications` / `onSelect`）。

### 1.5 窗口尺寸与最小尺寸

`Main.kt:117-128`：

| 项 | 值 | 行号 |
|---|---|---|
| 应用图标 | `androidx.compose.ui.res.painterResource("icon.png")` | `Main.kt:121` |
| 关闭行为 | `onCloseRequest = ::exitApplication` | `Main.kt:122` |
| 窗口标题 | `title = "JMNeXt"` | `Main.kt:123` |
| 初始尺寸 | `rememberWindowState(width = 1100.dp, height = 820.dp)` | `Main.kt:124` |
| 最小尺寸 | **未找到**（全目录 `minimumSize` / `window.minimumSize` 零命中） | - |
| 初始位置 | **未找到**（`rememberWindowState` 未传 `position`） | - |
| 无边框 / 全屏 / 置顶 | **未找到** | - |

### 1.6 菜单栏与快捷键

| 项 | 结论 | 证据 |
|---|---|---|
| 窗口菜单栏（`MenuBar`） | 未找到 | 全目录 `grep "MenuBar"` 零命中 |
| 全局快捷键（`KeyShortcut` / 窗口级 `onKeyEvent`） | 未找到 | 全目录 `grep "KeyShortcut"` 零命中 |
| 右键上下文菜单 | 未找到 | 无 `DropdownMenu` / 指针按键判断 |
| 拖拽（drag & drop / 文件拖入） | 未找到 | 无 `dragAndDropSource` / `onDrag` |
| 鼠标滚轮专门处理 | 未找到 | 依赖 `LazyColumn` / `LazyVerticalGrid` / `verticalScroll` / `HorizontalPager` 内建滚动 |
| 阅读页键盘翻页 | 存在 | `ReaderScreen.kt:302-325`：`onPreviewKeyEvent`，仅认 `KeyEventType.KeyDown`；`Key.DirectionLeft`/`Key.PageUp` → step -1，`Key.DirectionRight`/`Key.PageDown` → step +1；`mode == ReaderMode.Page` 走 `pagerState.scrollToPage`，否则走 `listState.scrollToItem`；命中后返回 `true` 消费事件 |
| 阅读页焦点申请 | 存在 | `FocusRequester` 建于 `ReaderScreen.kt:193`；`LaunchedEffect(mode, chapterId) { focusRequester.requestFocus() }` 在 `ReaderScreen.kt:196`（键里带 `mode`/`chapterId`，切换阅读模式重建内容区后重新申请焦点） |
| 搜索框回车与 Esc | 存在 | `SearchScreen.kt:199-211`：`Key.Enter` → `runSearch(1)`；`Key.Escape` → 清空 `query`、清空结果、状态回到"输入关键词后回车搜索" |
| 长按（鼠标按住） | 存在 | `RandomFab` 用 `combinedClickable(onClick, onLongClick = onOpenRandomList)`（`Main.kt:553-572`）；`DailyQuickFab` 只用 `combinedClickable(onClick = ...)`，没有 `onLongClick`（`Main.kt:635`） |
| 侧栏滑轨的点击/拖动 | 存在 | `PageRail.kt:121-139`：`detectTapGestures` + `detectVerticalDragGestures` |

---

## 2. 桌面端独有的布局惯例（逐条，带 file:line）

以下每一条都在源码里有明确落点；写成"惯例"是因为它在多个页面重复出现，不是某个页面的一次性写法。

| 序号 | 惯例 | 实现位置 | 数值 / 行为（照抄） |
|---|---|---|---|
| 1 | 应用外壳是「左侧常驻栏 + 右侧纵向两段」 | `Main.kt:344-350` | 外层 `Row(fillMaxWidth)`；左 `SideNav`；右 `Column(Modifier.weight(1f).fillMaxSize())` |
| 2 | 内容区之前有一条固定顶栏（自绘，不是 Material `TopAppBar`） | `Main.kt:366-393` | `Row(fillMaxWidth).padding(horizontal = 16.dp, vertical = 6.dp)`，`spacedBy(12.dp)`；内部三件：品牌字（可点回首页）、登录态文字（`weight(1f)`）、登录/退出按钮 |
| 3 | 内容区用 `Box(weight(1f))` 承载，方便叠覆盖层与浮层 | `Main.kt:395` | 覆盖层 `NoticeHost()` 与页面内容同层（`Main.kt:396`） |
| 4 | 品牌字出现两次（侧栏顶 + 顶栏），两处都可点击回首页 | `SideNav.kt:53-58`、`Main.kt:371-375` | 侧栏那处**不可点**（`SideNav.kt:53-58` 无 `clickable`）；顶栏那处可点（`Main.kt:374`）。同一文案两处显示 |
| 5 | 左侧导航宽度固定 176dp、无折叠 | `SideNav.kt:47` | 见 §1.4 |
| 6 | 列表页统一骨架：`Column(fillMaxSize)` → 顶部条 `Row(fillMaxWidth)` → 列表 `weight(1f)` | `HistoryScreen.kt:41-45`（注释写明约定） | 实例：`RandomScreen.kt:129-137` + `202`、`HistoryScreen.kt:84-105`、`TrackingScreen.kt:117-131`、`NotificationScreen.kt:123-148`、`DiscoverPages.kt:91-133` |
| 7 | 该骨架的成因写进了注释（防"数据到了但列表零高度"） | `HistoryScreen.kt:33-45` | 原实现用 `Row(Modifier.fillMaxSize())` 占满高度导致列表宽高为 0 |
| 8 | 作品网格列宽统一 `GridCells.Adaptive(168.dp)` | `Main.kt:217`、`SearchScreen.kt:334`、`CategoryScreen.kt:316`、`DiscoverPages.kt:129`、`MoreListScreen.kt:127`、`MoreListScreen.kt:272`、`FavoritesScreen.kt:244`、`HistoryScreen.kt:101`、`TrackingScreen.kt:127`、`RandomScreen.kt:198` | 例外：`CreatorScreen.kt:307` 用 `GridCells.Adaptive(150.dp)` |
| 9 | 网格间距统一 | 同上各处 | `contentPadding = PaddingValues(horizontal = 16.dp, vertical = 4.dp)`；`horizontalArrangement = Arrangement.spacedBy(14.dp)`；`verticalArrangement = Arrangement.spacedBy(16.dp)` |
| 10 | 网格追加用 `distinctBy { it.id }` 去重 | `Main.kt:153`、`SearchScreen.kt:159`、`CategoryScreen.kt:90`、`DiscoverPages.kt:57`、`MoreListScreen.kt:74`、`MoreListScreen.kt:206`、`FavoritesScreen.kt:77`、`HistoryScreen.kt:67`、`TrackingScreen.kt:98` | 注释理由见 `CreatorScreen.kt:58`（服务端翻页重叠，重复 key 会让 Compose 崩） |
| 11 | **左右分栏（详情）**：左栏固定 280dp 可滚，右栏占满剩余 | `DetailScreen.kt:291-296`、`DetailScreen.kt:567-568` | 左 `Column(Modifier.width(280.dp).verticalScroll(rememberScrollState()))`，`spacedBy(10.dp)`；右 `LazyColumn(Modifier.fillMaxSize().padding(start = 16.dp))`，`spacedBy(2.dp)` |
| 12 | 详情页封面在左栏，比例 3:4，圆角 10dp | `DetailScreen.kt:308-315` | `fillMaxWidth().aspectRatio(3f / 4f).clip(RoundedCornerShape(10.dp))` |
| 13 | 详情页"相关作品"是左栏内的横向滚动条，每项固定 140dp | `DetailScreen.kt:535-544` | `Row(fillMaxWidth().horizontalScroll(rememberScrollState()))`，`spacedBy(12.dp)`，`Box(Modifier.width(140.dp))` |
| 14 | **左右分栏（分类）**：左栏固定 220dp 可滚，右栏占满 | `CategoryScreen.kt:207-214`、`CategoryScreen.kt:249` | 左 `Column(Modifier.width(220.dp).fillMaxSize().verticalScroll(...).padding(12.dp))`，`spacedBy(2.dp)`；二级项缩进 `padding(start = 20.dp, top = 4.dp, bottom = 4.dp)` |
| 15 | **左右分栏（阅读）**：内容区 `weight(1f)` + 右侧栏固定 40dp 靠 `CenterEnd` | `ReaderScreen.kt:334`、`ReaderScreen.kt:339`、`ReaderScreen.kt:376` | `PageRail(..., modifier = Modifier.align(Alignment.CenterEnd).width(40.dp))` |
| 16 | `PageRail` 滑轨自己画（不用 material3 `Slider` 旋转），轨道**固定占侧栏高度的 70%** | `PageRail.kt:37-54`（理由注释）、`PageRail.kt:86` | `.fillMaxHeight(0.7f)`；底部轨道宽 `6.dp`、圆角 `RoundedCornerShape(3.dp)`，已读段同宽从顶往下，把手 `18.dp` 圆形 |
| 17 | `PageRail` 内部顺序固定（滑轨 → 页码 → 5 个功能按钮 → 上一话/下一话） | `PageRail.kt:82-162` | 页码 `labelSmall` 写 `"${current + 1}/$total"`；5 个按钮文案依次 `modeLabel`、`"章节"`、`"评论"`、`"收藏"`、`"点赞"`；上一话/下一话是 `"<"` / `">"`，各 `height(26.dp)` |
| 18 | `PageRail` 手势按**轨道实际像素高**换算页码 | `PageRail.kt:125-138` | `onSeek(((y / trackH) * max).roundToInt().coerceIn(0, max))`；`trackH` 来自 `onSizeChanged`（`PageRail.kt:88-91`） |
| 19 | **阅读底栏常开**（`visible = true` 硬编码），贴底居中 | `ReaderScreen.kt:440-443` | `ReaderBottomBar(..., modifier = Modifier.align(Alignment.BottomCenter), visible = true, ...)` |
| 20 | 阅读底栏是玻璃近似：半透明表面 + 1dp 描边 + 阴影 + 上圆角 10dp | `ReaderBottomBar.kt:73-79` | `surface.copy(alpha = 0.78f)`，`BorderStroke(1.dp, outline.copy(alpha = 0.35f))`，`tonalElevation = 6.dp`，`shadowElevation = 8.dp`，`RoundedCornerShape(topStart = 10.dp, topEnd = 10.dp)` |
| 21 | 底栏内部顺序固定（左到右） | `ReaderBottomBar.kt:81-113` | 进度条 `height(3.dp)` + 圆角 2dp；下一行 `上一话` / `<` / `页码` / `>` / `下一话`，右侧 `Spacer(weight(1f))` 后 5 个功能按钮（`spacedBy(6.dp)`） |
| 22 | 底栏进出场动画 | `ReaderBottomBar.kt:66-70` | `fadeIn() + slideInVertically { it }` / `fadeOut() + slideOutVertically { it }` |
| 23 | 阅读内容区两种形态共用同一个侧栏与底栏 | `ReaderScreen.kt:327-341` | `mode == ReaderMode.Page` → `PagedReader(weight(1f))`；否则 `LazyColumn(weight(1f).padding(horizontal = 8.dp))`，`spacedBy(0.dp)`（注释：8dp 间距会把跨页画面割断） |
| 24 | `DockedBottomBar` 在桌面端**未找到** | 全目录零命中 | 同名符号只存在于 Android 模块：`app/src/main/kotlin/com/jmnext/ui/JmNavHost.kt:842`（Android 底部标签栏）。桌面端阅读底栏的对应物是 `ReaderBottomBar.kt:44`；桌面端**没有**底部标签栏这一层 |
| 25 | 首页浮钮（Android FAB 的桌面替身）钉在右下角 | `Main.kt:195-247` | 外层 `Box(Modifier.fillMaxSize())` 里两个 `Box(Modifier.align(Alignment.BottomEnd))`；随机按钮 `size(52.dp)` 圆 + `padding(20.dp)`；签到按钮 `size(52.dp)` 圆 + `padding(end = 20.dp, bottom = 20.dp + 52.dp + 12.dp)`（正好叠在随机按钮正上方） |
| 26 | 玻璃面板扩展函数 `Modifier.glassPanel(alpha, corner, shape)` | `Wallpaper.kt:247-266` | `clip(shape)` + `background(surface.copy(alpha))` + `drawBehind` 画上沿高光（浅色 `Color(0x8CFFFFFF)`、深色 `Color(0x33FFFFFF)`，1px 从左上渐隐）+ `border(1.dp, outline, shape)` |
| 27 | `glassPanel` 在桌面端只有**一个**使用点 | `TagsScreen.kt:155` | `.glassPanel(alpha = Appearance.effectiveAlpha, corner = 999)`（胶囊形的本地统计标签） |
| 28 | 壁纸铺底 + 全窗口半透明面板（"玻璃"观感的实现方式） | `BlogTheme.kt:146-149`、`BlogTheme.kt:79`、`BlogTheme.kt:94`、`BlogTheme.kt:81`、`BlogTheme.kt:98` | 主题内 `Box(fillMaxSize) { WallpaperLayer(); content() }`；`background = Color.Transparent`（透出壁纸）；`surface = Tokens.surfaceXxx.copy(alpha = Appearance.effectiveAlpha)` |
| 29 | 壁纸层支持：内置渐变 / 本地图片（Skiko 直接解码）/ 在线壁纸（Bing / 二次元）/ 压暗层 | `Wallpaper.kt:168-237`、`WallpaperRemote.kt:36` | 模糊半径 `Appearance.wallpaperBlur.dp`，仅挡风格 `blurWallpaper = true` 时生效（`Wallpaper.kt:153`）；压暗层画在**所有**壁纸内容之后（`Wallpaper.kt:228-236`，注释写明此前被图片盖住的 bug） |
| 30 | 全应用统一的悬停/按下/键盘焦点反馈 | `Interactions.kt:40-85` | `hoverScale = 1.015f`（默认参数）、`pressScale = 0.985f`、`RoundedCornerShape(10.dp)`、悬停底色 `surfaceVariant.copy(alpha = 0.35f)`、焦点描边 `2.dp` + `primary.copy(alpha = 0.85f * ringAlpha)`；`indication = null` |
| 31 | `jmClickable` 在桌面端只有**一个**使用点 | `ComicCover.kt:47` | 首页用的是另一套 `ComicCard`（`Main.kt:251`，用裸 `clickable`），因此首页封面没有悬停放大与焦点框 |
| 32 | 状态行统一表达（忙时转圈 + 文案，闲时纯文案） | `LoadingHint.kt:53-61` | 转圈 `size(16.dp)` + `strokeWidth = 2.dp`（`LoadingHint.kt:38`）；`LoadingHint` 直接调用点：`DetailScreen.kt:284`、`ReaderScreen.kt:163` |
| 33 | 瞬时反馈覆盖层放应用根部（不随页面销毁） | `Main.kt:396`、`Notices.kt:61-66` | `Box(fillMaxSize, contentAlignment = BottomCenter)`；`padding(bottom = 28.dp)`；`ERROR` 停留 `4800ms`，其余 `2400ms`（`Notices.kt:64-65`、`Notices.kt:71`） |
| 34 | 动效 token 集中在 `Motion` | `Motion.kt:19-46` | `QUICK_MS = 120`、`NORMAL_MS = 220`、`PAGE_MS = 280`、`IMAGE_MS = 260`；`Standard = CubicBezierEasing(0.2f, 0f, 0f, 1f)`、`Enter = CubicBezierEasing(0.05f, 0.7f, 0.1f, 1f)`、`Exit = CubicBezierEasing(0.3f, 0f, 0.8f, 0.15f)` |
| 35 | 图片淡入用同一套 token | `ComicCover.kt:41-45`、`DetailScreen.kt:303-307` | `tween(Motion.IMAGE_MS, Motion.Standard)`；详情页封面入场 `tween(Motion.NORMAL_MS, Motion.Enter)` 且缩放从 `0.94f` 到 `1f` |
| 36 | 共享元素宿主包在 `AnimatedContent` 内 | `Main.kt:428`、`SharedElement.kt:69-79` | `SharedPageHost(this@AnimatedContent) { when (...) { ... } }`，内部建 `SharedTransitionLayout` 并提供 `LocalSharedScope` / `LocalPageVisibility` |
| 37 | 封面在两个页面用同一个 key 登记共享元素 | `ComicCover.kt:53`、`DetailScreen.kt:313`、`SharedElement.kt:28` | `jmCoverKey(comicId) = "jm-cover-$comicId"`；时长 `tween(Motion.PAGE_MS, Motion.Enter)`（`SharedElement.kt:43`） |
| 38 | 桌面端按操作系统显式挑 CJK 字体族 | `BlogTheme.kt:162-177` | Windows 候选 `"Microsoft YaHei UI"`/`"Microsoft YaHei"`/`"SimHei"`/`"SimSun"`；macOS `"PingFang SC"`/`"Hiragino Sans GB"`/`"STHeiti"`/`"Heiti SC"`；其它 `"Noto Sans CJK SC"`/`"Source Han Sans SC"`/`"Noto Sans SC"`/`"WenQuanYi Micro Hei"`/`"Droid Sans Fallback"`/`"DejaVu Sans"`；查找走 `org.jetbrains.skia.FontMgr.default.matchFamilyStyle(name, FontStyle.NORMAL)`，包装成 `FontFamily(Typeface(sk))` |
| 39 | 对话框统一用 M3 `AlertDialog`；章节选择自带分页 | `ChapterPickerDialog.kt:52-118`、`Main.kt:399-416` | `chunkSize = 50`；`heightIn(max = 380.dp).verticalScroll(scroll)`；滚动状态**按页取**（`remember(safePage) { ScrollState(0) }`，`ChapterPickerDialog.kt:66`）；当前话用 `primaryContainer` 底色 |
| 40 | 页面顶部的"状态/错误"文字可选可复制 | `DetailScreen.kt:286`、`DetailScreen.kt:613-615` | 用 `SelectionContainer` 包住；注释写明只在这两处加（`DetailScreen.kt:611-612`） |
| 41 | 屏蔽提示条统一在列表内容之上、且分两级 | `ComicCover.kt:88-96`、`TagBlocking.kt:40-54` | 数据层挡掉用 `BlockedNotice(hidden)`；标签级挡掉用 `BlockedByTagBanner(blocked) { idOf }` + "允许一次"按钮 |
| 42 | 取整部作品下载交给系统浏览器（桌面端没有 DownloadManager） | `DetailScreen.kt:249-251`、`DetailScreen.kt:501-527` | `java.awt.Desktop.getDesktop().browse(java.net.URI(url))`；失败时把地址一并显示（`DetailScreen.kt:259-260`） |

---

## 3. 桌面端 token：`BlogTheme.kt`

### 3.1 `private object Tokens`（`BlogTheme.kt:31-60`）字段清单

浅色（`BlogTheme.kt:33-42`）：

| 字段 | 值（照抄） |
|---|---|
| `accentLight` | `Color(0xFF0F6CBD)` |
| `accentLightHover` | `Color(0xFF115EA3)` |
| `accentLightSoft` | `Color(0x1A0F6CBD)` |
| `textLight` | `Color(0xFF16181D)` |
| `textSecondaryLight` | `Color(0xFF4A4F5A)` |
| `textTertiaryLight` | `Color(0xFF767C88)` |
| `surfaceLight` | `Color(0xFFF6F7FA)` |
| `surfaceSunkenLight` | `Color(0x090F172A)` |
| `surfaceHoverLight` | `Color(0x0D0F172A)` |
| `strokeLight` | `Color(0x170F172A)` |

深色（`BlogTheme.kt:45-54`）：

| 字段 | 值（照抄） |
|---|---|
| `accentDark` | `Color(0xFF60CDFF)` |
| `accentDarkHover` | `Color(0xFF7FD8FF)` |
| `accentDarkSoft` | `Color(0x2460CDFF)` |
| `textDark` | `Color(0xFFF3F4F7)` |
| `textSecondaryDark` | `Color(0xB8FFFFFF)` |
| `textTertiaryDark` | `Color(0x80FFFFFF)` |
| `surfaceDark` | `Color(0xFF16181E)` |
| `surfaceSunkenDark` | `Color(0x3D000000)` |
| `surfaceHoverDark` | `Color(0x14FFFFFF)` |
| `strokeDark` | `Color(0x1AFFFFFF)` |

圆角（`BlogTheme.kt:56-59`，注释写"博客的 --r-sm/md/lg 分别 8/12/18"）：

| 字段 | 值 |
|---|---|
| `rSm` | `8.dp` |
| `rMd` | `12.dp` |
| `rLg` | `18.dp` |

### 3.2 token 到 Material3 的映射（`BlogTheme.kt:73-105`）

深色 `darkColorScheme(...)`（`BlogTheme.kt:74-87`）：

| M3 槽位 | 取值 |
|---|---|
| `primary` | `Tokens.accentDark` |
| `onPrimary` | `Color(0xFF04263A)` |
| `primaryContainer` | `Tokens.accentDarkSoft` |
| `onPrimaryContainer` | `Tokens.accentDark` |
| `background` | `Color.Transparent`（注释：透出壁纸） |
| `onBackground` | `Tokens.textDark` |
| `surface` | `Tokens.surfaceDark.copy(alpha = Appearance.effectiveAlpha)` |
| `onSurface` | `Tokens.textDark` |
| `surfaceVariant` | `Tokens.surfaceSunkenDark` |
| `onSurfaceVariant` | `Tokens.textSecondaryDark` |
| `outline` | `Tokens.strokeDark` |
| `outlineVariant` | `Tokens.strokeDark` |

浅色 `lightColorScheme(...)`（`BlogTheme.kt:89-104`）：

| M3 槽位 | 取值 |
|---|---|
| `primary` | `Tokens.accentLight` |
| `onPrimary` | `Color.White` |
| `primaryContainer` | `Tokens.accentLightSoft` |
| `onPrimaryContainer` | `Tokens.accentLight` |
| `background` | `Color.Transparent`（注释：透出壁纸） |
| `onBackground` | `Tokens.textLight` |
| `surface` | `Tokens.surfaceLight.copy(alpha = Appearance.effectiveAlpha)` |
| `onSurface` | `Tokens.textLight` |
| `surfaceVariant` | `Tokens.surfaceSunkenLight` |
| `onSurfaceVariant` | `Tokens.textSecondaryLight` |
| `outline` | `Tokens.strokeLight` |
| `outlineVariant` | `Tokens.strokeLight` |

圆角到 M3 `Shapes`（`BlogTheme.kt:137-142`）：

| M3 `Shapes` 槽位 | 取值 |
|---|---|
| `extraSmall` | `RoundedCornerShape(Tokens.rSm / 2)` |
| `small` | `RoundedCornerShape(Tokens.rSm)` |
| `medium` | `RoundedCornerShape(Tokens.rMd)` |
| `large` | `RoundedCornerShape(Tokens.rLg)` |

字号（`BlogTheme.kt:112-133`）：

| 样式 | 覆盖项 |
|---|---|
| `titleMedium` | `fontFamily = cjk`, `fontSize = 17.sp`, `fontWeight = FontWeight.SemiBold` |
| `bodyMedium` | `fontFamily = cjk`, `fontSize = 15.sp`, `lineHeight = 22.sp` |
| `bodySmall` | `fontFamily = cjk`, `fontSize = 13.6.sp`, `lineHeight = 20.sp` |
| `labelSmall` | `fontFamily = cjk`, `fontSize = 12.5.sp` |
| `displayLarge` / `displayMedium` / `displaySmall` / `headlineLarge` / `headlineMedium` / `headlineSmall` / `titleLarge` / `titleSmall` / `bodyLarge` / `labelLarge` / `labelMedium` | 只覆盖 `fontFamily = cjk` |

深浅色的取值优先级（`BlogTheme.kt:62-71`）：`BlogTheme(dark = isSystemInDarkTheme(), content)`；实际用 `if (Appearance.userChoseDark) Appearance.dark else systemDark`。注释写明此前只看系统值，导致外观页切换深浅色"完全没反应"（`BlogTheme.kt:67-69`）。

### 3.3 定义了但从未使用的 token（源码实测）

| token | 声明位置 | 引用次数 |
|---|---|---|
| `accentLightHover` | `BlogTheme.kt:34` | 0 |
| `textTertiaryLight` | `BlogTheme.kt:38` | 0 |
| `surfaceHoverLight` | `BlogTheme.kt:41` | 0 |
| `accentDarkHover` | `BlogTheme.kt:46` | 0 |
| `textTertiaryDark` | `BlogTheme.kt:50` | 0 |
| `surfaceHoverDark` | `BlogTheme.kt:53` | 0 |

（判定方法：全目录 `grep "Tokens.<字段>"` 计数；`accentLightSoft` / `surfaceSunkenLight` / `strokeLight` / `textSecondaryLight` / `surfaceLight` 各 1 次，`accentLight` / `textLight` / `strokeLight` / 深色同理各 1-2 次，只有上表六个为 0。）

### 3.4 与 `app/.../ui/theme/`（`JmPalette` / `ThemeStyle`）的关系

**结论：两套完全独立，不存在复用；只是设计来源相同（都声称从博客 `src/styles/global.css` 逐项抄）与部分数值重合。**

证据：

| 判据 | 事实 |
|---|---|
| 模块可见性 | `Tokens` 是 `private object`（`BlogTheme.kt:31`），只在 `BlogTheme.kt` 内可见 |
| Gradle 依赖 | `desktop/build.gradle.kts` 只把 `../shared/src/main/kotlin` 加进 `sourceSets.main`，依赖只有 `compose.desktop.currentOs`、`compose.material3` 与数据层 5 个包；**没有** `project(":app")` |
| 符号引用 | `grep -rn "jmnext.ui.theme\|JmPalette\|ThemeStyle\|JmTheme" desktop/src/` → 零命中 |
| 对面那套的位置与规模 | `app/src/main/kotlin/com/jmnext/ui/theme/`：`Tokens.kt` 274 行（`JmPalette` data class、`LightPalette`、`DarkPalette`、`Radius`、`Spacing`、`Sizing`、`Motion`、`Glass`、`ElevationBase`/`Elevation`、`FontSize`）、`Palettes.kt` 232 行、`Theme.kt` 324 行、`ThemeStyle.kt` 420 行、`Shapes.kt` 174 行 |
| 桌面端有没有 `ThemeStyle` | 没有。桌面端对应概念是 `GlassStyle`（`Wallpaper.kt:43-49`），5 档：`WindowGlass("毛玻璃", 0.72f, 12, true)`、`Translucent("半透明", 0.55f, 18, true)`、`FlatBlur("扁平模糊", 0.85f, 8, true)`、`Miuix("MIUI", 0.90f, 22, false)`、`Material("Material", 1.00f, 12, false)`（参数依次为 `label, surfaceAlpha, corner, blurWallpaper`） |

数值重合与差异对照（只列能逐字对上的）：

| 项 | 桌面 `BlogTheme.kt` | Android `ui/theme/Tokens.kt` | 结论 |
|---|---|---|---|
| 浅色强调色 | `0xFF0F6CBD`（:33） | `LightPalette.accent = 0xFF0F6CBD` | 相同 |
| 浅色强调 hover | `0xFF115EA3`（:34） | `LightPalette.accentHover = 0xFF115EA3` | 相同 |
| 深色强调色 | `0xFF60CDFF`（:45） | `DarkPalette.accent = 0xFF60CDFF` | 相同 |
| 深色强调 hover | `0xFF7FD8FF`（:46） | `DarkPalette.accentHover = 0xFF7FD8FF` | 相同 |
| 浅色文字三档 | `0xFF16181D` / `0xFF4A4F5A` / `0xFF767C88`（:36-38） | `LightPalette.text` / `textSecondary` / `textTertiary` 同值 | 相同 |
| 深色文字三档 | `0xFFF3F4F7` / `0xB8FFFFFF` / `0x80FFFFFF`（:48-50） | `DarkPalette.text` / `textSecondary` / `textTertiary` 同值 | 相同 |
| 深色 sunken / hover / stroke | `0x3D000000` / `0x14FFFFFF` / `0x1AFFFFFF`（:52-54） | `DarkPalette.surfaceSunken` / `surfaceHover` / `stroke` 同值 | 相同 |
| 浅色 sunken / hover / stroke | `0x090F172A` / `0x0D0F172A` / `0x170F172A`（:40-42） | `LightPalette.surfaceSunken` / `surfaceHover` / `stroke` 同值 | 相同 |
| 深色表面基色 | `surfaceDark = 0xFF16181E`（:51），直接用 | `DarkPalette.surfaceMica = 0xB816181E` | 基色相同，alpha 不同（桌面在 `BlogTheme.kt:81` 另乘 `Appearance.effectiveAlpha`） |
| 浅色表面 | `surfaceLight = 0xFFF6F7FA`（:39） | `LightPalette.surfaceMica = 0xB8F6F7FA` | 同上 |
| 圆角 | `rSm = 8.dp` / `rMd = 12.dp` / `rLg = 18.dp`（:57-59） | `Radius` 随风格变（`ThemeStyle.kt:28-44` 的 `LocalJmSpec.current.radius`） | 桌面是固定三个值；Android 是随风格查表 |

Android 侧 `RadiusScale` 的 4 处字面量（用于判断桌面那三个值有没有对应物）：

| 风格 | 行号 | xs / sm / md / lg / xl |
|---|---|---|
| `WindowGlass` | `ThemeStyle.kt:258` | 2 / 4 / 6 / 8 / 12 |
| （`Translucent`） | `ThemeStyle.kt:290` | 由 `windowGlass.copy(...)` 继承，几何同上（2 / 4 / 6 / 8 / 12） |
| `Miuix` | `ThemeStyle.kt:311` | 4 / 8 / 12 / 16 / 24 |
| `Material` | `ThemeStyle.kt:346` | 4 / 8 / 12 / 12 / 28 |
| `FlatBlur` | `ThemeStyle.kt:382` | 8 / 12 / 16 / 20 / 28 |

据此：桌面 `rSm = 8.dp` 与 `Miuix`/`Material` 的 `sm` 一致；`rMd = 12.dp` 与 `Miuix`/`Material` 的 `md`、`FlatBlur` 的 `sm` 一致；**桌面 `rLg = 18.dp` 在上述五套风格里没有任何一个对应值**（`Miuix` 是 16、`Material` 是 12、`FlatBlur` 是 20、`WindowGlass`/`Translucent` 是 8）。也就是说桌面那套圆角不是"某一套风格的拷贝"，是独立取的一组值。

---

## 4. 逐屏清单（桌面端，按代码行数从多到少）

口径：只列**可路由的屏幕级 Composable**（`App()` 的 21 个分支能到达的），外加 2 个不可达项（`TrackingScreen`、`PageShell`）以及 1 个阅读子模式（`PagedReader`）单独标注。行数 = 函数首行到闭合 `}`。

| 排名 | 屏幕 | Composable | 文件:行 | 行数 | 布局骨架 | 关键组件（源码里实际出现的） | 依赖状态（Composable 内 `remember` 的状态 + 调用的 repository 接口） |
|---|---|---|---|---|---|---|---|
| 1 | 作品详情 | `DetailScreen` | `DetailScreen.kt:65` | 540 | `Column(fillMaxSize)` → 顶部条 `Row(fillMaxWidth)` → `Row(fillMaxSize, padding h=16)` 左右分栏（左 280dp 可滚 / 右 `LazyColumn(fillMaxSize)`） | `LoadingHint`、`SelectionContainer`、`jmSharedElement`、`InfoLine`、标签行（`Text` + `屏蔽`/`标星` 按钮）、下载条、`ComicCover(140dp)` 横向条、`Button`（收藏/点赞/追更/从头开始/继续观看） | 状态：`detail, status, loading, favorite, tracked, likeBusy, likeMessage, trackBusy, favMessage, favBusy, downloadBusy, downloadMessage, tagBusy, tagMessage, tagQuery, tagItems, tagResultStatus, starredTags`；接口：`album`、`isTracked`、`favoriteTags`、`updateFavoriteTags`、`search`、`toggleFavorite`、`toggleTracking`、`like`、`albumDownload`、`blockStore` |
| 2 | 阅读页 | `ReaderScreen` | `ReaderScreen.kt:69` | 424 | `Box(fillMaxSize)` → 顶部条 `Row` + 内容 `Row(focusable, onPreviewKeyEvent)`（`weight(1f)`）+ `PageRail`（`CenterEnd`）+ `ReaderBottomBar`（`BottomCenter`） | `LoadingHint`、`PageRail`、`ReaderBottomBar`、`LazyColumn`（纵向）/ `PagedReader`（横向）、`PageItem`、`ChapterPickerDialog`、`FocusRequester`、`RemoteImage` 预取、`SelfTuner` 采样 | 状态：`payload, series, pickerOpen, mode, status, loading, retryToken, prefetched, pageProgress, listState, pagerState, currentPage, prefetchInFlight`；接口：`read`、`needsUnscramble`、`album`、`toggleFavorite`、`like` |
| 3 | 画师与作品库 | `CreatorScreen` | `CreatorScreen.kt:61` | 326 | `Column(fillMaxSize)` → 顶部条 `Row` → 三个互斥区间（选中画师提示行 / 作品信息 `Column` / 作品内容 `LazyColumn(weight(1f))`）+ `LazyVerticalGrid(Adaptive(150.dp), weight(1f))` | `Button` 画师/作品切换、`OutlinedTextField(300dp)`、`Image`(圆形头像 `CircleShape`，作品封面 3:4)、`BlockedByTagBanner` ×2、相关作品横向条(140dp) | 状态：`mode, query, authors, works, total, page, busy, status, selectedAuthor, workInfo, workContent, contentLoading`；接口：`creatorAuthors`、`creatorWorks`、`creatorWorkInfo`、`creatorWorkContent`、`creatorWorksByAuthor`、`creatorContentUrl` |
| 4 | 分类 | `CategoryScreen` | `CategoryScreen.kt:51` | 300 | `Row(fillMaxWidth)` → 左栏 `Column(width(220.dp).fillMaxSize().verticalScroll())` + 右栏 `Column(fillMaxSize)`（顶部条 `Row` → `BlockedNotice` → 结果网格 `LazyVerticalGrid(fillMaxSize)`） | `NavChip`、`TagChip`、`FlowRow`（热门标签兜底与分组标签）、`BlockedByTagBanner`、`Button` 排序档（最新 / 最多爱心 / 总排行 / 月排行 / 周排行） | 状态：`nodes, current, expanded, currentName, sort, items, hidden, page, busy, status, blocks, hotTags, treeError, tagMode, tagPage`；接口：`categories`、`categoryFilter`、`hotTags`、`search` |
| 5 | 搜索 | `SearchScreen` | `SearchScreen.kt:94` | 257 | `Column(fillMaxSize)` → 搜索条 `Row` → 排序档 `Row` → 字段档 `Row` → 年月 `Row` → 历史 `Row` → 建议两行 → `BlockedNotice` → 标签屏蔽条 → 结果 `LazyVerticalGrid(Adaptive(168.dp), fillMaxSize)` | `OutlinedTextField(360dp, onKeyEvent)`、`SEARCH_ORDERS`(5 档)、`SEARCH_TYPES`(5 档)、年月输入框(150dp/130dp)、`SearchHistory`(最多 20 条)、`BlockedNotice`、`ComicCover` | 状态：`history, query, items, hidden, total, page, busy, status, order, type, year, month, hotTags, recommend, suggestBusy, hiddenIds`；接口：`search`、`hotTags`、`randomRecommend`；`TagBlocker.hidden` 直接订阅 |
| 6 | 收藏 | `FavoriteScreen` | `FavoritesScreen.kt:52` | 226 | `Column(fillMaxSize)` → 顶部条 `Row`（收藏/追更两个标签 + 状态 + 加载更多）→ [追更分支：内嵌 `TrackingList` 后 `return@Column`] → 收藏夹切换 `Row` → 收藏夹管理 `Row` → `LazyVerticalGrid(weight(1f))` | `TextButton` 标签、`Button` 文件夹（`· 全部` + 每个 folder）、`OutlinedTextField(220dp)` + `新建`/`改名`/`删除`、`ComicCover` + `取消收藏` | 状态：`tab, folders, selected, items, total, page, busy, status, notice, folderName`；接口：`favorites(page, folderId)`、`editFavoriteFolder(type=add/edit/del)`、`toggleFavorite`；`initialTab` 默认 `"favorite"` |
| 7 | 外观 | `AppearanceScreen` | `AppearanceScreen.kt:44` | 202 | `Column(fillMaxSize).verticalScroll().padding(24.dp)`，`spacedBy(18.dp)`，共 6 段 | `Button` ×5（GlassStyle 5 档）、`Slider` ×3（壁纸模糊 `0f..256f` / 面板浓度 `0f..1f` / 壁纸压暗 `0f..100f`，宽度 420dp）、`WallpaperPreset` 4 档预览块（96dp 宽、56dp 高渐变）、`WallpaperRemoteSection`、AWT `FileDialog` 选本地图、`Button` 连载提醒开关、`Button` 阅读形态（`ReaderMode.entries`） | 状态：`refresh`（自增触发重组）+ 全局对象 `Appearance` / `SerialReminder` / `RemoteWallpaper` / `ReaderModePref`；**不调用 repository** |
| 8 | 登录/注册/找回 | `LoginScreen` | `LoginScreen.kt:40` | 194 | `Column(fillMaxSize).padding(24.dp)`，`spacedBy(12.dp)` | 三个模式切换 `TextButton`、`OutlinedTextField`（用户名/密码/确认密码/邮箱，宽 360dp，密码用 `PasswordVisualTransformation`）、性别 `TextButton` 男/女（`"m"`/`"f"`）、提交 `Button`、已登录时 `退出登录` | 状态：`mode, username, password, passwordConfirm, email, gender, busy, message`；接口：`login`、`register`、`forgotPassword`、`logout` |
| 9 | 标签 | `TagsScreen` | `TagsScreen.kt:50` | 172 | `Column(fillMaxSize).verticalScroll().padding(24.dp)`，两大区：本地统计 `FlowRow` / 网站标星 `FlowRow` | 扫描 `Button`、`glassPanel(corner=999)` 统计片（标签 + 次数）、`OutlinedTextField(240dp)` + `添加标星`、标星片 + `移除` | 状态：`counts, cachedAt, starred, busy, status, draft`；接口：`FavoriteTags.cached/refresh/cachedAt`、`favoriteTags`、`updateFavoriteTags(add/remove)` |
| 10 | 通知 | `NotificationScreen` | `NotificationScreen.kt:52` | 160 | `Column(fillMaxSize)` → 顶部条 `Row`（标题 + 3 个筛选标签 + 未读数 + 状态 + 加载更多）→ `LazyColumn(weight(1f))` | 3 个筛选 `TextButton`（全部 / 追更 / 站内通知）、未读圆点 `Box(size(8.dp), CircleShape)`、追更条目走 `followedUpdates()`、站内通知正文走 `siteNoticeHtml()?.plainText()`、`标记已读` | 状态：`tab, items, unread, page, busy, status`；接口：`notifications(type, page)`、`notificationsUnread`、`markNotificationRead(id, true)` |
| 11 | 随机本子 | `RandomScreen` | `RandomScreen.kt:66` | 143 | `Column(fillMaxSize)` → 顶部条 `Row`（标题 + 状态 + 换一批 + 版式切换 + 偏好排序开关）→ 一行说明 → [列表版 `LazyColumn(weight(1f))` / 网格版 `LazyVerticalGrid(weight(1f))`] | `StatusLine`、`Button`、版式按钮（网格 ↔ 列表）、偏好排序 `TextButton`、列表版的 84dp 缩略图行、`ComicCover`、`BlockedByTagBanner` | 状态：`layout, items, busy, status, ranked`；接口：`randomRecommend`、`FavoriteTags.cached/refresh`、`album(id)`（批次标签，`chunked(3)` 限流）、`RandomRanking.rank`；prefs 键 `random_layout`（`RandomScreen.kt:215`） |
| 12 | 评论 | `CommentsScreen` | `CommentsScreen.kt:54` | 142 | `Column(fillMaxSize)` → 顶部条 `Row` → 发表区 `Column`（回复提示行 + 输入行）→ `LazyColumn(fillMaxSize)` | `OutlinedTextField(520dp)`、发表/回复 `Button`、`CommentRow`（递归一层，缩进 `(depth * 20).dp`、圆角 10dp、头像 28dp 圆）、`删除` 仅自己的评论显示 | 状态：`items, total, page, busy, status, input, replyTo, notice`；`authState.member?.uid` 用于比对；接口：`comments(aid, page)`、`sendComment`、`deleteComment` |
| 13 | 首页 | `HomeScreen` | `Main.kt:131` | 118 | `Box(fillMaxSize)` → 内层 `Column(fillMaxSize)`（状态 `Text` → 按钮 `Row` → `LazyVerticalGrid(Adaptive(168.dp), fillMaxSize)`）+ 右下角两个浮钮 | `Button` 刷新/加载更多、`LazyVerticalGrid`（首项跨整行放 `PromoteHeader`）、`ComicCard`、`DailyQuickFab`、`RandomFab`、`PromoteHeader` | 状态：`items, status, page, total, busy, updatedIds`；接口：`bootstrap`、`latest(page)`、`notifications(TYPE_COMIC_FOLLOW)`、`randomRecommend`、`daily(uid)`、`dailyCheck`；`TagBlocker.init`（`Main.kt:173`） |
| 14 | 周刊 | `WeekScreen` | `DiscoverPages.kt:40` | 107 | `Column(fillMaxSize)` → 头部 `Column`（标题 + 状态 + 刊期横滚 `Row` + 类型横滚 `Row`）→ `BlockedNotice` → `LazyVerticalGrid(Adaptive(168.dp), fillMaxSize)` | 刊期 `Button` ×N（横滚）、类型 `Button` ×N（`types.size > 1` 才显示）、`加载更多`、`ComicCover` | 状态：`issues, types, issueId, typeId, items, hidden, page, busy, status`；接口：`weekIssues`、`weekList(issueId, type, page)` |
| 15 | 关于 | `AboutScreen` | `AboutScreen.kt:43` | 101 | `Column(fillMaxSize).verticalScroll().padding(24.dp)`，6 个 `Section` 卡片 | `Section`（圆角 12dp + `surfaceVariant` 底 + 14dp 内距）、`InfoRow`、`Button` 检查更新、`TextButton` 逐资产下载链接 + 打开发布页、`LocalUriHandler` | 状态：`checking, result, updatable`；依赖：`DESKTOP_VERSION`、`UpdateCheck.fetchLatestTag(repository.okHttp)`、`UpdateCheck.isNewer/desktopAssetNames/assetUrl/releaseUrl` |
| 16 | 我的 | `ProfileScreen` | `ProfileScreen.kt:44` | 101 | `Column(fillMaxSize).verticalScroll().padding(24.dp)`，3 个 `Card` | `Card`（圆角 12dp）、`InfoRow`、`Button` 去登录、`Button` 签到、`StatusLine`；未登录时卡片里显示"未登录" | 状态：`dailyId, eventName, checkMsg, status, busy`；`authState`（`collectAsState`）；接口：`daily(uid)`、`dailyCheck(uid, dailyId)` |
| 17 | 连载更新（每周表） | `WeeklyUpdateScreen` | `MoreListScreen.kt:186` | 98 | `Column(fillMaxSize)` → 头部 `Column`（标题 + 类型 `Row` + 星期 `Row` + 操作 `Row`）→ `BlockedByTagBanner` → `LazyVerticalGrid(Adaptive(168.dp), fillMaxSize)` | 类型 `Button` ×3（全部/漫画/韩漫）、星期 `Button` ×8（周一..周日 + 完结）、刷新/加载更多、`ComicCover` | 状态：`items, page, type, day, busy, exhausted, status, hiddenIds`；接口：`weeklyUpdate(type, date, page)`（page 从 1 起算，无 total） |
| 18 | 分区更多 | `MoreListScreen` | `MoreListScreen.kt:48` | 91 | `Column(fillMaxSize)` → 头部 `Column`（标题 + 刷新/加载更多 `Row`）→ `BlockedNotice` → `BlockedByTagBanner` → `LazyVerticalGrid(Adaptive(168.dp), fillMaxSize)` | `Button` 刷新 / 加载更多、`BlockedNotice`、`BlockedByTagBanner`、`ComicCover` | 状态：`items, hiddenIds, hidden, total, page, busy, exhausted, status`；接口：`promoteList(id, page)`（page 从 0 起算） |
| 19 | 历史 | `HistoryScreen` | `HistoryScreen.kt:51` | 85 | `Column(fillMaxSize)` → 顶部条 `Row(fillMaxWidth)` → `LazyVerticalGrid(weight(1f))` | `StatusLine`、`加载更多`、`ComicCover` + 每项一个 `删除` | 状态：`items, total, page, busy, status, notice`；接口：`history(page)`、`deleteHistory(item.id)`；未登录只显示"需要登录后才能查看历史" |
| 20 | 屏蔽设置 | `BlockScreen` | `BlockScreen.kt:40` | 45 | `Column(fillMaxSize).verticalScroll().padding(24.dp)`，3 个 `BlockSection` | `BlockSection`（标题 + 命中数 + 提示 + `OutlinedTextField(320dp)` + 添加/移除）、三类名单（关键词 / 标签 / 分类名） | 状态：`rules`（`store.state.collectAsState()`）+ `BlockSection` 自己的 `input`；数据：`repository.blockStore`（`addWord/removeWord/addTag/removeTag/addCategory/removeCategory`）；`blockStore == null` 时提前返回一句提示 |
| - | 追更（**不可达**） | `TrackingScreen` | `TrackingScreen.kt:43` | 20 | `Column(fillMaxSize)` → 顶部条 `Row(fillMaxWidth)` → 内嵌 `TrackingList(weight(1f))` | `TrackingList` | 状态：`status`；**全目录无任何调用方**（`Main.kt:495` 把 `tracking` 路由到 `FavoriteScreen(initialTab = "tracking")`） |
| - | 占位页壳（**不可达**） | `PageShell` | `PageShell.kt:25` | 24 | `Column(fillMaxSize).padding(24.dp)` → 标题 + 一张 `surfaceVariant` 卡片 | 文案"这一页还没接数据" + `PAGE_PLANS[route]` | 无状态。15 条 NAV route 全部在 `Main.kt:470-495` 被具体分支吃掉，兜底 `Main.kt:514` 走不到（见 §7） |
| - | 阅读子模式 | `PagedReader` | `PagedReader.kt:28` | 20 | `HorizontalPager(state, fillMaxSize)`，每页一个居中 `Box` | `PageItem`（与纵向模式共用） | 受控组件：`PagerState` 由 `ReaderScreen` 创建并传入（`ReaderScreen.kt:187-189`）；`LaunchedEffect(state)` 把 `currentPage` 报给 `onPageChange` |

### 4.1 非屏幕级共用组件（供 Qt 复刻时对照，不参与上面的排序）

| 组件 | 文件:行 | 行数 | 作用 |
|---|---|---|---|
| `SideNav` | `SideNav.kt:44` | 39 | 左侧常驻导航（15 项 + 未读角标） |
| `PageRail` | `PageRail.kt:56` | 109 | 阅读页右侧竖排页码栏（自绘滑轨） |
| `ReaderBottomBar` | `ReaderBottomBar.kt:44` | 75 | 阅读页底部常开栏 |
| `ChapterPickerDialog` | `ChapterPickerDialog.kt:46` | 75 | 章节选择对话框（每页 50 话） |
| `ComicCover` | `ComicCover.kt:37` | 43 | 作品卡片（封面 3:4 + 标题 + 作者），用于 12 处 |
| `BlockedNotice` | `ComicCover.kt:88` | 9 | 数据层屏蔽条 |
| `ComicCard` | `Main.kt:251` | 47 | **首页专用**的作品卡片（与 `ComicCover` 重复实现，见 §7） |
| `PromoteHeader` | `HomeSections.kt:39` | 58 | 首页推荐分区（分区标题 + 更多 + 横滚封面 140dp） |
| `RandomFab` | `Main.kt:540` | 42 | 首页随机浮钮（单击抽一本，长按进随机页） |
| `DailyQuickFab` | `Main.kt:599` | 91 | 首页签到浮钮（未登录不占位） |
| `StatusLine` / `LoadingHint` | `LoadingHint.kt:53` / `LoadingHint.kt:28` | 9 / 14 | 状态行与转圈提示 |
| `WallpaperLayer` | `Wallpaper.kt:149` | 90 | 壁纸层（渐变 / 本地图 / 在线图 / 压暗） |
| `WallpaperRemoteSection` | `WallpaperRemoteSection.kt:35` | 98 | 外观页的在线壁纸一节 |
| `NoticeHost` | `Notices.kt:61` | 43 | 瞬时反馈覆盖层 |
| `CommentRow` | `CommentsScreen.kt:199` | 66 | 单条评论（含嵌套回复递归） |
| `BlockSection` | `BlockScreen.kt:87` | 70 | 一类屏蔽名单的增删区块 |
| `InfoLine` / `InfoRow` / `Card` / `Section` | `DetailScreen.kt:607` / `ProfileScreen.kt:163` / `ProfileScreen.kt:147` / `AboutScreen.kt:146` | 11 / 6 / 15 / 14 | 键值行与卡片容器 |
| `TagChip` / `NavChip` | `CategoryScreen.kt:354` / `CategoryScreen.kt:368` | 12 / 13 | 标签片与导航片 |
| `RailAction` / `BarAction` | `PageRail.kt:168` / `ReaderBottomBar.kt:122` | 10 / 10 | 侧栏/底栏的功能按钮（传 null 置灰） |
| `jmClickable` | `Interactions.kt:40` | 46 | 悬停/按下/焦点反馈（只有 `ComicCover` 用） |
| `jmSharedElement` / `SharedPageHost` / `jmCoverKey` | `SharedElement.kt:35` / `:69` / `:28` | 12 / 11 / 1 | 共享元素接入件 |
| `BlockedByTagBanner` / `rememberHiddenTagIds` / `splitBlockedByTag` | `TagBlocking.kt:40` / `:28` / `:35` | 15 / 6 / 2 | 标签级屏蔽三件套 |
| `PageItem` | `ReaderScreen.kt:495` | 53 | 阅读页单页（加载中/失败可点重试/成功） |

---

## 5. 四个核心阅读路径屏幕的实现细节

### 5.1 首页 `HomeScreen`（`Main.kt:131`，118 行）

**区域划分（源码实际嵌套顺序）**

| 区域 | 位置 | 尺寸 / 比例 | 组件 |
|---|---|---|---|
| 外层定位盒 | `Main.kt:195` | `Box(Modifier.fillMaxSize())` | 只用于把浮钮对齐到右下角（注释：`Column` 里没法 `align` 到角落） |
| A. 状态行 | `Main.kt:197-201` | `padding(horizontal = 16.dp, vertical = 12.dp)` | `Text(status, titleMedium)` |
| B. 操作条 | `Main.kt:202-214` | `fillMaxWidth().padding(horizontal = 16.dp, vertical = 4.dp)`，`spacedBy(10.dp)` | `Button` 刷新（`enabled = !busy`）、`Button` 加载更多（`enabled = !busy && items.size < total`）、`Text("已加载 N / total", labelSmall)` |
| C. 主列表 | `Main.kt:216-229` | `LazyVerticalGrid(columns = GridCells.Adaptive(168.dp))`，`contentPadding = PaddingValues(horizontal = 16.dp, vertical = 4.dp)`，`spacedBy(14.dp)` / `spacedBy(16.dp)`，`Modifier.fillMaxSize()` | 首项 `item(span = { GridItemSpan(maxLineSpan) })` 放 `PromoteHeader`；其余 `items(items, key = { it.id })` 放 `ComicCard` |
| D. 浮钮层 | `Main.kt:235-246` | 两个 `Box(Modifier.align(Alignment.BottomEnd))`：签到 `padding(end = 20.dp, bottom = 20.dp + 52.dp + 12.dp)`、随机 `padding(20.dp)`；两个都是 `size(52.dp)` 圆 | `DailyQuickFab`、`RandomFab` |

**推荐分区 `PromoteHeader`（`HomeSections.kt:39-96`）**

| 项 | 数值 / 行为 | 行号 |
|---|---|---|
| 数据 | `repository.promote()`，失败时**有限次重试**：最多 15 次、每次间隔 400ms（理由：主机发现可能未完成） | `HomeSections.kt:47-61` |
| 分区标题行 | `Row(padding(start = 4.dp, top = 12.dp, bottom = 8.dp))`：`Text(section.title, titleMedium, primary)` + `TextButton("更多")` | `HomeSections.kt:70-82` |
| 分区内容 | `Row(fillMaxWidth().horizontalScroll(rememberScrollState()))`，`spacedBy(12.dp)`，每项 `Box(Modifier.width(140.dp))` 包一个 `ComicCover` | `HomeSections.kt:83-93` |
| "更多"去向 | `onOpenSection(section.id, section.title)` → `Main.kt:432-435` 拼 `"more/<id>?title=" + URLEncoder.encode(title, "UTF-8")` | `Main.kt:434` |

**首页专用卡片 `ComicCard`（`Main.kt:251-297`）**

| 元素 | 数值 |
|---|---|
| 封面盒 | `fillMaxWidth().aspectRatio(3f / 4f).clip(RoundedCornerShape(10.dp)).background(surfaceVariant)` |
| 封面图 | `Image(bitmap, contentScale = ContentScale.Crop, fillMaxSize())`，**无淡入动画**（对比 `ComicCover.kt:41-45`） |
| 更新角标 | `Text("更新", labelSmall, primary)`，`padding(top = 4.dp)` |
| 标题 | `bodyMedium`，`maxLines = 2`，`padding(top = 8.dp)` |
| 作者 | `labelSmall`，`onSurfaceVariant`，`maxLines = 1`，`padding(top = 2.dp)` |
| 点击 | 裸 `Modifier.clickable { onOpen() }`（**没有** `jmClickable` 的悬停放大/焦点描边，也**没有** `jmSharedElement`） |

**交互清单（首页）**

| 交互 | 位置 | 行为 |
|---|---|---|
| 点卡片 | `Main.kt:256` | 进详情：`Main.kt:320-323` 设 `Screen.Detail(item.id, item.name)` |
| 点"刷新" | `Main.kt:207` | `reload(1)`：先 `bootstrap()` 再 `latest(1)` |
| 点"加载更多" | `Main.kt:210` | `reload(page + 1)` → `latest(next)` |
| 点"更多" | `HomeSections.kt:79` | 进分区更多页 |
| 点随机浮钮 | `Main.kt:554-570` | `bootstrap()` + `randomRecommend()` 后 `randomOrNull()`，抽到就进详情；列表空只记日志不跳转 |
| 长按随机浮钮 | `Main.kt:571` | `onOpenRandomList()` → `Screen.Page("random")` |
| 点签到浮钮 | `Main.kt:636-666` | 先 `daily(uid)` 判当天是否已签，再 `dailyCheck(uid, dailyId)`；未登录时整个浮钮不渲染（`Main.kt:621`） |
| 追更更新角标 | `Main.kt:174-189` | 登录后取 `notifications(TYPE_COMIC_FOLLOW)` 的未读项 → `followedUpdates()` → `comicIdText` 集合 |
| 滚轮 | 无专门代码 | 主列表纵向滚动由 `LazyVerticalGrid` 内建；推荐分区横向 `Row(horizontalScroll)` 未接滚轮事件（未找到） |
| 键盘 | 未找到 | - |
| 右键菜单 | 未找到 | - |
| 拖拽 | 未找到 | - |

### 5.2 作品详情 `DetailScreen`（`DetailScreen.kt:65`，540 行）

**区域划分**

| 区域 | 位置 | 尺寸 / 比例 | 组件 |
|---|---|---|---|
| 顶部条 | `DetailScreen.kt:275-288` | `fillMaxWidth().padding(horizontal = 12.dp, vertical = 8.dp)`，`spacedBy(8.dp)` | `TextButton("返回")`；`loading` 时 `LoadingHint("正在加载作品…")`，否则 `SelectionContainer { Text(status, titleMedium) }` |
| 主体分栏 | `DetailScreen.kt:291` | `Row(Modifier.fillMaxSize().padding(horizontal = 16.dp))` | 左栏 280dp + 右栏占满 |
| 左栏 | `DetailScreen.kt:293-546` | `Column(Modifier.width(280.dp).verticalScroll(rememberScrollState()))`，`spacedBy(10.dp)` | 见下 |
| 左栏-封面 | `DetailScreen.kt:308-319` | `fillMaxWidth().aspectRatio(3f / 4f).clip(RoundedCornerShape(10.dp))` | `.jmSharedElement(jmCoverKey(d.id))`；入场 `0.94f → 1f` + alpha |
| 左栏-按钮行 | `DetailScreen.kt:321-410` | `Row(spacedBy(8.dp))` | `收藏`/`已收藏`、`点赞`/`已赞 N`、`追更`/`已追更`（仅登录）、`评论`（**实际也只在登录时渲染**，见 §7） |
| 左栏-信息 | `DetailScreen.kt:419-499` | `InfoLine` ×N（标题 / 作者 / 页数 / 简介），值用 `SelectionContainer` | 标签区 `DetailScreen.kt:424-493` |
| 左栏-标签区 | `DetailScreen.kt:424-493` | 每个标签一行：`#tag`（`weight(1f)`，可点 → `openTag`）+ `屏蔽`/`已屏蔽` + `标星`/`取消标星` | 就地搜索结果 `DetailScreen.kt:471-491` |
| 左栏-下载条 | `DetailScreen.kt:507-527` | `fillMaxWidth().clip(RoundedCornerShape(8.dp)).background(surfaceVariant).padding(horizontal = 12.dp, vertical = 10.dp)` | `Text("下载整部作品")` + `Text("交给系统浏览器下载", labelSmall)` |
| 左栏-相关作品 | `DetailScreen.kt:533-545` | `Row(fillMaxWidth().horizontalScroll())`，`spacedBy(12.dp)`，每项 `Box(Modifier.width(140.dp))` | `ComicCover` |
| 右栏-无章节 | `DetailScreen.kt:552-565` | `LazyColumn(fillMaxSize().padding(start = 16.dp))` | 单项 `Text("开始阅读")` 可点 → `onOpenChapter(SeriesItem(id = d.id, sort = null, name = null), listOf(d.id))` |
| 右栏-有章节 | `DetailScreen.kt:567-600` | `LazyColumn(fillMaxSize().padding(start = 16.dp))`，`spacedBy(2.dp)` | 首项 `Row`：`从头开始`（`d.series.first()`）+ `继续观看`（`enabled = idx >= 0`，`idx` 由 `progress.lastChapterId(comicId)` 在 `ids` 里定位）；随后 `items(d.series, key = { it.id })`，每项 `Text("第 N 话 · name")`，`padding(vertical = 10.dp, horizontal = 4.dp)` |

**交互清单（详情）**

| 交互 | 位置 | 行为 |
|---|---|---|
| 点标签 | `DetailScreen.kt:445` → `openTag`（`150-176`） | `onOpenTag` 为 null（`Main.kt:440-452` 未传）时就地 `repository.search(query = tag)`，结果渲染在左栏标签区下方；点结果进该作品详情（`DetailScreen.kt:484`） |
| 屏蔽标签 | `DetailScreen.kt:455-457` → `blockTag`（`179-193`） | 写 `repository.blockStore.addTag(tag)`，已存在时提示"已经在屏蔽名单里" |
| 标星/取消标星 | `DetailScreen.kt:459-464` → `starTag`（`196-220`） | 未登录直接提示"请先登录再标星标签"；登录时 `updateFavoriteTags("add"/"remove", listOf(tag))` 后重读 `favoriteTags()` |
| 收藏/取消收藏 | `DetailScreen.kt:322-342` | 直接 `toggleFavorite(d.id)`，本地翻转 `favorite`；**无登录判断** |
| 点赞 | `DetailScreen.kt:345-377` | 已赞先提示"已经点过赞了"；未登录提示"请先登录再点赞"；成功 `d.copy(liked = true, likes = d.likes + 1)` |
| 追更 | `DetailScreen.kt:381-398` | 仅登录时渲染；`toggleTracking(d.id)` |
| 浏览评论 | `DetailScreen.kt:399` | `onOpenComments(d.id)` → `Screen.Page("comments:<aid>")`（**受登录条件限制**，见 §7） |
| 下载整部作品 | `DetailScreen.kt:507-527` → `downloadAlbum`（`233-272`） | 未登录提示"下载需要登录"；`payload.isOk` 为假时显示服务端 `msg`；成功时 `java.awt.Desktop.getDesktop().browse(URI(url))` |
| 从头开始 / 继续观看 | `DetailScreen.kt:581-586` | `onOpenChapter(chapter, d.series.map { it.id })`，把章节顺序（旧→新）传给阅读页 |
| 点章节 | `DetailScreen.kt:590-599` | 同上 |
| 滚轮 | 无专门代码 | 左栏 `verticalScroll` 与右栏 `LazyColumn` **各自独立滚动**（两栏都能滚，鼠标所在栏生效） |
| 键盘 | 未找到 | - |
| 右键菜单 / 拖拽 | 未找到 | - |

### 5.3 阅读器 `ReaderScreen`（`ReaderScreen.kt:69`，424 行）

**区域划分**

| 区域 | 位置 | 尺寸 / 比例 | 组件 |
|---|---|---|---|
| 根容器 | `ReaderScreen.kt:152-154` | `Box(Modifier.fillMaxSize())` | `PageRail` 与 `ReaderBottomBar` 都用 `align` 贴在这个 Box 上 |
| 顶部条 | `ReaderScreen.kt:155-179` | `fillMaxWidth().padding(horizontal = 12.dp, vertical = 8.dp)`，`spacedBy(8.dp)` | `TextButton("返回")`；`loading` → `LoadingHint("正在加载章节…")`，否则 `Text(status, titleMedium)`；`Text("第 X / N 话", labelSmall)`；`payload == null` 时 `TextButton("重试")`（`retryToken += 1`）；末尾一个**空的** `Row(Modifier.weight(1f), Arrangement.End) {}` |
| 内容区（横向模式） | `ReaderScreen.kt:328-335` | `PagedReader(modifier = Modifier.fillMaxWidth().weight(1f))` | `HorizontalPager`，每页一个 `PageItem`（`PagedReader.kt:42-45`） |
| 内容区（纵向模式） | `ReaderScreen.kt:337-362` | `LazyColumn(state = listState, modifier = Modifier.weight(1f).padding(horizontal = 8.dp), verticalArrangement = Arrangement.spacedBy(0.dp))` | `itemsIndexed(p.images, key = { _, img -> img.image })` → `PageItem`；末尾 `item`：`Button("下一话")`（`enabled = nextId != null`，否则文案"已经是最后一话"）+ `TextButton("回详情页")` |
| 内容区外层（键盘） | `ReaderScreen.kt:298-326` | `Row(Modifier.fillMaxSize().focusRequester(focusRequester).focusable().onPreviewKeyEvent { ... })` | 键盘翻页挂在这一行上 |
| 右侧栏 | `ReaderScreen.kt:374-439` | `Modifier.align(Alignment.CenterEnd).width(40.dp)` | `PageRail`（宽 40dp，滑轨占高 70%） |
| 底部栏 | `ReaderScreen.kt:440-490` | `Modifier.align(Alignment.BottomCenter)`，`visible = true` | `ReaderBottomBar` |
| 章节对话框 | `ReaderScreen.kt:139-150` | M3 `AlertDialog` | `pickerOpen && series.isNotEmpty()` 时才组合；`chapterSize = 50`，`heightIn(max = 380.dp)` |

**阅读模式（两种）**

| 模式 | 载体 | 状态载体 | 页索引来源 |
|---|---|---|---|
| `ReaderMode.Scroll`（纵向连续滚动） | `LazyColumn` | `rememberLazyListState()`（`ReaderScreen.kt:83`） | `derivedStateOf { listState.firstVisibleItemIndex }`（`ReaderScreen.kt:85`） |
| `ReaderMode.Page`（横向逐页） | `HorizontalPager`（`PagedReader`） | `rememberPagerState(initialPage = currentPage...)`（`ReaderScreen.kt:187-189`） | `pagerState.currentPage` |
| 默认取值 | `ReaderModePref.mode`（`ReaderScreen.kt:92`，定义在 `ReaderScreen.kt:562-570`，存储节点 `jm_reader_mode`、键 `mode`，值 `"page"`/`"scroll"`） | - | - |
| 切换时同步下标 | `onToggleMode` 两处（`ReaderScreen.kt:389-403` 侧栏、`469-476` 底栏）：先 `scrollToItem(pagerState.currentPage)` 或 `scrollToPage(currentPage)`，再翻转模式并写回 `ReaderModePref.mode` | - | - |

**交互清单（阅读器）**

| 交互 | 位置 | 行为 |
|---|---|---|
| 键盘翻页 | `ReaderScreen.kt:302-325` | 只认 `KeyEventType.KeyDown`；左方向/PageUp → 上一页，右方向/PageDown → 下一页；横向走 `pagerState.scrollToPage`，纵向走 `listState.scrollToItem`；越界用 `coerceIn(0, total - 1)`；命中返回 `true` |
| 焦点申请 | `ReaderScreen.kt:196` | `LaunchedEffect(mode, chapterId) { focusRequester.requestFocus() }`（注释说明：只写 `LaunchedEffect(Unit)` 会在切换模式后失效） |
| 侧栏点击跳页 | `PageRail.kt:126-130` | `onSeek(((o.y / trackH) * max).roundToInt().coerceIn(0, max))` |
| 侧栏竖向拖动跳页 | `PageRail.kt:132-138` | `detectVerticalDragGestures`，换算同上 |
| 侧栏五个功能 | `PageRail.kt:150-154` + `ReaderScreen.kt:388-438` | 模式（纵向/横向）、章节、评论、收藏、点赞；收藏/点赞直接调 `repository.toggleFavorite` / `repository.like`，**无登录判断** |
| 上一话 / 下一话（侧栏） | `PageRail.kt:157-162`，`ReaderScreen.kt:386-387` | `hasPrev = prevId != null`、`hasNext = nextId != null`；`prevId`/`nextId` 由 `chapterIds` 与当前 `chapterId` 的 `indexOf` 得到（`ReaderScreen.kt:98-100`） |
| 底栏进度条 | `ReaderBottomBar.kt:83-88` | `LinearProgressIndicator(progress = { frac }, height(3.dp))`，`frac = current / (total - 1)` |
| 底栏按钮 | `ReaderBottomBar.kt:93-103` | `上一话` / `<` / `页码` / `>` / `下一话` + 右侧 5 个功能（`ReaderScreen.kt:469-489` 传回调） |
| 章节选择 | `ReaderScreen.kt:404-418`（侧栏）、`477-486`（底栏） | 打开时才请求 `repository.album(comicId)` 取 `series`，再弹 `ChapterPickerDialog`；对话框内每页 50 话、有"上一页/下一页"；滚动状态按页取 `ScrollState(0)` |
| 单页失败重试 | `ReaderScreen.kt:536-542` | 失败文字"点这里重试"可点 → `attempt += 1` 重新 `LaunchedEffect(url, attempt)` 加载 |
| 下一页预取 | `ReaderScreen.kt:240-263` | **串行**预取，深度取 `SelfTuner.prefetchDepth.coerceAtLeast(1)`；`prefetchInFlight`（同步集合）做在途去重；需反切片的图走 `RemoteImage.loadScrambled` |
| 下一话预加载 | `ReaderScreen.kt:278-297` | 读到本话 `(images.size * 6) / 10`（60%）时预取 `repository.read(nextId)`，只取章节信息、不下载图片；本话只跑一次 |
| 页级进度恢复 | `ReaderScreen.kt:121-129` | `pageProgress.lastPage(comicId, chapterId)` → `listState.scrollToItem(saved)`（**只有纵向模式有恢复路径**） |
| 页级进度记录 | `ReaderScreen.kt:132-136`（纵向）、`267-271`（横向，仅 `mode == Page` 时写） | 存储 `PageProgress(PreferencesKeyValueStore("jm_read_page"))`，键 `"comicId|chapterId"`，值 0 基页码（`PageProgress.kt:29`） |
| 自学习采样 | `ReaderScreen.kt:205-238` | 进页记时、等本页图可用（最多 15 秒）、离页时把 `PageSample(latencyMs, bytes, hitCache, failed, dwellMs)` 喂 `SelfTuner.onPage`；未就绪时另记 `SelfTuner.onCancellation()` |
| 滚轮 | 无专门代码 | 纵向模式由 `LazyColumn` 内建；横向模式（`HorizontalPager`）**源码没有接滚轮事件**（未找到），能否滚轮翻页取决于 Compose 内建行为，源码未处理 |
| 缩放（双指/滚轮+Ctrl） | 未找到 | - |
| 右键菜单 / 拖拽 | 未找到 | - |

### 5.4 搜索 `SearchScreen`（`SearchScreen.kt:94`，257 行）

**区域划分（自上而下，全部在一个 `Column(fillMaxSize)` 里）**

| 区域 | 位置 | 尺寸 | 组件 |
|---|---|---|---|
| 1. 搜索条 | `SearchScreen.kt:189-217` | `fillMaxWidth().padding(horizontal = 16.dp, vertical = 10.dp)`，`spacedBy(10.dp)` | `OutlinedTextField(value = query, singleLine = true, modifier = Modifier.width(360.dp).onKeyEvent { ... })`、`Button`（`enabled = !busy && query.isNotBlank()`，文字"搜索"/"搜索中…"）、`StatusLine(status, busy, labelSmall)` |
| 2. 排序档 | `SearchScreen.kt:219-230` | `fillMaxWidth().padding(horizontal = 16.dp)`，`spacedBy(4.dp)` | `Text("排序：", labelMedium)` + 5 个 `TextButton`（`"" → 最新`、`mv → 最多点阅`、`mp → 最多图片`、`tf → 最多爱心`、`old → 最旧`；选中项文字前缀 `"· "`） |
| 3. 字段档 | `SearchScreen.kt:232-243` | 同上 | 5 个 `TextButton`（`site → 站内搜索`、`work → 作品`、`author → 作者`、`tag → 标签`、`character → 登场人物`） |
| 4. 年月 | `SearchScreen.kt:245-266` | `padding(horizontal = 16.dp, vertical = 4.dp)`，`spacedBy(8.dp)` | 年 `OutlinedTextField(width = 150.dp)`（只留数字、最多 4 位）、月 `OutlinedTextField(width = 130.dp)`（只留数字、最多 2 位）、`TextButton("不限")` |
| 5. 搜索历史 | `SearchScreen.kt:268-280` | `padding(horizontal = 16.dp)` | `Text("搜索历史：", labelMedium)` + 每条一个 `TextButton`（点了直接 `query = h; runSearch(1)`）+ `TextButton("清空")`。持久化在 `PreferencesKeyValueStore("jm_search_history")`，键 `history`，最多 20 条、最近的在前、大小写不敏感去重（`SearchScreen.kt:70-91`） |
| 6. 未搜索时的建议 | `SearchScreen.kt:282-310` | `items.isEmpty()` 时才显示两行 | 热门标签：`hotTags.take(12)`，点了 `query = tag; type = "tag"; runSearch(1)`；随机推荐：`recommend.take(6)`，用 `TextButton` 显示标题前 12 字（注释：`ComicCover` 是给网格用的，塞进行内会变形）；`TextButton("换一批")` 重新 `reloadSuggest()` |
| 7. 数据层屏蔽条 | `SearchScreen.kt:312` | - | `BlockedNotice(hidden)` |
| 8. 标签级屏蔽条 | `SearchScreen.kt:314-331` | `padding(horizontal = 16.dp, vertical = 4.dp)`，`spacedBy(10.dp)` | 文案"已按标签屏蔽 N 条（命中：tag1、tag2…）"，命中标签 `distinct().take(4)`；`TextButton("允许一次")` → `TagBlocker.allowOnce(ids)` |
| 9. 结果网格 | `SearchScreen.kt:333-348` | `LazyVerticalGrid(GridCells.Adaptive(168.dp), contentPadding = PaddingValues(horizontal = 16.dp, vertical = 4.dp), spacedBy(14.dp)/spacedBy(16.dp), Modifier.fillMaxSize())` | `items(visibleItems, key = { it.id })` → `ComicCover(modifier = Modifier.animateItem())`；末尾条件项 `Button("加载更多")`（`items.size < total` 时） |

**搜索语义（源码照抄 Android 的约定，写在本文件注释 `SearchScreen.kt:39-52`）**

| 约定 | 实现位置 | 行为 |
|---|---|---|
| 精确命中作品编号 | `SearchScreen.kt:151-158` | `result.redirectAid` 非空且 `nextPage == 1` 时直接 `onOpenComic(ListItem(id = redirect, name = query))`，不列结果 |
| 被屏蔽条数必须明示 | `SearchScreen.kt:312` | `BlockedNotice(hidden)`，文案含"可在「屏蔽设置」里调整"（`ComicCover.kt:91`） |
| "最旧"本地二次排序 | `SearchScreen.kt:160-163` | `if (order == "old") list = list.sortedBy { it.addDate.orEmpty() }` |
| 标签级屏蔽过滤 | `SearchScreen.kt:315-316` | `blockedItems = items.filter { it.id in hiddenIds }`；`visibleItems` 取反 |
| 标签补标签请求 | `SearchScreen.kt:166` | `list.forEach { TagBlocker.request(it.id) }` |
| 历史只在第 1 页记 | `SearchScreen.kt:136-137` | `if (nextPage == 1) history = historyStore.add(query)` |

**交互清单（搜索）**

| 交互 | 位置 | 行为 |
|---|---|---|
| 回车 | `SearchScreen.kt:201` | `runSearch(1)` 并消费事件 |
| Esc | `SearchScreen.kt:203-208` | 清空 `query`、清空结果、状态回到"输入关键词后回车搜索" |
| 点搜索按钮 | `SearchScreen.kt:213` | `runSearch(1)` |
| 切换排序/字段 | `SearchScreen.kt:226` / `239` | 只改状态；**不自动重搜**（需再按回车或点"搜索"），只有分类页的排序是立即重载（`CategoryScreen.kt:265`） |
| 点历史词 | `SearchScreen.kt:276` | 立即 `runSearch(1)` |
| 点热门标签 | `SearchScreen.kt:292` | `type = "tag"` + 立即搜索 |
| 点结果卡片 | `SearchScreen.kt:341` | `onOpenComic(item)` |
| 滚轮 | 无专门代码 | `LazyVerticalGrid` 内建纵向滚动 |
| 键盘（Enter/Esc 之外） | 未找到 | - |
| 右键菜单 / 拖拽 | 未找到 | - |

---

## 6. 哪些屏幕需要登录

判定依据：源码里实际出现的 `repository.auth.isLoggedIn` / `authState.loggedIn` / `authState.member` 判断（全目录 grep 命中的文件只有 `Main.kt`、`DetailScreen.kt`、`ProfileScreen.kt`、`FavoritesScreen.kt`、`TrackingScreen.kt`、`HistoryScreen.kt`、`TagsScreen.kt`、`CommentsScreen.kt`、`LoginScreen.kt`、`SerialReminder.kt`、`Smoke.kt`）。

| 屏幕 | 需要登录 | 证据（行号） | 未登录时的具体表现 |
|---|---|---|---|
| `HomeScreen` | 部分 | `Main.kt:177`（追更更新集合）、`Main.kt:601/621`（签到浮钮） | 首页本身可看；签到浮钮整块不渲染；"更新"角标不出现 |
| `DetailScreen` | 部分 | `DetailScreen.kt:123`（追更初始态）、`:197`（标星）、`:234`（下载）、`:350`（点赞）、`:380`（追更按钮） | 追更按钮不渲染；点赞提示"请先登录再点赞"；标星提示"请先登录再标星标签"；下载提示"下载需要登录"；**收藏按钮与评论入口的实际情况见 §7 第 2 条** |
| `ReaderScreen` | 否 | 源码内**没有** `isLoggedIn` 判断 | 读章节不需要登录；侧栏/底栏的收藏与点赞点了直接打接口（失败只记日志，`ReaderScreen.kt:422-426`/`432-436`） |
| `SearchScreen` | 否 | 无 | 全部功能可用 |
| `FavoriteScreen` | 需要 | `FavoritesScreen.kt:57`（`loggedIn`）、`:67`（初始状态文案）、`:94`（`if (loggedIn) load(1)`）、`:157`（收藏夹管理整块） | 状态显示"需要登录后才能查看收藏"；不发请求；收藏夹新建/改名/删除那一行不渲染；网格为空 |
| `TrackingList`（收藏页内嵌与追更页共用） | 需要 | `TrackingScreen.kt:86`（初始状态文案）、`:115`（`if (loggedIn) load(1)`） | 状态显示"需要登录后才能查看追更"；不发请求 |
| `TrackingScreen`（不可达） | 需要 | 同上 | - |
| `HistoryScreen` | 需要 | `HistoryScreen.kt:59`、`:82` | 状态显示"需要登录后才能查看历史"；不发请求 |
| `TagsScreen` | 部分 | `TagsScreen.kt:105`（扫描按钮 `enabled` 含 `isLoggedIn`）、`:127-134`（提示文案） | 扫描按钮置灰并显示"需要登录（收藏列表属于账号数据）"；`favoriteTags()` 仍在 `LaunchedEffect(Unit)`（`:95`）无条件调用，靠接口失败降级 |
| `ProfileScreen` | 部分 | `ProfileScreen.kt:57-59`（未登录直接返回并设文案）、`:103`（签到按钮 `enabled`） | 页面可进；账号卡显示"未登录"+"去登录"；签到按钮置灰；状态显示"登录后可查看账号信息与签到" |
| `CommentsScreen` | 读否、写是 | `CommentsScreen.kt:88`（读评论无条件）、`:125`（发表按钮 `enabled` 含 `authState.loggedIn`）、`:149-151`（提示）、`:247`（删除按钮需 `myUid` 比对） | 可浏览评论；发表/回复置灰并显示"需要登录"；删除按钮不显示 |
| `NotificationScreen` | 否（源码未做判断） | 该文件内零 `isLoggedIn` | 无论登录与否都调 `notifications(type, page)` 与 `notificationsUnread()`；侧栏未读角标则由 `Main.kt:330-331` 在未登录时强制为 0 |
| `WeeklyUpdateScreen` / `MoreListScreen` / `WeekScreen` / `CategoryScreen` / `RandomScreen` / `CreatorScreen` | 否 | 各自文件内零 `isLoggedIn` | 全部可用 |
| `BlockScreen` / `AboutScreen` / `AppearanceScreen` / `LoginScreen` | 否 | 无门槛 | 可用（`LoginScreen` 本身就是登录入口） |

---

## 7. 死代码、过时注释与不一致

### 7.1 死代码（有实现、无调用方，或不可达）

| 序号 | 项 | 位置 | 判定依据 |
|---|---|---|---|
| 1 | `TrackingScreen`（20 行） | `TrackingScreen.kt:43` | 全目录唯一引用是它自己的定义；`grep "TrackingScreen" *.kt` 只命中 1 行。`tracking` 路由实际走 `FavoriteScreen(initialTab = "tracking")`（`Main.kt:495`） |
| 2 | `PageShell` + `PAGE_PLANS`（24 行 + 13 条文案） | `PageShell.kt:25`、`PageShell.kt:51` | 兜底分支 `Main.kt:514-517` 不可达：`Screen.Page` 的全部构造点是 `Main.kt:347`（route 来自 `NAV_ITEMS` 的 15 项）、`:434`（`more/<id>?title=`）、`:437`（`random`）、`:444`（`comments:<aid>`）、`:460`（同）、`:488`（`search`）；这些 route 全部命中 `Main.kt:470-512` 的具体分支（含 `more/` 与 `comments:` 两条前缀分支），兜底走不到。另外 `PAGE_PLANS` 的 13 个键与 `NAV_ITEMS` 的 15 项相比缺 `appearance`（若兜底可达会显示"（待补）"），也不含 `home` |
| 3 | `alreadyBlocked` 扩展函数 | `BlockScreen.kt:158-160` | 带 `@Suppress("unused")`，零引用 |
| 4 | `Motion.gentleSpring()` / `Motion.quick()` / `Motion.normal()` | `Motion.kt:37-45` | 零引用；实际只用了 `QUICK_MS` / `NORMAL_MS` / `PAGE_MS` / `IMAGE_MS` 与 `Standard` / `Enter` / `Exit` |
| 5 | `RemoteImage.loadSized` 的外部用途 | `RemoteImage.kt:94` | 只被同文件 `load`（`RemoteImage.kt:110`）调用；文件注释（`:88-93`）称"阅读页用它喂自学习的样本"，但阅读页用的是 `RemoteImage.load` + `RemoteImage.downloadedSize`（`ReaderScreen.kt:254`、`:231`） |
| 6 | `PageProgress.clear` | `PageProgress.kt:27` | 零引用 |
| 7 | `SelfTuner.enabled` 的写入端 | `SelfTuner.kt:36-38` | 有 getter/setter，但全目录没有写入点（外观页只做 `SerialReminder.enabled`，`AppearanceScreen.kt:208`），实际恒为默认 `true` |
| 8 | 6 个 `Tokens` 字段 | `BlogTheme.kt:34,38,41,46,50,53` | 见 §3.3 |
| 9 | `ReaderScreen.kt:177-178` 的空 `Row` | `ReaderScreen.kt:177-178` | `Row(Modifier.weight(1f), horizontalArrangement = Arrangement.End) { }`，无子项 |

### 7.2 过时注释（注释说的与实际代码不符）

| 序号 | 位置 | 注释原文（要点） | 实际情况 |
|---|---|---|---|
| 1 | `Main.kt:299` | "桌面端的页面栈（2.0.0 第一版：**就两个页面**，用手写状态机而不是导航库）" | `Screen` 有 5 个成员（`Main.kt:300-306`），可路由屏幕 20 个 |
| 2 | `ReaderBottomBar.kt:41` | "本组件只负责外观与动画。**尚未接线**：这一步只保证它能编译。" | 已接线：`ReaderScreen.kt:440-490` 传入全部回调，且 `visible = true` 常显 |
| 3 | `SharedElement.kt:15-17` | "**当前状态：已就位但尚未接线。**…目前没有地方提供作用域" | 已接线：`Main.kt:428` 用 `SharedPageHost(this@AnimatedContent)` 提供两个作用域 |
| 4 | `SharedElement.kt:48-58` | "接线清单（**尚未执行**）1) … 2) …" | 清单两步都已执行：宿主在 `Main.kt:428`，两个 `.jmSharedElement(...)` 在 `ComicCover.kt:53` 与 `DetailScreen.kt:313` |
| 5 | `ReaderScreen.kt:186` | "用全限定名调用，避免再动 import。（**第 3 步接线时**由 PagedReader 使用。）" | `PagedReader` 已在 `ReaderScreen.kt:329` 使用 |
| 6 | `ReaderScreen.kt:192` | "第一步只把焦点基础设施就位；按键处理**在下一步**加到内容区那一行上。" | 按键处理已在 `ReaderScreen.kt:302-325` 实现 |
| 7 | `ReaderScreen.kt:369-372` | "暂以 `visible = true` 常显，下一轮再加…"；"改成浮层需要把**根布局 Column 换成 Box**，会连带改动 `weight` 的作用域，风险大，故先不做" | `visible = true` 仍是事实；但根布局**已经是 `Box`**（`ReaderScreen.kt:152`），`PageRail` 与 `ReaderBottomBar` 都靠 `Modifier.align(...)` 贴边，已不是"Column 的最后一个子项" |
| 8 | `ReaderScreen.kt:365-373` | 页码栏与底栏的两段说明被插在内容 `Row` 的闭合 `}` 之前（`:368-372` 在 `:373` 的 `}` 内），位置错乱 | 描述的对象其实是紧随其后的 `PageRail`（`:374`）与 `ReaderBottomBar`（`:440`） |
| 9 | `DetailScreen.kt:54`、`DetailScreen.kt:548` | "章节列表（**倒序**）"、"右栏：章节列表（**倒序**，最新的在最上面）" | 实际正序：`items(d.series, ...)`（`DetailScreen.kt:590`）按接口给的顺序渲染，且 `DetailScreen.kt:589` 自己的注释写"章节列表**正序**（用户要求：从上到下 1、2、3…，不要倒序）" |
| 10 | `AppearanceScreen.kt:170-175` | "本地图片壁纸这条路径**尚未验证**（当前实现借用了网络图片加载器读 `file://`，很可能不通）…我会改成用本地解码" | 已改成本地解码：`Wallpaper.kt:206-215` 用 `File(path).readBytes()` + `org.jetbrains.skia.Image.makeFromEncoded(bytes).toComposeImageBitmap()`；注释里承诺的改动已完成，但这段提示没更新 |
| 11 | `ComicCover.kt:31` | "抽成公共组件是因为**首页**、搜索、分类、周刊、随机、收藏、历史、追更都要用它" | 首页用的是 `ComicCard`（`Main.kt:228`、`Main.kt:251`），不用 `ComicCover`；`ComicCover` 实际有 12 处调用，首页不在其中 |
| 12 | `TrackingScreen.kt:32-36` | "…收藏页的「追更」标签可以直接内嵌它，**侧栏的独立入口也仍然可用**" | 侧栏独立入口已改道 `FavoriteScreen(initialTab = "tracking")`（`Main.kt:495`），`TrackingScreen` 整个函数无调用方 |
| 13 | `desktop/PARITY.md:1053` | "**未做**：…键盘翻页…" | 键盘翻页已实现（`ReaderScreen.kt:302-325`） |
| 14 | `desktop/PARITY.md:1047-1049` | "**调试插桩（发布前要清理）**：每 20 秒导出画面到 `~/jmnext-screen.png`…" | 源码里已无这些插桩（全目录 `grep "Robot\|jmnext-screen\|渲染内容\|内容区尺寸"` 零命中） |

### 7.3 不一致与可疑实现

| 序号 | 位置 | 现象 |
|---|---|---|
| 1 | `Main.kt:251`（`ComicCard`）与 `ComicCover.kt:37`（`ComicCover`） | 两套作品卡片：`ComicCover` 带 `jmClickable`（悬停放大 + 焦点描边）、`jmSharedElement`、图片淡入；`ComicCard` 三样都没有。结果是首页封面没有悬停/焦点反馈，也不参与共享元素过渡 |
| 2 | `DetailScreen.kt:380-400` | `TextButton("评论")`（`:399`）被写在 `if (repository.auth.isLoggedIn) { ... }`（`:380`）的块内、由 `:400` 的 `}` 关闭。因此**未登录用户在详情页看不到评论入口**，而上方的注释（`:379`）只说"未登录不显示"是为了追更按钮。评论页本身读评论不需要登录（`CommentsScreen.kt:88`） |
| 3 | `DetailScreen.kt:317` | 同一个 `graphicsLayer { scaleX = 0.94f + 0.06f * coverIn; scaleY = ...; alpha = coverIn }` 在**同一行内连续写了两遍**，属复制粘贴残留 |
| 4 | `SearchScreen.kt:167`、`DiscoverPages.kt:58`、`CategoryScreen.kt:91` | `hidden += paged.hidden` 且第一页不重置：重复搜索 / 换刊期 / 换分类会把"被屏蔽条数"累加。对比 `MoreListScreen.kt:75`（`next == 0` 时重置）与 `CategoryScreen.kt:169`（标签搜索 `next == 1` 时重置） |
| 5 | `SearchScreen.kt:113-114` + `:317-331` | 自己订阅 `TagBlocker.hidden` 并手写"已按标签屏蔽"提示条，没有用 `TagBlocking.kt` 的三件套（`rememberHiddenTagIds` / `splitBlockedByTag` / `BlockedByTagBanner`）。后者注释（`TagBlocking.kt:22`）举的例子是"分类、随机、作者、更多这四个列表页" |
| 6 | 标签搜索三条出口三种行为 | `TagsScreen` 的 `onSearch` 已接（`Main.kt:486-489`）但**不把标签名传过去**（只切到 `Page("search")`，搜索框仍是空的）；`DetailScreen.onOpenTag` 从未传（`Main.kt:440-452`）、`CategoryScreen.onSearch` 从未传（`Main.kt:474`），两处都退化为"就地搜索"（`DetailScreen.kt:150-176`、`CategoryScreen.kt:193-203`） |
| 7 | `ReaderScreen.kt:121-136` 与 `:267-271` | 两种阅读模式的进度路径不对称：纵向有"恢复 + 记录"，横向只有"记录"；`LaunchedEffect(payload)` 的恢复用 `listState.scrollToItem`，横向模式没有对应恢复 |
| 8 | `AppearanceScreen.kt:88` / `:96` / `:104` | 三条 `Slider` 中，"壁纸模糊"（`:88`）与"面板浓度"（`:96`）调了 `refresh++`，"壁纸压暗"（`:104`）没调（靠 `Slider` 自身重组刷新文案） |
| 9 | `SideNav.kt:53-58` 与 `Main.kt:371-375` | 品牌字"JMNeXt"显示两次：侧栏那处**不可点**，顶栏那处**可点回首页** |
| 10 | `Main.kt:395-396` | `NoticeHost()` 与页面内容同层放在 `Box(weight(1f))` 里，页面上任何全屏 `Box` 都可能把它盖住（当前 `NoticeHost` 是 `fillMaxSize` 的 `Box`，`contentAlignment = BottomCenter`，`Notices.kt:74`） |
| 11 | 全局 | 侧栏、顶栏、内容区之间没有任何分隔线（全目录无 `HorizontalDivider` / `VerticalDivider` / `Divider` 使用） |

### 7.4 开发期"未验证"标注（不是死代码，列出来供复刻时判定可信度）

| 位置 | 标注内容 |
|---|---|
| `DetailScreen.kt:56-59` | `toggleFavorite` 是写操作，"收藏成功"路径未经作者验证 |
| `FavoritesScreen.kt:48-49` | 列表内"取消收藏"与收藏夹增删改均未验证 |
| `HistoryScreen.kt:47-48` | `deleteHistory` 未验证 |
| `CommentsScreen.kt:49-51` | 发表、回复、删除三条写路径"均未经我验证" |
| `ProfileScreen.kt:36-39` | `dailyCheck` 写路径未验证，只验证过读取与显示 |
| `ReaderScreen.kt:422` / `:432` / `:488-489` | 收藏与点赞日志里直接写"（写操作，未验证）" |
| `RandomScreen.kt:61-63` | 偏好排序的收藏标签缓存"过期判断桌面端暂未做"（`RandomScreen.kt:94` 实际有 `FavoriteTags.isFresh` 判断，此处文件头注释与实现不一致） |
| `TagBlocker.kt:26-27` | 标签级屏蔽只在部分列表页接了 |

---

## 8. Qt 复刻时最难的五个桌面端特性（依据上面的实测）

| 序号 | 特性 | 为什么难（源码依据） |
|---|---|---|
| 1 | 双层壁纸 + 半透明面板构成的"玻璃"观感 | `BlogTheme.kt:146-149` 把 `WallpaperLayer` 铺在 `Box` 底层、`background = Color.Transparent`、`surface` 乘 `Appearance.effectiveAlpha`（`:81`/`:98`）；面板圆角与透明度随 5 档 `GlassStyle` 变（`Wallpaper.kt:43-49`），还有壁纸自身模糊（`Wallpaper.kt:153/175-182`）与压暗层必须在所有壁纸内容之上（`Wallpaper.kt:228-236`）。Qt 里 `QWidget` 的半透明链需要逐层 `WA_TranslucentBackground` + `QGraphicsBlurEffect`/自绘，且 `Modifier.blur` 只能模糊自身内容、做不出真正的 backdrop blur（`PARITY.md:977-980` 已确认这一限制） |
| 2 | 阅读页"一图流"管线：反切片 + LRU 字节缓存 + 串行预取 + 自学习调参 + 采样 | `RemoteImage.kt:113-177`（Skia 画布路径逐 band `drawImageRect` 反切片，失败退回 PNG + ImageIO 像素搬运的兜底）、`RemoteImage.kt:47`（缓存上限 `256L * 1024 * 1024` 字节、按 `LinkedHashMap` accessOrder 逐出）、`ReaderScreen.kt:240-263`（串行预取 + `prefetchInFlight` 去重 + 深度来自 `SelfTuner.prefetchDepth`）、`ReaderScreen.kt:205-238`（进页/离页采样，含 15 秒等待与 `awaitCancellation`）、`ReaderScreen.kt:278-297`（读到 60% 预加载下一话）。Qt 里要自己实现 band 搬运（`QImage` 逐行拷）、字节级 LRU、以及同一套调度与采样边界 |
| 3 | 两套阅读模式共用一份页索引与一套侧栏/底栏，并保持位置同步与键盘焦点 | `ReaderScreen.kt:83-85`（纵向状态）、`:187-189`（横向受控 `PagerState`）、`:329`（横向）/`:337-341`（纵向 `spacedBy(0.dp)`）、`:389-403` 与 `:469-476`（切换时先同步下标再翻转模式并落盘）、`:193-196`（焦点要在模式切换后重新申请，否则键盘失效）、`:302-325`（键盘翻页按模式走两条不同路径）。Qt 侧不存在"Compose 的焦点与重组"这套等价物，`QStackedWidget` + `QScrollArea`/自绘 pager 的索引同步与焦点策略要手写 |
| 4 | 自绘竖排页码栏（`PageRail`，40dp 宽、滑轨固定占高 70%、把手 18dp）与其像素级手势换算 | `PageRail.kt:79`（`width(40.dp)`）、`:86`（`fillMaxHeight(0.7f)`）、`:95-119`（底轨 6dp + 已读段 + 把手按 `trackH` 偏移）、`:125-138`（点击与竖向拖动都按 `trackH` 换算页码）。这是"不用 material3 Slider 旋转"的自绘替代，Qt 里要用 `paintEvent` 画轨道并在 `mousePressEvent`/`mouseMoveEvent` 里做同样的换算，且窗口尺寸变化时 `trackH` 必须实时更新（`onSizeChanged` 对应 `resizeEvent`） |
| 5 | 页面栈的动效与共享元素过渡 | `Main.kt:418-427`（`AnimatedContent` 淡入 + `slideInHorizontally { it / 14 }`，时长 280ms/120ms 走 `Motion` 的 cubic-bezier）、`Main.kt:428` + `SharedElement.kt:69-79`（`SharedTransitionLayout` 提供两个作用域）、`ComicCover.kt:53` 与 `DetailScreen.kt:313` 用同一 key 配对、`SharedElement.kt:43`（`boundsTransform` 用 `Motion.PAGE_MS`）。Qt 里没有共享元素机制，需要自己在两个页面之间做几何插值（起点从列表项位置算出、终点到详情页封面位置），并处理好缺失作用域时的降级（源码里 `jmSharedElement` 拿不到作用域就什么都不做，`SharedElement.kt:36-38`） |
