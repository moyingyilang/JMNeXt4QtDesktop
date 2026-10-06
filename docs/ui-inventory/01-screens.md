# JMComic_Next 全部界面清点（01-screens）

清点对象：`/data/data/com.termux/files/home/jmc/JMComic_Next`
- Android 端：`app/src/main/kotlin/com/jmnext/`
- 桌面端：`desktop/src/main/kotlin/com/jmnext/desktop/`
- 两端共用数据层：`shared/src/main/kotlin/com/jmnext/`

只记录在源码里实际看到的东西，全部给出行号。没有看到的（例如"某个按钮应该会做什么"）不写。

---

## 0. 屏幕总数

| 计数口径 | 数量 |
|---|---|
| 逻辑屏幕（两端合并去重后的界面清单） | **24** |
| Android 端 NavHost 注册的目的地 | 20 |
| 桌面端 `App()` 分发分支（不含兜底） | 21 |
| 桌面端额外界面：兜底占位页壳 `PageShell` | 1 |
| 桌面端有实现但没有任何调用方的界面 | 1（`TrackingScreen`，见 §9） |

逻辑屏幕名单（一行一个）：

1. 应用外壳（导航框架：Android 底部栏 / 桌面左侧栏）
2. 首页
3. 分类
4. 搜索
5. 我的
6. 分区更多（more/{id}）
7. 连载更新（每周更新表）
8. 周刊
9. 随机推荐
10. 通知
11. 收藏
12. 观看历史
13. 追更
14. 标签收藏
15. 画师与作品库
16. 作品信息（creator/work/{id}）
17. 作品详情
18. 阅读器
19. 评论
20. 登录 / 注册 / 找回密码
21. 屏蔽设置
22. 外观设置
23. 关于
24. 未实现页占位壳（仅桌面端 `PageShell`）

---

## 1. 两端 UI 框架的差别（先看这一节，后面表格里的"Android / 桌面"才读得通）

| 维度 | Android | 桌面 |
|---|---|---|
| 导航机制 | Navigation Compose（`NavHost` + 字符串路由） | 手写状态机（`sealed interface Screen` + 字符串 route），无导航库 |
| 定义处 | `app/src/main/kotlin/com/jmnext/ui/JmNavHost.kt:339` 的 `JmNavHost` | `desktop/src/main/kotlin/com/jmnext/desktop/Main.kt:309` 的 `App()` |
| 顶层导航控件 | 底部栏（`NavigationBar` 贴底 / `FloatingBottomBar` 悬浮，二选一） | 左侧常驻栏 `SideNav`（固定 176dp） |
| 顶层导航项 | 4 个主 Tab：首页 / 分类 / 搜索 / 我的（`JmNavHost.kt:101` 的 `MainTab` 枚举，值在 `:107-110`） | 15 项（`desktop/.../SideNav.kt:25` 的 `NAV_ITEMS`） |
| 转场 | Tab 之间整屏横滑（`JmNavHost.kt:291` `enterFor` / `:304` `exitFor`），其余淡入淡出 | 统一 `fadeIn + slideInHorizontally(1/14 宽)`（`Main.kt:418-425`） |
| 顶栏 | `GlassTopBar`（毛玻璃，自处理状态栏内边距；`app/.../ui/components/GlassTopBar.kt:34`，参数 `title/subtitle/navigation/actions`） | 无统一顶栏组件；每页自己写一行标题 + 按钮（多为 `Text` + `Button`/`TextButton`） |
| 共享元素 | `SharedTransitionLayout` + `jmComicSharedKey`（`JmNavHost.kt:432`、`app/.../ui/SharedTransition.kt`） | `SharedPageHost` + `Modifier.jmSharedElement`（`desktop/.../SharedElement.kt:35`、`:69`） |
| 占位/未实现页 | 无（每个目的地都有真实实现） | 有：`PageShell`（`desktop/.../PageShell.kt:25`），配 `PAGE_PLANS` 文案表（`:51`） |

Android 侧应用外壳链路：`MainActivity`（`app/src/main/kotlin/com/jmnext/MainActivity.kt:48` `setContent`）→ `CompositionLocalProvider(LocalRepository)`（`:84`）→ `JmTheme`（`:91`）→ `JmNavHost`（`:98`）。
桌面侧应用外壳链路：`main()`（`Main.kt:88`）→ `runApp()` / `Window`（`:117-127`）→ `BlogTheme { App() }`（`:126`）。

---

## 2. Android 导航结构

### 2.1 目的地总表

| # | 界面 | 路由常量 / 路由串 | 参数 | 承载 Composable（文件:行） | 需要登录 |
|---|---|---|---|---|---|
| 1 | 首页 | `MainTab.Home.route` = `"home"` | 无 | `HomeScreen` `app/src/main/kotlin/com/jmnext/ui/screens/home/HomeScreen.kt:76` | 否（追更标记与快捷签到需登录） |
| 2 | 分类 | `"category"` | 无 | `CategoryScreen` `.../screens/category/CategoryScreen.kt:266` | 否 |
| 3 | 搜索 | `SEARCH_PATTERN` = `"search?q={q}"` (`JmNavHost.kt:113`) | `q`（默认 `""`） | `SearchScreen` `.../screens/search/SearchScreen.kt:356` | 否 |
| 4 | 我的 | `"profile"` | 无 | `ProfileScreen` `.../screens/profile/ProfileScreen.kt:119` | 否（页内账号/签到/入口需登录） |
| 5 | 分区更多 | `MORE_PATTERN` = `"more/{id}?title={title}"` (`JmNavHost.kt:150`) | `id`、`title`（默认 `""`） | `MoreListScreen` `.../screens/more/MoreListScreen.kt:228` | 否 |
| 6 | 周刊 | `ROUTE_WEEK` = `"week"` (`:135`) | 无 | `WeekScreen` `.../screens/week/WeekScreen.kt:210` | 否 |
| 7 | 随机推荐 | `ROUTE_RANDOM` = `"random"` (`:138`) | 无 | `RandomListScreen` `.../screens/random/RandomListScreen.kt:91` | 否 |
| 8 | 通知 | `ROUTE_NOTIFICATIONS` = `"notifications"` (`:137`) | 无 | `NotificationsScreen` `.../screens/notifications/NotificationsScreen.kt:55` | 页内无登录门；入口只在"我的"的已登录区 |
| 9 | 收藏 | `ROUTE_FAVORITES` = `"favorites"` (`:132`) | 无 | `AccountListScreen(kind = Favorites)` `.../screens/favorites/FavoritesScreen.kt:352` | 是（`:426` 需要登录态） |
| 10 | 观看历史 | `ROUTE_HISTORY` = `"history"` (`:133`) | 无 | `AccountListScreen(kind = History)` 同上 | 是（同上） |
| 11 | 追更 | `ROUTE_TRACKING` = `"tracking"` (`:136`) | 无 | `AccountListScreen(kind = Tracking)` 同上 | 是（同上） |
| 12 | 标签收藏 | `ROUTE_TAGS` = `"tags"` (`:140`) | 无 | `TagFavoritesScreen` `.../screens/tags/TagFavoritesScreen.kt:122` | 是（`:162`） |
| 13 | 画师与作品库 | `ROUTE_CREATOR` = `"creator"` (`:141`) | 无 | `CreatorScreen` `.../screens/creator/CreatorScreen.kt:245` | 否 |
| 14 | 作品信息 | `ROUTE_CREATOR_WORK` = `"creator/work/{id}"` (`:142`) | `id` | `CreatorWorkScreen` `.../screens/creator/CreatorWorkScreen.kt:108` | 否 |
| 15 | 作品详情 | `ROUTE_DETAIL` = `"detail/{id}?cover={cover}&title={title}"` (`:125`) | `id`、`cover`（默认 `""`）、`title`（默认 `""`） | `DetailScreen` `.../screens/detail/DetailScreen.kt:527` | 否（收藏/追更/点赞/下载/标签收藏需登录） |
| 16 | 阅读器 | `ROUTE_READ` = `"read/{comicId}/{chapterId}"` (`:130`) | `comicId`、`chapterId` | `ReaderScreen` `.../screens/reader/ReaderScreen.kt:284` | 否（页内不校验登录，见 §7） |
| 17 | 评论 | `ROUTE_COMMENTS` = `"comments/{aid}"` (`:134`) | `aid` | `CommentsScreen` `.../screens/comments/CommentsScreen.kt:239` | 否（发表/删除需登录） |
| 18 | 登录/注册/找回 | `ROUTE_AUTH` = `"auth?reason={reason}"` (`:131`) | `reason`（默认 `""`） | `AuthScreen` `.../screens/auth/AuthScreen.kt:169` | 本身就是登录页 |
| 19 | 屏蔽设置 | `ROUTE_BLOCK` = `"block"` (`:143`) | 无 | `BlockSettingsScreen` `.../screens/settings/BlockSettingsScreen.kt:58` | 否 |
| 20 | 关于 | `ROUTE_ABOUT` = `"about"` (`:139`) | 无 | `AboutScreen` `.../screens/about/AboutScreen.kt:63` | 否 |

路由拼接辅助函数都在 `JmNavHost.kt`：`moreFor`(`:153`)、`searchFor`(`:157`)、`authFor`(`:160`)、`detailFor`(`:183`)、`push`(`:189`)。`ComicTarget`（`:169`）是"列表 -> 详情"携带的封面/标题载体。

### 2.2 导航关系图（自上而下）

```
MainActivity
└── JmTheme + JmNavHost (bottom nav: 首页 分类 搜索 我的)
    ├── home ──────► detail/{id} ─┬─► read/{comicId}/{chapterId}
    │   │                          ├─► comments/{aid}
    │   │                          ├─► detail/{id}          (相关推荐 / 换一部)
    │   │                          ├─► search?q={tag}       (标签 / 作者)
    │   │                          └─► auth?reason={reason} (收藏 / 追更 / 点赞 / 下载 / 标签收藏)
    │   ├── more/{id}?title=... ──► detail/{id}
    │   ├── week ──────────────► detail/{id}
    │   └── random ────────────► detail/{id}
    ├── category ──┬─► search?q={tag}
    │              ├─► detail/{id}
    │              └─► creator ──► creator/work/{id}
    ├── search?q={q} ──► detail/{id}
    └── profile ───┬─► auth?reason=
                   ├─► favorites ──► detail/{id} ──► auth?reason=
                   ├─► history   ──► detail/{id} ──► auth?reason=
                   ├─► tracking  ──► detail/{id} ──► auth?reason=追更需要登录
                   ├─► notifications ─► detail/{id}
                   ├─► tags      ──► search?q={tag}
                   ├─► block
                   └─► about

read/{comicId}/{chapterId} ──► comments/{aid}（底栏"评论"入口）
comments/{aid} ──► auth?reason=发表评论需要登录
```

以上箭头全部来自 `JmNavHost.kt` 的 `composable(...)` 块与各自传入的回调（`:510` 起至 `:791`）。

### 2.3 底栏与转场要点

- 底栏只在 4 个主 Tab 上出现：`showBottomBar = MainTab.entries.any { it.pattern == currentRoute }`（`JmNavHost.kt:357`）。
- 两种底栏形态由 `uiOptions.floatingBottomBar` 二选一（`:448`）：`FloatingBottomBar`（`:449`，参数 `items/selectedIndex/onSelect`）或 `DockedBottomBar`（`:455`，内部 `GlassSurface` + `NavigationBar`，`:842`）。
- 底栏高度是**量出来的**（`onSizeChanged`，`:447`）并写入 `LocalBottomBarInset`（`:462`），页面据此在列表底部留白。
- Tab 切换的横滑方向由 `tabShiftOf`（`:239`）用 `MainTab` 下标差算出；横滑期间两页摘掉共享元素键（`slidingTabRoutesOf`，`:274`；使用点 `:516`、`:538`、`:581`）。
- 预测性返回：`PredictiveBackHandler`（`:813`），只做透明度跟手、不缩放；`ownBackFeedback`（`:414`）限制在 API 33-34 自己画。

---

## 3. Android 逐屏清单

### 3.1 应用外壳（底部导航框架）
- 入口 Composable：`JmNavHost` — `app/src/main/kotlin/com/jmnext/ui/JmNavHost.kt:339`
- 一句话作用：全局导航容器，决定底栏显隐、页面转场与共享元素作用域。
- 布局骨架：`SharedTransitionLayout`（`:432`）→ `Scaffold`（`:441`，`snackbarHost = SnackbarHost(noticeHostState)`，`containerColor = Color.Transparent`）→ `bottomBar` 槽（`:445`，按 `showBottomBar` 条件渲染）→ `Box`（`:464`，承载跟手淡出 `graphicsLayer`）→ `NavHost`（`:475`）。
- 关键组件：`Scaffold`(snackbarHost/bottomBar/contentColor)、`SnackbarHost`、`NavHost`(startDestination = `"home"`，四段转场 `enter/exit/popEnter/popExit`，`:497-508`)、`FloatingBottomBar`/`DockedBottomBar`、`PredictiveBackHandler`(`enabled = ownBackFeedback && canGoBack`)。
- 状态/输入：函数参数 `readerMode / onReaderModeChange / themeMode / onThemeModeChange / dynamicColor / onDynamicColorChange / themeStyle / onThemeStyleChange / isDark / uiOptions / onUiOptionsChange`（`:340-350`）；内部状态 `currentRoute`(`:356`)、`showBottomBar`(`:357`)、`barItems`(`:361`)、`selectedTabIndex`(`:362`)、`slidingTabRoutes`(`:379`)、`backFeedback`(`:401`)、`barInsetPx`(`:428`)、`noticeHostState`(`:435`)。
- 交互：点底栏项 -> `switchTab`（`:382`，`popUpTo("home"){saveState}` + `launchSingleTop` + `restoreState`）；收到 `Notices` 通知 -> 弹 Snackbar（`:437`）；返回手势 -> 跟手淡出并 `popBackStack()`（`:813-832`）。
- 需要登录：否。

### 3.2 首页
- 入口 Composable：`HomeScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/home/HomeScreen.kt:76`（内部 `HomeContent` `:166`、`SectionTitle` `:379`）
- 一句话作用：推荐分区（横向滚动）+ 最新上架（纵向无限列表），右下角两个浮钮。
- 布局骨架：`Column(fillMaxSize)`（`:92`）→ 1) `GlassTopBar`（`:93`，title = app_name，subtitle = 加载中...，actions 三个 IconButton）2) `when` 四态：`LoadingBox`（`:126`）/ `ErrorBox`（`:128`）/ 正常 -> `Box`（`:137`）内叠 `HomeContent` + `DailyQuickFab`（`:146`）+ `RandomFab`（`:153`，都靠 `Modifier.align(BottomEnd)`）。
  `HomeContent` 内部：`LazyColumn`（`:238`）item 顺序 = 每个分区的 `SectionTitle`(`:249`) + `LazyRow`(`:259`) -> "展开更多分区"胶囊（`:307`）-> "最新上架"标题（`:327`）-> `items(state.latest)` 的 `ComicRow`（`:336`，包在 `Box(padding horizontal = Spacing.lg)` 里）-> `LoadMoreFooter`（`:352`）。
- 关键组件：`GlassTopBar`(title/subtitle/actions)、`IconButton` 三个（周刊 `CalendarMonth` `:98`、刷新 `Refresh` `:106`、明暗切换 `:114`）、`LazyColumn`(contentPadding bottom = `Spacing.xxl + LocalBottomBarInset.current`)、`LazyRow`、`ComicCard`(`item/coverUrl/onClick/sharedKey/updated/modifier`，`:273`)、`ComicRow`(`:340`)、`GlassSurface`(level=Card, shape=pill, onClick，`:307`、`:416`)、`LoadMoreFooter`(loading/error/exhausted/onLoadMore/onRetry)、`DailyQuickFab`(`.../home/RandomFab.kt:128`)、`RandomFab`(`RandomFab.kt:63`)。
- 状态/输入：参数 `dark/onToggleTheme/onOpenComic/onOpenSection/onOpenWeek/onOpenRandomList/modifier`（`:76-85`）；ViewModel 状态 `HomeUiState`（`.../home/HomeViewModel.kt:21`：`loading/sections/promoteError/latest/latestTotal/latestError/loadingMore/loadMoreError/latestExhausted`）；本地状态 `visibleSections`(`:179`)、`updatedIds`(`:192`)、`duplicatedComicIds`(`:229`)、`hiddenFlow`(`:223`)。
- 交互：点封面卡 -> `onOpenComic(ComicTarget(id, cover, name))` -> `detail/{id}?cover=&title=`（`:277`、`:343`）；点分区标题旁"更多" -> `onOpenSection(section)` -> `more/{id}?title=`（`:252`）；点"展开更多分区" -> `visibleSections += 6`（`:310`）；顶栏日历 -> `week`（`:98`）；刷新 -> `vm.refresh()`（`:106`）；明暗 -> `onThemeModeChange`（`:114`）；`RandomFab` 单击 -> `randomRecommend()` 抽一本进详情；长按 -> `random` 列表页；`DailyQuickFab` -> `daily(uid)` / `dailyCheck(uid,dailyId)`（`RandomFab.kt:149`、`:635-664`）。
- 需要登录：页面本身否。`updatedIds` 未登录时为空集（`:192-204`）；`DailyQuickFab` 未登录直接 `return` 不占位（`RandomFab.kt:164`）。

