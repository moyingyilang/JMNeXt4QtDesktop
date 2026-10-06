# 02 · 可复用 UI 组件与设计系统清点

**清点对象**：`/data/data/com.termux/files/home/jmc/JMComic_Next`
**主要范围**：`app/src/main/kotlin/com/jmnext/ui/`（`theme/`、`components/`、`screens/`、根目录）
**清点方式**：全量扫描该目录下所有 `.kt` 的 `@Composable` 声明；行号均为源码中的 1-based 行号。
**姊妹文档**：`../THEME-TOKENS.md` 已经逐行摘录过配色与表面工艺的原始数值行，本文的颜色一节只做字段分组与用途归类，不重复贴色值。

> **口径声明**：本文只写源码里实际存在的东西。凡是我没有读到、无法确认的，一律标「未确认」。
> `ui/` 之外（`desktop/` 模块、`shared/`、`JmApp.kt`、`MainActivity.kt`）只在必要处作为调用方提及。

---

## 0. 总数与统计口径

`app/src/main/kotlin/com/jmnext/ui/` 下共 **105 个 `@Composable` 函数声明**：

| 分类 | 数量 | 说明 |
| --- | --- | --- |
| A. 设计系统核心（`ui/components/` + `ui/` 根 + `ui/theme/`） | 23 | 跨屏复用的底座，含 `jmShape`、`JmTheme` 两个非「组件」的入口 |
| B. 屏幕级 public（`ui/screens/**`） | 27 | 18 个页面入口 + 9 个可复用组件（`FolderDialogs` 4 个、`TagPickerDialog`、`ChapterPickerDialog`、`RandomFab`、`DailyQuickFab`、`DailyHistorySection`） |
| C. 屏幕内 private / 局部 | 55 | 只在本文件被调用，但**在 Qt 里仍然要重建**（卡片、行、chip、占位、底栏…） |
| **合计** | **105** | |

另有 3 个**不是 `@Composable`、但属于组件层**的扩展/类：

| 名称 | 位置 | 说明 |
| --- | --- | --- |
| `Modifier.jmAnimateItem(scope: LazyItemScope)` | `ui/components/ItemMotion.kt:22` | 列表条目进出场动画；`lite` 下不挂 |
| `Modifier.jmAnimateItem(scope: LazyGridItemScope)` | `ui/components/ItemMotion.kt:29` | 网格版重载 |
| `class SquircleShape(radius, steps = 12, exponent = 5f)` | `ui/theme/Shapes.kt:37` | 连续圆角（超椭圆）`Shape`，只给 Miuix 用 |

**统计边界**：`ui/` 目录内被 grep 命中的 `@Composable` 注解共 130 处，差额来自函数参数上的 `content: @Composable () -> Unit` 这类 **lambda 类型标注**，不构成独立组件。

---

## 1. 组件清单

### 1.1 设计系统核心（A 类，23 个）

#### 1.1.1 `GlassSurface` —— 全部「卡片」的唯一落点

- **位置**：`ui/components/Glass.kt:84`
- **用途**：一层表面（卡片 / 浮起 / 浮层），五套风格的**唯一**表面绘制分支点。
- **参数**：

| 参数 | 类型 | 默认值 |
| --- | --- | --- |
| `modifier` | `Modifier` | `Modifier` |
| `level` | `GlassLevel` | `GlassLevel.Card` |
| `shape` | `Shape` | `jmShape(Radius.lg)` |
| `tinted` | `Boolean` | `false` |
| `onClick` | `(() -> Unit)?` | `null` |
| `content` | `@Composable BoxScope.() -> Unit` | 无（必填） |

- **内部拼成**（`Glass.kt:148-241`）：
  `Box` → `.graphicsLayer { scaleX/scaleY = scale }` → `.shadow(elevation, shape, clip = false)` → `.clip(shape)` → `.background(fill)` → 条件 `.drawBehind { drawRect(c.accent, alpha = accentTint) }` → 条件 `.drawBehind { drawRect(tintBrush) }`（橙→蓝 `Brush.linearGradient`，`Offset.Zero` → `Offset.Infinite`）→ 条件 `.border(surface.hairline, c.stroke, shape)` → `.drawBehind { noiseBrush(BlendMode.Overlay) ; 上缘高光 verticalGradient }` → 条件 `.clickable(...)`。
- **变体**：
  1. **层级 3 档**（`GlassLevel`，`Glass.kt:54`）：`Card` → `surface1` + `Elevation.sm`；`Raised` → `surface2` + `Elevation.card`；`Flyout` → `surface3` + `Elevation.flyout`。
  2. **工艺 5 种**（`SurfaceCraft`，`ThemeStyle.kt:83`）：`Acrylic` / `Glass` / `Blur` / `Card` / `Tonal`，由 `JmTheme.spec.surface.craft` 决定填充、描边、颗粒、染色是否生效。
  3. **`tinted` 开关**：只有 `Acrylic` 与 `Glass` 会真的画橙蓝薄层（`Glass.kt:175-176`）。
  4. **通透模式**（`ultraTranslucent`）：填充 alpha 乘 `ULTRA_TRANSLUCENT_FILL = 0.08f`（`Glass.kt:62`），且不画薄层。
  5. **按压**：`pressScale`（仅 Miuix 为 `0.97f`）；弹性风格用 `spring(dampingRatio = 0.55f, stiffness = 900f)`，否则 `tween(spec.motion.fast)`（`Glass.kt:106-114`）。
- **调用点**：ui/ 内 32 处（另有 `ComicRow`、`GlassCircle`、`FloatingBottomBar`、`GlassTopBar` 内部调用）。

#### 1.1.2 `GlassCircle`

- **位置**：`ui/components/Glass.kt:262`
- **用途**：圆形玻璃表面，注释标明用于头像、图标按钮等小尺寸场景。
- **参数**：`modifier: Modifier = Modifier`、`diameter: Dp = 40.dp`、`level: GlassLevel = GlassLevel.Raised`、`onClick: (() -> Unit)? = null`、`content: @Composable BoxScope.() -> Unit`
- **内部拼成**：`GlassSurface(modifier.size(diameter), shape = RoundedCornerShape(percent = 50))`。
- **变体**：仅 `level` 三档。
- **调用点**：**全仓库 0 处调用（截至本次清点）**——目前是未被使用的组件，Qt 侧可最后做。

#### 1.1.3 `Modifier.accentBar(color: Color, show: Boolean = true)`

- **位置**：`ui/components/Glass.kt:247`（不是 `@Composable`）
- **用途**：左侧 MIUI 强调条，标出「当前项」。
- **组成**：`drawWithContent`；条宽 `Sizing.accentBar = 3.dp`，条高 = 组件高度 × `0.56f`，垂直居中，`drawRoundRect`，圆角 = 宽的一半。
- **调用点**：**0 处**（同样是尚未接线的组件）。

#### 1.1.4 `ComicCard`

- **位置**：`ui/components/ComicCard.kt:65`
- **用途**：竖版漫画卡片：封面 + 标题 + 作者，用于首页推荐区横向滚动。
- **参数**：

| 参数 | 类型 | 默认值 |
| --- | --- | --- |
| `item` | `ListItem` | 必填 |
| `coverUrl` | `String` | 必填 |
| `onClick` | `() -> Unit` | 必填 |
| `modifier` | `Modifier` | `Modifier` |
| `width` | `Dp` | `CardSizes.row`（= `132.dp`，`ComicCard.kt:53`） |
| `sharedKey` | `String?` | `null` |
| `updated` | `Boolean` | `false` |

- **内部拼成**：`Column(width, clip(jmShape(Radius.lg)), clickable, padding(bottom = Spacing.sm))` → `Box` { `Cover(...)` + 条件「更新」角标 `Text` } → `Column` { 标题 `Text`（`minLines = 2, maxLines = 2`）+ 作者/分类 `Text` }。
- **变体**：
  - `width` 只有两个约定值：`CardSizes.row = 132.dp`（横向推荐流）与 `CardSizes.grid = 148.dp`（网格，`ComicCard.kt:56`）。
  - `updated = true` 时左上角画强调色「更新」胶囊（`RoundedCornerShape(Radius.sm)`，`padding(horizontal = Spacing.xs, vertical = 2.dp)`）。
  - 封面比例固定 `COVER_RATIO = 3f / 4f`（`ComicCard.kt:40`）。
- **调用点**：ui/ 内 7 处。

#### 1.1.5 `ComicRow`

- **位置**：`ui/components/ComicCard.kt:149`
- **用途**：横向漫画条目：左封面右文字，用于纵向列表（最新、搜索结果、收藏、历史）。
- **参数**：`item: ListItem`、`coverUrl: String`、`onClick: () -> Unit`、`modifier: Modifier = Modifier`、`trailing: (@Composable () -> Unit)? = null`
- **内部拼成**：`GlassSurface(level = Card, onClick)` → `Row(padding(Spacing.md), spacedBy(Spacing.md))` { `Cover(width = 76.dp, aspectRatio(3/4), clip(jmShape(Radius.md)))` + `Column(weight(1f))` { 标题 `titleMedium` + 作者 `bodyMedium` + `CategoryChip` } + `trailing?.invoke()` }。
- **变体**：`trailing` 为空时布局不变（历史列表用它挂删除按钮）。
- **调用点**：3 处。

#### 1.1.6 `CategoryChip`

- **位置**：`ui/components/ComicCard.kt:205`
- **用途**：分类小标签；可点可长按，可标「已屏蔽」。
- **参数**：`text: String`、`modifier: Modifier = Modifier`、`onClick: (() -> Unit)? = null`、`onLongClick: (() -> Unit)? = null`、`blocked: Boolean = false`
- **内部拼成**：`Box(clip(jmShape(Radius.xs)), background(...), combinedClickable?, padding(horizontal = Spacing.sm, vertical = Spacing.xxs))` → `Text(labelSmall, maxLines = 1)`。
- **变体**（2 × 2 = 4 种可见状态）：
  - 底色：`blocked` 时 `c.surfaceSunken`，否则 `c.accentSoft`。
  - 文字：`blocked` 时 `c.textTertiary` + `TextDecoration.LineThrough`，否则 `c.accent`。
  - 交互：`onClick`/`onLongClick` 都为 null 时不挂点击。
- **调用点**：5 处。

#### 1.1.7 `Cover`（private）

