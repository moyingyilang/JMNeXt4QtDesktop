# 与 Kotlin 版的差距清单（移植工作的起点）

> 目的：把"完整移植 Kotlin 版"拆成可核对的项目。
> 基准：`JMNeXt/app/src/main`（60 个 Kotlin 文件、约 16.3k 行）。
> 现状核对日期：2026-10-07。**只写实际看到的，不写推测。**

## 一、结构差距（最根本的一条）

| 维度 | Kotlin 版 | Qt 版现状 |
| --- | --- | --- |
| 界面组织 | **每屏一个文件**（`ui/screens/` 下 25 个）+ 通用组件（`ui/components/` 8 个） | **整个界面集中在 `src/qml/Main.qml` 一个文件** |
| 导航 | `JmNavHost` 统一路由与转场 | 尚未见到对应结构 |
| 主题 | `ui/theme/` 五个文件（token / 调色板 / 形状 / 风格） | 已接 token（P3 记录），能否覆盖四套风格待核 |

**结论**：Qt 侧的界面不是"少几屏"，而是**组织形式不同**。要"完整移植"，
第一步必须把 `Main.qml` 的单体拆成**按屏分文件**，与 Kotlin 的屏幕一一对应 ——
否则每加一屏都在同一个文件里堆，越往后越难对齐（这正是主项目当初分文件的理由）。

## 二、屏幕对照（Kotlin 有 25 个文件；Qt 侧目前 1 个 QML）

| Kotlin 屏幕 | Qt 侧 | 备注 |
| --- | --- | --- |
| `home/HomeScreen` + `HomeViewModel` + `RandomFab` | 部分（STATUS 记"首页列表"） | 需核对是否含分区横向行、随机入口 |
| `search/SearchScreen` + `SearchFilters` | 部分（STATUS 记"搜索"） | 筛选行依赖接口，主项目也有缺口 |
| `detail/DetailScreen` | 部分（STATUS 记"详情"） | 需核对封面/标签/简介/章节列表/继续阅读 |
| `reader/ReaderScreen` + `PageRatio` + `ChapterPickerDialog` | 部分（STATUS 记"阅读器"） | 需核对翻页手势、缩放、章节选择器 |
| `favorites/FavoritesScreen` + `FolderDialogs` | **未见** | 需登录 |
| `category/CategoryScreen` | **未见** | |
| `creator/CreatorScreen` + `CreatorWorkScreen` | **未见** | |
| `comments/CommentsScreen` | **未见** | |
| `random/RandomListScreen` | **未见** | |
| `profile/ProfileScreen` + `DailyHistorySection` | **未见** | 需登录 |
| `more/MoreListScreen` | **未见** | |
| `auth/AuthScreen` | **未见** | 需真实账号 |
| `settings/BlockSettingsScreen` | **未见** | 屏蔽规则的本地页 |
| `tags/TagFavoritesScreen` | **未见** | |
| `week/WeekScreen` | **未见** | |
| `notifications/NotificationsScreen` | **未见** | |
| `about/AboutScreen` | **未见** | |

## 三、通用组件对照（Kotlin 8 个）

`AmbientBackdrop`、`ComicCard`、`FloatingBottomBar`、`Glass`、`GlassTopBar`、`ItemMotion`、
`LoadMoreFooter`、`StateBox` —— **Qt 侧尚未见到对应实现**（QML 里可能散落在 `Main.qml` 内）。

## 四、数据层对照（Qt 侧已有 `/ 需要补`）

Qt 侧 `src/core` 已具备：`HostDiscovery`、`JmCrypto`、`JmApi`、`JmParse`、`JmSession`、
`JmUrls`、`ImageUnscramble` + `UnscrambleApply`、`ReadProgress`、`BlockRules`、`AesEcb`、
`Base64`、`Md5`、`UpdateCheck` —— 底子是好的。

按主项目的能力清单，**需要核对或补**：收藏（读/写）、历史、评论、画师、分类、随机、
签到、收藏夹管理、标签屏蔽的设置界面、以及各接口的分页细节。

## 五、建议的实施顺序（本目标的路线）

1. **拆结构**：把 `Main.qml` 按屏拆成 `screens/*.qml`，与 Kotlin 的 `ui/screens` 同名对应；
   同时把 8 个通用组件落成 `components/*.qml`（这一步不追求 1:1 观感，只求结构对上）；
2. **补屏幕**：按第二节的表，从"未见"里挑**不依赖登录**的先做（分类、画师、评论、随机、更多）；
3. **补交互**：翻页手势与缩放、下拉刷新、加载更多、共享元素过渡与动效；
4. **补数据层**：随屏幕需要逐个接（收藏/历史/评论/画师/分类/随机/签到）；
5. **视觉对齐**：四套主题风格 + 组件观感逐项与 Kotlin 对照。

每一步都要：容器内编译通过 + `--selftest` 或截图作为证据 + 写进 `STATUS.md`。
