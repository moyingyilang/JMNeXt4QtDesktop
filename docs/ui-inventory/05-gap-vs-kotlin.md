# 与 Kotlin 版的差距清单（按仓库现状重写）

> **本文替换 2026-10-07 的旧版。** 旧版写的「Qt 侧目前只有 1 个 QML」「多屏未见」与仓库现状不符，
> 现已作废。以后判断现状只以文件为准：文件在就是有，文件里没接的信号就是没接。
>
> - **核对日期**：2026-11-08（核对时刻约 21:40，Asia/Shanghai）
> - **核对方式**：只读文件、不跑构建、不跑自检。逐项打开 `.qml` 看有哪些屏、看 `JmBackend.h` 有哪些接口与信号，
>   再用 `01-screens.md` §5 里列的 Kotlin 屏能力反查对照。
> - **状态词只有三个**：已实现 / 部分 / 未见。判定只用两样东西：仓库里存在哪些文件、文件里接了什么信号。
> - **不写"已验证"**：本次没有运行任何测试或脚本。仓库有测试，也有自检脚本（见 §4 末），但那是"存在"，不是"跑过"。
> - **快照性质**：核对期间仓库仍在被改动（`NotificationsScreen.qml` 21:38、`FavoritesScreen.qml` 21:39 才出现，
>   `Main.qml` 21:39 也在改）。本文是那一刻的快照，之后再新增屏，请按文件重新核对本表。

## 本次实际读过的文件

| 文件 / 命令 | 读它为了什么 |
| --- | --- |
| `ls src/qml/*Screen.qml`（16 个，逐个打开） | 判定某屏在 Qt 侧有没有文件、文件里有什么信号 |
| `src/qml/Main.qml`（核对时 557 行） | 挂了哪些屏实例、路由怎么走、面板尺寸怎么设 |
| `src/qml/JmBackend.h`（核对时 116 行）与 `src/qml/JmBackend.cpp` | 后端暴露了哪些接口与信号、哪些真被 QML 调用 |
| `src/qml/{About,Category,ComicList,Comment,Detail,Favorites,History,Home,Login,More,Notifications,Profile,Reader,Search,Settings,Week}Screen.qml` | 每屏内部到底做了什么（逐屏判定"部分"的具体缺项） |
| `src/core/FavoriteStore.h`、`src/core/HistoryStore.h`、`ls src/core` | 本地收藏 / 本地历史是否真的存在 |
| `src/qt/JmWorker.h` | 确认 `openChapterId` 的注释用途（"章节选择器用"） |
| `docs/ui-inventory/01-screens.md` §0 / §4 / §5 | Kotlin 24 个逻辑屏的名单与逐屏能力（对照基准） |
| `scripts/verify-all.sh`、`ls tests` | 仓库里有哪些自检入口与测试（只统计，不运行） |
| `grep -c Q_INVOKABLE src/qml/JmBackend.h` | 得 27：其中 1 处在文件头注释里，**实际声明 26 条** |
| `grep -o "backend\.<方法>(" src/qml/*.qml` | 统计哪些后端接口被 QML 调用过（注释里的提及也会计到，只作有无调用的粗判） |

---

## 一、结构差距

旧版最根本的那条结论（"整个界面挤在 `Main.qml` 一个文件里"）**已经不成立**：拆屏这件事做完了。

| 维度 | Kotlin 版（`01-screens.md` §1、§4） | Qt 版现状（2026-11-08） |
| --- | --- | --- |
| 界面组织 | 每屏一个 Composable 文件，另有通用组件目录 | **已按屏分文件**：`src/qml/` 共 28 个 `.qml` = 16 个 `*Screen.qml` + 11 个组件 + `Main.qml` |
| 导航 | 手写状态机 `sealed interface Screen` + `Page(route)` 字符串路由，统一淡入 + 横移转场 | `Main.qml` 用 **12 个布尔属性当路由**（`showAbout` / `showSettings` / `showHistory` / `showFavorites` / `showNotifications` / `showCategory` / `showMore` / `showList` / `showLogin` / `showWeek` / `showComments` / `showProfile`），屏实例靠 `visible` 切换；**没有页面栈，也没有"返回上一屏"** |
| 外壳 | 左侧常驻 `SideNav`，15 项；顶部标题 + 登录态行 | **无侧栏**。常态是两栏：左栏 380px（搜索行 + 首页列表）、右栏（详情 / 阅读器二选一）；其余屏是铺满窗口的模态面板（`anchors.fill` + `anchors.margins: 32`） |
| 主题 | `Appearance` 全局对象 + 五套玻璃风格 + 壁纸/模糊/浓度/压暗 | `applyThemeTokens()` 从 `themeInit` 复制 8 个 token，`themeController.applied` 触发重读，运行时换色有实现；壁纸只有 `--wallpaper` 与 `wallpaperSource`，**没有模糊/浓度/压暗的调节界面** |