### 3.3 分类
- 入口 Composable：`CategoryScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/category/CategoryScreen.kt:266`（内部 `CategoryChipRow` `:376`、`CategoryGrid` `:412`、`TagBlocks` `:499`、`TagFallback` `:545`）
- 一句话作用：按分类树（父类 / 子类）+ 排序浏览作品，分类树失败时退回热门标签。
- 布局骨架：`Column(fillMaxSize)`（`:279`）→ 1) `GlassTopBar`（`:280`，title="分类"，subtitle=当前父类名或"按分类浏览"）2) `when`：`LoadingBox`（`:286`）/ `TagFallback`（`:288`）/ 正常分支（`:295`）依次为 `CategoryChipRow("分类")`（`:297`）-> 有子类时 `CategoryChipRow("子类")`（`:308`，选项首位是 `"" to "全部"`）-> 一行"画师与作品库 / 进入 >"（`:318`）-> `CategoryChipRow("排序")`（`:333`）-> 列表四态：`LoadingBox` / `ErrorBox` / `MessageState("这个分类下暂时没有作品")` / `CategoryGrid`（`:357`）。
  `CategoryGrid`：`LazyVerticalGrid(GridCells.Adaptive(CardSizes.grid))`（`:441`），`ComicCard` 项（`:460`），尾部整行 `TagBlocks`（`:474`）与 `LoadMoreFooter`（`:480`）。
- 关键组件：`GlassTopBar`、`FilterChip`(`selected/onClick/label`，`:395`)、`TextButton`("进入 >" `:328`)、`LazyRow`(`:393`)、`LazyVerticalGrid`(columns=Adaptive, `state`, contentPadding, 横向/纵向 spacing)、`ComicCard`、`GlassSurface` 等。
- 状态/输入：参数 `onOpenTag/onOpenComic/onOpenCreators/modifier`（`:266-270`）；状态来自 `CategoryViewModel.state`：`loading/categories/parent/sub/sort/comics/blocks/loadingList/listError/loadingMore/loadMoreError/exhausted/fallbackTags/error`（本文件内使用于 `:282-368`）；`gridState`(`:424`)、`atBottom`(`:425`)、`hiddenIds`(`:438`)。
- 交互：点分类 chip -> `vm.selectParent`（`:302`）/ `vm.selectSub`（`:313`）；点排序 chip -> `vm.setSort`（`:338`）；点"进入 >" -> `onOpenCreators` -> `creator`（`:328`）；点卡片 -> `onOpenComic(ComicTarget(...))` -> `detail/{id}`（`:464`）；点标签块 -> `onOpenTag` -> `search?q={tag}`（`:475`）；触底（最后一项可见）-> `onLoadMore()`（`:431`）；失败重试 -> `vm.loadTree()` / `vm.loadList()`。
- 需要登录：否。

### 3.4 搜索
- 入口 Composable：`SearchScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/search/SearchScreen.kt:356`（内部 `TagBlockedNotice` `:615`、`SuggestionPanel` `:666`、`WordChips` `:769`、`SearchFilterRows` `:794`、`FilterRow` `:815`、`DateFilterRows` `:855`）
- 一句话作用：关键词/编号检索；未搜索时给历史、热词与随机推荐；结果可排序、按年份筛、按标签屏蔽过滤。
- 布局骨架：`Column(fillMaxSize)`（`:420`）→ 1) `GlassTopBar(title = "搜索")`（`:421`）2) `OutlinedTextField`（`:423`，`trailingIcon` 是搜索 IconButton `:433`，`keyboardActions.onSearch` `:438`）3) 提示行 `state.hint`（`:441`）4) 年份摘要行 + 展开/收起 `IconButton`（`:451-469`）5) `showDateFilter` 时 `DateFilterRows`（`:471`）6) `TagBlockedNotice`（`:491`，**在列表之外**）7) `when`：`LoadingBox` / `ErrorBox` / `!searched -> SuggestionPanel`（`:506`）/ `results.isEmpty() -> SearchFilterRows + MessageState`（`:526`）/ 正常 -> `LazyColumn`（`:538`）：`item("filters")` `SearchFilterRows`（`:552`）-> `item("count")` 结果计数文案（`:556`）-> 结果卡。
- 关键组件：`GlassTopBar`、`OutlinedTextField`(value/onValueChange/placeholder/singleLine/trailingIcon/keyboardOptions)、`IconButton`(`ExpandLess`/`ExpandMore`)、`FilterChip`(`:835` 在 `FilterRow` 内)、`LazyColumn`、`LazyRow`(`:737`、`:833`)、`TagBlockedNotice`(count/tags/onAllowOnce)、`SuggestionPanel`(history/hotTags/recommend/onPick/onClearHistory/onShuffle/onRandomOne)、`LoadingBox`/`ErrorBox`/`MessageState`。
- 状态/输入：参数 `onOpenComic/initialQuery/modifier`（`:356-360`）；`SearchViewModel.state`：`loading/error/searched/results/history/hotTags/recommend/total/hidden/filters/searchId/redirectAid/hint`；本地状态 `input`(`:368`)、`showDateFilter`(`:369`)、`autoSearched`(`:378`)、`hiddenIds`(`:384`)、`blockedBy`(`:387`)、`lastSearchId`(`:394`)。
- 交互：输入 -> `vm.onQueryChange`（`:427`）；回车/点放大镜 -> `vm.search()`（`:433`、`:438`）；点历史/热词/推荐词 -> 填入并搜索（`:512`）；点"换一批"推荐 -> `vm.shuffleRecommend()`（`:518`）；点"随机一本" -> `vm.openRandomOne { onOpenComic(ComicTarget(id)) }`（`:519`）；清空历史 -> `vm.clearHistory()`（`:517`）；"允许一次" -> `tagBlocker.allowOnce(...)`（`:495`）；`state.redirectAid` 非空 -> `vm.consumeRedirect()` 后直接 `onOpenComic(ComicTarget(id))`（`:403-408`）；从分类带标签进来 -> 自动搜一次（`:412`）；点结果卡 -> `detail/{id}`。
- 需要登录：否。

### 3.5 我的
- 入口 Composable：`ProfileScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/profile/ProfileScreen.kt:119`
- 一句话作用：账号信息 + 签到 + 全部设置项 + 各二级入口的集散页。
- 布局骨架：`Column(fillMaxSize)`（`:146`）→ 1) `GlassTopBar(title = "我的")`（`:147`）2) `LazyColumn`（`:149`，contentPadding 四边，bottom 预留 `LocalBottomBarInset`）逐 item：`AccountCard`（`:161`）-> `DailyCard`（`:173`）-> `AppearanceCard`（`:175`）-> 非 lite 时 `WallpaperCard`（`:188`）-> `BlockCard`（`:189`）-> `ReadingCard`（`:190`）-> `PrivacyCard`（`:191`）-> `AboutCard`（`:192`）-> `ServerCard`（`:193`）。
- 关键组件（含各自行号）：`AccountCard`(`:199`，未登录显示"登录 / 注册"按钮 `:224`，已登录显示 `member.displayName` + 等级 chip `:235`、`InfoRow` 金币/等级/免广告 `:242-245`、两行 `EntryButton`：我的收藏 `:255`、观看历史 `:256`、通知(带 `badge = unread`) `:276`、我的追更 `:277`、标签收藏 `:278`、`TextButton("退出登录")` `:291`)、`RowScope.EntryButton(label/icon/onClick/badge)`(`:301`)、`DailyCard`(`:1138` -> `SettingCard("每日签到")` `:1165`，按周铺格子 `:1213`，`Button` 签到 `:1249`)、`AppearanceCard`(`:403`，含 `Switch(dynamicColor)` `:495`、`OptionSwitch` `:628`/`Switch` `:653`、`StyleOption` `:676`)、`WallpaperCard`(`:757`，`OutlinedTextField` `:800`、`IconButton` 换一张 `:826`、`Slider` 两根 `:864`/`:875`)、`BlockCard`(`:349`)、`ReadingCard`(`:922`)、`PrivacyCard`(`:958`)、`AboutCard`(`:978`)、`ServerCard`(`:1009`)、`SettingCard(title, content)`(`:1089`)、`InfoRow(label, value)`(`:1108`)。
- 状态/输入：参数 17 个（`:119-141`）：`themeMode/onThemeModeChange/dynamicColor/onDynamicColorChange/readerMode/onReaderModeChange/themeStyle/onThemeStyleChange/isDark/uiOptions/onUiOptionsChange/onLogin/onLogout/onOpenFavorites/onOpenHistory/onOpenTracking/onOpenNotifications/onOpenAbout/onOpenTags/onOpenBlock`；`auth = repo.auth.state.collectAsStateWithLifecycle()`（`:144`）；`DailyCard` 内部读 `authStore.state.member.uid`（`:1145-1149`）与 `repo.daily(uid)`（`:1161`）；未读角标 `produceState` + `repo.notificationsUnread()`（`:267`）。
- 交互：登录/注册 -> `auth?reason=`（`JmNavHost.kt:603`）；退出登录 -> `repo.logout()`（`:606`）；我的收藏 -> `favorites`；观看历史 -> `history`；我的追更 -> `tracking`；通知 -> `notifications`；标签收藏 -> `tags`；屏蔽设置 -> `block`；关于 -> `about`（`JmNavHost.kt:608-614`）；签到 -> `DailyCard` 内 `repo.dailyCheck(...)`；主题/动效/阅读形态/壁纸改动 -> 写偏好并经回调上抛（`:495`、`:864`、`:875` 等）。
- 需要登录：页面否；四个入口（收藏/历史/追更/标签收藏）落地页需要；签到未登录时只显示"登录后再签到"（`:1167`）。

### 3.6 分区更多（more/{id}?title=）
- 入口 Composable：`MoreListScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/more/MoreListScreen.kt:228`（内部 `WeeklyFilterBar` `:319`）
- 一句话作用：某个推荐分区的完整列表；当 `id == 26`（连载更新）时切换成"每周更新表"形态。
- 布局骨架：`Column(fillMaxSize)`（`:243`）→ 1) `GlassTopBar`（`:244`，title 为空时回退"更多"，subtitle 显示"共 N 项"，`navigation` 里是返回 `IconButton` `:252`）2) `state.weekly` 时 `WeeklyFilterBar`（`:262`，两个维度：类型 / 星期）3) `when`：`LoadingBox` / `ErrorBox` / `MessageState`（周更时空表提示"这天的更新是空的"，否则"这个分区暂时没有内容"）/ `LazyVerticalGrid`（`:284`，`ComicCard` 项 `:293` + 整行 `LoadMoreFooter` `:303`）。
- 关键组件：`GlassTopBar`(title/subtitle/navigation)、`IconButton(ArrowBack)`、`WeeklyFilterBar`(`:319` -> `LazyRow` `:358` + `FilterChip` `:360`)、`LazyVerticalGrid(GridCells.Adaptive(CardSizes.grid))`、`ComicCard`、`LoadMoreFooter`。
- 状态/输入：参数 `sectionId/title/onBack/onOpenComic/modifier`（`:228-233`）；`MoreListViewModel`（`viewModelFactory`，`key = "more-$sectionId"`，`:236`）状态：`loading/error/items/total/loadingMore/loadMoreError/exhausted/weekly/type/day`。
- 交互：点返回 -> `onBack()` -> `popBackStack`；切换类型/星期 -> `vm.setType` / `vm.setDay`（`:266-267`）；点卡 -> `detail/{id}?cover=&title=`（`:297`）；页脚/触底 -> `vm.loadMore()`（`:308`）。
- 需要登录：否。

### 3.7 周刊
- 入口 Composable：`WeekScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/week/WeekScreen.kt:210`
- 一句话作用：按刊期 + 类型浏览官方周刊榜单。
- 布局骨架：`Column(fillMaxSize)`（`:222`）→ 1) `GlassTopBar`（`:223`，title="周刊"，subtitle = 刊期标签 + 共 N 部，navigation 返回 `:229`）2) 有刊期时 `ChipRow("刊期")`（`:240`）3) 有类型时 `ChipRow("类型")`（`:249`）4) `when`：`LoadingBox`（`:259`）/ `ErrorBox`（`:261`）/ `LoadingBox`（`:264`）/ `MessageState("这一期没有作品")`（`:266`）/ `LazyVerticalGrid`（`:273`）+ 整行 `LoadMoreFooter`（`:291`）。
- 关键组件：`GlassTopBar`、`ChipRow`（本文件 `:306` 起，内部 `LazyRow` `:315` + `FilterChip` `:334`，参数 `label/options/selectedId/labelOf/onSelect`）、`LazyVerticalGrid`、`ComicCard`、`LoadMoreFooter`。
- 状态/输入：参数 `onBack/onOpenComic/modifier`（`:210-213`）；`WeekViewModel.state`：`loading/error/loadingList/issues/issue/types/type/items/total/loadingMore/loadMoreError/exhausted`。
- 交互：点刊期 chip -> `vm.selectIssue(it)`（`:245`）；点类型 chip -> `vm.selectType(it)`（`:254`）；点卡 -> `detail/{id}`（`:286`）；页脚 -> `vm.loadMore()`（`:296`）。
- 需要登录：否。

### 3.8 随机推荐
- 入口 Composable：`RandomListScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/random/RandomListScreen.kt:91`（内部 `Hint` `:317`）
- 一句话作用：一次抽一批随机作品，网格/列表两种版式，并按收藏偏好标签重排、按屏蔽规则剔除。
- 布局骨架：`Column(fillMaxSize)`（`:197`）→ 1) `GlassTopBar`（`:198`，title="随机推荐"，subtitle 说明是否已排除屏蔽/按偏好排序；`navigation` 返回 `:207`；`actions` = 版式切换 `IconButton` `:213` + `TextButton("换一批")` `:223`）2) `when`：`Hint("正在随机...")` / `Hint("没拿到：$error")` / `Hint("这次没抽到...")` / 网格版 `LazyVerticalGrid(GridCells.Fixed(3))`（`:230`，每项 `Column` 内 `AsyncImage` + 标题 `:241-262`）/ 列表版 `LazyColumn`（`:265`，每项 `Row`：`AsyncImage` 82x114 dp + 标题/作者/分类 `:274-304`）。
- 关键组件：`GlassTopBar`(title/subtitle/navigation/actions)、`IconButton`(ViewList/GridView 切换 `:217-221`)、`TextButton("换一批")`、`LazyVerticalGrid(columns = Fixed(3), state = gridState)`、`LazyColumn(state = listState)`、`AsyncImage(model = repo.coverUrl(comic), aspectRatio 0.72f)`、`Modifier.jmAnimateItem`。
- 状态/输入：参数 `onBack/onOpenComic/modifier`（`:91-94`）；本地状态 `layout`(`:100`，持久化到 `AppPrefs.randomLayout` `:215`)、`items_`(`:101`)、`loading`(`:102`)、`error`(`:103`)、`round`(`:104`)、`favoriteTagCounts`(`:115`)、`knownTags`(`:117`)、`visibleIds`(`:124`)；依赖 `FavoriteTags`(`:114`)、`app.blockStore.state`(`:113`)、`LocalTagBlocker`(`:112`)。
- 交互：进页 / 点"换一批" -> `repo.bootstrap(); repo.randomRecommend()`（`:188-195`、`:223`）；切换版式 -> 写 `prefs.randomLayout`（`:213-216`）；点条目 -> `onOpenComic(target(repo, comic))`（`:245`、`:277`，`target` 在 `:313`）；可见条目触发标签读取（`:152-176`，并发上限 3）。
- 需要登录：否（收藏标签统计未登录时为空，仅影响排序）。