- **位置**：`ui/components/ComicCard.kt:249`
- **用途**：封面图，带统一的加载中 / 失败占位。
- **参数**：`url: String`、`contentDescription: String?`、`modifier: Modifier = Modifier`
- **内部拼成**：`SubcomposeAsyncImage(model = url, contentScale = ContentScale.Crop)`，`loading = { CoverFallback() }`、`error = { CoverFallback(showIcon = true) }`。
- **变体**：加载中（无图标）与失败（`Icons.Filled.BrokenImage`）两态。

#### 1.1.8 `CoverFallback`（private）

- **位置**：`ui/components/ComicCard.kt:265`
- **用途**：封面的占位底。
- **参数**：`showIcon: Boolean = false`
- **内部拼成**：`Box(fillMaxSize, background(c.surfaceSunken), contentAlignment = Center)` → 条件 `Icon(Icons.Filled.BrokenImage, size(20.dp), tint = c.textTertiary.copy(alpha = 0.5f))`。
- **要点**：占位填满外部传入尺寸，因此不会引起布局跳动。

#### 1.1.9 `FloatingBottomBar`

- **位置**：`ui/components/FloatingBottomBar.kt:111`
- **用途**：悬浮胶囊底栏（可拖、可吸附、连续染色）。
- **参数**：`items: List<BottomBarItem>`、`selectedIndex: Int`、`onSelect: (Int) -> Unit`、`modifier: Modifier = Modifier`；`items` 为空时直接 `return`。
- **配套数据类型**：`data class BottomBarItem(label: String, icon: ImageVector)`（`FloatingBottomBar.kt:61`）
- **内部拼成**：`Box(navigationBarsPadding, padding(lg/lg/sm/sm))` → `GlassSurface(level = Raised, shape = RoundedCornerShape(percent = 50), tinted = true)` → `BoxWithConstraints(height(64.dp))` → 先画滑动指示器 `Box(offset(x = itemWidth * pill.value), width(itemWidth), padding(xs/sm), graphicsLayer(scale), clip(percent 50), background(c.accentSoft))` → `Row(selectableGroup, pointerInput { detectHorizontalDragGestures })` → 每栏 `Column(weight(1f), clickable, semantics { role = Tab; selected })` { `Icon(size(22.dp), graphicsLayer(scale))` + `Text(labelSmall)` }。
- **变体 / 状态**：
  - 拖动中：`lift` 0→1，指示器放大 `1f + 0.04f * lift`，图标放大 `1f + 0.08f * weight * lift`。
  - 配色连续插值：`lerp(c.textSecondary, c.accent, weight)`，`weight = tabSelectionWeight(pill, index)`。
  - 弹簧随风格：`springy` 时 `spring(dampingRatio = 0.45f, stiffness = 900f)`（拖动）与 `spring(dampingRatio = 0.72f, stiffness = 420f)`（吸附）；否则 `spring(dampingRatio = 1f, stiffness = 700f)` 与 `spring(dampingRatio = 1f, stiffness = 300f)`。
  - 纯函数（可测）：`clampPill`（`:68`）、`nearestTab`（`:72`）、`tabIndexAt`（`:81`）、`tabSelectionWeight`（`:91`）。

#### 1.1.10 `GlassTopBar`

- **位置**：`ui/components/GlassTopBar.kt:34`
- **用途**：毛玻璃顶栏，带橙蓝薄层，自己处理状态栏内边距（edge-to-edge）。
- **参数**：`title: String`、`modifier: Modifier = Modifier`、`subtitle: String? = null`、`navigation: (@Composable () -> Unit)? = null`、`actions: @Composable RowScope.() -> Unit = {}`
- **内部拼成**：`GlassSurface(level = Raised, shape = RoundedCornerShape(0.dp), tinted = true)` → `Row(statusBarsPadding, height(Sizing.appBar = 56.dp), padding(horizontal = Spacing.md))` { `navigation?.invoke()` + `Box(weight(1f))` { `Column` { 标题 `titleLarge` + 条件副标题 `bodyMedium` } } + `Row(spacedBy(Spacing.xxs), content = actions)` }。
- **变体**：`navigation` 为 null 时左侧起 `Spacing.sm` 内边距；`subtitle` 为 null 时只画一行。
- **调用点**：18 处（几乎所有非阅读页的顶层）。

#### 1.1.11 `LoadMoreFooter`

- **位置**：`ui/components/LoadMoreFooter.kt:35`
- **用途**：列表末尾的「续加」触发器，把触发器本身当列表项渲染。
- **参数**：`loading: Boolean`、`modifier: Modifier = Modifier`、`error: String? = null`、`exhausted: Boolean = false`、`onLoadMore: () -> Unit`、`onRetry: () -> Unit = onLoadMore`
- **内部拼成**：`Box(fillMaxWidth, height(56.dp), center)` → `when`：
  - `loading` → `CircularProgressIndicator(color = c.accent, strokeWidth = 2.dp, size(22.dp))`
  - `error != null` → `TextButton(onRetry)` + `Text("加载失败，点击重试", labelSmall, c.accent)`
  - `exhausted` → `Text("已经到底了", labelSmall, c.textTertiary)`
  - 否则 → `LaunchedEffect(Unit) { onLoadMore() }` + `Text("上滑加载更多", labelSmall, c.textTertiary)`
- **变体**：上述 4 态。
- **调用点**：9 处。

#### 1.1.12 `LoadingBox`

- **位置**：`ui/components/StateBox.kt:28`
- **用途**：整屏加载中。
- **参数**：`modifier: Modifier = Modifier`
- **内部拼成**：`Box(fillMaxSize, center)` → `CircularProgressIndicator(color = JmTheme.colors.accent, strokeWidth = 2.5.dp, size(36.dp))`。
- **调用点**：14 处。

#### 1.1.13 `MessageState`

- **位置**：`ui/components/StateBox.kt:45`
- **用途**：空态 / 错误态（刻意做成同一个组件）。
- **参数**：`title: String`、`modifier: Modifier = Modifier`、`description: String? = null`、`icon: ImageVector = Icons.Filled.Inbox`、`onRetry: (() -> Unit)? = null`
- **内部拼成**：`Column(fillMaxSize, padding(Spacing.xl), center)` { `Icon(size(40.dp), tint = c.textTertiary)` + `Text(title, titleMedium, c.text, center)` + 条件 `Text(description, bodyLarge, c.textSecondary, center)` + 条件 `TextButton` + `Text("重试", c.accent)` }。
- **变体**：`description` 有无、`onRetry` 有无，共 4 种组合。
- **调用点**：15 处。

#### 1.1.14 `ErrorBox`

- **位置**：`ui/components/StateBox.kt:90`
- **用途**：网络错误态。
- **参数**：`message: String`、`modifier: Modifier = Modifier`、`onRetry: (() -> Unit)? = null`
- **内部拼成**：直接转调 `MessageState(title = "加载失败", description = message, icon = Icons.Filled.CloudOff, onRetry = ...)`。
- **调用点**：17 处。

#### 1.1.15 `AmbientBackdrop`

- **位置**：`ui/components/AmbientBackdrop.kt:41`
- **用途**：整个应用的采样底：渐变网格 + 可选壁纸 + 可选莫奈上色层 + 壁纸遮罩。
- **参数**：`modifier: Modifier = Modifier`、`content: @Composable BoxScope.() -> Unit`
- **内部拼成**：`Box(fillMaxSize)` → ① `Box(fillMaxSize.ambientBase())` → ② 条件 `AsyncImage(model = wall.url, contentScale = Crop, colorFilter = ColorMatrix().setToSaturation(saturate), modifier = fillMaxSize.blur(effectiveBlur))` → ③ 条件莫奈层 `Box(fillMaxSize.background(Brush.linearGradient(colors = listOf(monet[0].copy(alpha = 0.16f), monet[1].copy(alpha = 0.10f), monet[2].copy(alpha = 0.14f)), start = Offset.Zero, end = Offset.Infinite)))` → ④ 条件遮罩 `Box(fillMaxSize.background(Color(0xFF06080E).copy(alpha = (wall.dim + spec.wallpaperScrim).coerceIn(0f, 0.86f))))` → `content()`。
- **变体**：壁纸开/关（`wall.showsImage`）、莫奈开关（`options.monetBlur` 且 `monetTints.size >= 3`）。
- **调用点**：`app/src/main/kotlin/com/jmnext/MainActivity.kt:97`（ui/ 之外）。

#### 1.1.16 `Modifier.ambientBase()`

- **位置**：`ui/components/AmbientBackdrop.kt:146`
- **用途**：底的「底」：一层线性渐变 + 三团径向光斑；不透明，阅读页与设置页共用。
- **参数**：无。
- **组成**：`drawWithCache`，`onDrawBehind` 里：底 `Brush.linearGradient(colors = c.backdrop, start = Offset(0f, 0f), end = Offset(w * 0.35f, h))`；`glow = spec.backdropGlow` 大于 0 时再画三团 `Brush.radialGradient`：
  - A：center `(w * 0.12f, -h * 0.08f)`，radius `maxOf(w, h) * 0.75f`，alpha `0.45f * glow`
  - B：center `(w * 0.88f, h * 0.04f)`，radius `maxOf(w, h) * 0.68f`，alpha `0.34f * glow`
  - C：center `(w * 0.62f, h * 1.08f)`，radius `maxOf(w, h) * 0.72f`，alpha `0.30f * glow`
- **变体**：莫奈开启时三团分别改用 `primary/secondary/tertiary`；`backdropGlow = 0f` 的风格（Miuix、Material）不画光斑。
- **配套纯函数**：`internal data class BackdropFrosting(blur: Dp, saturate: Float)`（`:215`）、`internal fun backdropFrosting(userBlur: Int, styleSaturate: Float)`（`:217`）——用户模糊为 0 时不模糊且饱和度不变。

#### 1.1.17 `JmTheme`（主题入口）

- **位置**：`ui/theme/Theme.kt:224`
- **用途**：应用主题入口，一次性算出 palette / spec / typography / ColorScheme 并注入。
- **参数**：`darkTheme: Boolean = isSystemInDarkTheme()`、`dynamicColor: Boolean = false`、`style: ThemeStyle = ThemeStyle.Default`、`options: UiOptions = UiOptions()`、`content: @Composable () -> Unit`
- **内部拼成**：`CompositionLocalProvider(LocalJmPalette, LocalJmSpec, LocalUiOptions)` → `MaterialTheme(colorScheme, typography = typographyOf(spec.type, shadow = readabilityShadow(darkTheme) 或 null))`。
- **变体**：
  - `LiteFeatures.ENABLED` 时强制 `ThemeStyle.Miuix` + `UiOptions()`（`Theme.kt:239-240`）。
  - `style == Material` 时配色与 ColorScheme **都**来自 M3 的 `ColorScheme`（`Theme.kt:265`、`:276`）。
  - 动效性格：`MotionStyle.Standard / Plasma / HyperOS` 覆盖 `spec.motion`。