**结论**：差距已经从"组织形式不对"变成"每屏内部的能力不对齐"，加上两个结构性缺口——**没有侧栏导航、没有页面栈**。

### 1.1 QML 屏幕文件（16 个，逐个打开确认）

| 文件 | 根类型 | 自有信号 |
| --- | --- | --- |
| `AboutScreen.qml` | Rectangle | `closeRequested` |
| `CategoryScreen.qml` | Rectangle | `closeRequested` / `tagClicked` / `resultClicked` |
| `ComicListScreen.qml` | ColumnLayout | `closeRequested` / `comicClicked` |
| `CommentScreen.qml` | ColumnLayout | `closeRequested` |
| `DetailScreen.qml` | Rectangle | `chapterClicked` / `favoriteToggled` / `trackToggled` / `commentsRequested` |
| `FavoritesScreen.qml` | Rectangle | `closeRequested` / `comicClicked` |
| `HistoryScreen.qml` | Rectangle | `closeRequested` / `comicClicked` |
| `HomeScreen.qml` | ColumnLayout | `cardClicked` / `retryClicked` / `moreClicked` / `randomClicked` |
| `LoginScreen.qml` | Rectangle | `closeRequested` / `submit` / `logoutRequested` |
| `MoreScreen.qml` | Rectangle | `closeRequested` / `entryClicked` |
| `NotificationsScreen.qml` | Rectangle | `closeRequested` |
| `ProfileScreen.qml` | ColumnLayout | `closeRequested` / `loginRequested` |
| `ReaderScreen.qml` | Rectangle | `stepRequested` |
| `SearchScreen.qml` | ColumnLayout | `searchRequested` / `aboutRequested` / `settingsRequested` / `categoryRequested` / `moreRequested` |
| `SettingsScreen.qml` | Rectangle | `closeRequested` / `twoPageToggled` / `blockWordsChanged` |
| `WeekScreen.qml` | ColumnLayout | `closeRequested` / `comicClicked` |

### 1.2 `Main.qml` 的挂载与路由（据文件本身）

- **常驻两栏**：`SearchScreen`（导航行 + 搜索框）、`HomeScreen`（列表）在左；`ReaderScreen`（`visible: root.reading`）与 `DetailScreen`（`visible: !root.reading`）在右，靠 `opacity` 做 140ms 交叉淡入。
- **模态面板 12 个**：`AboutScreen`、`SettingsScreen`、`HistoryScreen`、`FavoritesScreen`、`NotificationsScreen`、`CategoryScreen`、`MoreScreen`、`ComicListScreen`、`LoginScreen`、`WeekScreen`、`CommentScreen`、`ProfileScreen`。
- **路由入口**：`SearchScreen` 的四个按钮（关于 / 设置 / 分类 / 更多）直接置位布尔属性；`MoreScreen.onEntryClicked` 用一张 10 项的 key→（标题, 路径, 查询）表分派，`__login__` / `__profile__` / `__week__` / `__history__` / `__favorites__` / `__notify__` / `__action__` 是特例，其余走 `showList` + `ComicListScreen`（同一屏按不同 `apiPath` 复用）。
- **未识别的 key**：只 `console.log("QML 更多：该项还没有对应接口 -> " + key)`，不报错也不跳转。
- **一处写法不一致（如实记录）**：`WeekScreen`、`CommentScreen`、`ProfileScreen` 三个实例**没有写 `anchors.fill`**，其余面板都有。是否影响实际显示**未核对**（本次不跑界面）。