### 3.9 通知
- 入口 Composable：`NotificationsScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/notifications/NotificationsScreen.kt:55`（内部 `Hint` `:157`、`NotificationCard` `:167`）
- 一句话作用：服务端通知列表，三个类型页签，可标记已读。
- 布局骨架：`Column(fillMaxSize)`（`:91`）→ 1) `GlassTopBar`（`:92`，title="通知"，navigation 返回 `:95`）2) 页签行 `Row` + `TextButton` 三个（`:100-114`，点击同时把 `page` 复位为 1）3) `when`：`Hint("正在读取通知...")` / `Hint("读取失败：$error")` / `Hint("还没有通知。")` / `LazyColumn`（`:119`）：`NotificationCard` 项（`:127`）+ 未读满时 `item("more")` 的"加载更多（已显示 x / y）"（`:143`）。
- 关键组件：`GlassTopBar`、`TextButton`(页签 `:106`)、`LazyColumn`、`NotificationCard`(`:167`，参数 `item/onOpenComic/onMarkRead`)、`TextButton("加载更多")`。
- 状态/输入：参数 `onBack/onOpenComic/modifier`（`:55-58`）；本地状态 `tab`(`:64`，取值 `TYPE_ALL` / `comic_follow` / `site_notice`，见文件顶部注释 `:51-52`)、`items_`(`:65`)、`total`(`:66`)、`page`(`:67`)、`loading`(`:68`)、`error`(`:69`)。
- 交互：切页签 -> `tab = value; page = 1`（`:106`）后 `repo.notifications(type, page)`（`:78`）；点"加载更多" -> `page += 1`（`:144`）；点通知里的作品 -> `onOpenComic(id)` -> `detail/{id}`（`JmNavHost.kt:696`）；点卡片标记已读 -> 先本地改状态再 `repo.markNotificationRead(id, true)`（`:130-136`）。
- 需要登录：页面内**没有**登录门（本文件无 `isLoggedIn` 判断）；唯一入口在"我的"的已登录分支里（`ProfileScreen.kt:276`）。未登录直接进会因接口需要账号而报错并显示 `Hint`。

### 3.10 收藏 / 观看历史 / 追更（同一 Composable，三种 kind）
- 入口 Composable：`AccountListScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/favorites/FavoritesScreen.kt:352`（内部 `FolderRow` `:579`）
- 一句话作用：账号绑定的三种作品列表，共用一个 ViewModel 与一套渲染，差异在接口、收藏夹与尾部按钮。
- 三种 kind 的定义：`enum class AccountListKind` — `FavoritesScreen.kt:75`
  - `Favorites("我的收藏", BookmarkBorder, "还没有收藏，去详情页点收藏试试")` — `:76`
  - `History("观看历史", History, "还没有观看记录")` — `:77`
  - `Tracking("我的追更", NotificationsNone, "还没有追更，去详情页点追更")` — `:84`
  - `limit`：仅追更为 500（`:88`），因此副标题显示"N / 500"而不是"共 N 项"（`:387`）
  - 数据源分工：`Favorites -> repo.favorites(...)`、`History -> repo.history(page)`、`Tracking -> repo.trackingList(page)`（`:306-309`）
- 布局骨架：`Column(fillMaxSize)`（`:382`）→ 1) `GlassTopBar`（`:383`，title = kind.title，subtitle 按登录态/上限/总数三分支；navigation 返回 `:392`；actions 仅收藏且有登录时给"管理收藏夹" `IconButton` `:402`）2) `state.notice` 一行文案（`:414`）3) `when`：`!state.loggedIn -> MessageState("需要登录", onRetry = onLogin)`（`:426`）/ `loading 且空 -> LoadingBox`（`:434`）/ `error 且空 -> ErrorBox`（`:436`）/ 正常（`:439`）：收藏且有收藏夹时 `FolderRow`（`:441`）-> 空则 `MessageState(kind.emptyHint)`（`:449`）-> 否则 `LazyColumn`（`:451`），每项 `ComicRow`（`:463`）带 `trailing` 差异化按钮，尾部 `item("footer")`（`:510`）。
- 差异化的行内按钮（`:471-506`）：追更 -> `IconButton(NotificationsOff)` 取消追更；收藏 -> `IconButton(DriveFileMove)` 移入收藏夹；历史 -> `IconButton(DeleteOutline)` 删除这条历史。
- 关键组件：`GlassTopBar`、`IconButton`、`MessageState`、`FolderRow`(`folders/selected/onSelect`，`:579` -> `LazyRow` `:595` + `FilterChip` `:597`)、`LazyColumn`、`ComicRow(item/coverUrl/onClick/trailing)`、`LoadMoreFooter`、`LifecycleEventEffect(ON_RESUME)` 静默刷新（`:372`）。
- 状态/输入：参数 `kind/onBack/onOpenComic/onLogin/modifier`（`:352-357`）；`AccountListUiState`（`:91`）：`loading/error/loggedIn/items/folders/selectedFolder/total/loadingMore/loadMoreError/exhausted/notice`；本地 `dialog: FolderDialog`(`:367`)。
- 交互：点卡 -> `detail/{id}?cover=&title=`（`:467`）；三种尾部按钮见上；点"管理收藏夹" -> `dialog = FolderDialog.Manage`（`:402`）；`FolderRow` 切收藏夹 -> `vm.selectFolder(it)`（`:444`）；未登录点 `MessageState` 的重试 -> `onLogin`（`JmNavHost.kt:640/705/726`，分别对应 favorites/tracking/history 传不同 reason）。
- 需要登录：**是**。门在 `:426`（UI）与 `:148`（ViewModel 里 `if (!loggedIn) return`）。

### 3.11 标签收藏
- 入口 Composable：`TagFavoritesScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/tags/TagFavoritesScreen.kt:122`（内部 `TagPickerDialog` `:226`，该对话框也供详情页调用）
- 一句话作用：账号上收藏的标签清单，可删除、可点进搜索。
- 布局骨架：`Column(fillMaxSize)`（`:137`）→ 1) `GlassTopBar`（`:138`，title="标签收藏"，subtitle = "N / LIMIT"（仅登录））2) `state.notice` 文案（`:152`）3) `when`：未登录 -> `MessageState("需要登录", onRetry = onLogin)`（`:162`）/ `LoadingBox` / `ErrorBox` / `MessageState("还没有收藏标签")` / `LazyColumn`（`:180`）：每项 `Surface(shape, color = c.surface1, onClick = onOpenTag)`（`:186`）内含 `Text("#tag")` + `IconButton(Delete)`（`:202`）。
- 关键组件：`GlassTopBar`、`MessageState`、`LazyColumn`、`Surface(onClick)`、`IconButton`。
- 状态/输入：参数 `onBack/onLogin/onOpenTag/modifier`（`:122-126`）；`TagFavoritesUiState`：`loggedIn/loading/error/tags/notice`（`:58`、`:73`）；`TagFavoritesViewModel.LIMIT`（`:140`）。
- 交互：`LaunchedEffect(Unit) { vm.load() }`（`:135`）；点标签行 -> `onOpenTag(tag)` -> `search?q={tag}`（`JmNavHost.kt:713`）；点删除 -> `vm.remove(tag)`（`:202`）；未登录点重试 -> `auth?reason=标签收藏需要登录`。
- 需要登录：**是**（`:162`）。

### 3.12 画师与作品库
- 入口 Composable：`CreatorScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/creator/CreatorScreen.kt:245`（内部 `AuthorCard` `:383`、`WorkCard` `:420`）
- 一句话作用：作品库的两个维度（画师 / 作品），可搜索；点画师就地展开其作品，点作品进作品信息页。
- 布局骨架：`Column(fillMaxSize)`（`:257`）→ 1) `GlassTopBar`（`:258`，title = 作者名或"画师与作品库"，navigation 的返回按钮在"已进入某个画师"时改为关闭该画师 `:263`）2) 仅未选中画师时显示：`CreatorTab` 的 `FilterChip` 行（`:279`）+ 搜索 `OutlinedTextField`（`:288`，`trailingIcon` 搜索 `IconButton` `:300`）3) `when`：`LoadingBox` / `ErrorBox` / 画师 Tab -> `LazyVerticalGrid(GridCells.Adaptive(168.dp))`（`:323`，`AuthorCard` + `LoadMoreFooter`）/ 作品 Tab -> `LazyVerticalGrid`（`:352`，`WorkCard` + 页脚）。
- 关键组件：`GlassTopBar`、`FilterChip`、`OutlinedTextField`(placeholder/imeAction Search/keyboardActions)、`LazyVerticalGrid`、`AuthorCard`(`:383`，含 `AsyncImage` 头像 `:395`)、`WorkCard`(`:420`，含 `AsyncImage` `:433`)、`LoadMoreFooter`、`MessageState`。
- 状态/输入：参数 `onBack/onOpenWork/modifier`（`:245-248`）；`CreatorViewModel.state`：`loading/error/tab/query/authors/works/total/page/loadingMore/loadMoreError/exhausted/authorId/authorName`。
- 交互：切 Tab -> `vm.setTab(tab)`（`:282`）；输入 -> `vm.setQuery`（`:290`）；回车/放大镜 -> `vm.load()`（`:300`、`:308`）；点画师 -> `vm.openAuthor(author)`（`:334`）；点作品 -> `onOpenWork(id)` -> `creator/work/{id}`（`JmNavHost.kt:656`）。
- 需要登录：否。

### 3.13 作品信息（creator/work/{id}）
- 入口 Composable：`CreatorWorkScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/creator/CreatorWorkScreen.kt:108`
- 一句话作用：作品库里一个作品的信息与可看内容（长图阅读式列表）。
- 布局骨架：`Column(fillMaxSize)`（`:124`）→ 1) `GlassTopBar`（`:125`，title = 作品标题或"作品信息"，subtitle = 作者，navigation 返回 `:129`）2) `when`：`LoadingBox` / `ErrorBox` / `MessageState("没有这个作品的信息")` / `LazyColumn`（`:150`）：`item("meta")` 日期与作者（`:155`）-> 图片序列 `items(images.size)`（`:168`，每张 `AsyncImage(contentScale = FillWidth)` `:169`）-> 末尾 `LazyRow` 相关作品（`:212`）。
- 关键组件：`GlassTopBar`、`LazyColumn`、`AsyncImage`、`LazyRow`、`MessageState`。
- 状态/输入：参数 `workId/onBack/onOpenWork/modifier`（`:108-112`）；`CreatorWorkViewModel.state`：`loading/error/info/content`。
- 交互：点相关作品 -> `onOpenWork(id)`（`:212` 区块内）；失败重试 -> `vm.load()`（`:142`、`:147`）。
- 需要登录：否。

### 3.14 作品详情
见 §6.2 详解。入口 `DetailScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/detail/DetailScreen.kt:527`。

### 3.15 阅读器
见 §6.1 详解。入口 `ReaderScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/reader/ReaderScreen.kt:284`。

### 3.16 评论
- 入口 Composable：`CommentsScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/comments/CommentsScreen.kt:239`（内部 `CommentCard` `:350`）
- 一句话作用：一部作品的评论列表 + 发表框；只能删自己的评论。
- 布局骨架：`Column(fillMaxSize)`（`:253`）→ 1) `GlassTopBar`（`:254`，title="评论"，subtitle="共 N 条"，navigation 返回 `:257`）2) 发表行 `Row`（`:271`）：`OutlinedTextField`（`:275`，placeholder 按登录态二分："说点什么（会公开发布）" / "登录后可发表评论"）+ `TextButton("发送")`（`:288`，draft 为空或 sending 时禁用）3) `state.notice` 文案（`:296`）4) `when`：`LoadingBox` / `ErrorBox` / `MessageState("还没有评论")` / `LazyColumn`（`:317`）：`CommentCard`（`:323`，删除入口仅当 `selfUid != null && comment.uid == selfUid` `:328`）+ `item("footer")` `LoadMoreFooter`（`:335`）。
- 关键组件：`GlassTopBar`、`OutlinedTextField(maxLines = 3, enabled = !sending)`、`TextButton`、`LazyColumn`、`CommentCard`(`:350`，含 `AsyncImage` 头像 `:363` 与 `IconButton` 删除 `:395`)、`LoadMoreFooter`。
- 状态/输入：参数 `comicId/onBack/onNeedLogin/modifier`（`:239-243`）；`CommentsUiState`：`selfUid/draft/sending/notice/loading/error/comments/loadingMore/loadMoreError/exhausted/total`；`CommentsViewModel.loggedIn`(`:97`)。
- 交互：输入 -> `vm.onDraftChange`（`:277`）；发送 -> `vm.send(onNeedLogin)`（`:289`），未登录时内部调 `onNeedLogin()` -> `auth?reason=发表评论需要登录`（`JmNavHost.kt:678`）；删除 -> `vm.delete(comment)`（`:329`）；页脚 -> `vm.loadMore()`（`:340`）。
- 需要登录：浏览否；发表与删除需要（`CommentsScreen.kt:175` 判断 `!loggedIn` 即回调 `onNeedLogin`）。

### 3.17 登录 / 注册 / 找回密码
- 入口 Composable：`AuthScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/auth/AuthScreen.kt:169`
- 一句话作用：三种模式的表单页（登录 / 注册 / 找回密码），共用一个 Composable。
- 布局骨架：`Column(fillMaxSize)`（`:198`）→ 1) `GlassTopBar`（`:199`，title = state.mode.label，navigation 返回 `:202`）2) `reason` 非空时一行说明"...登录后请再点一次刚才那个按钮"（`:214`）3) 可滚 `Column`（`:223`）：`SingleChoiceSegmentedButtonRow` 模式切换（`:230`，`SegmentedButton` 三个 `:232`）-> 用户名 `OutlinedTextField`（`:242`）-> 非 Forgot 时密码（`:252`）-> 仅 Register 时确认密码（`:267`）-> 非 Login 时邮箱（`:282`）-> 仅 Register 时性别 `FilterChip`（`:295-305`）-> `state.error` 卡（`:308`）-> `state.notice` 卡（`:318`）-> 主按钮 `Button`（`:329`，按模式分派 `vm.login/register/forgot`）-> 一段凭证说明文字（`:344`）。
- 关键组件：`GlassTopBar`、`SingleChoiceSegmentedButtonRow` + `SegmentedButton`、`OutlinedTextField`(label/singleLine/visualTransformation/keyboardOptions)、`PasswordVisualTransformation`、`FilterChip`、`GlassSurface`、`Button`。
- 状态/输入：参数 `onBack/onLoggedIn/reason/modifier`（`:169-174`）；`AuthViewModel.state`：`mode/submitting/error/notice/loggedIn`；本地 `username`/`email`/`gender` 用 `rememberSaveable`，`password`/`passwordConfirm` 刻意用 `remember`（`:183-191` 有说明）。
- 交互：切模式 -> `vm.setMode`（`:234`）；点主按钮 -> 按 `state.mode` 调 `vm.login(username, password)` / `vm.register(...)` / `vm.forgot(email)`（`:331-336`）；`state.loggedIn` 变 true -> `onLoggedIn()` 回退上一页（`:194`）。
- 需要登录：本身就是登录页。