- **读取入口**：`object JmTheme`（`Theme.kt:307`）提供 `colors` / `spec` / `motion` 三个 `@Composable @ReadOnlyComposable` 属性。

#### 1.1.18 `jmShape(radius: Dp): Shape`

- **位置**：`ui/theme/Shapes.kt:169`（`@Composable @ReadOnlyComposable`）
- **用途**：按当前风格取圆角形状；Miuix（`SurfaceCraft.Card`）返回 `SquircleShape(radius)`，其余返回 `RoundedCornerShape(radius)`。
- **参数**：`radius: Dp`
- **调用点**：16 处。
- **注意**：胶囊形（`Radius.pill`）不走它。

#### 1.1.19 `SquircleShape`（class，非 composable）

- **位置**：`ui/theme/Shapes.kt:37`
- **参数**：`radius: Dp`、`steps: Int = 12`、`exponent: Float = 5f`
- **实现**：`createOutline` 用 `superellipseRoundRect(width, height, r, steps, exponent)`（`:80`）生成轮廓点列，`Path.lineTo` 连成一圈。
- **配套纯函数**：`superellipseUnit(theta, exponent)`（`:140`）、`cornerInsetRatio(exponent)`（`:154`）。
- **变体**：只有 Miuix 用到；`exponent` 越大越方。

#### 1.1.20 `Modifier.jmSharedElement(key: String?)`

- **位置**：`ui/SharedTransition.kt:89`
- **用途**：按「触发前位置 → 触发后位置」做共享元素动画（封面从列表飞到详情）。
- **参数**：`key: String?`（null 或不在共享容器里时什么都不做）
- **组成**：`rememberSharedContentState(key, sharedContentConfig())` + `sharedElement(contentState, visibility, bounds, placeholderSize = PlaceholderSize.ContentSize, renderInOverlayDuringTransition = true)`；`bounds = BoundsTransform { _, _ -> tween(durationMillis = motion.base, easing = motion.enter) }`。
- **变体**：由 `LocalSharedElementEnabled`（`:43`）控制开关；`sharedContentConfig()`（private，`:60`）把开关翻译成库的 `SharedContentConfig`，且 `shouldKeepEnabledForOngoingAnimation = false`。
- **配套**：`fun jmComicSharedKey(comicId: String): String = "jm-cover-$comicId"`（`:139`）。

#### 1.1.21 `Modifier.jmVanishWhenLeaving()`

- **位置**：`ui/SharedTransition.kt:164`
- **用途**：退出时立刻隐藏本页内容（不做淡出），只留封面在飞。
- **参数**：无。
- **组成**：读 `visibility.transition.targetState != EnterExitState.Visible` 且 `shared?.isTransitionActive == true` 时返回 `graphicsLayer { alpha = 0f }`，否则原样返回。

#### 1.1.22 `NoticeHost`

- **位置**：`ui/Notices.kt:61`
- **用途**：把 `Notices` 的瞬时反馈显示出来（文档要求放在应用根部）。
- **参数**：`enterMs: Int = 220`、`exitMs: Int = 120`、`infoMs: Long = 2400`、`errorMs: Long = 4800`
- **内部拼成**：`Box(fillMaxSize, BottomCenter)` → `AnimatedVisibility(fadeIn + slideInVertically{ it / 3 }, fadeOut + slideOutVertically{ it / 3 })` → `Text(bodyMedium, padding(bottom = 28.dp), clip(RoundedCornerShape(10.dp)), background(errorContainer 或 surfaceVariant), padding(horizontal = 16.dp, vertical = 10.dp))`。
- **变体**：`NoticeKind.ERROR` 用 `onErrorContainer` / `errorContainer`，`INFO`、`SUCCESS` 用 `onSurface` / `surfaceVariant`。
- **实际状态**：**app 模块内 0 处调用**。`JmNavHost.kt:436-445` 改用 Material3 的 `SnackbarHost` + `SnackbarHostState.showSnackbar` 显示同一条 `Notices.state`。所以这个组件在 Android 侧目前是死代码；`desktop/` 有一份同名同签名的实现（`desktop/src/main/kotlin/com/jmnext/desktop/Notices.kt:61`，被 `desktop/.../Main.kt:396` 调用）。

#### 1.1.23 `JmNavHost` 与 `DockedBottomBar`

- **`JmNavHost`**：`ui/JmNavHost.kt:339`
  - **用途**：应用导航图 + 底栏（4 个主 Tab 才显示）。
  - **参数**：`readerMode: ReaderMode`、`onReaderModeChange: (ReaderMode) -> Unit`、`themeMode: ThemeMode`、`onThemeModeChange: (ThemeMode) -> Unit`、`dynamicColor: Boolean`、`onDynamicColorChange: (Boolean) -> Unit`、`themeStyle: ThemeStyle`、`onThemeStyleChange: (ThemeStyle) -> Unit`、`isDark: Boolean`、`uiOptions: UiOptions`、`onUiOptionsChange: (UiOptions) -> Unit`
  - **内部拼成**：`SharedTransitionLayout` → `CompositionLocalProvider(LocalSharedTransitionScope)` → `Scaffold(snackbarHost = SnackbarHost, containerColor = Color.Transparent, bottomBar = { ... })` → `NavHost`。
  - **变体**：底栏两套——`uiOptions.floatingBottomBar` 为 true 用 `FloatingBottomBar`，否则 `DockedBottomBar`；Tab 之间横滑转场（`TAB_SLIDE_FRACTION`，位移 0.3 屏宽），其余目的地只淡入淡出；`LocalBottomBarInset` 由底栏实测高度下发。
  - **Tab 表**：`internal enum class MainTab`（`JmNavHost.kt:101`）：Home「首页」`Icons.Filled.Whatshot`、Category「分类」`Icons.Filled.Sell`、Search「搜索」`Icons.Filled.Search`、Profile「我的」`Icons.Filled.Person`。
- **`DockedBottomBar`**（private）：`ui/JmNavHost.kt:842`
  - **参数**：`currentRoute: String?`、`onSelect: (MainTab) -> Unit`
  - **内部拼成**：`GlassSurface(level = Raised, shape = RoundedCornerShape(0.dp), tinted = true)` → `NavigationBar(containerColor = Color.Transparent, tonalElevation = 0.dp)` → `NavigationBarItem(icon = Icon, label = Text(labelSmall))` 每 Tab 一个。

---

### 1.2 跨屏复用的屏幕级组件（B 类中的 9 个真组件）

> 另外 18 个 B 类条目是页面入口（`HomeScreen`、`DetailScreen`、`ReaderScreen`…），它们各自只被路由调用一次，不按「可复用组件」计。完整名单见 §1.3 末尾。

#### 1.2.1 `RandomFab` —— 首页随机骰子

- **位置**：`ui/screens/home/RandomFab.kt:63`
- **用途**：首页右下角浮动圆按钮，单击随机跳一本（骰子转两圈），长按去随机列表页。
- **参数**：`repo: JmRepository`、`bottomInset: Dp`、`onOpenComic: (ComicTarget) -> Unit`、`onOpenRandomList: () -> Unit`、`modifier: Modifier = Modifier`
- **内部拼成**：`Box(padding(end = Spacing.lg, bottom = bottomInset + Spacing.md))` → `GlassSurface(level = Raised, shape = RoundedCornerShape(percent = 50), size(52.dp), combinedClickable)` → `Icon(Icons.Filled.Casino, padding(14.dp), rotate(spin.value))`。
- **变体 / 动效**：`Animatable(0f)` 每次点击累加 `+720f`，`tween(600)`。

#### 1.2.2 `DailyQuickFab` —— 快捷签到

- **位置**：`ui/screens/home/RandomFab.kt:128`
- **用途**：首页骰子上方的日历按钮，点一下直接签到；已签到显示对钩。
- **参数**：`repo: JmRepository`、`bottomInset: Dp`、`modifier: Modifier = Modifier`
- **内部拼成**：`Box(padding(end = Spacing.lg, bottom = bottomInset + Spacing.md + 52.dp + Spacing.sm))` → `GlassSurface(level = Raised, tinted = true, shape = RoundedCornerShape(percent = 50), size(52.dp), combinedClickable(enabled = !disabled))` → `Icon(if (signedToday) Icons.Filled.Check else Icons.Filled.CalendarMonth, padding(14.dp), tint = c.text)`；另有一层 `GlassSurface(level = Card, shape = RoundedCornerShape(Radius.pill))` + `Text(labelMedium, padding(horizontal = Spacing.md, vertical = Spacing.xs))` 显示提示。
- **变体**：未登录直接 `return`；已签到 / 未签到两种图标；`disabled = signedToday || alreadyPrompted >= MAX_ALREADY_PROMPTS (5)`（`:250`）。

#### 1.2.3 `DailyHistorySection` —— 签到历史日历

- **位置**：`ui/screens/profile/DailyHistorySection.kt:47`
- **用途**：签到历史：年份筛选 + 该年记录缩略图 + 就地看大图；默认折叠且不请求。
- **参数**：`repo: JmRepository`、`uid: String`、`modifier: Modifier = Modifier`
- **内部拼成**：`Column` { `Row` { 「查看/收起签到历史」`Text(bodySmall, c.accent, clickable)` + 条件「读取中…」 } + 条件错误/空文案 + `LazyRow` 年份 `Text(clip(RoundedCornerShape(Radius.pill)), background(accent 或 surface2), padding(horizontal = Spacing.md, vertical = Spacing.xs))` + `LazyRow` 记录 `Box` { `AsyncImage(size(64.dp), clip(RoundedCornerShape(Radius.sm)), Crop)` 或纯色占位 `Box(size(64.dp), background(c.surface2))` + 月份角标 `Text(labelSmall, background(c.accent))` } + 条件大图 `AsyncImage(fillMaxWidth, height(220.dp), ContentScale.Fit, clip(RoundedCornerShape(Radius.md)))` }。
- **变体**：折叠 / 展开；加载中 / 错误 / 无历史 / 有记录；有图 / 无图（无图也保留格子）。

#### 1.2.4 `FolderNameDialog`