### 1.3 通用组件对照（原第三节，现并入本节）

旧版说"Qt 侧尚未见到对应实现"，**不对**。Kotlin 的 8 个通用组件，Qt 侧都有同名 `.qml`：

`AmbientBackdrop.qml`、`ComicCard.qml`、`FloatingBottomBar.qml`、`Glass.qml`、`GlassTopBar.qml`、`ItemMotion.qml`、`LoadMoreFooter.qml`、`StateBox.qml`。

Qt 侧另有 3 个自建组件：`GlassBlur.qml`（真背景模糊，Qt 6.5+ 专用）、`ExpandableOptionRow.qml`（可展开选项行，设置页用）、`ThemeModeIcon.qml`（深浅色图标）。

---

## 二、屏幕对照表

对照基准是 `01-screens.md` §0 的 **24 个逻辑屏**；「状态」按 §5 里该屏列出的能力逐项反查文件，缺任一关键项即记"部分"。

| # | Kotlin 屏幕（`01-screens.md` §0 / §5） | Qt 侧文件 | 状态 | 判定依据（文件与信号） |
| --- | --- | --- | --- | --- |
| 1 | 应用外壳（导航框架） | `Main.qml` | 部分 | 两栏布局 + 12 个 `showXxx` 布尔路由；无侧栏 15 项、无页面栈 |
| 2 | 首页 | `HomeScreen.qml` | 部分 | 列表 / `StateBox` / `LoadMoreFooter` / 重试 / 加载更多 / 随机信号都有；**无推荐分区横向行**，无每日签到浮钮 |
| 3 | 分类 | `CategoryScreen.qml` | 部分 | `hotTagsReady` + `categoryReady` 都渲染到 `tagModel` / `resultModel`；但 `resultClicked` **没有接收方**（`Main.qml` 只接 `onCloseRequested` / `onTagClicked`），点结果无动作；无分类树、无排序行 |
| 4 | 搜索 | `SearchScreen.qml` | 部分 | 只有关键词行 + 条数标题，结果复用首页 `listModel`；无排序 / 字段 / 年月筛选 |
| 5 | 我的 | `ProfileScreen.qml` | 部分 | 登录态文案 + `setting` 配置列表；`Main.qml` 没给 `loggedIn` / `userName` 赋值，界面恒显示未登录；无等级 / 金币 / 签到卡 |
| 6 | 分区更多（more/{id}） | 无 | 未见 | 没有任何屏消费分区 id；`HomeScreen.qml` 里没有分区模型 |
| 7 | 连载更新（每周更新表） | 无 | 未见 | `WeekScreen.qml` 处理的是 `week` / `week/filter`，不处理 `more/26` |
| 8 | 周刊 | `WeekScreen.qml` | 部分 | 刊期列表 + `week/filter` 作品 + 封面；`type` 参数恒为空串（`WeekScreen.qml:87`），无类型行、无加载更多 |
| 9 | 随机推荐 | `ComicListScreen.qml`（经 `Main.qml` 的 `random` 项） | 部分 | 右下角"随机"按钮 → `listPath = random_recommend` → 通用列表；无"换一批"、无版式切换、无按收藏偏好排序 |
| 10 | 通知 | `NotificationsScreen.qml` | 部分 | 只把 `notifications` 的原始 JSON 填进只读 `TextArea`（`rawText`），**未解析成列表**；页签 / 标记已读 / 分页都未见 |
| 11 | 收藏 | `FavoritesScreen.qml` | 部分 | 本地 `backend.favorites()` 列表 + 清空 + 点击打开；**无收藏夹管理**（新建 / 改名 / 删除），**无逐条取消收藏**（`removeFavorite` 无 QML 调用方） |
| 12 | 观看历史 | `HistoryScreen.qml` | 部分 | 本地 `backend.history()` 列表 + 清空；无封面、无单条删除（Kotlin 侧有"删除"） |
| 13 | 追更 | 无 | 未见 | 只有 `DetailScreen` 的单个追更开关（`trackToggled`），没有追更列表屏 |
| 14 | 标签收藏 | `ComicListScreen.qml`（`tags` → `tags_favorite`） | 部分 | 只是一份作品列表；无"扫描收藏更新统计"、无标星增删；`MoreScreen.qml` 里该项 `ready: false` 且界面标"未实现"，与路由表不一致 |
| 15 | 画师与作品库 | `ComicListScreen.qml`（`creator` → `creator_author`） | 部分 | 只是一份作者列表；无画师/作品双模式、无点画师展开作品库 |
| 16 | 作品信息（creator/work/{id}） | 无 | 未见 | 无对应屏，也没有对应接口声明 |
| 17 | 作品详情 | `DetailScreen.qml` | 部分 | 封面 / 标题 / 作者 / 标签 / 章节列表 / 收藏 / 追更 / 评论都在；但 `albumReady` 只带 `name`、`author`、`tags`（`JmBackend.h:79`），**没有简介字段**；无相关作品、无"继续阅读" |
| 18 | 阅读器 | `ReaderScreen.qml` | 部分 | 上一页/下一页按钮、拖动翻页、滚轮 + 捏合缩放、双击复位、双页右页都有；**无章节选择器**（`src/qml` 无对应文件与信号），无页码/进度显示 |
| 19 | 评论 | `CommentScreen.qml` | 部分 | 只读：`loadPaged("comments", "forum", "mode=all&page=1&aid=…")` 并显示原始行；无发表/回复/删除、无加载更多 |
| 20 | 登录 / 注册 / 找回密码 | `LoginScreen.qml` | 部分 | 登录 + 退出登录两个动作；**无注册、无找回密码**（文件里没有对应控件） |
| 21 | 屏蔽设置 | `SettingsScreen.qml` | 部分 | 只有一个"屏蔽词（空格分隔）"输入框 → `blockWordsChanged` → `backend.setBlockWords`；无关键词/标签/分类三类分栏与增删列表 |
| 22 | 外观设置 | `SettingsScreen.qml`（合并进设置） | 部分 | 有"界面风格"行（`themeController`）与深浅色；无壁纸 / 模糊 / 浓度 / 压暗滑杆，无独立 `AppearanceScreen.qml` |
| 23 | 关于 | `AboutScreen.qml` | 部分 | 静态说明 + 许可文字 + 关闭；**无版本号、无检查更新、无链接**（文件里没有 `appVersion`，也没有按钮） |
| 24 | 未实现页占位壳（`PageShell`） | 无 | 未见 | 未识别的 key 只 `console.log`，没有占位页 |