### 3.18 屏蔽设置
- 入口 Composable：`BlockSettingsScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/settings/BlockSettingsScreen.kt:58`（内部 `BlockSection` `:171`、`AddBlockDialog` `:242`）
- 一句话作用：本地屏蔽名单（关键词 / 分类 / 标签）的增删。
- 布局骨架：`Column(fillMaxSize)`（`:69`）→ 1) `GlassTopBar`（`:70`，title="屏蔽设置"，subtitle="本地规则，不上传"，navigation 返回 `:74`）2) `LazyColumn`（`:84`）：`item("notice")`（`:90`）-> `item("hint")` 规则说明 `GlassSurface`（`:96`）-> `item("words")` `BlockSection("关键词屏蔽")`（`:110`）-> `item("categories")` `BlockSection("分类屏蔽")`（`:120`）-> `item("tags")` `BlockSection("标签屏蔽")`（`:130`）；列表外 `dialog?.let { AddBlockDialog(...) }`（`:141`）。
- 关键组件：`GlassTopBar`、`GlassSurface(level = Card)`、`LazyColumn`、`BlockSection(title/hint/values/onAdd/onRemove)`(`:171` -> `IconButton` 添加 `:185`、`IconButton` 删除 `:220`)、`AddBlockDialog(kind/onDismiss/onConfirm)`(`:242` -> `AlertDialog` `:255` + `OutlinedTextField` `:260`)。
- 状态/输入：参数 `onBack/modifier`（`:58-60`）；`rules = repo.blockStore?.state`（`:63-64`）；本地 `dialog: BlockKind?`(`:66`)、`notice`(`:67`)；长度校验常量 `MIN_LENGTH`/`MAX_LENGTH`（`:148`）。
- 交互：三类各自的"+ 添加" -> 打开 `AddBlockDialog`（`:114`、`:124`、`:134`）；对话框确认 -> 写 `store.addWord/addCategory/addTag` 并回写 `notice`（`:145` 起）；删除条目 -> `store?.removeWord/removeCategory/removeTag`（`:115`、`:125`、`:135`）。
- 需要登录：否（本地存储）。

### 3.19 关于
- 入口 Composable：`AboutScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/about/AboutScreen.kt:63`（内部 `InfoCard` `:208`、`InfoRow` `:222`、`LinkRow` `:230`）
- 一句话作用：版本号 / 变体、检查更新、项目说明与许可。
- 布局骨架：`Column(fillMaxSize)`（`:80`）→ 1) `GlassTopBar`（`:81`，title="关于"，navigation 返回 `:84`）2) `Column(verticalScroll)`（`:89`）：`InfoCard("版本")`（`:96`，含版本号与变体，lite 时附一段说明 `:100`）-> `InfoCard("检查更新")`（`:109`，说明文字 + `Row` 内 `TextButton("检查更新")` `:120`，有新版时给下载入口 `:139` 起）-> 后续 `InfoCard` 区块（`:208` 之后：项目说明 / 许可 / 链接等）。
- 关键组件：`GlassTopBar`、`Column(verticalScroll(rememberScrollState()))`、`InfoCard(title, content)`、`InfoRow(label, value)`、`LinkRow(label, onClick)`、`TextButton`。
- 状态/输入：参数 `onBack/modifier`（`:63-65`）；本地 `checking`(`:71`)、`result`(`:72`)、`updatable`(`:73`)；`variant` 由 `LiteFeatures.ENABLED` 与 `BuildConfig.DEBUG` 拼出（`:75`）。
- 交互：点"检查更新" -> `fetchLatestTag(repo.okHttp)` 后经 `UpdateCheck.isNewer` 比较并显示结果（`:122-135`）；有新版 -> 打开下载/发布页（`:139` 起，用 `LocalUriHandler`）；链接行 -> `uri.openUri(...)`（`LinkRow` `:230`）。
- 需要登录：否。

---

## 4. 桌面端导航结构

### 4.1 状态机与左侧栏

- 页面栈：`private sealed interface Screen` — `desktop/src/main/kotlin/com/jmnext/desktop/Main.kt:300`
  - `Home`（`:301`）
  - `Detail(id, title)`（`:302`）
  - `Reader(comicId, chapterId, chapterIds)`（`:303`）
  - `Login`（`:304`）
  - `Page(route: String)`（`:305`）—— 其余所有页面都走这一支，route 是字符串
- 分发点：`App()` 里 `when (val s = target)` — `Main.kt:429`，各分支行号见下表。
- 左侧栏条目：`NAV_ITEMS` — `desktop/src/main/kotlin/com/jmnext/desktop/SideNav.kt:25`，15 项，顺序为首页 / 搜索 / 分类 / 周刊 / 随机本子 / 收藏 / 历史 / 追更 / 标签 / 画师与作品库 / 通知 / 屏蔽设置 / 外观 / 关于 / 我的。
- `SideNav(selected, onSelect, unreadNotifications)` — `SideNav.kt:44`；未读数只在 >0 时以"通知（N）"形式拼在标题后（`:64-68`，>99 显示 `99+`）。
- 外壳还有：顶部一行标题 + 登录态 + "登录/退出登录"按钮（`Main.kt:366-393`）、退出登录二次确认 `AlertDialog`（`:398-417`）、连载更新降级提示行（`:352-365`）、瞬时反馈层 `NoticeHost()`（`:396`）。
- 未读数与通知角标：`LaunchedEffect(authState.loggedIn, screen) { repository.notificationsUnread().total }`（`:329-335`）。

### 4.2 目的地总表

| # | 界面 | 路由 / 状态 | 承载 Composable（文件:行） | 分发处 | 需要登录 |
|---|---|---|---|---|---|
| 1 | 首页 | `Screen.Home` / nav "home" | `HomeScreen` `Main.kt:131` | `Main.kt:430` | 否（快捷签到与追更标记需登录） |
| 2 | 作品详情 | `Screen.Detail(id, title)` | `DetailScreen` `.../desktop/DetailScreen.kt:65` | `Main.kt:440` | 否（收藏/点赞/追更/标星/下载需登录） |
| 3 | 阅读器 | `Screen.Reader(comicId, chapterId, chapterIds)` | `ReaderScreen` `.../desktop/ReaderScreen.kt:69` | `Main.kt:454` | 否（页内不校验） |
| 4 | 登录/注册/找回 | `Screen.Login` | `LoginScreen` `.../desktop/LoginScreen.kt:40` | `Main.kt:468` | 本身就是登录页 |
| 5 | 随机推荐 | `Page("random")` | `RandomScreen` `.../desktop/RandomScreen.kt:66` | `Main.kt:470` | 否 |
| 6 | 周刊 | `Page("week")` | `WeekScreen` `.../desktop/DiscoverPages.kt:40` | `Main.kt:471` | 否 |
| 7 | 关于 | `Page("about")` | `AboutScreen` `.../desktop/AboutScreen.kt:43` | `Main.kt:473` | 否 |
| 8 | 分类 | `Page("category")` | `CategoryScreen` `.../desktop/CategoryScreen.kt:51` | `Main.kt:474` | 否 |
| 9 | 我的 | `Page("profile")` | `ProfileScreen` `.../desktop/ProfileScreen.kt:44` | `Main.kt:475` | 否（账号/签到需登录） |
| 10 | 通知 | `Page("notifications")` | `NotificationScreen` `.../desktop/NotificationScreen.kt:52` | `Main.kt:476` | 页内无门；需要账号数据 |
| 11 | 画师与作品库 | `Page("creator")` | `CreatorScreen` `.../desktop/CreatorScreen.kt:61` | `Main.kt:477` | 否 |
| 12 | 评论 | `Page("comments:<aid>")` | `CommentsScreen` `.../desktop/CommentsScreen.kt:54` | `Main.kt:479` | 否（发表/删除需登录，`:125`、`:149`） |
| 13 | 屏蔽设置 | `Page("block")` | `BlockScreen` `.../desktop/BlockScreen.kt:40` | `Main.kt:484` | 否 |
| 14 | 外观 | `Page("appearance")` | `AppearanceScreen` `.../desktop/AppearanceScreen.kt:44` | `Main.kt:485` | 否 |
| 15 | 标签 | `Page("tags")` | `TagsScreen` `.../desktop/TagsScreen.kt:50` | `Main.kt:486` | 部分（扫描与标星需登录，`:105`、`:127`） |
| 16 | 搜索 | `Page("search")` | `SearchScreen` `.../desktop/SearchScreen.kt:94` | `Main.kt:490` | 否 |
| 17 | 收藏（含追更标签） | `Page("favorites")` | `FavoriteScreen` `.../desktop/FavoritesScreen.kt:52` | `Main.kt:491` | 是（`:57`、`:157`） |
| 18 | 观看历史 | `Page("history")` | `HistoryScreen` `.../desktop/HistoryScreen.kt:51` | `Main.kt:492` | 是（`:52`） |
| 19 | 追更（侧栏入口） | `Page("tracking")` | `FavoriteScreen(initialTab = "tracking")` 同上 | `Main.kt:495` | 是 |
| 20 | 分区更多 | `Page("more/<id>?title=<enc>")` | `MoreListScreen` `.../desktop/MoreListScreen.kt:48` | `Main.kt:505` | 否 |
| 21 | 连载更新（每周更新表） | `Page("more/26?title=<enc>")` | `WeeklyUpdateScreen` `.../desktop/MoreListScreen.kt:186` | `Main.kt:503` | 否 |
| 22 | 未实现页占位壳 | 以上都不匹配时 | `PageShell(title, planned)` `.../desktop/PageShell.kt:25` | `Main.kt:516` | 否 |

说明：分区 26 的判定在 `Main.kt:502`（`id == JmRepository.WEEKLY_SECTION_ID`）。`PageShell` 用 `NAV_ITEMS` 反查标题、用 `PAGE_PLANS`(`PageShell.kt:51`) 取"这一页将做什么"的文案。

### 4.3 导航关系图（自上而下）

```
main() -> Window -> BlogTheme -> App()          (左侧 SideNav 常驻 176dp)
├── Home --► Detail --┬--► Reader --┬--► Reader(下一话/上一话：同一页替换)
│                     │             └--► Comments(comments:<aid>)
│                     ├--► Comments(comments:<aid>) --► 返回 Detail(id, "")
│                     ├--► Detail(相关作品，递归)
│                     └--► 站内搜索（标签就地搜；onOpenTag 未接线）
│   ├── more/<id> --► Detail
│   └── random ----► Detail
├── search ---------------► Detail
├── category -------------► Detail
├── week -----------------► Detail
├── creator --------------► Detail（作品库条目转成 ListItem 后走 openComic）
├── favorites --┬--► Detail
│               └── 内嵌 TrackingList（追更标签）
├── history --------------► Detail
├── tracking -------------► 等同 favorites + initialTab=tracking
├── notifications --------► （无作品跳转；只标记已读）
├── comments:<aid> -------► 返回上级
├── Login ----------------► onDone 回 Home
├── block / appearance / about / tags / profile
└── 兜底 PageShell
```

箭头来自 `Main.kt:430-516` 的分支与传入的回调（例如 `openComic` `:320`、`onOpenChapter` `:448`、`onOpenComments` `:444`、`onSwitchChapter` `:462`）。

---

## 5. 桌面端逐屏清单

### 5.1 应用外壳
- 入口 Composable：`App` — `desktop/src/main/kotlin/com/jmnext/desktop/Main.kt:309`（窗口入口 `runApp` `:117`）
- 一句话作用：左侧常驻导航 + 顶部标题/登录态行 + 页面切换容器。
- 布局骨架：`Row(fillMaxWidth)`（`:344`）→ 左 `SideNav`（`:345`）→ 右 `Column(weight(1f))`（`:350`）：连载提示行（`:352`）→ 头部 `Row`（`:366`：`JMNeXt` 可点回首页 `:374`、登录态文字 `:376`、`TextButton` 登录/退出登录 `:386`）→ `Box(weight(1f))`（`:395`）：`NoticeHost()`（`:396`）+ 退出确认 `AlertDialog`（`:398`）+ `AnimatedContent`（`:418`）→ `SharedPageHost`（`:428`）→ `when(screen)` 分发（`:429`）。
- 关键组件：`SideNav`(selected/unreadNotifications/onSelect)、`AnimatedContent`(transitionSpec = fadeIn + slideInHorizontally(1/14 宽) togetherWith fadeOut，`:421-425`，`label = "route"`)、`AlertDialog`(title/text/confirmButton/dismissButton，`:399-416`)、`NoticeHost`(`Notices.kt:61`)、`SharedPageHost`(`SharedElement.kt:69`)、`TextButton`。
- 状态/输入：`screen`(`:310`)、`showLogoutConfirm`(`:311`)、`authState`(`:313`)、`navSelection`(`:314`)、`unreadNotifications`(`:328`)、`serialNotice`(`:339`)、`openComic`(`:320`)。
- 交互：点侧栏项 -> `screen = Home` 或 `Screen.Page(route)`（`:345-348`）；点 `JMNeXt` -> 回首页（`:374`）；点"登录" -> `Screen.Login`（`:390`）；点"退出登录" -> 弹 `AlertDialog`，确认后 `repository.logout()`（`:404-411`）；连载提醒出现时点"知道了" -> 清空 `serialNotice`（`:363`）。
- 需要登录：否。

### 5.2 首页
- 入口 Composable：`HomeScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/Main.kt:131`（内部 `ComicCard` `:251`、`RandomFab` `:540`、`DailyQuickFab` `:599`；推荐分区在 `HomeSections.kt:39`）
- 一句话作用：状态行 + 刷新/加载更多 + 推荐分区整行表头 + 最新作品网格 + 右下角两个浮钮。
- 布局骨架：`Box(fillMaxSize)`（`:195`，为浮钮定位）→ `Column(fillMaxSize)`（`:196`）：状态 `Text`（`:197`）→ 按钮 `Row`（`:202`：刷新 `:207`、加载更多 `:210`、计数 `Text` `:213`）→ `LazyVerticalGrid(GridCells.Adaptive(168.dp))`（`:216`）：`item(span = maxLineSpan)` 放 `PromoteHeader`（`:224`）→ `items(items, key = { it.id })` 放 `ComicCard`（`:228`）；`Box` 内右下角 `DailyQuickFab`（`:235`）与 `RandomFab`（`:241`）。
  `PromoteHeader`（`HomeSections.kt:39`）：`Column`（`:68`）→ 每个分区：`Row` 标题 + "更多" `TextButton`（`:70-82`）→ `Row(horizontalScroll)`（`:83`）内一串 `Box(width 140dp)` 包 `ComicCover`（`:89`）。
- 关键组件：`Button`(刷新/加载更多/换一批)、`LazyVerticalGrid(columns = Adaptive(168.dp), contentPadding, horizontal/verticalArrangement)`、`ComicCover(repository/item/modifier/onOpen)`(`ComicCover.kt:37`)、`PromoteHeader`、`DailyQuickFab`(`:599`，未登录时 `return` 不渲染 `:621`)、`RandomFab`(`:540`，`combinedClickable(onClick, onLongClick)`)、`StatusLine`/`LoadingHint`(`LoadingHint.kt:28`/`:53`)、`BlockedNotice`(`ComicCover.kt:88`)。
- 状态/输入：参数 `onOpen: (ListItem) -> Unit`、`onOpenSection: (String, String) -> Unit`、`onOpenRandom: () -> Unit`（`:131-135`）；本地 `items/status/page/total/busy/updatedIds`（`:136-141`）；推荐分区状态在 `PromoteHeader` 内 `sections`（`HomeSections.kt:40`）。
- 交互：点封面 -> `openComic(item)` -> `Screen.Detail(id, name)`（`:320-323`、`:228`）；点分区"更多" -> `Page("more/<id>?title=<URLEncoder>")`（`:432-435`）；点"加载更多" -> `repository.latest(page+1)`（`:210`）；`RandomFab` 单击 -> `repository.randomRecommend()` 抽一本进详情（`:554-570`），长按 -> `Page("random")`（`:437`、`:571`）；`DailyQuickFab` 点击 -> `repository.daily(uid)` / `dailyCheck(uid, dailyId)`（`:636-664`）。
- 需要登录：页面否。`updatedIds`（追更里有更新的作品）只在已登录时请求（`:177-187`）；`DailyQuickFab` 未登录不渲染（`:621`）。

### 5.3 作品详情
见 §6.2 详解。入口 `DetailScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/DetailScreen.kt:65`。

### 5.4 阅读器
见 §6.1 详解。入口 `ReaderScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/ReaderScreen.kt:69`。