- **位置**：`ui/screens/favorites/FolderDialogs.kt:48`
- **用途**：新建 / 重命名收藏夹（两者只差标题与初值）。
- **参数**：`title: String`、`initialName: String`、`onDismiss: () -> Unit`、`onConfirm: (String) -> Unit`
- **内部拼成**：`AlertDialog` { title `Text` + `OutlinedTextField(singleLine = true, label = "收藏夹名称")` + `confirmButton = TextButton(enabled = name.isNotBlank()) { Text("确定", c.accent) }` + `dismissButton = TextButton { Text("取消", c.textSecondary) }` }。

#### 1.2.5 `FolderDeleteDialog`

- **位置**：`ui/screens/favorites/FolderDialogs.kt:85`
- **用途**：删除收藏夹的二次确认。
- **参数**：`folder: FavoriteFolder`、`onDismiss: () -> Unit`、`onConfirm: () -> Unit`
- **内部拼成**：`AlertDialog` + 说明文案 + `TextButton { Text("删除", c.error) }` + `TextButton { Text("取消", c.textSecondary) }`。

#### 1.2.6 `FolderPickerDialog`

- **位置**：`ui/screens/favorites/FolderDialogs.kt:112`
- **用途**：选择目标收藏夹，用于收藏后归类与列表内移动。
- **参数**：`title: String`、`folders: List<FavoriteFolder>`、`onDismiss: () -> Unit`、`onPick: (FavoriteFolder) -> Unit`、`onSkip: (() -> Unit)? = null`、`skipLabel: String = "不归类"`
- **内部拼成**：`AlertDialog` + 条件空态文案 / `Column(heightIn(max = 320.dp), verticalScroll, spacedBy(Spacing.xxs))` { 每个夹一行 `Row(padding(vertical = Spacing.sm))` + `TextButton { Text(bodyLarge) }` } + `confirmButton = TextButton(if (onSkip != null) skipLabel else "关闭")`。
- **变体**：空列表态；有 `onSkip` 与没有（按钮文案与动作都不同）。

#### 1.2.7 `ManageFoldersDialog`

- **位置**：`ui/screens/favorites/FolderDialogs.kt:166`
- **用途**：收藏夹管理：列出、改名、删除、新建。
- **参数**：`folders: List<FavoriteFolder>`、`onDismiss: () -> Unit`、`onCreate: () -> Unit`、`onRename: (FavoriteFolder) -> Unit`、`onDelete: (FavoriteFolder) -> Unit`
- **内部拼成**：`AlertDialog` + 条件空态文案 / `Column(heightIn(max = 360.dp), verticalScroll)` { 每夹 `Row` { `Text(bodyLarge, weight(1f))` + `TextButton("改名", c.accent)` + `TextButton("删除", c.error)` } } + `confirmButton = TextButton("新建收藏夹", c.accent)` + `dismissButton = TextButton("关闭", c.textSecondary)`。
- **配套**：`sealed interface FolderDialog`（`FolderDialogs.kt:34`）表达互斥的 6 种对话框状态。

#### 1.2.8 `ChapterPickerDialog`

- **位置**：`ui/screens/reader/ChapterPickerDialog.kt:48`
- **用途**：章节选择器，每页 10 话，打开时定位到当前话所在页。
- **参数**：`series: List<SeriesItem>`、`currentChapterId: String`、`onDismiss: () -> Unit`、`onPick: (String) -> Unit`
- **内部拼成**：`AlertDialog` { `Column(heightIn(max = 380.dp), verticalScroll(ScrollState(0) 按页重建), spacedBy(Spacing.xxs))` { 每话 `Surface(shape = jmShape(Radius.xs), color = if (isCurrent) c.accentSoft else c.surface1, onClick)` → `Row(padding(Spacing.md))` { 序号 `Text(titleMedium)` + 名称 `Text(bodyLarge, maxLines = 1)` } } + `confirmButton = Row` { `IconButton(ChevronLeft)` + `Text("${safePage + 1} / $pageCount", labelSmall)` + `IconButton(ChevronRight)` } + `dismissButton = TextButton("关闭")` }。
- **变体**：当前话高亮；上/下页按钮的可用与置灰（`c.accent` / `c.textTertiary`）。

#### 1.2.9 `TagPickerDialog`

- **位置**：`ui/screens/tags/TagFavoritesScreen.kt:226`
- **用途**：把某个作品的标签加进标签收藏，多选后一次提交。
- **参数**：`tags: List<String>`、`onDismiss: () -> Unit`、`onConfirm: (List<String>) -> Unit`
- **内部拼成**：`AlertDialog` { 说明 `Text(labelSmall, c.textTertiary)` + `FlowRow(spacedBy(Spacing.xs))` { 每标签 `Surface(shape = jmShape(Radius.xs), color = if (isSelected) c.accentSoft else c.surface1, onClick)` → `Text("#$tag", labelSmall, padding(horizontal = Spacing.sm, vertical = Spacing.xs))` } + `confirmButton = TextButton(enabled = selected.isNotEmpty()) { Text("收藏 ${selected.size} 个", c.accent) }` + `dismissButton = TextButton("取消")` }。
- **变体**：选中 / 未选中；确认按钮随选中数禁用与改文案。

---

### 1.3 屏幕内私有组件（C 类，55 个）

这些只在所属文件里被调用，但都是 Qt 侧要重建的具体控件。组成列是**实际调用到的** Compose 原语与项目组件。

#### detail/DetailScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `DetailHeader`（:736） | 详情页头部：封面 + 右侧信息 | `Row` + `AsyncImage(width(120.dp), aspectRatio(3/4), jmSharedElement, clip(jmShape(Radius.lg)))` + `Column` + `Text` + 条件点赞 `Surface(shape = jmShape(Radius.xs), color = if (liked) c.accentSoft else c.surfaceSunken)` { `Icon(14.dp)` + `Text` } | 加载态与完成态共用；`liked` 两态（`Favorite`/`FavoriteBorder`）；`blockedAuthor` 弱化作者色；`onLike == null` 时不画点赞 |
| `DetailContent`（:841） | 详情主体：标签、章节、推荐 | `LazyColumn` + `ComicCard` + `LazyRow` + `CategoryChip` + `GlassSurface` + `TextButton` + `ErrorBox` + `LoadingBox` | 加载 / 错误 / 有数据；标签 `blocked` |
| `ChapterPager`（:1237） | 章节目录分页器，每页 10 章 | `GlassSurface` + `Row` + `TextButton` + `IconButton` + `Icon` + `Text` | 页码边界（首页 / 末页） |

#### home/HomeScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `HomeContent`（:166） | 首页内容：推荐分区 + 最新列表 | `LazyColumn` + `LazyRow` + `ComicCard` + `ComicRow` + `GlassSurface` + `Text` + `LoadMoreFooter` + `jmAnimateItem` + `ErrorBox` | 加载 / 错误 / 有数据；续加三态由 `LoadMoreFooter` 承担 |
| `SectionTitle`（:379） | 分区标题 + 「更多」入口 | `Row` + `GlassSurface` + `Text` | `onMore == null` 时不可点；`updatedCount > 0` 时显示更新计数胶囊 |

#### category/CategoryScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `CategoryChipRow`（:376） | 一行分类/排序 chip | `LazyRow` + `FilterChip` + `Text` | 选中 / 未选中 |
| `CategoryGrid`（:412） | 结果网格 | `LazyVerticalGrid` + `ComicCard` + `LoadMoreFooter` + `jmAnimateItem` | 续加三态 |
| `TagBlocks`（:499） | 分组标签（自然换行） | `FlowRow` + `Surface(shape = jmShape(...))` + `Text` + `Column` | 无 |
| `TagFallback`（:545） | 分类树拿不到时的热门标签兜底 | `Box` + `LazyVerticalGrid` + `GlassSurface` + `jmShape` + `Text` + `MessageState` + `ErrorBox` | 加载 / 错误 / 有标签 |

#### week/WeekScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `ChipRow<T>`（:307） | 一行可横向滚动的选择条 | `LazyRow` + `FilterChip` + `Column` + `Text` | 选中 / 未选中 |

#### more/MoreListScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `WeeklyFilterBar`（:319） | 连载更新的两个筛选维度（类型 / 星期） | `Column`（内含两个 `ChipRow`） | 未确认（内部细节由 `ChipRow` 承担） |
| `ChipRow<T>`（:342） | 通用 chip 行 | `LazyRow` + `FilterChip` + `Row` + `Text` | 选中 / 未选中 |

#### profile/ProfileScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `SettingCard`（:1089） | 设置页统一的「卡片 + 标题 + 内容」外壳 | `GlassSurface(level = Card)` + `Column(padding(Spacing.lg))` + 标题 `Text` | 无 |
| `AccountCard`（:199） | 账号卡 | `SettingCard` + `TextButton` + `Row` + `Icon` + `CategoryChip` + `Text` + `EntryButton` | 未登录 / 已登录两态 |
| `RowScope.EntryButton`（:301） | 入口按钮（图标 + 文字 + 角标） | `GlassSurface(level = Card, weight(1f))` + `Row(padding(Spacing.md))` + `Icon(18.dp)` + `Text(bodyMedium)` + 条件角标 `Text(clip(Radius.pill), background(c.accent))` | `badge > 0` 显示 `99+` 或数字 |
| `BlockCard`（:349） | 内容屏蔽汇总与入口 | `SettingCard` + `Text`（组成细节未逐行确认） | 未确认 |
| `AppearanceCard`（:403） | 外观：风格预览 + 深浅 + 动态取色 + 可选项 | `Spacer` + `Row` + `Column` + `Switch` + `Text` + `StyleOption` + `OptionSwitch` | 五套风格 × 深浅 × 各开关 |
| `OptionSwitch`（:628） | 「标题 + 说明 + 开关」一行 | `Row` + `Column` + `Switch` + `Text` | `enabled = false` 时置灰且点不动 |
| `StyleOption`（:676） | 一张风格预览卡（**临时提供该风格的 palette/spec 再画**） | `CompositionLocalProvider(LocalJmPalette, LocalJmSpec)` + `GlassSurface(level = Card, shape = jmShape(Radius.lg))` + 迷你场景 `Box(height(46.dp), background(backdrop.first()))` + `Box(height(28.dp), background(surface2 × fillAlphaScale))` + `Box(size(12.dp), clip(Radius.pill), background(accent))` + `Text` | 选中（显示「当前」）/ 未选中 |
| `WallpaperCard`（:757） | 壁纸设置 | `SettingCard` + `FlowRow` + `Slider` + `TextButton` + `Row` + `Icon` + `Text` + `Surface` + `IconButton` + `OutlinedTextField` | 未确认（来源档位细节未逐行读） |
| `ReadingCard`（:922） | 阅读形态设置 | `SettingCard` + `Text`（其余由回调承担） | 未确认 |
| `PrivacyCard`（:958） | 隐私与广告 | `SettingCard` + `Text` | 未确认 |
| `AboutCard`（:978） | 关于入口 | `SettingCard` + `Text` | 未确认 |
| `ServerCard`（:1009） | 服务端信息与协议对齐 | `SettingCard` + …（组成细节未逐行确认） | 未确认 |
| `InfoRow`（:1108） | 「标签 — 值」一行 | `Row` + `Text` | 无 |
| `DailyCard`（:1138） | 每日签到 | `SettingCard` + `DailyHistorySection`（组成细节未逐行确认） | 未登录 / 已登录 |