统计：**24 行中，已实现 0、部分 19、未见 5**（未见的是：分区更多、连载更新、追更、作品信息、未实现页占位壳）。

"已实现 0"不是说 16 个 QML 文件白写了。按"能打开、能看到真实数据"这个弱口径，其中十几屏已经可用；但按 `01-screens.md` 逐屏列出的能力对齐，每一屏都还缺至少一项（多为简介、章节选择器、筛选行、发布框这类），所以这里给 0。这是口径问题，不是工作量问题——宁可按严格口径写，也不把"能显示"写成"已实现"。

---

## 三、数据层差距

### 3.1 `JmBackend` 已暴露的接口（26 条 `Q_INVOKABLE`，逐条抄自 `JmBackend.h`）

| 行 | 声明 | 用途 | QML 是否调用 |
| --- | --- | --- | --- |
| 24 | `void loadList()` | 首页最新列表 | 是 |
| 25 | `void loadMore()` | 列表下一页 | 是 |
| 26 | `void search(word, page = 1)` | 关键词搜索 | 是 |
| 27 | `void searchOrdered(word, page, order)` | 带排序的搜索 | **否** |
| 28 | `void openChapter(aid, page = 0)` | 按作品打开章节 | **否** |
| 29 | `void loadAlbum(aid)` | 取详情（名称/作者/标签/章节） | 是 |
| 37 | `void openChapterId(chapterId, page = 0)` | 按章节 id 直接打开（`JmWorker.h:71` 注明"章节选择器用"） | 是 |
| 38 | `void step(delta)` | 翻页 | 是 |
| 39 | `void setBlockWords(QStringList)` | 屏蔽词 | 是 |
| 40 | `void setTwoPage(bool)` | 双页模式 | 是 |
| 41 | `void loadCovers(n)` | 批量取封面 | 是 |
| 43 | `void loadHotTags()` | 热门标签（分类页数据源） | 是 |
| 45 | `void categoryFilter(c, page = 1)` | 按分类标签筛选 | 是 |
| 47 | `void loadPaged(tag, path, query)` | 通用分页列表（收藏/画师/标签/周更/评论共用） | 是 |
| 48 | `void login(username, password)` | 登录 | 是 |
| 49 | `void logout()` | 退出登录 | 是 |
| 51 | `void action(tag, path, form)` | 通用动作（收藏/追更/点赞/签到/评论） | 是 |
| 52 | `void fetch(tag, path, query)` | 取原始文本（`textReady` 回传） | 是 |
| 53 | `void reportCacheStats()` | 缓存统计 | **否** |
| 56 | `QVariantList history() const` | 本地阅读历史 | 是 |
| 57 | `void clearHistory()` | 清空历史 | 是 |
| 58 | `void rememberHistory(aid, title)` | 记录历史 | 是 |
| 63 | `QVariantList favorites() const` | 本地收藏 | 是 |
| 64 | `void rememberFavorite(aid, title, folder)` | 写入本地收藏 | 是（`Main.qml:135`） |
| 65 | `void removeFavorite(aid)` | 删除一条本地收藏 | **否** |
| 66 | `void clearFavorites()` | 清空本地收藏 | 是 |