### 5.5 登录 / 注册 / 找回密码
- 入口 Composable：`LoginScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/LoginScreen.kt:40`
- 一句话作用：三种模式的表单页（用一个字符串 `mode` 区分）。
- 布局骨架：`Column(fillMaxSize, padding 24.dp)`（`:51`）→ 标题 `Text`（`:55`，随 mode 变）→ 说明 `Text`（`:63`）→ 模式切换 `Row` + `TextButton` 三个（`:69-73`）→ 非 forgot 时用户名/密码 `OutlinedTextField`（`:76`、`:83`）→ register 时确认密码 / 邮箱 / 性别按钮（`:94`、`:102`、`:109`）→ forgot 时注册邮箱（`:118`）→ 主按钮 `Row`（`:127`）：按 mode 分派注册 `:129` / 发重置邮件 `:169` / 登录 `:194`，已登录时右侧有"退出登录"`TextButton`（`:218`）→ `message` 文案（`:229`）。
- 关键组件：`OutlinedTextField`(label/singleLine/visualTransformation)、`PasswordVisualTransformation`、`Button`、`TextButton`。
- 状态/输入：参数 `repository/onDone`（`:40`）；本地 `mode`/`username`/`password`/`passwordConfirm`/`email`/`gender`/`busy`/`message`（`:41-48`）。
- 交互：切模式 -> 改 `mode`（`:70-72`）；登录成功 -> `onDone()`（`:205`，回到 `Screen.Home`，见 `Main.kt:468`）；注册成功 -> 切回 login 并清空口令（`:152-156`）；退出登录 -> `repository.logout()`（`:219-225`）。
- 需要登录：本身就是登录页。

### 5.6 随机推荐
- 入口 Composable：`RandomScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/RandomScreen.kt:66`
- 一句话作用：抽一批随机作品，网格/列表版式可切，"按收藏偏好排序"可选（会额外请求标签）。
- 布局骨架：`Column(fillMaxSize)`（`:129`）→ 头部 `Row`（`:130`：标题 `:135`、`StatusLine` `:136`、"换一批" `Button` `:137`、版式 `TextButton` `:139`、偏好排序 `TextButton` `:146`）→ 说明 `Text`（`:151`）→ 列表版 `LazyColumn(weight(1f))`（`:161`，每项 `Row`：84dp x 3:4 封面 + 标题/作者/分类 `:166-194`）/ 网格版 `LazyVerticalGrid(weight(1f))`（`:197`，`ComicCover` `:204`）。
- 关键组件：`Button`、`TextButton`、`LazyColumn`、`LazyVerticalGrid(columns = Adaptive(168.dp))`、`ComicCover`、`BlockedByTagBanner`（`:160`）、`Image`+`rememberRemoteImage`（列表版封面 `:172-181`）。
- 状态/输入：参数 `repository/onOpenComic`（`:66`）；本地 `layout`(`:71`，持久化键 `random_layout` `:215`)、`items`(`:73`)、`busy`(`:78`)、`status`(`:79`)、`ranked`(`:80`)；依赖 `FavoriteTags`(`:68`)、`PreferencesKeyValueStore("jm_prefs")`(`:70`)、`TagBlocker`(`:75-77`)。
- 交互：进页 / "换一批" -> `repository.randomRecommend()`（`:82-127`、`:137`）；点版式按钮 -> 写 prefs 并切换（`:139-145`）；点"按收藏偏好排序" -> 切换后重抽，必要时先 `tagStore.refresh(repository)` 再按批（每批 3 条）取标签（`:93-111`）；点条目 -> `onOpenComic(item)` -> 详情。
- 需要登录：否；偏好排序在未登录时因收藏标签为空而退化为原顺序（`:98`）。

### 5.7 周刊
- 入口 Composable：`WeekScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/DiscoverPages.kt:40`
- 一句话作用：按刊期与类型浏览周刊。
- 布局骨架：`Column(fillMaxSize)`（`:91`）→ 头部 `Column`（`:92`：标题 `:93`、`StatusLine` `:94`、刊期 `Row(horizontalScroll)` 一串 `Button`（`:96-108`）、类型 `Row`（`:112-124`））→ `BlockedNotice(hidden)`（`:127`）→ `LazyVerticalGrid(Adaptive(168.dp))`（`:128`）：`ComicCover` 项（`:135`）+ 有内容时"加载更多" `Button`（`:138`）。
- 关键组件：`Button`(enabled = !busy && id != 当前)、`LazyVerticalGrid`、`ComicCover`、`BlockedNotice`、`StatusLine`。
- 状态/输入：参数 `repository/onOpenComic`（`:40`）；本地 `issues/types/issueId/typeId/items/hidden/page/busy/status`（`:41-49`）。
- 交互：`LaunchedEffect(Unit)` 先 `repository.weekIssues()` 再 `weekList(issueId, type, 1)`（`:72-89`）；点刊期 -> 换 `issueId` 重拉第 1 页（`:104-106`）；点类型 -> 同上（`:119-122`）；"加载更多" -> `weekList(..., page+1)`（`:140`）；点封面 -> `onOpenComic(item)` -> 详情。
- 需要登录：否。

### 5.8 关于
- 入口 Composable：`AboutScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/AboutScreen.kt:43`（内部 `Section` `:146`、`InfoRow` `:162`）
- 一句话作用：版本/平台信息、检查更新、项目说明、链接与许可。
- 布局骨架：`Column(fillMaxSize, verticalScroll, padding 24.dp)`（`:50`）→ 标题 `Text`（`:51`）→ `Section("版本")`（`:53`：版本号/平台/运行时/界面 `:54-57`）→ `Section("检查更新")`（`:60`：说明 + `Button("检查更新")` `:70`，有新版时列出资产下载按钮 `:99` 与发布页 `:108`）→ `Section("说明")`（`:117`）→ `Section("链接")`（`:126`：项目主页/发布页/问题反馈 `:127-131`）→ `Section("许可")`（`:134`）。
- 关键组件：`Column(verticalScroll)`、`Section(title, content)`、`InfoRow(label, value)`、`Button`、`TextButton`（用 `LocalUriHandler` 打开链接 `:45`）。
- 状态/输入：参数 `repository`（`:43`）；本地 `checking`/`result`/`updatable`（`:46-48`）；版本常量 `DESKTOP_VERSION`（`Version.kt:1`）。
- 交互：点"检查更新" -> `fetchLatestTag(repository.okHttp)` + `UpdateCheck.isNewer`（`:77-83`）；点资产按钮 -> `uri.openUri(UpdateCheck.assetUrl(...))`（`:99`）；点链接 -> `uri.openUri(...)`（`:127-131`）。
- 需要登录：否。

### 5.9 分类
- 入口 Composable：`CategoryScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/CategoryScreen.kt:51`（内部 `TagChip` `:354`、`NavChip` `:368`）
- 一句话作用：分类树（父/子）+ 排序 + 结果网格；分类拿不到时用热门标签兜底，并支持就地按标签搜索。
- 布局骨架：`Column(fillMaxSize)`（约 `:230` 起）→ 头部：标题/`StatusLine`/`NavChip` 行（分类与子类）/排序 `TextButton` 行（`:258-268`：最新 / 最多爱心 / 总排行 / 月排行 / 周排行）/"返回"与"加载更多"按钮（`:271-287`）→ `BlockedNotice(hidden)`（`:289`）→ 分支：分类树为空且非标签模式 -> 可滚 `Column`：错误文案 + `Button("重试")`（`:300`）或"热门标签"`FlowRow` + `TagChip`（`:303-309`）；否则 `BlockedByTagBanner`（`:314`）+ `LazyVerticalGrid(Adaptive(168.dp))`（`:315`）：`ComicCover` 项 + 整行的分组标签 `item(span = maxLineSpan)`（`:326`）。
- 关键组件：`NavChip(label, active, onClick)`(`:368`)、`TextButton` 排序、`Button`（刷新/加载更多/重试）、`FlowRow`、`TagChip(tag, onClick)`(`:354`)、`LazyVerticalGrid`、`ComicCover`、`BlockedNotice`、`BlockedByTagBanner`。
- 状态/输入：参数 `repository/onOpenComic/onSearch`（`:51-55`）；本地 `nodes/current/expanded/currentName/sort/items/hidden/page/busy/status/blocks/hotTags/treeError/tagMode/tagPage`（`:58-77`）；`hiddenIds`(`:80`)。
- 交互：`loadCategories()` 拉 `repository.categories()`，失败或空则 `loadHotTags()`（`:129-160`）；点分类/子类 -> `filter(slug, name, 1)`（`:83`）；点排序 -> `filter(...)`（`:265`）；点标签 -> `openTag`：有 `onSearch` 就走路由，否则 `searchTag(tag, 1)`（`:163`）；"加载更多" -> 按 `tagMode` 走 `searchTag` 或 `filter`（`:282-284`）；点封面 -> `onOpenComic` -> 详情。
- 需要登录：否。

### 5.10 我的
- 入口 Composable：`ProfileScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/ProfileScreen.kt:44`（内部 `Card` `:147`、`InfoRow` `:163`）
- 一句话作用：账号信息 + 每日签到 + 一段说明（桌面端没有主题/壁纸等设置项，那些搬到了"外观"页，见 `:136-142` 的说明文字）。
- 布局骨架：`Column(fillMaxSize, verticalScroll, padding 24.dp)`（`:75`）→ 标题 `Text("我的")`（`:76`）→ `Card("账号")`（`:78`：未登录时 `Text("未登录")` + `Button("去登录")` `:83`；已登录列 `InfoRow` 用户名/UID/等级/金币/免广告特权 `:87-91`）→ `Card("每日签到")`（`:96`：`StatusLine` `:97`、`Button("签到")` `:102`、活动名 `Text` `:125`、结果文案 `:131`）→ `Card("说明")`（`:136`）。
- 关键组件：`Card(title, content)`(`:147`)、`InfoRow(label, value)`(`:163`)、`Button`、`StatusLine`。
- 状态/输入：参数 `repository/onGoLogin`（`:44`）；`authState = repository.auth.state.collectAsState()`（`:46`）；本地 `dailyId/eventName/checkMsg/status/busy`（`:48-52`）。
- 交互：点"去登录" -> `onGoLogin()` -> `Screen.Login`（`Main.kt:475`、`ProfileScreen.kt:83`）；点"签到" -> `repository.dailyCheck(uid, dailyId)`（`:110`），按钮在未登录或没有进行中活动时禁用（`:103`）；进页只读 `repository.daily(uid)`（`:61`）。
- 需要登录：页面否；账号卡与签到需要（`:103`、`:57-60`）。

### 5.11 通知
- 入口 Composable：`NotificationScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/NotificationScreen.kt:52`
- 一句话作用：通知列表（三页签）+ 未读数 + 标记已读。
- 布局骨架：`Column(fillMaxSize)`（`:123`）→ 头部 `Row`（`:124`：标题 `:129`、三个页签 `TextButton`（`:130-137`）、未读 `Text`（`:138`，>0 才显示）、`StatusLine`（`:141`）、"加载更多" `Button`（`:143`））→ `LazyColumn(weight(1f))`（`:147`）：每项 `Row`（`:152`）：未读圆点 `Column(size 8.dp, clip(CircleShape))`（`:162-170`）+ 文本列（`:171`）。
- 关键组件：`TextButton`(页签)、`LazyColumn`、`Row`+`CircleShape` 未读点、`Button`、`StatusLine`。
- 状态/输入：参数 `repository`（`:52`）；本地 `tabs`(`:56-60`，取值 `all` / `comic_follow` / `site_notice`)、`tab/items/unread/page/busy/status`（`:62-67`）。
- 交互：切页签 -> `load(1)` + 读未读数（`:109-121`）；点条目里的标记已读 -> `repository.markNotificationRead(id, true)` 后重拉第一页与未读数（`:93-107`）；"加载更多" -> `load(page+1)`（`:143`）。
- 需要登录：页内无门；属账号数据，未登录会显示加载失败文案（`:82`）。

### 5.12 画师与作品库
- 入口 Composable：`CreatorScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/CreatorScreen.kt:61`
- 一句话作用：画师 / 作品两个模式的浏览与搜索；点画师显示其作品，点作品就地展开作品信息与内容（桌面端没有单独的作品信息页）。
- 布局骨架：`Column(fillMaxSize)`（`:150`）→ 头部 `Row`（`:151`：模式 `Button("画师")` `:156` / `Button("作品")` `:159`、搜索 `OutlinedTextField` `:162`、`Button("搜索")` `:169`、`StatusLine` `:170`）→ 选中画师时的 `Row`（`:174-187`：画师名 + "返回列表"）→ 选中作品时的 `Column`（`:188-196`：作品名 + "关闭"）→ 有 `workInfo` 时 `LazyColumn(weight(1f))`（`:202`，内容与图片）→ 否则 `LazyVerticalGrid`（`:306`）列结果 + "加载更多" `Button`（`:381`）。
- 关键组件：`Button`、`OutlinedTextField`、`TextButton`、`LazyColumn`、`LazyVerticalGrid`、`Image`/`rememberRemoteImage`（作品图片）、`BlockedByTagBanner`、`StatusLine`。
- 状态/输入：参数 `repository`（`:61`）；本地 `mode/query/authors/works/total/page/busy/status/selectedAuthor/workInfo/workContent/contentLoading`（`:62-82`）；`hiddenIds`(`:67`)。
- 交互：切模式 -> `load(1)`（`:148`）；点"搜索" -> `load(1)`（`:169`）；点画师/作品条目 -> `load` 或 `openWork(id, title)`（`:124-146`，同时取 `creatorWorkInfo` 与 `creatorWorkContent`）；"加载更多" -> `load(page+1)`（`:381`）。
- 需要登录：否。

### 5.13 评论
- 入口 Composable：`CommentsScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/CommentsScreen.kt:54`（内部 `CommentRow` `:199`）
- 一句话作用：一部作品的评论列表 + 发表/回复框 + 删除自己的评论。
- 布局骨架：`Column(fillMaxSize)`（`:90`）→ 头部 `Row`（`:91`：返回 `TextButton` `:96`、标题 `:97`、`StatusLine` `:98`）→ 发表区 `Column`（`:102`）：回复目标提示行 + "取消回复" `TextButton`（`:103-112`）→ 输入 `Row`（`:113`：`OutlinedTextField` `:117`、`Button`（发表/回复）`:124`，右侧未登录时显示"需要登录" `:149`）→ `notice` 文案（`:153`）→ `LazyColumn`（`:158`）：`CommentRow`（`:163`，参数 `repository/comment/myUid/depth/onReply/onDelete`）+ 未满时 `Button("加载更多")`（`:190`）。
- 关键组件：`TextButton`、`OutlinedTextField(singleLine, width 520.dp)`、`Button`、`LazyColumn`、`CommentRow`(`:199`，含嵌套回复，见 `:197` 注释：服务端只下发展开的一层)。
- 状态/输入：参数 `repository/aid/onBack`（`:54`）；`authState`(`:56`)、`myUid`(`:57`)；本地 `items/total/page/busy/status/input/replyTo/notice`（`:59-66`）。
- 交互：点"返回" -> `onBack()`，回到 `Screen.Detail(aid, "")`（`Main.kt:482`）；点某条评论的"回复" -> `replyTo = it`（`:168`）；发表/回复 -> `repository.sendComment(aid, text, commentId = target?.commentId)`（`:132`）；删除 -> `repository.deleteComment(commentId, aid)`（`:172`）；"加载更多" -> `load(page+1)`（`:190`）。
- 需要登录：浏览否；发表/回复与删除需要（按钮 `enabled = ... && authState.loggedIn`，`:125`）。

### 5.14 屏蔽设置
- 入口 Composable：`BlockScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/BlockScreen.kt:40`（内部 `BlockSection` `:87`）
- 一句话作用：本地屏蔽名单（关键词 / 标签 / 分类名）的增删。
- 布局骨架：先判空：`repository.blockStore == null` 时只渲染一段"本地屏蔽存储未初始化。"（`:42-48`）并 `return`；否则 `Column(fillMaxSize, verticalScroll, padding 24.dp)`（`:52`）→ 标题 + 说明（`:53-60`）→ `BlockSection("关键词")`（`:62`）→ `BlockSection("标签")`（`:69`）→ `BlockSection("分类名")`（`:76`）。
- 关键组件：`Column(verticalScroll)`、`BlockSection(title/hint/values/onAdd/onRemove)`(`:87` -> `OutlinedTextField` `:109` + `Button("添加")` `:116` + 值列表行与删除按钮 `:139-146` 起)、`Text`。
- 状态/输入：参数 `repository/modifier`（`:40`）；`rules by store.state.collectAsState()`（`:50`）；每个 `BlockSection` 自己的 `input`（`:94`）。
- 交互：输入 + "添加" -> `store.addWord/addTag/addCategory`（`:66`、`:73`、`:80`）；点条目删除 -> `store.removeWord/removeTag/removeCategory`（`:67`、`:74`、`:81`）。
- 需要登录：否。