#### settings/BlockSettingsScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `BlockSection`（:171） | 一份名单：标题 + 范围 + 逐条可删 + 添加 | `FlowRow` + `Row` + `Icon` + `Column` + `GlassSurface` + `Text` + `Surface` + `IconButton` | 空名单 / 有条目 |
| `AddBlockDialog`（:242） | 添加一条屏蔽词 | `AlertDialog` + `OutlinedTextField` + `TextButton` + `Text` | 输入为空时确认禁用（未逐行确认） |

#### tags/TagFavoritesScreen.kt

该文件**没有 private 组件**：只有 `TagFavoritesScreen`（:122）与 `TagPickerDialog`（:226）两个 public 入口，列表项在页面函数体内直接拼装。

#### about/AboutScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `InfoCard`（:208） | 「卡片 + 标题 + 内容」外壳 | `GlassSurface(level = Card, shape = RoundedCornerShape(Radius.lg))` + `Column(padding(Spacing.md), spacedBy(Spacing.xs))` + 标题 `Text(titleSmall)` + `content()` | 无 |
| `InfoRow`（:222） | 「标签 — 值」一行（两端对齐） | `Row(SpaceBetween)` + `Text(bodySmall)` ×2 | 无 |
| `LinkRow`（:230） | 整行可点的外链 | `TextButton(fillMaxWidth)` + `Text(fillMaxWidth)` | 无 |

#### comments/CommentsScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `CommentCard`（:350） | 单条评论 | `GlassSurface(level = Card, shape = RoundedCornerShape(Radius.md))` + `Column(padding(Spacing.md))` + `Row` { `AsyncImage(size(28.dp), clip(percent 50), background(surfaceSunken))` + 作者 + 条件 `CategoryChip("Lv$level")` + 时间 + 条件删除 `IconButton(size(28.dp)) { Icon(DeleteOutline, 16.dp) }` } + 正文 `Text(plainText(), bodyLarge, textSecondary)` + 条件「N 条回复」 | `onDelete == null` 时无删除按钮；有 / 无等级、时间、回复 |

#### notifications/NotificationsScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `Hint`（:157） | 一行灰色提示文案 | `Text(bodyMedium, c.textSecondary, padding(Spacing.lg))` | 无 |
| `NotificationCard`（:167） | 单条通知 | `GlassSurface(level = Card, shape = RoundedCornerShape(Radius.md), onClick = onMarkRead)` + `Column(padding(Spacing.md), spacedBy(Spacing.xs))` + `Row` { 条件未读圆点 + 标题 `titleSmall` + 日期 } + 追更条目列表或站内通知正文 | 未读 / 已读；追更型（多条作品行）/ 站内通知型（去标签正文） |
| `Box16`（:237） | 未读圆点 | `Box(size(8.dp), clip(CircleShape), background(color))` | **函数名是 `Box16` 但实际画 `8.dp`** |

#### creator/CreatorScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `AuthorCard`（:383） | 画师卡片：头像 + 名字 + 最近更新 | `GlassSurface(level = Card, shape = jmShape(Radius.md), onClick)` + `Row(padding(Spacing.sm))` + `AsyncImage(size(48.dp), clip(percent 50), Crop)` + `Column` + `Text` | 有 / 无 `updateDate` |
| `WorkCard`（:420） | 作品卡片：封面 + 标题 + 来源平台 | `Column` + `Box(fillMaxWidth, aspectRatio(3f/4f), clip(jmShape(Radius.md)), background(c.surfaceSunken))` { `AsyncImage(Crop, clip(jmShape(Radius.md)))` } + `Text` ×2 | 有 / 无 `platform`；`coverUrl` 可为 null |

#### reader/ReaderScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `ReaderBottomBar`（:515） | 阅读页底栏（两行） | `GlassSurface` + `Column(navigationBarsPadding)` + `PageSeekRow` + `ReaderActions` | 悬浮版（`level = Raised`, `RoundedCornerShape(percent = 50)`, `tinted = true`, 动作不带文字）与贴底版（`level = Flyout`, `RoundedCornerShape(0.dp)`, 带文字） |
| `PageSeekRow`（:606） | 上行：上一话 · 当前页 · 滑块 · 总页数 · 下一话 | `Row` + `IconButton` + `Icon(ChevronLeft/Right)` + `Text(labelSmall)` + `Slider(weight(1f))` | `enabled && hasPrev/hasNext` 决定图标颜色 |
| `ReaderActions`（:658） | 下行五个动作 | `Row(SpaceEvenly)` + `ReaderAction` ×5 | `showLabels` 开关；收藏 / 点赞各两态 |
| `ReaderAction`（:709） | 单个动作：图标 +（可选）文字 | `Column(clip(RoundedCornerShape(Radius.sm)), clickable, padding(horizontal = Spacing.sm, vertical = Spacing.xxs))` + `Icon(22.dp)` + 条件 `Text(labelSmall)` | 禁用时用 `c.textTertiary` |
| `ScrollReader`（:735） | 纵向连续滚动 | `LazyColumn` + `itemsIndexed` + `ReaderImage` | 页间无间距；`placeholderRatio = 0.72f` |
| `PagedReader`（:809） | 横向逐页翻动 | `HorizontalPager`（`rememberPagerState`）+ `Box` + `Text` + `GlassSurface`（页码指示） | `showIndicator` 开关 |
| `ZoomableReaderImage`（:884） | 可缩放单页 | `Box` + 手势 | 双指 1x–5x（`MAX_ZOOM = 5f`）、双击适应/`DOUBLE_TAP_ZOOM = 2.5f` |
| `ReaderImage`（:950） | 单页图片（带重试与比例记忆） | `Box` + `Image(painter)` + `PageFallback` | 加载成功 / 加载中 / 失败（失败可点击重试，`attempt++` 换 `memoryCacheKey`） |
| `PageFallback`（:1026） | 图片加载中 / 失败占位 | `Box(ratio 或 fillMaxSize)` + 条件 `background(c.surfaceSunken)` + 条件 `clickable` + `Text(labelSmall, c.accent)` 或 `CircularProgressIndicator(strokeWidth = 2.dp, size(22.dp))` | `text == null` 是加载中（不画底色），否则失败态 |
| `PrefetchPages`（:1086） | 预取当前页前后若干页 | 无 UI，纯副作用 | 窗口由 `LiteFeatures.prefetchBefore/After` 经 `scalePrefetch` 缩放，范围 `1..12` |

#### search/SearchScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `TagBlockedNotice`（:615） | 「有结果被标签屏蔽挡住了」提示条 | `GlassSurface` + `Row` + `Column` + `Icon` + `Text` + `TextButton` | 无 |
| `SuggestionPanel`（:666） | 未搜索时的建议面板 | `Column` + `LazyRow` + `ComicCard` + `TextButton` + `Text` + `MessageState` + `jmAnimateItem` | 有 / 无历史、热门、随机推荐 |
| `WordChips`（:769） | 一组可点检索词 | `FlowRow` + `Surface(shape = jmShape(...))` + `Text` | 无 |
| `SearchFilterRows`（:794） | 两行筛选（排序 / 检索字段） | `Column` + `FilterRow` | 无 |
| `FilterRow`（:815） | 一行筛选 chip | `LazyRow` + `FilterChip` + `Row` + `Text` | 选中（强调色）/ 未选中；`enabled` |
| `DateFilterRows`（:855） | 年份 + 月份筛选 | 由两个 `FilterRow` 组成 | 未选年份时不展示月份 |

#### favorites/FavoritesScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `FolderRow`（:579） | 收藏夹筛选行 | `LazyRow` + `FilterChip` + `Row` + `Text` | 选中 / 未选中 |

#### random/RandomListScreen.kt

| 组件（行号） | 用途 | 组成 | 变体 / 状态 |
| --- | --- | --- | --- |
| `Hint`（:317） | 一行提示文案 | `Box` + `Text` | 无 |

#### 页面入口（B 类中的 18 个）

`AboutScreen`（`about/AboutScreen.kt:63`）、`AuthScreen`（`auth/AuthScreen.kt:169`）、`CategoryScreen`（`category/CategoryScreen.kt:266`）、`CommentsScreen`（`comments/CommentsScreen.kt:239`）、`CreatorScreen`（`creator/CreatorScreen.kt:245`）、`CreatorWorkScreen`（`creator/CreatorWorkScreen.kt:108`）、`DetailScreen`（`detail/DetailScreen.kt:527`）、`AccountListScreen`（`favorites/FavoritesScreen.kt:352`）、`HomeScreen`（`home/HomeScreen.kt:76`）、`MoreListScreen`（`more/MoreListScreen.kt:228`）、`NotificationsScreen`（`notifications/NotificationsScreen.kt:55`）、`ProfileScreen`（`profile/ProfileScreen.kt:119`）、`RandomListScreen`（`random/RandomListScreen.kt:91`）、`ReaderScreen`（`reader/ReaderScreen.kt:284`）、`SearchScreen`（`search/SearchScreen.kt:356`）、`BlockSettingsScreen`（`settings/BlockSettingsScreen.kt:58`）、`TagFavoritesScreen`（`tags/TagFavoritesScreen.kt:122`）、`WeekScreen`（`week/WeekScreen.kt:210`）。

每个页面的通用骨架是：`Column` → `GlassTopBar`（返回键 + 标题）→ 状态分支（`LoadingBox` / `ErrorBox` / `MessageState`）→ 内容（`LazyColumn` 或 `LazyVerticalGrid` + `LoadMoreFooter`）。

---

## 2. 设计 token

### 2.1 排版层级

#### 2.1.1 `object FontSize`（`ui/theme/Tokens.kt:266-274`）—— WindowGlass 用的那一套