信号共 **23 个**：`signals:` 块 21 个（`listReady` / `listAppended` / `hotTagsReady` / `categoryReady` / `pagedReady` / `loginResult` / `actionDone` / `textReady` / `albumReady` / `albumCoverReady` / `currentAidChanged` / `pageChanged` / `previewChanged` / `chaptersReady` / `pageReady` / `previewReady` / `coverReady` / `coverUrlReady` / `cacheStats` / `status` / `failed`），加两条 `Q_SIGNAL`：`historyChanged`、`favoritesChanged`。

### 3.2 `src/core` 已有的底子

`HostDiscovery`、`JmCrypto`、`JmApi`、`JmParse`、`JmSession`、`JmUrls`、`ImageUnscramble`、`UnscrambleApply`、`ReadProgress`、`BlockRules`、`AesEcb`、`Base64`、`Md5`、`UpdateCheck`，加两个本地表 `FavoriteStore.h` / `HistoryStore.h`，以及 `JmPaths.h`、`JmCore.h`、`Version.h`。

### 3.3 仍然缺的能力

| 能力 | 现状 | 依据 |
| --- | --- | --- |
| 本地收藏 | **已有**（旧版写的"需补本地收藏"已不成立）：`FavoriteStore.h` 被 `JmBackend.cpp` include，4 个接口 + `favoritesChanged` 都在 | `src/core/FavoriteStore.h`、`JmBackend.cpp:7`、`JmBackend.h:63-67` |
| 收藏夹管理 | 缺。Kotlin 有新建/改名/删除（`editFavoriteFolder`），Qt 无对应接口 | `JmBackend.h` 里没有 `folder` 相关的增删接口（`rememberFavorite` 只带一个 `folder` 字符串参数） |
| 通知解析 | 缺。没有通知 DTO/模型，界面只是把原始 JSON 显示出来；未读数是 QML 侧用正则 `:\s*(\d+)` 求和（`MoreScreen.qml`） | `NotificationsScreen.qml` 的 `rawText` 绑定与注释 |
| 账号相关 | 部分。`login` / `logout` 有；**注册、找回密码、用户资料（uid/等级/金币/免广告）解析都未见**；凭证只在进程内，未落盘 | `LoginScreen.qml` 只有登录控件；`ProfileScreen.qml` 注释自述"资料明细尚未做"；`Main.qml` 未给 `loggedIn` / `userName` 赋值 |
| 服务端历史（watch_list / 删除单条） | 未见。只有本地 `history()`，与 Kotlin 的账号历史不同源 | `JmBackend.h:56-58` 全是本地接口 |
| 追更列表 | 未见。只有 `action("track", "album_sertracking", …)` 的开关 | `DetailScreen.qml:46`、`Main.qml` 追更动作 |
| 签到 | 只有通用动作 `action("checkin","daily_chk","")`，没有界面 | `Main.qml` 的 `__action__` 分支 |
| 搜索排序 / 筛选 | 接口在（`searchOrdered`），界面没接；字段与年月筛选连接口都没有 | 调用统计里 `searchOrdered` 为 0 |
| 作品简介 | 缺。`albumReady(name, author, tags)` 三个字段里没有简介 | `JmBackend.h:79` |
| 更新检查 | 缺。`core/UpdateCheck.h` 存在，但没有暴露给 QML 的接口；`appVersion` 虽是上下文属性，却只用在窗口标题上 | `grep -rn appVersion src/qml/*.qml` 只有 `Main.qml:12` 一处；`AboutScreen.qml` 没引用 |