### 5.15 外观
- 入口 Composable：`AppearanceScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/AppearanceScreen.kt:44`（内部 `pickImageFile` `:248`，壁纸在线部分在 `WallpaperRemoteSection.kt:35`）
- 一句话作用：桌面端全部观感设置：界面风格、三根滑杆、壁纸（预设/在线/本地图片）、深浅色、连载更新提醒、阅读默认形态。
- 布局骨架：`Column(fillMaxSize, verticalScroll, padding 24.dp, spacedBy 18.dp)`（`:47`）→ 标题（`:51`）→ "界面风格" `Column`（`:54`：`GlassStyle.entries` 一串 `Button` `:64`、当前值文案 `:75`）→ 可调项 `Column`（`:84`：三根 `Slider` —— 壁纸模糊 `:86`、面板浓度 `:94`、壁纸压暗 `:102`，`valueRange` 分别 0..256 / 0..1 / 0..100）→ "壁纸" `Column`（`:111`：`WallpaperPreset.entries` 预设卡 `:114-142`、`WallpaperRemoteSection(onChanged)` `:147`、`Button("选择本地图片...")` `:152` + 文件名与"清除" `:161-168`、以及一行"本地图片壁纸尚未验证"的诚实说明 `:170-175`）→ "深浅色" `Column`（`:179`：浅色/深色 `TextButton` `:182-183`）→ "连载更新提醒" `Column`（`:198`：开启/关闭 `Button` `:207`）→ "阅读形态" `Column`（`:217`：`ReaderMode.entries` `Button` `:227` + 当前值 `:238`）。
- 关键组件：`Button`、`TextButton`、`Slider`(value/onValueChange/valueRange/width 420.dp)、`OutlinedTextField`（在 `WallpaperRemoteSection` 里 `:109`）、`Brush.linearGradient` 预设缩略图（`:134`）、`WallpaperRemoteSection`（模式 `TextButton` `:45`、换一张 `Button` `:66`、自动轮换 `TextButton` `:95`、直链 `Button("应用")` `:116`）。
- 状态/输入：参数：无（`:44`）；本地 `refresh`（`:45`，用于强制重组）；全局设置对象 `Appearance`（`style/wallpaperBlur/alphaScale/dim/preset/wallpaperPath/dark`，见 `:63-186`）、`RemoteWallpaper`（`WallpaperRemoteSection.kt:37-120`）、`SerialReminder.enabled`（`:208`）、`ReaderModePref.mode`（`:228`）。
- 交互：切风格 / 拖滑杆 / 选预设 / 选在线壁纸 / 选本地图片 / 切深浅色 / 开关提醒 / 切阅读形态 -> 全部写进对应的持久化对象并 `refresh++`；`pickImageFile()` 用 AWT `FileDialog` 取路径（`:248-256`）。
- 需要登录：否。

### 5.16 标签
- 入口 Composable：`TagsScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/TagsScreen.kt:50`
- 一句话作用：本地"收藏标签统计"的扫描与查看，以及网站标星标签的增删。
- 布局骨架：`Column(fillMaxSize, verticalScroll, padding 24.dp)`（`:97`）→ 标题（`:98`）→ 第一行 `Row`（`:100`：`Button("扫描收藏更新统计")` `:104`（`enabled = !busy && repository.auth.isLoggedIn`）、未登录提示"需要登录（收藏列表属于账号数据）" `:127`）→ 说明 `Text`（`:137` 起）→ 标星区（`OutlinedTextField` `:183` + `Button` `:190`）。
- 关键组件：`Button`(enabled 受登录态约束)、`Text`、`OutlinedTextField`、`Column(verticalScroll)`。
- 状态/输入：参数 `repository/onSearch`（`:50`）；本地 `counts/cachedAt/starred/busy/status/draft`（`:52-58`）；`FavoriteTags` 存储（`:51`）。
- 交互：`LaunchedEffect(Unit) { reloadStarred() }`（`:95`）-> `repository.favoriteTags()`（`:64`）；点"扫描" -> `store.refresh(repository)`（`:110`，逐部作品请求）；增删标星 -> `repository.updateFavoriteTags("add"/"remove", listOf(tag))` 后重读（`:77-93`、`:190`）；点标签搜索 -> `onSearch(q)`（`Main.kt:486-489`，当前只记日志并跳到 `Page("search")`）。
- 需要登录：部分。扫描按钮要求已登录（`:105`），未登录显示提示（`:127`）；标星接口也属账号数据。