| 字段 | 值 | 行号 |
| --- | --- | --- |
| `display` | `28.sp` | :267 |
| `title` | `20.sp` | :268 |
| `subtitle` | `16.sp` | :269 |
| `body` | `15.5.sp`（注释：15.5px → 15.5sp） | :271 |
| `label` | `13.sp` | :272 |
| `caption` | `11.5.sp` | :273 |

字体族：注释明确「刻意不指定」，用平台原生字体栈；`typographyOf` 里全部是 `fontFamily = FontFamily.Default`。

#### 2.1.2 `data class TypeScale`（`ui/theme/ThemeStyle.kt:127-138`）—— 按风格生成

字段：`display`、`title`、`subtitle`、`body`、`label`、`caption`（都是 `TextUnit`）+ `titleWeight`、`subtitleWeight`、`bodyWeight`（`FontWeight`）+ `lineHeightFactor`（`Float`）。

各风格取值（`ThemeStyle.kt`）：

| 风格 | display | title | subtitle | body | label | caption | titleWeight | subtitleWeight | bodyWeight | lineHeightFactor | 行号 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `WindowGlass` | `FontSize.display` (28.sp) | `FontSize.title` (20.sp) | `FontSize.subtitle` (16.sp) | `FontSize.body` (15.5.sp) | `FontSize.label` (13.sp) | `FontSize.caption` (11.5.sp) | `SemiBold` | `Medium` | `Normal` | `1.72f` | :273-278 |
| `Translucent` | 同 WindowGlass（`windowGlass.copy(...)`，未改 `type`） | 同左 | 同左 | 同左 | 同左 | 同左 | 同左 | 同左 | 同左 | 同左 | :289-303 |
| `Miuix` | `30.sp` | `22.sp` | `16.sp` | `15.sp` | `13.5.sp` | `12.sp` | `Bold` | `SemiBold` | `Medium` | `1.45f` | :324-329 |
| `Material` | `28.sp` | `22.sp` | `16.sp` | `16.sp` | `14.sp` | `11.sp` | `Medium` | `Medium` | `Normal` | `1.50f` | :359-364 |
| `FlatBlur` | `28.sp` | `21.sp` | `16.sp` | `15.sp` | `13.sp` | `11.5.sp` | `SemiBold` | `Medium` | `Normal` | `1.60f` | :394-399 |

#### 2.1.3 M3 `Typography` 槽位映射（`ui/theme/Theme.kt:90-133`）

`typographyOf(scale, shadow)` 只覆盖 **6 个槽位**：

| M3 槽位 | fontFamily | fontWeight | fontSize | lineHeight |
| --- | --- | --- | --- | --- |
| `displaySmall` | `FontFamily.Default` | `scale.titleWeight` | `scale.display` | `scale.display * 1.30f` |
| `titleLarge` | 同上 | `scale.titleWeight` | `scale.title` | `scale.title * 1.40f` |
| `titleMedium` | 同上 | `scale.subtitleWeight` | `scale.subtitle` | `scale.subtitle * 1.50f` |
| `bodyLarge` | 同上 | `scale.bodyWeight` | `scale.body` | `scale.body * scale.lineHeightFactor` |
| `bodyMedium` | 同上 | `scale.bodyWeight` | `scale.label` | `scale.label * 1.60f` |
| `labelSmall` | 同上 | `FontWeight.Medium`（写死，不随风格） | `scale.caption` | `scale.caption * 1.50f` |

**`shadow` 参数**：`ultraTranslucent` 开启时传入 `readabilityShadow(darkTheme)`（`Theme.kt:145-149`）——深色：`Shadow(color = Color.Black.copy(alpha = 0.72f), offset = Offset(0f, 1f), blurRadius = 5f)`；浅色：`Shadow(color = Color.White.copy(alpha = 0.90f), offset = Offset(0f, 1f), blurRadius = 5f)`。

**实际被使用的槽位与覆盖情况**（全 `ui/` 统计）：

| 槽位 | 使用次数 | 是否被 `typographyOf` 覆盖 |
| --- | --- | --- |
| `labelSmall` | 104 | 是 |
| `bodyMedium` | 28 | 是 |
| `bodyLarge` | 16 | 是 |
| `titleMedium` | 14 | 是 |
| `bodySmall` | 11 | **否**（落回 M3 库默认值；具体数值不在本仓库源码里，**未确认**） |
| `titleLarge` | 5 | 是 |
| `titleSmall` | 3 | **否**（同上，未确认） |
| `labelLarge` | 2 | **否**（同上，未确认） |
| `labelMedium` | 1 | **否**（同上，未确认） |

> 给 Qt 的提醒：这四个未覆盖槽位是「风格切换时字号不跟着变」的隐蔽来源。它们的实际字号来自 `androidx.compose.material3` 的默认 `Typography`，本仓库没有写死任何数值，因此这里不给数字。

### 2.2 间距与尺寸

#### 2.2.1 `object Spacing`（`ui/theme/Tokens.kt:46-55`）—— 4dp 基准栅格

| 字段 | 值 | 行号 |
| --- | --- | --- |
| `xxs` | `2.dp` | :48 |
| `xs` | `4.dp` | :49 |
| `sm` | `8.dp` | :50 |
| `md` | `12.dp` | :51 |
| `lg` | `16.dp` | :52 |
| `xl` | `24.dp` | :53 |
| `xxl` | `32.dp` | :54 |

实际使用频次（ui/ 内）：`sm` 97、`lg` 93、`xs` 71、`md` 59、`xxs` 23、`xxl` 7、`xl` 2。

#### 2.2.2 `object Sizing`（`ui/theme/Tokens.kt:57-66`）

| 字段 | 值 | 行号 | 实际使用 |
| --- | --- | --- | --- |
| `appBar` | `56.dp`（`--appbar-h`） | :59 | `GlassTopBar.kt:53` |
| `accentBar` | `3.dp`（MIUI 强调条宽度） | :61 | `Glass.kt:250` |
| `hairline` | `1.dp`（玻璃发丝描边） | :63 | **0 处**（实际用的是 `SurfaceSpec.hairline`） |
| `minTouch` | `48.dp`（无障碍最小触控高度） | :65 | **0 处** |

#### 2.2.3 组件级硬编码尺寸（不属 token，但 Qt 侧要照抄）

`CardSizes.row = 132.dp`、`CardSizes.grid = 148.dp`（`ComicCard.kt:51-57`）；`ComicRow` 封面 `76.dp`（`ComicCard.kt:172`）；`CoverFallback` 图标 `20.dp`（`ComicCard.kt:276`）；`FloatingBottomBar` 高 `64.dp`、图标 `22.dp`（`FloatingBottomBar.kt:186`、`:265`）；`GlassCircle` 直径 `40.dp`（`Glass.kt:264`）；`LoadingBox` 圈 `36.dp` / `strokeWidth 2.5.dp`（`StateBox.kt:32-33`）；`MessageState` 图标 `40.dp`（`StateBox.kt:62`）；`LoadMoreFooter` 高 `56.dp`、圈 `22.dp` / `strokeWidth 2.dp`（`LoadMoreFooter.kt:45,50-52`）；`RandomFab` / `DailyQuickFab` 直径 `52.dp`、图标内边距 `14.dp`（`RandomFab.kt:81,106,176,223`）；`DetailHeader` 封面 `120.dp`（`DetailScreen.kt:755-757`）；`AuthorCard` 头像 `48.dp`（`CreatorScreen.kt:396`）；`CommentCard` 头像 `28.dp`、删除按钮 `28.dp` / 图标 `16.dp`（`CommentsScreen.kt:363,404,411`）；`NotificationsScreen.Box16` 圆点 `8.dp`（`NotificationsScreen.kt:239`）；`DailyHistorySection` 缩略图 `64.dp`、大图 `height(220.dp)`（`DailyHistorySection.kt:160,198`）；`RandomListScreen` 网格封面 `aspectRatio(0.72f)`、列表封面 `82.dp × 114.dp`（`RandomListScreen.kt:252,276-277`）；`ReaderAction` 图标 `22.dp`（`ReaderScreen.kt:715`）。

### 2.3 圆角

#### 2.3.1 `object Radius`（`ui/theme/Tokens.kt:28-44`）

读法是 `@Composable @ReadOnlyComposable`，值来自当前风格的 `RadiusScale`：

| 字段 | 来源 | 行号 |
| --- | --- | --- |
| `xs` | `scale.xs`（`--r-xs`） | :33 |
| `sm` | `scale.sm`（`--r-sm`） | :35 |
| `md` | `scale.md`（`--r-md`） | :37 |
| `lg` | `scale.lg`（`--r-lg`） | :39 |
| `xl` | `scale.xl`（`--r-xl`） | :41 |
| `pill` | **写死 `999.dp`**，与风格无关 | :43 |

#### 2.3.2 `data class RadiusScale`（`ui/theme/ThemeStyle.kt:63-69`）—— 五套风格各自的取值

| 风格 | xs | sm | md | lg | xl | 行号 |
| --- | --- | --- | --- | --- | --- | --- |
| `WindowGlass` | `2.dp` | `4.dp` | `6.dp` | `8.dp` | `12.dp` | :258 |
| `Translucent` | `2.dp` | `4.dp` | `6.dp` | `8.dp` | `12.dp` | :289（`windowGlass.copy`，未改 radius） |
| `Miuix` | `4.dp` | `8.dp` | `12.dp` | `16.dp` | `24.dp` | :311 |
| `Material` | `4.dp` | `8.dp` | `12.dp` | `12.dp` | `28.dp` | :346 |
| `FlatBlur` | `8.dp` | `12.dp` | `16.dp` | `20.dp` | `28.dp` | :382 |

配套规则：Miuix（`SurfaceCraft.Card`）的所有 `jmShape(r)` 变成**连续圆角** `SquircleShape(r)`（`exponent = 5f`、`steps = 12`），其余风格是普通 `RoundedCornerShape`；`Radius.pill` 不走 `jmShape`。

实际使用频次：`pill` 11、`md` 11、`sm` 10、`xs` 7、`lg` 6。

### 2.4 颜色角色

#### 2.4.1 `data class JmPalette`（`ui/theme/Tokens.kt:134-183`）—— 字段清单（按用途分组）