---

## 四、下一步优先级建议

排序原则：**不依赖登录、且用现有接口或纯 QML 就能验证的排前面**。每条都写明"为什么现在能做"。

1. **分类结果点击接线**（纯 QML，最便宜）。`CategoryScreen` 已经发 `resultClicked(aid)`，`Main.qml` 的 `categoryPane` 实例只接了 `onCloseRequested` / `onTagClicked`，补一个 `onResultClicked` 打开详情即可。不依赖登录。
2. **搜索排序接线**。`JmBackend::searchOrdered` 已声明并实现，QML 一次都没调；加一行排序按钮就够了。不依赖登录。
3. **阅读器章节选择器**。`chaptersReady(names, ids)` 已经在发，`openChapterId(chapterId, page)` 也已经能用（`JmWorker.h:71` 就注明它是给章节选择器用的），缺的只是一个 QML 弹层。不依赖登录。
4. **收藏逐条取消**。`removeFavorite(aid)` 已声明、`favoritesChanged` 已发，`FavoritesScreen` 缺一个 delegate 按钮。不依赖登录（本地表）。
5. **关于屏补版本与更新检查**。`UpdateCheck.h` 在 core 里，`appVersion` 已是上下文属性，只差一个 `Q_INVOKABLE` + 界面上两行字。不依赖登录，离线也能验证"版本号显示对了"。
6. **通知解析成列表**。先定字段（未读、页签、条目），再把 `NotificationsScreen` 从 `rawText` 换成模型，并补"标记已读"。接口要登录，**能写代码但不能在未登录状态验证**。
7. **观看历史 / 收藏补封面**。`coverReady(index, image)` 是按索引回图的，界面侧要把行序与索引对上；这一点**本次未核对**，动手前先读 `JmWorker` 的索引约定。
8. **需登录才有意义的一组，放最后**：收藏夹管理、追更列表、签到界面、注册/找回密码、个人资料卡。理由不是难，而是**无法用未登录的自检证明它对**。

以下两件**建议先别做**：

- **侧栏导航 + 页面栈**：要重写 `Main.qml` 的 12 个布尔路由，改动面最大，且和"逐屏对齐能力"不冲突，收益排在后面。
- **四套玻璃风格逐屏比对**：需要成对截图，仓库目前只有 `--shot` 单张，没有基线图与像素比对脚本。

### 关于"测试"与"脚本"的存在情况（不代表跑过）

- `tests/` 下 16 个 C++ 单测，全部针对 `core` 层（Base64/加密、屏蔽规则、信封、主机发现、图片解法、API、解析、URL、离线流程、路径、阅读进度、搜索解析、会话、更新检查等）。**没有 QML 界面测试。**
- `scripts/verify-all.sh` 串了 8 步：构建 → `ctest` → `--shot` 截图 → `--ui-selftest` → `--hot-tags` → `--category-filter` → `--paged week` → `--logged` 登录加收藏探测。
- 这些是"仓库里有"，**本次核对没有运行任何一条**，所以本文没有任何一处写"已验证"。要声称某屏可用，得先跑对应步骤并把输出记进 `STATUS.md`。