### 5.17 搜索
- 入口 Composable：`SearchScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/SearchScreen.kt:94`
- 一句话作用：关键词搜索 + 排序/字段/年月筛选 + 未搜索时的热词与随机推荐。
- 布局骨架：`Column(fillMaxSize)`（`:188`）→ 第一行 `Row`（`:189`：`OutlinedTextField` `:194`（回车搜、Esc 清空）、`Button("搜索")` `:213`、`StatusLine` `:216`）→ 排序 `Row`（`:219`：`SEARCH_ORDERS` 的 `TextButton` `:226`）→ 字段与年月 `Row`（含 `OutlinedTextField` `:251`、`:258`）→ 未搜索时的建议区（`:280-310` 起：热门标签 `TextButton` `:292`、随机推荐 `TextButton` `:304`、"换一批" `:308`）→ `BlockedNotice(hidden)`（`:312`）→ 标签屏蔽提示行 + "允许一次" `TextButton`（`:317-331`）→ `LazyVerticalGrid(Adaptive(168.dp))`（`:333`）：`ComicCover` 项（`:340`）+ "加载更多" `Button`（`:345`）。
- 关键组件：`OutlinedTextField`(label/singleLine/width 360.dp/**`onKeyEvent`**：`Key.Enter` 搜索、`Key.Escape` 清空并收起，`:199-211`)、`Button`、`TextButton`、`LazyVerticalGrid`、`ComicCover`、`BlockedNotice`、`StatusLine`。
- 状态/输入：参数 `repository/onOpenComic`（`:94`）；本地 `query/items/hidden/total/page/busy/status/order/type/year/month`（`:100-110`）、`hotTags/recommend/suggestBusy`（`:117-119`）、`hiddenIds`(`:114`)；`SearchHistory`（`:98`）。
- 交互：回车或点"搜索" -> `runSearch(1)`（`:134-184`）：命中作品编号（`result.redirectAid`）时直接 `onOpenComic(ListItem(id = redirect, name = query))` 进详情（`:152-158`）；翻页 -> `runSearch(page+1)`（`:345`）；点热词 -> `query = tag; type = "tag"; runSearch(1)`（`:292`）；点推荐 -> `onOpenComic(item)`（`:304`）；"换一批" -> `reloadSuggest()`（`:308`）；"允许一次" -> `TagBlocker.allowOnce(...)`（`:329`）。
- 需要登录：否。

### 5.18 收藏（含追更标签）
- 入口 Composable：`FavoriteScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/FavoritesScreen.kt:52`
- 一句话作用：收藏作品网格 + 收藏夹切换与管理；同页的"追更"标签内嵌追更列表。
- 布局骨架：`Column(fillMaxSize)`（`:96`）→ 头部 `Row`（`:97`：两个 `TextButton` 页签"收藏"/"追更" `:103`、`:106`，收藏标签下还有 `StatusLine` `:110`、"加载更多" `:112`、`notice` `:114`）→ 若 `tab == "tracking"` -> `TrackingList(...)`（`:122`）然后 `return@Column` -> 收藏夹 `Row`（`:131`：`Button("全部")` `:136` + 每个夹一个 `Button` `:141`）→ 收藏夹管理 `Row`（`:157`：`OutlinedTextField("收藏夹名称")` `:163` + `Button("新建")` `:170` + 选中时 `Button("改名")` `:194` / `Button("删除")` `:217`）→ `LazyVerticalGrid(Adaptive(168.dp), weight(1f))`（`:243`）：每项 `Column` = `ComicCover` + `TextButton("取消收藏")`（`:253`）。
- 关键组件：`TextButton`(页签)、`Button`、`OutlinedTextField`、`LazyVerticalGrid`、`ComicCover`、`TrackingList`(`TrackingScreen.kt:73`)、`StatusLine`。
- 状态/输入：参数 `repository/onOpenComic/initialTab = "favorite"`（`:52-56`，Main 传 `"tracking"` 时进入追更标签，`Main.kt:495`）；本地 `tab/folders/selected/items/total/page/busy/status/notice/folderName`（`:60-70`）；`loggedIn`（`:57`）。
- 交互：`LaunchedEffect(loggedIn) { if (loggedIn) load(1) }`（`:94`）；切收藏夹 -> `selected = f.folderId; load(1)`（`:143`）；"新建/改名/删除" -> `repository.editFavoriteFolder(type = "add"/"edit"/"del", ...)`（`:177`、`:202`、`:224`）；"取消收藏" -> `repository.toggleFavorite(item.id)`（`:258`）；点封面 -> `onOpenComic(item)` -> 详情。
- 需要登录：**是**。未登录时状态文案为"需要登录后才能查看收藏"（`:67`）且不发请求（`:94`）；收藏夹管理区仅在 `loggedIn` 时渲染（`:157`）。

### 5.19 观看历史
- 入口 Composable：`HistoryScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/HistoryScreen.kt:51`
- 一句话作用：观看历史网格，可单条删除。
- 布局骨架：`Column(fillMaxSize)`（`:84`）→ 头部 `Row`（`:85`：标题 `:90`、`StatusLine` `:91`、"加载更多" `:93`、`notice` `:95`）→ `LazyVerticalGrid(Adaptive(168.dp), weight(1f))`（`:100`）：每项 `Column` = `ComicCover`（`:109`）+ `TextButton("删除")`（`:110`）。
- 关键组件：`LazyVerticalGrid`、`ComicCover`、`TextButton`、`StatusLine`。
- 状态/输入：参数 `repository/onOpenComic`（`:51`）；本地 `items/total/page/busy/status/notice`（`:55-60`）；`loggedIn`（`:52`）。
- 交互：`LaunchedEffect(loggedIn) { if (loggedIn) load(1) }`（`:82`）-> `repository.history(page)`；"加载更多" -> `load(page+1)`（`:93`）；"删除" -> `repository.deleteHistory(item.id)`（`:115`，失败时把原因写进 `notice` `:124`）；点封面 -> 详情（`:109`）。
- 需要登录：**是**。未登录时文案"需要登录后才能查看历史"（`:59`）、不发请求（`:82`）。

### 5.20 追更（侧栏入口）
- 入口 Composable：侧栏"追更"落到 `FavoriteScreen(initialTab = "tracking")` — `Main.kt:495`；本文件里实现追更列表的是 `TrackingList` — `desktop/src/main/kotlin/com/jmnext/desktop/TrackingScreen.kt:73`
- 一句话作用：追更的作品网格（与收藏页的"追更"标签是同一份实现）。
- 布局骨架（`TrackingList`）：`Column(modifier)`（`:117`）→ 未满时 `Row` 内 `Button("加载更多")`（`:118-125`）→ `LazyVerticalGrid(Adaptive(168.dp), weight(1f))`（`:126`）：`ComicCover` 项（`:133`）。
- 关键组件：`Button`、`LazyVerticalGrid`、`ComicCover`。
- 状态/输入：`repository/onOpenComic/modifier/onStatus`（`:73-77`）；本地 `items/total/page/busy/status`（`:82-86`）；`loggedIn`（`:79`）。
- 交互：`LaunchedEffect(loggedIn) { if (loggedIn) load(1) }`（`:115`）-> `repository.trackingList(page)`；"加载更多" -> `load(page+1)`（`:123`）；点封面 -> 详情（`:133`）。
- 需要登录：**是**（未登录文案"需要登录后才能查看追更" `:86`）。
- 另有一个**没有任何调用方**的独立页面 `TrackingScreen` — `TrackingScreen.kt:43`，见 §9。

### 5.21 分区更多
- 入口 Composable：`MoreListScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/MoreListScreen.kt:48`
- 一句话作用：某个推荐分区的完整列表（`promoteList` 分页）。
- 布局骨架：`Column(fillMaxSize)`（`:99`）→ 头部 `Column`（`:100`：标题 `:101`、`Row` 内 `Button("刷新")` `:110` + `Button("加载更多")` `:115` + `StatusLine` `:119`）→ `BlockedNotice(hidden)`（`:123`）→ `BlockedByTagBanner`（`:125`）→ `LazyVerticalGrid(Adaptive(168.dp))`（`:126`）：`ComicCover` 项（`:133`）。
- 关键组件：`Button`、`StatusLine`、`BlockedNotice`、`BlockedByTagBanner`、`LazyVerticalGrid`、`ComicCover`。
- 状态/输入：参数 `repository/sectionId/title/onOpenComic`（`:48-53`）；本地 `items/hidden/total/page/busy/exhausted/status`（`:54-66`）；`hiddenIds`(`:56`)。
- 交互：`LaunchedEffect(sectionId) { load(0) }`（`:97`，注意 `promoteList` 的 page **从 0 起算**，见 `:61` 注释）；"刷新" -> `load(0)`（`:110`）；"加载更多" -> `load(page+1)`（`:117`）；点封面 -> `onOpenComic(item)` -> `Screen.Detail`。
- 需要登录：否。

### 5.22 连载更新（每周更新表）
- 入口 Composable：`WeeklyUpdateScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/MoreListScreen.kt:186`
- 一句话作用：分区 26 的专用形态：按类型 + 星期切的连载更新表。
- 布局骨架：`Column(fillMaxSize)`（`:224`）→ 头部 `Column`（`:225`：标题 `:226`、类型 `Row`（`:228`，`WEEKLY_TYPES` 的 `Button` `:234`）、星期 `Row`（`:241`，`WEEKLY_DAYS` 的 `Button` `:247`）、`Row` 内 `Button("刷新")` `:259` + `Button("加载更多")` `:262` + `StatusLine` `:266`）→ `BlockedByTagBanner`（`:270`）→ `LazyVerticalGrid(Adaptive(168.dp))`（`:271`）：`ComicCover` 项（`:278`）。
- 关键组件：`Button`（类型/星期/刷新/加载更多）、`StatusLine`、`BlockedByTagBanner`、`LazyVerticalGrid`、`ComicCover`。
- 状态/输入：参数 `repository/onOpenComic`（`:186`）；本地 `items/page/type/day/busy/exhausted/status`（`:187-198`）；档位常量 `WEEKLY_DAYS`（`:141`，1..7 = 周一..周日，0 = 完结）、`WEEKLY_TYPES`（`:153`，全部/漫画/韩漫）；`todayWeekDay()`（`:165`）把 `Calendar.DAY_OF_WEEK` 换算成服务端的"周一 = 1"。
- 交互：`LaunchedEffect(Unit) { load(1) }`（`:222`，`serialization` 的 page **从 1 起算**，见 `:192` 注释）-> `repository.weeklyUpdate(type, date, page)`；点类型 -> `load(1, key, day)`（`:235`）；点星期 -> `load(1, type, key)`（`:248`）；"刷新" -> `load(1)`（`:259`）；"加载更多" -> `load(page+1)`（`:264`）；点封面 -> 详情（`:278`）。
- 需要登录：否。

### 5.23 未实现页占位壳
- 入口 Composable：`PageShell` — `desktop/src/main/kotlin/com/jmnext/desktop/PageShell.kt:25`
- 一句话作用：当 `Screen.Page(route)` 的 route 不匹配任何已知页面时的兜底外壳，明说"这一页还没接数据"。
- 布局骨架：`Column(fillMaxSize, padding 24.dp)`（`:26`）→ 标题 `Text`（`:30`）→ `Column(圆角背景 surfaceVariant)`（`:31-38`）：`Text("这一页还没接数据")`（`:39`）+ 该页的计划文案（`:40`，取自 `PAGE_PLANS[s.route]`）+ 一句说明（`:41-45`）。
- 关键组件：`Column`、`Text`（无交互控件）。
- 状态/输入：参数 `title: String`、`planned: String`（`:25`）；调用处从 `NAV_ITEMS` 反查标题、从 `PAGE_PLANS`（`:51`，13 条）取计划文案（`Main.kt:515-516`）。
- 交互：无。
- 需要登录：否。

### 5.24 应用外壳里挂着的其它 UI（不算独立屏幕）
- 退出登录确认 `AlertDialog` — `Main.kt:398`（title"退出登录"，text 说明，确认 -> `repository.logout()`）。
- 连载更新降级提示行 + "知道了" — `Main.kt:352-365`。
- 瞬时反馈层 `NoticeHost()` — `Main.kt:396`，实现 `desktop/src/main/kotlin/com/jmnext/desktop/Notices.kt:61`。
- 壁纸层 `WallpaperLayer` — `desktop/src/main/kotlin/com/jmnext/desktop/Wallpaper.kt:149`；玻璃面板修饰符 `glassPanel` — `Wallpaper.kt:247`；主题包装 `BlogTheme` — `desktop/src/main/kotlin/com/jmnext/desktop/BlogTheme.kt:63`。

---

## 6. 重点屏幕详解

### 6.1 阅读器

#### 6.1.1 Android `ReaderScreen`
入口：`ReaderScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/reader/ReaderScreen.kt:284`

参数（`:284-293`）：`comicId`、`chapterId`、`mode: ReaderMode`、`onModeChange`、`onBack`、`onOpenComments`、`modifier`。线上状态：`ReaderViewModel`（`viewModel(key = "reader-$chapterId")`，`:299`）的 `state`（`loading/error/payload/series/currentChapterId/switching/favorited/liked`），以及本地 `barsVisible`（`:310`）、`pickerOpen`（`:311`）。

顶层结构（`Box(fillMaxSize)`，`:318`）：

1. **内容区**（`when`，`:320-486`）
   - `state.loading` -> `LoadingBox()`（`:321`）
   - `error != null && payload == null` -> `ErrorBox(onRetry = vm.load)`（`:323`）
   - `payload == null || images.isEmpty()` -> `ErrorBox("这一话没有可显示的图片")`（`:326`）
   - 正常 -> `key(state.currentChapterId)`（`:358`）内一个 `Box`，背景是 `.ambientBase()`（`:373`，与设置页同一段绘制，保证伪长图接缝不可见），并挂 `detectTapGestures(onTap = { barsVisible = !barsVisible })`（`:374`）；按 `mode` 分派 `ScrollReader`（`:380`）或 `PagedReader`（`:385`）。
   - 阅读页采样：`ReaderPageLink`（`:339`）+ `PageSampler`/`SelfTuner`（`:345-356`，只在离开页时结算停留）。
2. **错误飘条**：`AnimatedVisibility(visible = barsVisible)` 的 `GlassSurface(level = Flyout)`，位置 `align(TopCenter).padding(top = 64.dp)`（`:391-407`）。
3. **顶栏**：`AnimatedVisibility(visible = barsVisible, 顶入顶出)` 的 `GlassTopBar`（`:409-456`）—— title = 作品名或"第 N 话"，subtitle = "共 N 页 · index/series.size"，`navigation` 是返回 `IconButton`（`:423`），`actions` 是模式切换 `IconButton`（`:432`，Scroll 时显示 `SwapVert`、Page 时显示 `SwapHoriz`）。
4. **底栏**：`AnimatedVisibility(visible = barsVisible, 底入底出)` 的 `ReaderBottomBar`（`:459-484`），参数见下。
5. **章节选择对话框**：`pickerOpen` 时 `ChapterPickerDialog`（`:489-499`）。

私有子 Composable 一览（文件内）：

| 行号 | 名称 | 角色 |
|---|---|---|
| `:515` | `ReaderBottomBar` | 底栏容器，悬浮/贴底两套 |
| `:606` | `PageSeekRow` | 上行：上一话 · 当前页 · 滑块 · 总页数 · 下一话 |
| `:658` | `ReaderActions` | 下行：五个动作 |
| `:709` | `ReaderAction` | 单个动作（图标 + 可选文字） |
| `:735` | `ScrollReader` | 纵向连续滚动形态 |
| `:809` | `PagedReader` | 横向逐页形态 |
| `:884` | `ZoomableReaderImage` | 可缩放单页（仅翻页形态用） |
| `:950` | `ReaderImage` | 单页图片加载（含反切片） |
| `:1026` | `PageFallback` | 加载中/失败占位 |
| `:1086` | `PrefetchPages` | 预取前后若干页 |

底栏两种形态（`:562-601`）：
- 悬浮版（`floating = true`）：`Box` + `GlassSurface(level = Raised, shape = RoundedCornerShape(percent = 50), tinted = true)`，`Column(navigationBarsPadding)` 内 `seek()` + `actions(false)`——**不带文字标签**。
- 贴底版：`GlassSurface(level = Flyout, shape = RoundedCornerShape(0.dp), tinted = true)`，`Column(navigationBarsPadding)` 内 `seek()` + `actions(true)`——**带文字标签**。

`PageSeekRow`（`:606`）控件：`IconButton(ChevronLeft, "上一话", enabled = enabled && hasPrev)`（`:623`）、当前页 `Text`（`:630`）、`Slider(value = currentPage, onValueChange = onJump, valueRange = 0..max, Modifier.weight(1f))`（`:635`）、总页数 `Text`（`:641`）、`IconButton(ChevronRight, "下一话")`（`:646`）。

`ReaderActions`（`:658`）五个 `ReaderAction`：模式切换（横向/纵向，`:676`）、章节（`:681`）、评论（`:686`）、收藏（`:691`，图标随 `favorited` 变）、点赞（`:698`，图标随 `liked` 变）。`ReaderAction(label/icon/showLabel/tint/enabled/onClick)` 在 `:709`，`enabled = false` 时用 `c.textTertiary` 置灰（`:718`）。

两种形态：
- 滚动：`ScrollReader`（`:735`）用 `LazyColumn`（`:773`）+ `itemsIndexed(payload.images, key = "{i}-{image}")`（`:784`），每项 `ReaderImage(contentScale = FillWidth, placeholderRatio = 0.72f)`（`:785`）；**页间 spacing 为 0**（`:781` 注释：伪长图必须严丝合缝）；顶部按 `WindowInsets.statusBars` 留白（`:761`）；当前页通过 `snapshotFlow { listState.firstVisibleItemIndex }` 上报（`:748`），`jump` 通过 `animateScrollToItem`（`:753`）；`PrefetchPages` 在列表上方调用（`:767`）。
- 翻页：`PagedReader`（`:809`）用 `HorizontalPager(pageCount = payload.images.size)`（`:838`），每页 `ZoomableReaderImage`（`:843`）；`userScrollEnabled = !zoomed`（`:840`，缩放与翻页的手势仲裁）；顶部叠一个页码指示胶囊 `"${page+1} / ${size}"`（`:853-871`）。
- 缩放：`ZoomableReaderImage`（`:884`）—— `detectTransformGestures` 做双指缩放 1x-5x 与放大后单指平移（`:905-908`），`detectTapGestures` 做单击（切工具栏）与双击（在适应屏幕与 2.5 倍之间切换）（`:913-918`），`graphicsLayer` 应用 `scaleX/scaleY/translationX/translationY`（`:929-934`）。
- 预取：`PrefetchPages`（`:1086`），`scalePrefetch(base)`（`:1066`）按 lite 与自学习结果调整深度。

交互汇总：点内容区 -> 切工具栏显隐（`:375`，翻页形态由图片自己处理，见 `:381-385` 注释）；点返回 -> `onBack()`；点模式图标 -> `onModeChange(另一形态)`（`:434`，写回上层的 `AppPrefs.readerMode`）；拖动滑块 -> `pageLink.jump(page)`（`:469`）；点"上一话/下一话" -> `vm.openChapter(id)`（`:473-474`，栈内替换当前话，`:219-230` 有说明）；点"章节" -> `pickerOpen = true`（`:475`）-> `ChapterPickerDialog` 内选话 -> `vm.openChapter(chapterId)`（`:494-497`）；点"评论" -> `onOpenComments()` -> `comments/{comicId}`（`JmNavHost.kt:789`）；点"收藏/点赞" -> `vm.toggleFavorite()` / `vm.toggleLike()`（`:481-482`），二者都是**乐观更新 + 失败回滚**（`:191-217`）。

需要登录：**页面不校验登录态**。`toggleLike`（`:191`）与 `toggleFavorite`（`:208`）直接调 `repo.like` / `repo.toggleFavorite`，没有 `isLoggedIn` 判断；未登录时会请求失败并静默回滚（仅 `error` 字段被写成原值，见 `:202`、`:214`）。这一点与详情页/收藏页的显式登录门不同，如实记下。

#### 6.1.2 桌面端 `ReaderScreen`
入口：`ReaderScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/ReaderScreen.kt:69`

参数（`:69-79`）：`repository`、`progress: ReadProgressStore`、`comicId`、`chapterId`、`chapterIds`、`onBack`、`onSwitchChapter`、`onOpenComments`。

顶层结构（`Box(fillMaxSize)`，`:152`）：

| 区域 | 行号 | 内容 |
|---|---|---|
| 顶栏行 | `:155-179` | `TextButton("返回")`、加载中 `LoadingHint` 或状态 `Text`、"第 i / n 话" `Text`、`payload == null` 时的"重试" `TextButton` |
| 键盘层 | `:298-326` | `Row(focusRequester + focusable + onPreviewKeyEvent)`：`DirectionLeft/PageUp` 上一页、`DirectionRight/PageDown` 下一页，按当前模式分别驱动 `pagerState.scrollToPage` 或 `listState.scrollToItem` |
| 内容区 | `:328-363` | `mode == Page` -> `PagedReader`；否则 `LazyColumn`（`itemsIndexed` -> `PageItem`，末尾一个"下一话" `Button` `:352` 与"回详情页" `TextButton` `:357`） |
| 右侧页码栏 | `:374-439` | `PageRail`（对齐 `CenterEnd`，宽 40dp） |
| 底栏 | `:440-490` | `ReaderBottomBar(visible = true, ...)` |

私有/内部子 Composable：`PageItem` — `:495`（单页加载、反切片 `needsUnscramble`、失败可点重试 `:536-542`）；同文件还有 `ReaderModePref` 对象（`:562`，键 `jm_reader_mode/mode`）。

`PageRail`（`desktop/src/main/kotlin/com/jmnext/desktop/PageRail.kt:56`）自上而下四块（文件注释 `:47-51` 也这么写）：
1. 滑轨 `Box(fillMaxHeight(0.7f))`（`:83`）—— 底轨（`:95`）、已读部分（`:103`）、把手 `18dp` 圆点（`:112`）、手势层（点击 `:126` 与竖向拖动 `:133` 都按轨道实际像素换算页码）；**不是 material3 的 Slider 旋转**（理由见 `:40-45`）。
2. 页码 `Text("${current+1}/$total")`（`:143`）。
3. 五个 `RailAction`：模式 / 章节 / 评论 / 收藏 / 点赞（`:150-154`，`RailAction(label, action)` 在 `:168`，`action == null` 置灰）。
4. "上一话" `<`（`:157`）与"下一话" `>`（`:159` 起）上下相邻。

`ReaderBottomBar`（`desktop/src/main/kotlin/com/jmnext/desktop/ReaderBottomBar.kt:44`）：`AnimatedVisibility(visible)`（`:66`）-> `Surface(color = surface 0.78f, border 1dp, tonalElevation 6dp, shadowElevation 8dp, shape 顶部圆角 10dp)`（`:73-79`）-> `Column`：`LinearProgressIndicator(progress = frac, height 3dp)`（`:85`）+ 一行「上一话 / `<` / 页码 / `>` / 下一话」（`:93-103`）+ 右侧五个 `BarAction`（`:106-111`，`BarAction` 在 `:122`，传 null 置灰）。接线在 `:440-490`，其中 `visible = true` 常显（`:443`；`:369-372` 的注释已过时，实际已接线）。

与 Android 的差异（源码里明确写了的）：键盘翻页是桌面特有（`:191-196`、`:303`）；底栏是 `Column` 的最后一个子项而不是浮层（`:370-372` 注释）；页级进度用桌面自己的 `PageProgress`（`:82`）。

需要登录：**页面不校验**。收藏/点赞直接 `repository.toggleFavorite` / `repository.like`（`:419-438`、`:488-489`），源码注释标明"写操作，未验证"。

#### 6.1.3 章节选择对话框（两端）
- Android：`ChapterPickerDialog` — `app/src/main/kotlin/com/jmnext/ui/screens/reader/ChapterPickerDialog.kt:48`。`AlertDialog`（`:66`）：title"选择章节"；`text` 是可滚 `Column(heightIn(max = 380.dp), verticalScroll(scroll))`（`:73`），每页 10 章（`chunkSize = 10`，`:55`），当前话用 `c.accentSoft` 底色标出（`:83`）；`confirmButton` 是 `IconButton(ChevronLeft)` + "x / y" + `IconButton(ChevronRight)`（`:109-137`）；`dismissButton` 是"关闭"。打开时初值直接定位到当前话所在页（`:59-62`）。
- 桌面：`ChapterPickerDialog` — `desktop/src/main/kotlin/com/jmnext/desktop/ChapterPickerDialog.kt:46`。同样是 `AlertDialog`（`:61`），但每页 **50** 章（`:52`），翻页用文字按钮"上一页/下一页"（`:107`、`:113`）。

### 6.2 作品详情

#### 6.2.1 Android `DetailScreen`
入口：`DetailScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/detail/DetailScreen.kt:527`

参数（`:527-545`）：`comicId`、`onBack`、`onOpenComic`、`onReadChapter`、`onOpenTag`、`onNeedLogin`、`onOpenComments`、`modifier`、`initialCoverUrl = ""`、`initialTitle = ""`（后两者是共享元素首帧的目标矩形，`:536-545` 注释）。

顶层：`Column(fillMaxSize().jmVanishWhenLeaving())`（`:609`）-> `GlassTopBar`（`:610`，title = 详情名或 `initialTitle` 或"作品详情"；`navigation` 返回；`actions` 里**仅登录时**才有追更 `IconButton`（`:626-639`），收藏 `IconButton` 总是显示（`:640-654`））-> 提示条 `GlassSurface`（`:659-672`，优先级 `actionNotice ?: favoriteNotice ?: likeNotice`）-> `DetailContent`（`:679-701`）。

`DetailContent`（`:841`）的 `LazyColumn`（`:870`）分区顺序（item key 也一并列出）：

| 顺序 | item key | 行号 | 内容 |
|---|---|---|---|
| 1 | `"head"` | `:880` | `DetailHeader`（`:736`）：左封面（`AsyncImage` `:755`，120dp / 3:4）+ 右侧标题/作者/页数/点赞/更新时间；加载态与完成态共用，`onLike = if (detail == null) null else onLike`（`:901`） |
| 2 | `"state"` | `:910` | 数据未到时只放 `LoadingBox`/`ErrorBox`，然后 `return@LazyColumn`（`:921`） |
| 3 | `"authors"` | `:926` | `LazyRow` + `CategoryChip(author, onClick = onOpenTag)`（`:932`） |
| 4 | `"meta-<label>"` | `:944` | 作品 / 演员行，`LazyRow` + `CategoryChip`（`:958`） |
| 5 | `"tags"` | `:966` | `LazyRow` + `CategoryChip(text, onClick = onOpenTag, onLongClick = onBlockTag, blocked = ...)`（`:983-988`），行首提示"标签：点开搜索，长按屏蔽"（`:973`） |
| 6 | `"blocked"` | `:997` | 命中屏蔽时的说明条 + "不再屏蔽" `TextButton`（`:1025`） |
| 7 | `"desc"` | `:1036` | 简介 `GlassSurface` |
| 8 | `"download"` | `:1054` | "下载整部作品" `GlassSurface(onClick = onDownload)`，右侧标"交给系统下载器" |
| 9 | `"tag-favorite"` | `:1087` | "收藏这些标签" `GlassSurface(onClick = onOpenTagPicker)` |
| 10 | `"comments"` | `:1116` | 评论入口 `GlassSurface(onClick = onOpenComments)`，右侧显示条数或"暂无" |
| 11 | `"read-entry"` | `:1156` | 阅读入口 `GlassSurface(level = Raised, tinted = true, onClick = onReadChapter(entry.chapterId))`，标签文字 `entry.label` |
| 12 | `"chapters-head"` + `"chapters"` | `:1186`、`:1194` | 章节数 > 1 时显示标题 + `ChapterPager`（`:1237`，每页 10 章，内部 `IconButton` 翻页 `:1288`/`:1302`） |
| 13 | `"related-head"` + `"related"` | `:1206`、`:1214` | 相关推荐 `LazyRow` + `ComicCard`（`:1221`） |

页内两个对话框：`TagPickerDialog`（`:704-710`，来自 `.../screens/tags/TagFavoritesScreen.kt:226`）与 `FolderPickerDialog`（`:712-721`，来自 `.../screens/favorites/FolderDialogs.kt:112`）。

下载实现：`onDownloadReady`（`:556-581`）用系统 `DownloadManager` 入队，失败退回浏览器 Intent。`LifecycleEventEffect(ON_RESUME)` 静默校正收藏状态（`:598`）。

交互：点封面/标签/作者 -> `onOpenTag` -> `search?q={tag}`（`:932`、`:958`、`:985`）；长按标签 -> `onBlockTag`（`:986`）-> 写本地屏蔽名单；点"下载整部作品" -> `vm.requestDownload(onNeedLogin, onDownloadReady)`（`:694`）；点"收藏这些标签" -> 打开 `TagPickerDialog`（`:695`）；点评论 -> `onOpenComments` -> `comments/{id}`（`:693`、`JmNavHost.kt:768`）；点阅读入口 / 章节 -> `onReadChapter(chapterId)` -> `read/{comicId}/{chapterId}`（`:691`、`JmNavHost.kt:765`）；点相关推荐卡 -> `onOpenComic` -> `detail/{next}`（`:690`、`:763`）。

需要登录：浏览否。以下操作需要，未登录时经 `onNeedLogin(reason)` 跳到 `auth?reason=...`（`JmNavHost.kt:767`）：
- 追更 `toggleTracking` -> "追更需要登录"（`DetailScreen.kt:283`）
- 收藏标签 `favoriteTags` -> "收藏标签需要登录"（`:319`）
- 下载 `requestDownload` -> "下载需要登录"（`:344`）
- 点赞 `like` -> "点赞需要登录"（`:433`）
- 收藏 `toggleFavorite` -> "收藏需要登录"（`:485`）
另外追更按钮只在已登录时渲染（`:626`）。

#### 6.2.2 桌面端 `DetailScreen`
入口：`DetailScreen` — `desktop/src/main/kotlin/com/jmnext/desktop/DetailScreen.kt:65`

参数（`:65-76`）：`repository`、`comicId`、`onBack`、`onOpenChapter: (SeriesItem, List<String>) -> Unit`、`progress: ReadProgressStore`、`onOpenComments: (String) -> Unit`、`onOpenComic: (ListItem) -> Unit`、`onOpenTag: ((String) -> Unit)? = null`（Main 当前没传，见 `:74`）。

顶层：`Column(fillMaxSize)`（`:274`）-> 头部 `Row`（`:275-288`：`TextButton("返回")` `:280`、加载中 `LoadingHint` 或状态文本 `SelectionContainer { Text(status) }` `:286`）-> `detail ?: return@Column`（`:290`）-> `Row(fillMaxSize)`（`:291`）左右分栏：

**左栏 `Column(width = 280.dp, verticalScroll)`（`:293-546`）**：
- 封面 `Box(3:4, clip 10dp, jmSharedElement(jmCoverKey(d.id)))`（`:308-319`），带 `animateFloatAsState` 入场缩放（`:303`）。
- 按钮组 `Row`（`:321`）：「收藏」`Button`（`:322`，`repository.toggleFavorite`）、「点赞」`Button`（`:345`，已赞时提示"已经点过赞了" `:349`；未登录提示"请先登录再点赞" `:351`）、**仅登录时**的「追更」`Button`（`:381`，`repository.toggleTracking`）与「评论」`TextButton`（`:399`）；未登录时显示一行"未登录"（`:402-409`）。
- `InfoLine("标题")` / `InfoLine("作者")`（`:419-420`）—— `InfoLine` 内部用 `SelectionContainer`（`:606-614`）。
- 标签区（`:424-493`）：每个标签一行 = 可点文本（`openTag(tag)`，`:445`）+ "屏蔽" `TextButton`（`:455`，已屏蔽时改显示"已屏蔽" `:449`）+ "标星/取消标星" `TextButton`（`:459`）；`tagMessage` 提示（`:467`）；标签搜索结果就地铺开（标题 + 结果行 + "关闭标签搜索结果" `:471-491`）。
- `InfoLine("页数")`（`:494`）；简介 `plainText()`（`:498`）。
- 下载入口 `Row(clickable -> downloadAlbum())`（`:507-527`）+ `downloadMessage`（`:528`）。
- 相关作品 `Row(horizontalScroll)` + `ComicCover`（`:533-545`）。

**右栏（`:548-601`）**：
- `series.isEmpty()` -> `LazyColumn` 只有一个"开始阅读"入口（`:552-565`，用 `SeriesItem(id = d.id)` 打开整本）。
- 否则 `LazyColumn`（`:567`）：`item` 两个按钮「从头开始」（`:581`）与「继续观看」（`:583`，无记录时置灰，`:582` 注释说明是**置灰**不是隐藏）；其后 `items(d.series)` **正序**列章节（`:590-599`，注释 `:589` 说明用户要求正序）。

页内局部状态（`:77-99`）：`detail/status/loading/favorite/tracked/likeBusy/likeMessage/trackBusy/favMessage/favBusy/downloadBusy/downloadMessage/tagBusy/tagMessage/tagQuery/tagItems/tagResultStatus/starredTags`。

需要登录（显式判断行号）：下载 `:234`（"下载需要登录"）、标星 `:197`（"请先登录再标星标签"）、追更初始态 `:123`、追更按钮渲染 `:380`、点赞 `:350`（"请先登录再点赞"）。收藏 `toggleFavorite`（`:328`）**没有**登录判断。

### 6.3 我的（Android `ProfileScreen`，1293 行）
入口 `ProfileScreen` — `app/src/main/kotlin/com/jmnext/ui/screens/profile/ProfileScreen.kt:119`。内部区块（全部私有 Composable，按行号）：

| 行号 | 子 Composable | 角色 |
|---|---|---|
| `:199` | `AccountCard` | 账号卡：未登录态 / 已登录态 + 两行入口按钮 |
| `:301` | `RowScope.EntryButton` | 单个入口按钮（`label/icon/onClick/badge`） |
| `:349` | `BlockCard` | 屏蔽设置入口 |
| `:403` | `AppearanceCard` | 界面风格、动效、悬浮底栏等外观开关 |
| `:628` | `OptionSwitch` | 一个开关项（`Switch` 在 `:653`） |
| `:676` | `StyleOption` | 一个风格选项 |
| `:757` | `WallpaperCard` | 壁纸（`OutlinedTextField` `:800`、换一张 `:826`、两个 `Slider` `:864`/`:875`） |
| `:922` | `ReadingCard` | 阅读形态（纵向/横向） |
| `:958` | `PrivacyCard` | 隐私相关项 |
| `:978` | `AboutCard` | 关于入口 |
| `:1009` | `ServerCard` | 接口主机信息 |
| `:1089` | `SettingCard` | 卡片外壳（title + ColumnScope content） |
| `:1108` | `InfoRow` | 标签 + 值一行 |
| `:1138` | `DailyCard` | 每日签到（含 `DailyHistorySection`） |

`DailyCard` 内部（`:1138-1293`）：`SettingCard("每日签到")`（`:1165`）-> 未登录时显示可点的"登录后再签到"（`:1167-1177`）-> 加载中/无活动两种文案（`:1179-1190`）-> 有活动时显示活动名（`:1201`）、"已签 N / M 天"（`:1207`）、按周铺的日历格子（`:1213-1216`）、`Button` 签到（`:1249`）。历史部分在 `.../screens/profile/DailyHistorySection.kt:47`：默认 `expanded = false` 不请求（`:53`、`:43-44` 注释），展开后 `repo.dailyHistoryOptions(uid)`（`:67`）-> `repo.dailyHistory(year)`（`:86`）。

---

## 7. 登录要求总表

| 屏幕 | 浏览是否需要登录 | 需要登录的操作（源码依据） |
|---|---|---|
| 应用外壳 | 否 | 无 |
| 首页 | 否 | 追更更新标记（`HomeScreen.kt:192-204`）、快捷签到（`RandomFab.kt:164`）——未登录自动降级，不弹登录页 |
| 分类 | 否 | 无 |
| 搜索 | 否 | 无 |
| 我的 | 否 | 入口按钮与签到；未登录时显示"登录后再签到"（`ProfileScreen.kt:1167`） |
| 分区更多 | 否 | 无 |
| 连载更新 | 否 | 无 |
| 周刊 | 否 | 无 |
| 随机推荐 | 否 | 偏好排序降级（未登录收藏标签为空） |
| 通知 | 接口需要账号 | 页面无登录门；入口只在已登录区（`ProfileScreen.kt:276`） |
| 收藏 | **是** | 未登录显示"需要登录"页（`FavoritesScreen.kt:426`、ViewModel `:148`） |
| 观看历史 | **是** | 同上（Android）；桌面 `HistoryScreen.kt:59`、`:82` |
| 追更 | **是** | 同上（Android）；桌面 `TrackingScreen.kt:86`、`:115` |
| 标签收藏 | **是** | `TagFavoritesScreen.kt:162`；桌面"标签"页的扫描与标星（`TagsScreen.kt:105`、`:127`） |
| 画师与作品库 | 否 | 无 |
| 作品信息 | 否 | 无 |
| 作品详情 | 否 | 收藏/追更/点赞/下载/标签收藏，五项都有显式门（Android `DetailScreen.kt:283/319/344/433/485`；桌面 `DetailScreen.kt:197/234/350/380`） |
| 阅读器 | 否 | **两端都不校验登录**：Android `ReaderScreen.kt:191/208` 直接调接口；桌面 `ReaderScreen.kt:419-438`、`:488-489` 同样直接调 |
| 评论 | 否 | 发表/回复与删除：Android `CommentsScreen.kt:175`；桌面 `CommentsScreen.kt:125`、`:149` |
| 登录/注册/找回 | 不适用 | 本身就是登录页 |
| 屏蔽设置 | 否 | 本地存储 |
| 外观设置 | 否 | 无 |
| 关于 | 否 | 无 |
| 占位壳 `PageShell` | 否 | 无，无交互 |

登录入口统一为：Android `auth?reason={reason}`（`JmNavHost.kt:160` 的 `authFor`，各调用点 `:603/640/678/705/712/726/767`）；桌面 `Screen.Login`（`Main.kt:390`、`:468`、`:475`）。

---

## 8. 对话框 / 覆盖层 / 弹出层汇总（两端）

| 名称 | 平台 | 文件:行 | 触发点 | 关键参数 |
|---|---|---|---|---|
| `SnackbarHost`（瞬时反馈） | Android | `JmNavHost.kt:442` | `Notices.state` 变化（`:437`） | `SnackbarHostState.showSnackbar(text)` |
| `TagPickerDialog` | Android | `.../tags/TagFavoritesScreen.kt:226` | 详情页"收藏这些标签"（`DetailScreen.kt:704`） | `tags/onDismiss/onConfirm` |
| `FolderNameDialog` | Android | `.../favorites/FolderDialogs.kt:48` | 收藏页 `FolderDialog` 分支 | `onDismiss/onConfirm` |
| `FolderDeleteDialog` | Android | `.../favorites/FolderDialogs.kt:85` | 同上 | `onDismiss/onConfirm` |
| `FolderPickerDialog` | Android | `.../favorites/FolderDialogs.kt:112` | 详情页"移入收藏夹"（`DetailScreen.kt:712`）与收藏页 | `title/folders/onDismiss/onPick/onSkip/skipLabel` |
| `ManageFoldersDialog` | Android | `.../favorites/FolderDialogs.kt:166` | 收藏页顶栏"管理收藏夹"（`FavoritesScreen.kt:402`） | `folders/onDismiss/...` |
| `AddBlockDialog` | Android | `.../settings/BlockSettingsScreen.kt:242`（`AlertDialog` `:255`） | 三个 `BlockSection` 的"添加" | `kind/onDismiss/onConfirm` |
| `ChapterPickerDialog` | Android | `.../reader/ChapterPickerDialog.kt:48` | 阅读底栏"章节" | `series/currentChapterId/onDismiss/onPick` |
| `ChapterPickerDialog` | 桌面 | `.../desktop/ChapterPickerDialog.kt:46` | `PageRail`/`ReaderBottomBar` 的"章节"，先请求 `album` 拿 series（`ReaderScreen.kt:404-418`） | 同上 |
| 退出登录确认 `AlertDialog` | 桌面 | `Main.kt:398` | 顶部"退出登录"按钮（`:386`） | `title/text/confirmButton/dismissButton` |
| `NoticeHost`（瞬时反馈） | 桌面 | `Notices.kt:61` | `Notices.success/error/show` | 由 `Main.kt:396` 挂载 |
| 连载提醒降级提示行 | 桌面 | `Main.kt:352-365` | `SerialReminder.start(...)` 回调（`:341-342`） | "知道了"清空 |
| 阅读页错误飘条 | Android | `ReaderScreen.kt:391-407` | `state.error != null` 且有内容 | `GlassSurface(level = Flyout)` |
| 阅读页页码指示 | Android | `ReaderScreen.kt:853-871` | 翻页形态且工具栏可见 | `"${page+1} / ${size}"` |

---

## 9. 未接入 / 已作废的界面（如实记录）

| 名称 | 文件:行 | 状态与依据 |
|---|---|---|
| 桌面 `TrackingScreen` | `desktop/src/main/kotlin/com/jmnext/desktop/TrackingScreen.kt:43` | 有完整实现（标题行 + `TrackingList`），但**整个工程里没有任何调用方**：`Main.kt:495` 的追更入口用的是 `FavoriteScreen(initialTab = "tracking")`，只有它的内部组件 `TrackingList`（`:73`）被 `FavoritesScreen.kt:122` 复用（依据：对 `TrackingScreen` / `TrackingList(` 的全仓检索，`TrackingScreen` 只在自身文件出现）。 |
| 桌面 `ReaderBottomBar` 的注释 | `desktop/src/main/kotlin/com/jmnext/desktop/ReaderBottomBar.kt:41`、`ReaderScreen.kt:368-372` | 注释仍写"**尚未接线**"、"改成浮层...故先不做"，但 `ReaderScreen.kt:440` 实际已经调用它（`visible = true` 常显）。注释与代码不一致，以实现为准。 |
| Android 阅读器的登录校验 | `app/src/main/kotlin/com/jmnext/ui/screens/reader/ReaderScreen.kt:191`、`:208` | 收藏/点赞没有 `isLoggedIn` 判断，与详情页的显式登录门不同；未登录会走接口并静默回滚。属于"未做"而不是"按设计省略"（详情页 `DetailScreen.kt:433/485` 都有门并给原因）。 |
| 桌面 `DetailScreen` 的 `onOpenTag` | `desktop/src/main/kotlin/com/jmnext/desktop/DetailScreen.kt:74-75`、`Main.kt:440-452` | 参数存在但 `Main.kt` 未传（源码注释明确写着"Main.kt 当前没传"），因此详情页点标签走的是**本页就地搜索**分支（`:150-176`）。 |
| 桌面 `PAGE_PLANS` 与已实现页面的重叠 | `desktop/src/main/kotlin/com/jmnext/desktop/PageShell.kt:51-65` | 表里有 13 条计划文案，但其中 `search/category/week/random/favorites/history/tracking/tags/creator/notifications/block/about/profile` 都已有真实页面接入（`Main.kt:470-495`），当前只有未知 route 才会落到 `PageShell`（`Main.kt:514-517`）。 |

---

## 10. 我没有验证的部分（如实标注）

- 所有页面**没有实际运行**（未启动 Android 模拟器，也未启动桌面进程），结论全部来自静态阅读源码。
- 对话框实际上屏后的观感、动画细节、以及需要网络的路径（下载、签到、标星、发表评论等）都没有执行验证。
- 各 `ViewModel` 的完整状态字段清单只列了在对应 Composable 里被读到的那些；未读到的字段不列。
- 登录要求一栏以源码里能看到的判断为唯一依据；"服务端其实也要求登录但客户端没写判断"的情况（例如通知页）我按源码写成了"页内无门"。