| 分组 | 字段 | 语义 |
| --- | --- | --- |
| 强调色 | `accent` | 主强调色 |
| | `accentHover` | 悬停态 |
| | `accentActive` | 按下态 |
| | `accentFg` | 强调色之上的前景色 |
| | `accentSoft` | 强调色薄底（chip、选中态容器） |
| | `accentGlow` | 强调色辉光 |
| 表面（数字越大越靠近用户） | `surfaceMica` | Mica 底 / 顶层容器底 |
| | `surface1` | 卡片（GlassLevel.Card） |
| | `surface2` | 浮起（GlassLevel.Raised） |
| | `surface3` | 浮层（GlassLevel.Flyout） |
| | `surfaceSunken` | 下陷（占位底、输入槽） |
| | `surfaceHover` | 悬停叠色 |
| | `surfaceActive` | 按下叠色 |
| 描边 | `stroke` | 常规描边 / 发丝线 |
| | `strokeStrong` | 强描边 |
| | `strokeInner` | 内高光（玻璃上缘反光） |
| 文字 | `text` | 正文 |
| | `textSecondary` | 次要 |
| | `textTertiary` | 三级 / 禁用 |
| | `textOnAccent` | 强调色上的文字 |
| 语义色 | `error` | 错误 |
| | `errorFg` | 错误底上的文字 |
| 渐变薄层 | `tintWarm` | MIUI 橙→蓝薄层的暖端 |
| | `tintCool` | 冷端 |
| 环境底 | `backdrop: List<Color>` | 三色渐变底（`ambientBase` 的线性和光斑采样源） |
| 动态取色 | `monetTints: List<Color> = emptyList()` | `primary/secondary/tertiary` 三个色相，仅供「莫奈套用在模糊上」；空列表 = 无动态取色 |

#### 2.4.2 各有几份调色板，各自来自哪里

| 调色板 | 位置 | 说明 |
| --- | --- | --- |
| `LightPalette` | `Tokens.kt:186-222` | WindowGlass 浅色；`surfaceMica = Color(0xB8F6F7FA)` 等 |
| `DarkPalette` | `Tokens.kt:225-261` | WindowGlass 深色 |
| `TranslucentLight` / `TranslucentDark` | `Palettes.kt:29-42` / `:44-57` | `LightPalette.copy(...)` / `DarkPalette.copy(...)`，只改表面与描边 |
| `FlatBlurLight` / `FlatBlurDark` | `Palettes.kt:67-77` / `:79-92` | 同上是 copy，描边类全部清零（`Color(0x00000000)`） |
| `MiuixLight` / `MiuixDark` | `Palettes.kt:103-130` / `:132-159` | 同上是 copy，强调色换成 HyperOS 蓝、表面换成实色 |
| Material | `Palettes.kt:180-219`（`ColorScheme.toJmPalette()`） | **没有独立色板**：整份来自 M3 的 `ColorScheme`，`monetTints = listOf(primary, secondary, tertiary)` |
| M3 基线 | `Palettes.kt:222-223`（`baselineM3Palette(dark)`） | 关动态取色时的 Material：`lightColorScheme()` / `darkColorScheme()` |
| 取色入口 | `Palettes.kt:226-232`（`paletteFor(style, dark)`） | 按风格分派；Material 走基线 M3 |

具体色值请直接看 `../THEME-TOKENS.md`（那里是带行号的原文摘录），本文不重复。

**投影 token**（颜色体系的一部分）：

| 名称 | 位置 | 值 |
| --- | --- | --- |
| `ElevationBase.sm` / `.card` / `.flyout` | `Tokens.kt:117-121` | `2.dp` / `6.dp` / `12.dp`（基准值，不一定生效） |
| `Elevation.sm` / `.card` / `.flyout` | `Tokens.kt:124-128` | 读当前风格 `surface.shadowOf(0/1/2)` |
| 各风格 `shadows` | `ThemeStyle.kt:271` / `:298` / `:321` / `:357` / `:392` | WindowGlass `listOf(0.dp, 0.dp, 0.dp)`；Translucent 同；Miuix `listOf(0.dp, 1.dp, 3.dp)`；Material `listOf(0.dp, 1.dp, 6.dp)`；FlatBlur `listOf(0.dp, 0.dp, 0.dp)` |

### 2.5 动效曲线与时长

#### 2.5.1 `object JmEasing`（`ui/theme/Theme.kt:45-79`）—— 8 条曲线

| 字段 | CSS / Qt 等价 | `CubicBezierEasing` 参数 | 行号 |
| --- | --- | --- | --- |
| `standard` | `--ease-standard` `cubic-bezier(0.33, 0, 0.67, 1)` | `(0.33f, 0f, 0.67f, 1f)` | :47 |
| `decel` | `--ease-decel` `cubic-bezier(0.1, 0.9, 0.2, 1)` | `(0.1f, 0.9f, 0.2f, 1f)` | :50 |
| `fluent` | `--ease-fluent` `cubic-bezier(0.16, 1, 0.3, 1)` | `(0.16f, 1f, 0.3f, 1f)` | :53 |
| `outCubic` | Qt `QEasingCurve::OutCubic` | `(0.215f, 0.61f, 0.355f, 1f)` | :61 |
| `inCubic` | Qt `QEasingCurve::InCubic` | `(0.55f, 0.055f, 0.675f, 0.19f)` | :64 |
| `inOutQuad` | Qt `QEasingCurve::InOutQuad` | `(0.455f, 0.03f, 0.515f, 0.955f)` | :67 |
| `hyperOS` | HyperOS 进入曲线 | `(0.35f, 0f, 0.10f, 1f)` | :75 |
| `hyperOSOut` | HyperOS 退出曲线 | `(0.5f, 0f, 0.30f, 1f)` | :78 |

`decel` 与 `inOutQuad` 在本次清点中**未被任何组件直接引用**（未确认是否有间接使用）。

#### 2.5.2 `object Motion`（`ui/theme/Tokens.kt:74-83`）—— 基准时长（毫秒）

| 字段 | 值 | 行号 |
| --- | --- | --- |
| `FAST` | `120`（`--dur-fast`） | :76 |
| `BASE` | `200`（`--dur`） | :78 |
| `SLOW` | `320`（`--dur-slow`） | :80 |
| `FADE_ONLY` | `80`（小于它的入场不做位移，只淡入） | :82 |

#### 2.5.3 `data class MotionSpec`（`ui/theme/ThemeStyle.kt:172-209`）

字段：`fast: Int`、`base: Int`、`slow: Int`、`springy: Boolean`、`enter: Easing = JmEasing.fluent`、`exit: Easing = JmEasing.standard`。

各风格（`ThemeStyle.kt`）：

| 风格 | fast | base | slow | springy | enter | exit | 行号 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `WindowGlass` | `Motion.FAST` (120) | `Motion.BASE` (200) | `Motion.SLOW` (320) | `false` | `JmEasing.fluent`（默认） | `JmEasing.standard`（默认） | :279 |
| `Translucent` | 120 | 200 | 320 | `false` | `fluent` | `standard` | :289（copy） |
| `Miuix` | `150` | `240` | `360` | `true` | `fluent` | `standard` | :330 |
| `Material` | `100` | `200` | `300` | `false` | `fluent` | `standard` | :365 |
| `FlatBlur` | `120` | `220` | `320` | `false` | `fluent` | `standard` | :400 |

动效性格覆盖（与风格正交，`MotionStyle`，`ThemeStyle.kt:149-169`）：

| 性格 | fast | base | slow | enter | exit | 行号 |
| --- | --- | --- | --- | --- | --- | --- |
| `Standard` | 用风格自己的值 | 同左 | 同左 | 同左 | 同左 | — |
| `Plasma` | `150` | `250` | `420` | `JmEasing.outCubic` | `JmEasing.inCubic` | :187-193 |
| `HyperOS` | `200` | `350` | `500` | `JmEasing.hyperOS` | `JmEasing.hyperOSOut` | :202-208 |

#### 2.5.4 动画实际用到的参数（散落在组件里）

| 位置 | 参数 |
| --- | --- |
| `Glass.kt:108-112` | `spring(dampingRatio = 0.55f, stiffness = 900f)` 或 `tween(spec.motion.fast)` |
| `FloatingBottomBar.kt:137-141` | `spring(dampingRatio = 0.45f, stiffness = 900f)` / `spring(dampingRatio = 1f, stiffness = 700f)` |
| `FloatingBottomBar.kt:147-153` | 吸附：`spring(dampingRatio = 0.72f, stiffness = 420f)` / `spring(dampingRatio = 1f, stiffness = 300f)` |
| `RandomFab.kt:86` | 骰子旋转：`tween(600)`，累计 `+720f` |
| `Notices.kt:77-78` | `fadeIn(tween(enterMs = 220)) + slideInVertically(tween(220)) { it / 3 }`；退出 `tween(120)` |
| `SharedTransition.kt:99` | 共享元素边界：`tween(durationMillis = motion.base, easing = motion.enter)` |
| `JmNavHost.kt`（Tab 横滑） | 位移 = `TAB_SLIDE_FRACTION`（0.3 屏宽）；常量定义在同一文件 |
| `PageRatio.kt:74` | Coil `crossfade(true)` |

---

## 3. 图标

**图标族：只有 Material Icons Extended**（依赖 `androidx.compose.material:material-icons-extended`，见 `gradle/libs.versions.toml:37`）。

- **没有任何自定义绘制图标**：全项目只有 `ui/theme/Shapes.kt` 里出现 `Path()`，那是圆角形状轮廓，不是图标。`ImageVector.Builder` 在 `ui/` 下 0 处。
- **没有任何图标字体 / 图片图标**（未确认：`res/` 下未做完整清点）。

实际出现的图标名（`Icons.Filled.*` 与 `Icons.AutoMirrored.Filled.*`）：

| 图标 | 次数 | 图标 | 次数 |
| --- | --- | --- | --- |
| `ArrowBack`（AutoMirrored） | 14 | `Notifications` | 1 |
| `Search` | 5 | `NotificationsActive` | 1 |
| `ChevronRight` | 4 | `NotificationsNone` | 3 |
| `BookmarkBorder` | 4 | `NotificationsOff` | 1 |
| `BookmarkAdd` | 4 | `Person` | 1 |
| `Sell` | 3 | `PlayArrow` | 1 |
| `ChevronLeft` | 3 | `Refresh` | 2 |
| `CalendarMonth` | 3 | `SwapHoriz` | 2 |
| `Block` | 3 | `SwapVert` | 2 |
| `MenuBook`（AutoMirrored） | 3 | `Whatshot` | 1 |
| `Favorite` | 2 | `GridView` | 1 |
| `FavoriteBorder` | 2 | `History` | 2 |
| `Bookmark` | 2 | `Inbox` | 1 |
| `DeleteOutline` | 2 | `Folder` | 1 |
| `ChatBubbleOutline` | 2 | `ExpandLess` | 1 |
| `LightMode` | 1 | `ExpandMore` | 1 |
| `DarkMode` | 1 | `Download` | 1 |
| `CloudOff` | 1 | `Delete` | 1 |
| `Close` | 1 | `Check` | 1 |
| `Casino` | 1 | `Brush` | 1 |
| `BrokenImage` | 1 | `Add` | 1 |
| `ViewList`（AutoMirrored） | 1 | `Logout`（AutoMirrored） | 1 |
| `List`（AutoMirrored） | 1 | `DriveFileMove`（AutoMirrored） | 1 |
| `Comment`（AutoMirrored） | 1 | | |

**共 47 个不同图标。**

两处**导入了但未使用**（Qt 侧不要照抄）：

| 导入 | 文件 | 说明 |
| --- | --- | --- |
| `androidx.compose.material.icons.filled.DownloadDone` | `ui/screens/detail/DetailScreen.kt:27` | 全文未引用 |
| `...filled.Logout` / `...filled.MenuBook` / `...filled.DriveFileMove` | `ProfileScreen.kt`、`CreatorScreen.kt`、`FavoritesScreen.kt` | 实际用的是 `AutoMirrored` 版本，非 AutoMirrored 的导入是遗留 |

Qt 侧建议：Material Symbols 有对应字形，直接按上表替换；`AutoMirrored` 在 Qt 侧需要按 `LayoutDirection` 手动镜像。

---

## 4. 图片 / 封面相关组件

### 4.1 封面：`Cover` + `CoverFallback`（`ui/components/ComicCard.kt:249` / `:265`）

- **加载器**：Coil 3（`io.coil-kt.coil3:coil-compose`，`gradle/libs.versions.toml:13` 版本 `3.6.3`）的 `SubcomposeAsyncImage`。
- **为什么用 `SubcomposeAsyncImage` 而不是 `AsyncImage`**（注释原文）：首页同时有几十张图，加载中与失败态若不给统一占位，滚动时会显得很乱。
- **占位**：`CoverFallback`，`Box(fillMaxSize, background(c.surfaceSunken), center)`；失败态额外画 `Icons.Filled.BrokenImage`，`size(20.dp)`，`tint = c.textTertiary.copy(alpha = 0.5f)`。
- **比例固定**：`COVER_RATIO = 3f / 4f`（`ComicCard.kt:40`），注释说明与服务端 `_3x4` 裁切一致，「占位与实图之间不会跳变」。
- **无布局跳动**：占位与失败态都填满外部传入的尺寸。

### 4.2 骨架屏 / shimmer：**不存在**

全 `ui/` 目录内 grep `skeleton`、`shimmer`、`骨架` **0 命中**。加载态一律走三种实现：

1. `LoadingBox`（整屏转圈，`StateBox.kt:28`）
2. `CoverFallback`（封面位置一块 `surfaceSunken` 平色）
3. `PageFallback(text = null)`（阅读页按比例撑高的透明占位 + 转圈，`ReaderScreen.kt:1026`）

Qt 侧若要加骨架屏，属于**新增**，不是复刻。

### 4.3 阅读页图片：`ReaderImage` + `PageFallback`（`ReaderScreen.kt:950` / `:1026`）

- **请求构造**：`internal fun readerImageRequest(context, image, aid, scrambleId, repo, attempt = 0): ImageRequest`（`ui/screens/reader/PageRatio.kt:65`）——`crossfade(true)`；重试时换 `memoryCacheKey("${image.image}#retry$attempt")`；需要反切片时挂 `ScrambleTransformation(aid, page = image.fileNameStem)`。
- **占位比例三档优先级**（`ReaderScreen.kt:970-972`）：`PageRatioMemory.ratioFor(url)` → `PageRatioMemory.albumRatio(aid)` → `placeholderRatio`（连续滚动传 `0.72f`）。
- **比例记忆**：`internal object PageRatioMemory`（`PageRatio.kt:25`）——`byUrl` LRU 上限 `MAX_URLS = 600`，`byAlbum` LRU 上限 `MAX_ALBUMS = 16`，可信区间 `VALID = 0.1f..2.5f`。
- **加载中 / 失败**：`PageFallback(text, placeholderRatio, onClick)`；`text == null` 表示加载中（**不画底色**，只画 `CircularProgressIndicator(strokeWidth = 2.dp, size(22.dp))`）；失败时才画 `background(c.surfaceSunken)` 与 `Text("这一页加载失败 · 点击重试", labelSmall, c.accent)`，并挂 `clickable { attempt++ }`。
- **预取**：`PrefetchPages`（`ReaderScreen.kt:1086`），窗口经 `scalePrefetch(base)` 缩放，范围 `1..12`。
- **缩放**：`ZoomableReaderImage`（`ReaderScreen.kt:884`），`MAX_ZOOM = 5f`、`DOUBLE_TAP_ZOOM = 2.5f`。

### 4.4 直接使用 `AsyncImage` 的位置（没有统一封装，Qt 侧要各自处理）

| 位置 | 用途 | 尺寸 / 比例 |
| --- | --- | --- |
| `ui/components/AmbientBackdrop.kt:73` | 壁纸底 | `fillMaxSize`，`ContentScale.Crop`，可选 `blur` + `ColorMatrix().setToSaturation(saturate)` |
| `ui/screens/detail/DetailScreen.kt:755` | 详情页封面 | `width(120.dp)`, `aspectRatio(3f/4f)`, `jmSharedElement(jmComicSharedKey(comicId))`, `clip(jmShape(Radius.lg))` |
| `ui/screens/creator/CreatorScreen.kt:395` | 画师头像 | `size(48.dp)`, `clip(RoundedCornerShape(percent = 50))` |
| `ui/screens/creator/CreatorScreen.kt:433` | 作品封面 | `aspectRatio(3f/4f)`，底为 `surfaceSunken` |
| `ui/screens/comments/CommentsScreen.kt:363` | 评论头像 | `size(28.dp)`, `clip(percent 50)`, `background(c.surfaceSunken)`，URL 来自 `repo.avatarUrl(comment.photo)` |
| `ui/screens/profile/DailyHistorySection.kt:155` / `:192` | 签到缩略图 / 大图 | `size(64.dp)` + `clip(RoundedCornerShape(Radius.sm))` / `fillMaxWidth().height(220.dp)` + `ContentScale.Fit` |
| `ui/screens/random/RandomListScreen.kt:248` / `:280` | 随机结果网格 / 列表封面 | `aspectRatio(0.72f)` + `clip(RoundedCornerShape(Radius.md))` / `width(82.dp).height(114.dp)` + `clip(RoundedCornerShape(Radius.sm))` |

**没有** `placeholder` / `error` 参数的 `AsyncImage` 调用不会显示任何占位（上述这些位置在加载中就是空白）——这是与 `Cover` 的差别。

---

## 5. 未确认 / 不存在（如实记录）

| 项 | 结论 |
| --- | --- |
| 头像加载失败占位 | 未确认（`AuthorCard` / `CommentCard` 的 `AsyncImage` 没有 `error` 分支） |
| `res/` 下的图标资源 | 未清点 |
| `titleSmall` / `bodySmall` / `labelLarge` / `labelMedium` 的实际字号 | 未确认：落回 M3 库默认值，本仓库未写死 |
| `WallpaperCard`、`ServerCard`、`BlockCard`、`ReadingCard`、`PrivacyCard`、`AboutCard` 的内部逐行组成 | 未逐行确认（已给出行号与用途，组成是按扫描到的调用点归纳） |
| `WeeklyFilterBar`、`AddBlockDialog` 的变体细节 | 未确认 |
| 骨架屏 / shimmer | **不存在** |
| `GlassCircle`、`Modifier.accentBar`、`ui/Notices.kt` 的 `NoticeHost` | 源码里有，但**当前 0 处调用**（`NoticeHost` 只被 `desktop/` 的同名实现替代） |
| `Sizing.hairline`、`Sizing.minTouch` | 已定义，**0 处使用** |
| `JmEasing.decel`、`JmEasing.inOutQuad` | 已定义，本次清点未见直接引用（未确认是否间接使用） |
| `ui/` 之外的 UI 代码 | `desktop/src/main/kotlin/com/jmnext/desktop/`（约 8700 行，含 `BlogTheme.kt` / `Motion.kt` / `ComicCover.kt` / `RemoteImage.kt` / `PageShell.kt` / `SideNav.kt` 等）**本次未清点**；它与 app 侧是两套独立实现（桌面端 `BlogTheme.kt` 自带一套 `Tokens`，圆角 `rSm = 8.dp` / `rMd = 12.dp` / `rLg = 18.dp`） |

---

## 6. 附：Qt 复刻难度最高的五个（判断依据一并给出）

| 组件 | 为什么难 |
| --- | --- |
| `GlassSurface`（`Glass.kt:84`） | 一处分支出五种表面工艺：半透明分层、发丝描边、上缘高光、`BlendMode.Overlay` 的 64×64 平铺噪点、以及仅在 API 31+ 由 `RenderEffect` 实现的真背景模糊；Qt 侧要自己实现背景采样，`QGraphicsBlurEffect` 与 QML `ShaderEffect` 语义都不同。 |
| `SquircleShape`（`Shapes.kt:37`） | 超椭圆 `|x/a|^n + |y/b|^n = 1`（`exponent = 5f`，`steps = 12`）逐点构造轮廓；Qt 的 `QPainterPath` 只有圆弧圆角，要自己生成轮廓，并且必须保证顶点顺序不自交（注释记录了曾经反向扫角导致少填约 9.46% 面积）。 |
| `Modifier.jmSharedElement`（`SharedTransition.kt:89`） | 依赖 Compose 的 `SharedTransitionLayout` + overlay 渲染，语义是「按触发前后的矩形插值，且原位置留空缺」；Qt 没有等价机制，要在窗口或 QML 层自己维护前后矩形、overlay 层与空缺保留。 |
| `FloatingBottomBar`（`FloatingBottomBar.kt:111`） | 胶囊位置是连续 Float、可拖动、松手吸附，且图标与指示器的缩放/染色都随连续权重插值；Qt 要用 `QPropertyAnimation` + 自定义手势才能贴近，临界阻尼弹簧也得手工实现。 |
| `ReaderImage` / `PageFallback`（`ReaderScreen.kt:950` / `:1026`） | 三档比例回退 + 每页实测比例回写 LRU + 预取窗口（`1..12`）+ 反切片解码转换；Qt 侧要接自己的图片管线，且「预取与显示必须用完全相同的请求」这条约束在 Qt 缓存键体系里要重新设计。 |
