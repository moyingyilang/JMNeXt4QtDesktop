# 03 视觉效果与动画清点（Compose Multiplatform → Qt 桌面可行性）

**范围**：只清点**表面工艺、绘制效果、动画**三件事，目的是回答「哪些能在 Qt Widgets/QSS 里做、哪些必须 Qt Quick(QML) 或自绘」。
配色数值不在本文（见 [THEME-TOKENS.md](../THEME-TOKENS.md)）；布局/交互行为不在本文。

**清点对象**：`JMComic_Next`（主项目），源码根 `app/src/main/kotlin/com/jmnext/`。
下文表格里的路径都省略这个前缀，写成 `ui/components/Glass.kt:133` 这种形式。

**读法约定**

- 数值一律照抄源码（含单位）。注释里出现的与代码不一致的值，另立一节如实标出。
- 「未找到」= 我搜过整个 `app/src/`（含测试）而没有读点，不猜。
- 判断列只有四个字母，且**每行只给一个**：
  - **(a)** Qt Widgets + QSS 能表达
  - **(b)** Qt Widgets + QPainter 自绘能表达（QSS 不够）
  - **(c)** Qt Quick/QML（+ QtQuick.Effects / Qt5Compat.GraphicalEffects）更合适或必须
  - **(d)** 四个栈都做不到（要么是规格缺口，要么要靠平台原生 API）

**关于当前 Qt 侧栈的事实**（判据来源）：`JMNeXt4QtDesktop/CMakeLists.txt:31` 只 `find_package(Qt6 COMPONENTS Widgets)`，
界面目录是 `src/qt/`（`MainWindow.cpp` 等），**目前没有 Qt Quick**。所以：

- 「(c)」意味着要引入 `Qt6::Quick` + `Qt6::QuickControls2`，并在 `CMakeLists.txt` 加模块。
- QtQuick.Effects 的 `MultiEffect`（blur/shadow/saturation/mask）要 **Qt 6.5+**；更早的版本走 `Qt5Compat.GraphicalEffects`（GaussianBlur / DropShadow / Blend）。

**QSS 的能力边界**（下文大量判断的依据，逐条已核对 Qt 文档级事实）：

| 能 | 不能 |
| --- | --- |
| `rgba(...)` 半透明背景 | `box-shadow`（没有投影） |
| `border` + `border-radius`（圆弧） | 任何 `blur` / `backdrop-filter` / `filter` |
| `qlineargradient` / `qradialgradient`（多 stop） | `transform` / `scale` / `rotate` |
| 字体、padding、margin | `transition` / `animation`（Qt 的 QSS 不实现 CSS 动画） |
| 普通 QWidget 要吃背景样式需 `WA_StyledBackground` 或 QFrame 子类 | 混合模式（Overlay 等）、超椭圆圆角、文字阴影 |

---

## 0. 摘要

- **效果条目 23 项**（E01–E23），**动画条目 19 项**（A01–A19），共 42 项判断。
- 判断分布：**(a) 8 项 · (b) 25 项 · (c) 5 项 · (d) 4 项**（42 项合计）。
- **只有 Qt Quick 能做**（Qt Widgets 侧无论如何都要引入 Qt Quick 才能做出来的）**2 类 / 4 条**：
  1. **物理弹簧参数化的动效**（A01 表面按压缩放、A02 胶囊吸附、A03 胶囊拖起放大）——Widgets 只有解析曲线 `QEasingCurve`，没有弹簧积分器；QML 有 `SpringAnimation`。
  2. **列表条目增删/位移动画**（A15，对应 Compose 的 `animateItem()`）——Widgets 的 `QListView/QListWidget` 没有条目级布局动画，只能从零写自绘列表；QML `ListView` 的 add/remove/move/displaced transition 是内建的。
- **四个栈都做不到 4 项**：E21 系统栏图标明暗、E22 真·backdrop-filter、E23 动态取色（Monet 角色推导）、A06 系统级预测性返回跟手。
  另有 1 条附注：真 OS Acrylic/Mica（Win32 DWM 取样窗口之后的内容）。**重要**：主项目本来就没用真 OS 亚克力，它是自绘模拟的（渐变底 + 半透明层 + 颗粒），所以这条不构成移植阻塞。
- 一句话结论：**这套视觉里绝大多数东西是「自绘」的，不是「Qt 特性」的**——(b) 占 25/42。QSS 只够撑 8 项纯色/描边/单层渐变，(a) 的适用范围比直觉小得多。

---

## 1. 五套表面工艺参数

### 1.1 `SurfaceSpec` 定义与风格表

字段定义：`ui/theme/ThemeStyle.kt:91-124`（`craft` / `fillAlphaScale` / `accentTint` / `hairline` / `innerHighlight` /
`backdropSaturate` / `noise` / `shadows` / `pressScale`，`pressScale` 默认 `1f` 见 `:121`）。
`RadiusScale` 定义：`ui/theme/ThemeStyle.kt:63-69`。风格表：`object Styles` `ui/theme/ThemeStyle.kt:253-419`。

| 风格 / 枚举位 | RadiusScale xs,sm,md,lg,xl | craft | fillAlphaScale | accentTint | hairline | innerHighlight | backdropSaturate | noise | shadows（三档） | pressScale |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| WindowGlass（默认） | 2, 4, 6, 8, 12 dp | Acrylic | 1f | 0f | 1 dp | true | 1.65f | 0.5f | 0, 0, 0 dp | 1f（未写，用默认） |
| Translucent | 2, 4, 6, 8, 12 dp（继承，未覆盖） | Glass | 0.58f | 0.12f | 1 dp（继承） | true（继承） | 1.65f | 0.2f | 0, 0, 0 dp | 1f（继承） |
| FlatBlur | 8, 12, 16, 20, 28 dp | Blur | 1f | 0f | 0 dp | false | 1.25f | 0f | 0, 0, 0 dp | 1f |
| Miuix | 4, 8, 12, 16, 24 dp | Card | 1f | 0f | 0 dp | false | 1f | 0f | 0, 1, 3 dp | **0.97f** |
| Material | 4, 8, 12, 12, 28 dp | Tonal | 1f | 0f | 0 dp | false | 1f | 0f | 0, 1, 6 dp | 1f |

逐项出处（行号精确到字段赋值那一行）：

| 风格 | 行号块 |
| --- | --- |
| WindowGlass | `radius`:258 · `craft`:260 · `fillAlphaScale`:261 · `accentTint`:262 · `hairline`:263 · `innerHighlight`:264 · `backdropSaturate`:265 · `noise`:266 · `shadows`:271 · `wallpaperScrim`:280 · `backdropGlow`:281 · `motion`:279 |
| Translucent | 整块是 `windowGlass.copy(...)` `:289-303`：`craft`:292 · `fillAlphaScale`:293 · `accentTint`:294 · `backdropSaturate`:295 · `noise`:296 · `shadows`:298 · `wallpaperScrim`:301 · `backdropGlow`:302（radius/type/motion/hairline/innerHighlight 未覆盖 = 继承） |
| FlatBlur | `radius`:382 · `craft`:384 · `fillAlphaScale`:385 · `accentTint`:386 · `hairline`:387 · `innerHighlight`:388 · `backdropSaturate`:389 · `noise`:390 · `shadows`:392 · `wallpaperScrim`:401 · `backdropGlow`:402 · `motion`:400 |
| Miuix | `radius`:311 · `craft`:313 · `fillAlphaScale`:314 · `accentTint`:315 · `hairline`:316 · `innerHighlight`:317 · `backdropSaturate`:318 · `noise`:319 · `shadows`:321 · `pressScale`:322 · `motion`:330 · `wallpaperScrim`:331 · `backdropGlow`:332 |
| Material | `radius`:346 · `craft`:348 · `fillAlphaScale`:349 · `accentTint`:350 · `hairline`:351 · `innerHighlight`:352 · `backdropSaturate`:353 · `noise`:354 · `shadows`:357 · `motion`:365 · `wallpaperScrim`:366 · `backdropGlow`:367 |

`Radius.pill = 999.dp`（与风格无关），`ui/theme/Tokens.kt:43`。
lite 变体（`LiteFeatures.ENABLED`）在 `Styles.of` 里**只返回 miuix**，另外三套的规格被 R8 删掉：`ui/theme/ThemeStyle.kt:405-419`；主题入口处又把风格强制成 Miuix：`ui/theme/Theme.kt:239-240`。

### 1.2 表面实际不透明度（调色板 alpha × fillAlphaScale）

`GlassSurface` 取 `surface1/2/3` 作为「Card / Raised / Flyout」三档的基色（`ui/components/Glass.kt:116-120`），
再乘 `fillAlphaScale`（`ui/components/Glass.kt:133-134`）。下表是**乘完之后的实际 alpha**，Qt 侧可直接照抄。

| 风格 | 浅色 surface1 / 2 / 3 | 深色 surface1 / 2 / 3 | 出处 |
| --- | --- | --- | --- |
| WindowGlass | 0.580 / 0.737 / 0.859（`0x94`/`0xBC`/`0xDB`） | 0.055 / 0.722 / 0.902（`0x0E`/`0xB8`/`0xE6`） | `Tokens.kt:195-197`、`Tokens.kt:234-236` |
| Translucent | 0.214 / 0.303 / 0.378（`0x5E`/`0x85`/`0xA6` ×0.58） | 0.023 / 0.278 / 0.371（`0x0A`/`0x7A`/`0xA3` ×0.58） | `Palettes.kt:31-33`、`Palettes.kt:46-48` |
| FlatBlur | 0.702 / 0.851 / 0.949（`0xB3`/`0xD9`/`0xF2`） | 0.600 / 0.800 / 0.902（`0x99`/`0xCC`/`0xE6`） | `Palettes.kt:69-71`、`Palettes.kt:81-83` |
| Miuix | 1.0 / 1.0 / 1.0（`0xFF`，实心） | 1.0 / 1.0 / 1.0（`0xFF`，实心） | `Palettes.kt:112-114`、`Palettes.kt:141-143` |
| Material | 1.0（M3 `surfaceContainerLow/Container/High`，不透明） | 1.0（同上，深色） | `Palettes.kt:191-193`；测试断言见 `app/src/test/kotlin/com/jmnext/ThemeStyleTest.kt` 的 `every style has a palette for both themes` |

交叉核对：`ui/components/FloatingBottomBar.kt:178-180` 的注释写「Raised 浅 73.7% / 深 72.2%」，
与上表 WindowGlass 的 surface2（0.737 / 0.722）一致 —— 说明没有别的 alpha 变换被漏掉。

通透模式（`UiOptions.ultraTranslucent`）会在上述基础上**再乘 0.08**（仅 Acrylic/Glass/Blur 三种 craft）：`ui/components/Glass.kt:62`、`:128-134`。
于是 Translucent 深色 Card 档最低可到 0.023 × 0.08 ≈ 0.0018（几乎只剩描边），这与 `ui/components/Glass.kt:123-127` 的说明相符。

### 1.3 动效参数（`MotionSpec` × 三个 `MotionStyle`）

`MotionSpec` 定义：`ui/theme/ThemeStyle.kt:172-209`；`MotionStyle` 枚举：`:149-169`；曲线表 `JmEasing`：`ui/theme/Theme.kt:45-79`；
基准时长常量 `object Motion`：`ui/theme/Tokens.kt:74-83`。切换逻辑：`ui/theme/Theme.kt:247-252`。

| 风格 | Standard（fast/base/slow，springy，enter，exit） | Plasma（`asPlasma()` `:187-193`） | HyperOS（`asHyperOS()` `:202-208`） |
| --- | --- | --- | --- |
| WindowGlass | 120 / 200 / 320，false，enter=fluent，exit=standard | 150 / 250 / 420，enter=outCubic，exit=inCubic（springy 保留 false） | 200 / 350 / 500，enter=hyperOS，exit=hyperOSOut |
| Translucent | 同上（继承） | 同上 | 同上 |
| FlatBlur | 120 / 220 / 320，false，enter=fluent，exit=standard | 同上 | 同上 |
| Miuix | 150 / 240 / 360，**true**，enter=fluent，exit=standard | 150/250/420，springy **保留 true** | 200/350/500，springy 保留 true |
| Material | 100 / 200 / 300，false | 同上 | 同上 |

曲线数值（照抄）：

| 曲线 | cubic-bezier / 参数 | 出处 |
| --- | --- | --- |
| standard | (0.33, 0, 0.67, 1) | `Theme.kt:47` |
| decel | (0.1, 0.9, 0.2, 1) | `Theme.kt:50` |
| fluent | (0.16, 1, 0.3, 1) | `Theme.kt:53` |
| outCubic | (0.215, 0.61, 0.355, 1) | `Theme.kt:61` |
| inCubic | (0.55, 0.055, 0.675, 0.19) | `Theme.kt:64` |
| inOutQuad | (0.455, 0.03, 0.515, 0.955) | `Theme.kt:67` |
| hyperOS | (0.35, 0, 0.10, 1) | `Theme.kt:75` |
| hyperOSOut | (0.5, 0, 0.30, 1) | `Theme.kt:78` |

`Motion.FADE_ONLY = 80`（`Tokens.kt:82`）**未找到读点**（声明了「小于该时长只做淡入」的降级规则，没有实现）。
`JmEasing.decel` / `inOutQuad` 也**未找到读点**。

---

## 2. 这些参数在渲染时被谁读到

| 参数 | 读取点（文件:行） | 读到之后做什么 |
| --- | --- | --- |
| `radius`（全部五个） | `ui/theme/Tokens.kt:30-41`（`Radius` 对象）→ `ui/theme/Shapes.kt:169-173`（`jmShape`） | `RoundedCornerShape(radius)`；Miuix（`craft == Card`）换成 `SquircleShape(radius)` |
| `craft` | `ui/components/Glass.kt:129-132`（通透模式资格）、`:175-176`（薄层资格）、`ui/theme/Shapes.kt:170-172` | 决定是否参与 ultra、是否画橙蓝薄层、是否用连续圆角 |
| `fillAlphaScale` | `ui/components/Glass.kt:133-134` | `fill = raw.copy(alpha = raw.alpha * fillAlphaScale * ultraFactor)` |
| `accentTint` | `ui/components/Glass.kt:163-166` | `drawBehind { drawRect(color = accent.copy(alpha = accentTint)) }` |
| `hairline` | `ui/components/Glass.kt:188-189`（描边）、`:217`（高光高度 = `hairline.toPx() * 3f`） | `Modifier.border(...)`；上缘高光条的高度 |
| `innerHighlight` | `ui/components/Glass.kt:216-226` | 画一条 3×hairline 高的竖向渐变条 |
| `backdropSaturate` | `ui/components/AmbientBackdrop.kt:58` → `:79-83` | 壁纸层的 `ColorMatrix().setToSaturation(saturate)`；用户 blur=0 时被强制成 1f（`:217-222`） |
| `noise` | `ui/components/Glass.kt:202-213`（配合 `:101` 的 `backdropHasTexture`） | `drawRect(brush = noiseBrush, alpha = noise, blendMode = Overlay)` |
| `shadows` | `ui/components/Glass.kt:135-139` → `:157`；令牌读口 `ui/theme/Tokens.kt:124-128` | `Modifier.shadow(elevation, shape, clip = false)`，三档对应 Card/Raised/Flyout |
| `pressScale` | `ui/components/Glass.kt:107` | `animateFloatAsState` 的目标值（按下 → `pressScale`，松开 → `1f`） |
| `wallpaperScrim` | `ui/components/AmbientBackdrop.kt:124` | 与用户的 `wall.dim` 相加后 `coerceIn(0f, 0.86f)` 作遮罩 alpha |
| `backdropGlow` | `ui/components/AmbientBackdrop.kt:166-199` | 三团径向光斑的 alpha 乘数（0 时不画） |
| `Radius.pill` | `ui/components/Glass.kt:272`、`ui/components/FloatingBottomBar.kt:183`、`ui/screens/reader/ReaderScreen.kt:856` | `RoundedCornerShape(percent = 50)` |
| `motion.fast` | `ui/components/Glass.kt:111` | 按压缩放的 `tween(fast)` |
| `motion.base` / `enter` / `exit` | `ui/JmNavHost.kt:216,297,310,329`、`ui/SharedTransition.kt:98-100`、`ui/JmNavHost.kt:828` | 页面转场时长与曲线、共享元素 bounds、跟手归零 |
| `motion.slow` | `ui/JmNavHost.kt:216` | Tab 横滑时长 = `(base + slow) / 2` |
| `motion.springy` | `ui/components/Glass.kt:108-112`、`ui/components/FloatingBottomBar.kt:137-153`、`ui/JmNavHost.kt:326-327` | 在「spring 手感」与「tween 定位」之间分支 |
| `type`（字号/字重/行高） | `ui/theme/Theme.kt:90-133`（`typographyOf`）、`:297-300` | 生成 M3 `Typography`；通透模式下所有层级挂 `readabilityShadow` |

`Radius.xx` 的消费点很广（20 个文件、约 46 处），代表行：`ui/components/ComicCard.kt:88,102,174,217`、`ui/components/Glass.kt:87`（`GlassSurface` 默认 `shape`）。
`jmShape(...)` 出现在 11 个文件、约 21 处。

---

## 3. 效果清单（E01–E23）

「出处」列给出定义/赋值行；「实现」列给出**真正读它并绘制**的那一行。

| 编号 | 效果 | 出处 | 实现方式（Compose API + 参数） |
| --- | --- | --- | --- |
| E01 | 半透明表面填充 | `ui/components/Glass.kt:116-120,133-134,159` | `Modifier.background(fill)`，`fill` = `surface1/2/3` 的色值再乘 `fillAlphaScale`（数值见 1.2 表） |
| E02 | 发丝描边 | `ui/components/Glass.kt:188-189` | `Modifier.border(surface.hairline, c.stroke, shape)`；`hairline=1.dp`，色 `stroke = 0x170F172A`（浅）/`0x1AFFFFFF`（深）`Tokens.kt:202,241` |
| E03 | 上缘高光 | `ui/components/Glass.kt:216-226` | `drawRect(brush = Brush.verticalGradient(listOf(c.strokeInner, Color.Transparent), startY = 0f, endY = hairline.toPx() * 3f), size = Size(width, h))`；`strokeInner` 浅 `0xBFDFFFFF` / 深 `0x1FFFFFFF` |
| E04 | 强调色薄染 12%（Translucent） | `ui/components/Glass.kt:163-166` | `drawBehind { drawRect(color = c.accent.copy(alpha = 0.12f)) }`，条件 `accentTint > 0f && !ultra` |
| E05 | MIUI 橙→蓝对角薄染（顶栏/底栏） | `ui/components/Glass.kt:142-146,175-185` | `Brush.linearGradient(listOf(c.tintWarm, c.tintCool), Offset.Zero, Offset.Infinite)` + `drawBehind { drawRect(brush = tintBrush) }`；只在 `tinted = true` 且 craft 是 Acrylic/Glass 时画。调用点：`ui/components/GlassTopBar.kt:47`、`ui/components/FloatingBottomBar.kt:184`、`ui/JmNavHost.kt:850`。色值 `Tokens.kt:214-215`（浅 `0x33F78736`/`0x33367DF7`）、`Tokens.kt:253-254`（深 `0x24F78736`/`0x2E367DF7`） |
| E06 | Acrylic 颗粒 | `ui/components/Glass.kt:194-214,305-339` | 预生成 64×64 位图（`noisePixels(64)`，xorshift 确定性伪随机，`lum = 128 + (v-128)/3`，alpha `0x80`，`:327-336`）→ `ShaderBrush(ImageShader(bitmap, TileMode.Repeated, TileMode.Repeated))` `:311` → `drawRect(brush = noiseBrush, alpha = 0.5f, blendMode = BlendMode.Overlay)` `:209-213`。**只在底有真实纹理时画**（`backdropHasTexture`，`:101,202`） |
| E07 | 背景高斯模糊 | `ui/components/AmbientBackdrop.kt:58,86-88` | `Modifier.blur(effectiveBlur)`（API 31+ 映射到 RenderEffect，低版本忽略）；`effectiveBlur` = 用户壁纸 blur 的 dp 值（0..24），**只由用户设置驱动**，`backdropFrosting()` `:215-222` |
| E08 | 底图饱和度补偿 | `ui/components/AmbientBackdrop.kt:79-83` | `ColorFilter.colorMatrix(ColorMatrix().apply { setToSaturation(saturate) })`；Acrylic 1.65f、FlatBlur 1.25f、实心风格 1f（不变） |
| E09 | 环境底：线性渐变 + 三团径向光斑 | `ui/components/AmbientBackdrop.kt:152-201` | `drawWithCache { ... onDrawBehind { drawRect(brush = base); if (glow > 0) 三个 drawRect(brush = glowA/B/C) } }`。base = `Brush.linearGradient(c.backdrop, Offset(0,0) → Offset(w*0.35, h))`；glowA = radial alpha `0.45f*glow` center `(0.12w, -0.08h)` radius `max(w,h)*0.75`；glowB = `0.34f*glow` center `(0.88w, 0.04h)` radius `*0.68`；glowC = `0.30f*glow` center `(0.62w, 1.08h)` radius `*0.72`。`glow = spec.backdropGlow`（WindowGlass/Translucent/FlatBlur = 1f，Miuix/Material = 0f） |
| E10 | 莫奈上色层（`monetBlur`） | `ui/components/AmbientBackdrop.kt:94-114` | 整屏 `Brush.linearGradient` 三段 alpha：`0.16f / 0.10f / 0.14f`（`:104-108`），叠在底之上、遮罩之下 |
| E11 | 壁纸遮罩 | `ui/components/AmbientBackdrop.kt:117-128` | `Color(0xFF06080E).copy(alpha = (wall.dim + spec.wallpaperScrim).coerceIn(0f, 0.86f))`；`dim` 默认 `0.12f`（`data/wallpaper/WallpaperStore.kt:78`） |
| E12 | 投影（三档） | `ui/components/Glass.kt:135-139,157` | `Modifier.shadow(elevation, shape, clip = false)`；elevation = 各风格 `shadows` 的第 0/1/2 项（0/1/3 dp 或 0/1/6 dp；玻璃三套全是 0） |
| E13 | 圆角：5 档 + 连续圆角 + 胶囊 | `ui/theme/Shapes.kt:37-65,100-160,169-173` | `SquircleShape`：超椭圆 `\|x/a\|^n + \|y/b\|^n = 1`，`exponent = 5f`、每角 `steps = 12` 段，45° 处到角心距离约 `0.86r`（圆弧是 `0.707r`）；`createOutline` 返回 `Outline.Generic(Path)`。仅 Miuix 用 |
| E14 | 按压缩放（Miuix） | `ui/components/Glass.kt:153-156` | `graphicsLayer { scaleX = scale; scaleY = scale }`，`scale` 在 `pressScale = 0.97f` 与 `1f` 之间动画（见 A01） |
| E15 | 左侧强调条 | `ui/components/Glass.kt:247-258` | `Modifier.accentBar(...)` = `drawWithContent { drawContent(); drawRoundRect(color, topLeft = (0, (h - 0.56h)/2), size = (3.dp, 0.56h), cornerRadius = w/2) }`；宽 `Sizing.accentBar = 3.dp`（`Tokens.kt:63`）。**调用点未找到**（只有定义与测试） |
| E16 | 文字反色柔光（通透模式） | `ui/theme/Theme.kt:145-149,297-300` | `Shadow(color = Black.copy(alpha = 0.72f) / White.copy(alpha = 0.90f), offset = Offset(0f, 1f), blurRadius = 5f)`，挂到 `typographyOf` 的每一个层级（`:90-133`） |
| E17 | 图片淡入 | `JmApp.kt:103,120` | Coil `crossfade(true)` / `.crossfade(LiteFeatures.imageCrossfade)`（完整版 true，lite false：`LiteFeatures.kt:39`） |
| E18 | Material Tonal 分层 | `ui/theme/ThemeStyle.kt:348`、`ui/theme/Theme.kt:262-276`、`ui/theme/Palettes.kt:180-219` | Material 风格整份配色来自 M3 角色（`toJmPalette`），层级由 `surfaceContainerLow/Container/High` → `surface1/2/3` 承担，不靠投影 |
| E19 | 表面三档层级 | `ui/components/Glass.kt:116-120` + 1.2 表 | Card/Raised/Flyout 各取一个基色 + 一个投影档 |
| E20 | 通透模式（`ultraTranslucent`） | `ui/components/Glass.kt:62,128-134` | `alphaScale = fillAlphaScale * 0.08f`，且抑制 accentTint 与橙蓝薄层；同时开 E16 的文字柔光 |
| E21 | 系统栏图标明暗 | `ui/theme/Theme.kt:278-287` | `WindowCompat.getInsetsController(window, view).isAppearanceLightStatusBars = !darkTheme`（Android 平台 API） |
| E22 | 真·逐表面背景模糊（规格缺口） | `ui/components/Glass.kt:79-81`（注释自述）、`Tokens.kt:86-98` | 未实现。源码原话：「Compose 读不到『已绘制内容』再作模糊，所以真正的背景模糊由 AmbientBackdrop 的 RenderEffect 完成，这里的 blur 值只是**协议**」——即**每块表面各自的 background blur 在本项目里也不存在**，只有「整块底」被模糊过 |
| E23 | 动态取色（Monet → M3 角色体系） | `ui/theme/Theme.kt:204-214,243,256-272`、`ui/theme/Palettes.kt:218` | `dynamicDarkColorScheme(context)` / `dynamicLightColorScheme(context)`（Android 12+）产出整套角色；玻璃体系只借 `primary` 并把 `primary/secondary/tertiary` 存进 `monetTints` 供 E10 用 |

---

## 4. 动画清单（A01–A19）

「曲线与时长」列照抄源码数值；`fast/base/slow` 的含义见 1.3，随 `MotionStyle` 取三组之一。

| 编号 | 动画 / 位置 | 出处 | 动什么属性 | 曲线与时长 |
| --- | --- | --- | --- | --- |
| A01 | 表面按压缩放 `surfacePress` | `ui/components/Glass.kt:104-114` | `scaleX/scaleY`（`pressScale ↔ 1f`），经 `graphicsLayer` `:153-156` 应用 | `spec.motion.springy` 为真（仅 Miuix）：`spring(dampingRatio = 0.55f, stiffness = 900f)`；否则 `tween(spec.motion.fast)`（120 / 150 / 240 / 100 ms 视风格）。其它风格 `pressScale = 1f`，目标恒为 1f，**不产生可见动画** |
| A02 | 悬浮底栏胶囊位置 `pill` | `ui/components/FloatingBottomBar.kt:124,147-161,194,225-238`；应用 `:201`（`offset(x = itemWidth * pill.value)`） | 胶囊横向位移（单位「第几栏」，可小数） | `springy`（Miuix）：`spring<Float>(dampingRatio = 0.72f, stiffness = 420f)`；其它：`spring<Float>(dampingRatio = 1f, stiffness = 300f)`。拖动期间 `snapTo` 跟手（无插值） |
| A03 | 胶囊拖起放大 `lift` | `ui/components/FloatingBottomBar.kt:135-143`；应用 `:205-209`（`s = 1f + 0.04f * lift`） | 胶囊 `scaleX/scaleY`（1 → 1.04） | `springy`：`spring(dampingRatio = 0.45f, stiffness = 900f)`；其它：`spring(dampingRatio = 1f, stiffness = 700f)` |
| A04 | 图标随选中权重放大 | `ui/components/FloatingBottomBar.kt:266-271` | 图标 `scaleX/scaleY` = `1f + 0.08f * weight * lift` | 无独立 spec：每帧按 `pill.value` 与 `lift` 直接算 |
| A05 | 底栏连续染色 | `ui/components/FloatingBottomBar.kt:245-246` | 图标与文字颜色：`lerp(c.textSecondary, c.accent, weight)`，`weight = (1f - abs(pill - index)).coerceIn(0f, 1f)` `:91-92` | 无独立 spec：颜色是 `pill.value` 的连续函数（拖动时自然连续） |
| A06 | 预测性返回跟手淡出 `backFeedback` | `ui/JmNavHost.kt:401,414-417,813-832`；应用 `:467-473` | 整页 alpha = `1f - 0.35f * p`（不缩放，注释 `:395-397` 说明理由） | 跟手：`snapTo(backEvent.progress)`（系统进度，无插值）；收尾：`animateTo(0f, tween(motion.base, easing = motion.exit))`。只在 API 33–34 且开关为真时生效（`:414-417`） |
| A07 | 骰子旋转 `spin` | `ui/screens/home/RandomFab.kt:74,86,107` | `Modifier.rotate(spin.value)`（每次点击 +720°，即两圈） | `tween(600)`；源码未显式给 easing，用的是 Compose 的默认 `FastOutSlowInEasing` |
| A08 | 页面转场（push/pop/Tab） | `ui/JmNavHost.kt:291-301`（enter）、`:304-314`（exit）、`:325-330`（`tabSlideSpec`）、`:223-226`（位移 ±整屏宽） | 透明度（fade）或整页水平位移（`slideInHorizontally` / `slideOutHorizontally`），Tab 横滑位移 = `width * shift`，两页首尾相接 | 非 Tab：`fadeIn(tween(motion.base, easing = motion.enter))` / `fadeOut(tween(motion.base, easing = motion.exit))`；Tab：`springy` 时 `spring(dampingRatio = 0.9f, stiffness = 320f)`，否则 `tween(durationMillis = tabSlideDurationMs(motion), easing = motion.enter)`，其中 `tabSlideDurationMs = (motion.base + motion.slow) / 2`（`:216`） |
| A09 | 共享元素（封面）bounds | `ui/SharedTransition.kt:89-130`；调用点 `ui/components/ComicCard.kt:101`、`ui/screens/detail/DetailScreen.kt:764`；键 `:139` | 元素矩形（触发前位置 → 触发后位置），画在 SharedTransitionScope 的 overlay 里；原位置留 `PlaceholderSize.ContentSize` 的空缺 | `BoundsTransform { _, _ -> tween(durationMillis = motion.base, easing = motion.enter) }` `:98-100`（刻意与页面转场同一时间线） |
| A10 | 退出瞬间隐藏 | `ui/SharedTransition.kt:164-171` | 整页内容 `alpha = 0f`（立即，不是淡出） | 无插值（`graphicsLayer { alpha = 0f }`），条件 `:168-169`（正在离开且共享元素在飞） |
| A11 | 通知条（Snackbar 之上的一层） | `ui/Notices.kt:61-79` | 透明度 + 垂直位移 | 进入：`fadeIn(tween(220)) + slideInVertically(tween(220)) { it / 3 }`；退出：`fadeOut(tween(120)) + slideOutVertically(tween(120)) { it / 3 }`；停留 `infoMs = 2400` / `errorMs = 4800`（`:69-72`） |
| A12 | 阅读器顶栏 / 错误提示条 | `ui/screens/reader/ReaderScreen.kt:392-397,409-415` | 垂直位移（`slideInVertically { -it }` / `slideOutVertically { -it }`） | 未写 spec → 用 `AnimatedVisibility` 默认（`spring()` 的默认参数，Compose 内建） |
| A13 | 阅读器底栏 | `ui/screens/reader/ReaderScreen.kt:459-465` | 垂直位移（`slideInVertically { it }` / `slideOutVertically { it }`） | 同上（默认 spring） |
| A14 | 页码指示条 | `ui/screens/reader/ReaderScreen.kt:853-859` | 垂直位移（`slideInVertically { it }` / `slideOutVertically { it }`） | 同上（默认 spring） |
| A15 | 列表条目进出场/位移 | `ui/components/ItemMotion.kt:22-30`；调用点 `HomeScreen.kt:287`、`CategoryScreen.kt:466`、`RandomListScreen.kt:244,276`、`SearchScreen.kt:576,749`（共 6 处） | 条目的出现/消失/位置（由 `LazyColumn` 的 `animateItem()` 接管：淡入淡出 + 位移） | 数值不在本仓库：`animateItem()` 用 Compose Foundation 的默认 spec。lite 变体关掉（`LiteFeatures.kt:42`） |
| A16 | 图片淡入 | `JmApp.kt:103,120` | 图片交叉淡入 | Coil `crossfade(true)`（Coil 默认 100ms，本仓库未给参数）；lite 关闭 |
| A17 | 阅读器图片缩放/平移 | `ui/screens/reader/ReaderScreen.kt:892-941`（应用 `:929-933`） | `scaleX/scaleY/translationX/translationY`（1x–5x） | 跟手，无动画 spec；双击在 1f 与 2.5f 之间**瞬时**切换（`:916`，无过渡）。常量 `MIN_ZOOM = 1f` `:939`、`MAX_ZOOM = 5f` `:940`、`DOUBLE_TAP_ZOOM = 2.5f` `:941` |
| A18 | 阅读器跳页 / 跳章 | `ui/screens/reader/ReaderScreen.kt:753,828` | 滚动位置 | `listState.animateScrollToItem(...)` / `pagerState.animateScrollToPage(...)`：Compose 内建滚动动画，曲线与时长不可配 |
| A19 | M3 组件内建动效 | `ui/JmNavHost.kt:852-858`（`NavigationBar`/`NavigationBarItem`）、设置页与阅读页的 `Slider`/`IconButton` 等 | 指示器位移与宽度、涟漪（state layer）、滑块 thumb | Material3 内建，数值不在本仓库 |

---

## 5. 可行性判断表（本文核心产出）

### 5.1 效果（E01–E23）

| 编号 | 判断 | 理由（一句） |
| --- | --- | --- |
| E01 半透明填充 | (a) | QSS 的 `background: rgba(...)` + `border-radius` 就是同一件事（普通 QWidget 需 `WA_StyledBackground` 或 QFrame 子类）。 |
| E02 发丝描边 | (a) | `border: 1px solid rgba(...)` + 同一 `border-radius`，QSS 原生支持。 |
| E03 上缘高光 | (b) | QSS 一个控件只有一层 `background`，无法在圆角约束内再叠一条 3dp 的竖向渐变；QPainter 里是 `drawRect` + `QLinearGradient` 一次调用。 |
| E04 强调色薄染 12% | (b) | 要把「半透明填充」与「12% 强调色」分别控制，QSS 只能靠嵌套两层控件近似（且圆角会双份）；自绘直接画第二层 `fillRect`。 |
| E05 橙→蓝对角薄染 | (a) | QSS 的 `qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 rgba(...), stop:1 rgba(...))` 就是这条 120° 对角渐变。 |
| E06 Acrylic 颗粒 | (b) | QSS 没有混合模式也没有可控平铺；QPainter 的 `drawTiledPixmap` + `setCompositionMode(CompositionMode_Overlay)` + `setOpacity(0.5)` 精确对应 `BlendMode.Overlay` 与 `alpha = noise`。 |
| E07 背景高斯模糊 | (b) | 模糊对象是**应用自绘的底图**（渐变 + 壁纸），与系统合成无关：Widgets 侧把底图先模糊一次并缓存、按表面矩形裁剪即可（半径 0..24dp，可交互变化时逐档重算一次）；QML 的 `MultiEffect` 更省事但不是必须。 |
| E08 饱和度补偿 | (b) | Qt 没有现成饱和度滤镜（`QGraphicsColorizeEffect` 是染色不是饱和），要自己对 `QImage` 做色彩矩阵/HSV 换算；同样只需对底图做一次并缓存。QML 的 `MultiEffect.saturation` 一个属性即可。 |
| E09 环境底（渐变 + 三光斑） | (b) | QSS 一个控件只能一层背景，三团径向光斑要拆成三个嵌套控件且圆角/裁剪难对齐；自绘是 `QLinearGradient` + 三个 `QRadialGradient` 的 `fillRect`。 |
| E10 莫奈上色层 | (a) | 它就是一条多 stop 的半透明线性渐变，QSS 的 `qlineargradient` 能写。 |
| E11 壁纸遮罩 | (a) | 单色 `rgba(6,8,14,alpha)` 铺满，QSS 足够。 |
| E12 投影三档 | (b) | QSS 没有 `box-shadow`；`QGraphicsDropShadowEffect`（有模糊半径，但色/偏移可配）或自绘模糊阴影都可以，且本项目的投影值只有 0/1/3/6dp、互相可切换。 |
| E13 圆角 / 连续圆角 / 胶囊 | (b) | 圆弧圆角 QSS 能写，但 Miuix 的**超椭圆（n=5、每角 12 段）**QSS 与 `QRect::roundedRect` 都表达不了，必须 `QPainterPath`（`cubicTo` 或采样 `lineTo`）自绘。 |
| E14 按压缩放 0.97 | (b) | QSS 无 `transform`，只能在自绘里 `QTransform::scale` + 重绘（动画部分见 A01）。 |
| E15 左侧强调条 | (b) | 需要一个「宽 3dp、高 56%、居中、两端全圆角」的细条：QSS 得靠一个固定尺寸子控件且百分比高度要手算；自绘一次 `drawRoundedRect` 更直接。 |
| E16 文字反色柔光 | (b) | QSS 无 `text-shadow`；要 blurRadius 5 的柔光得在绘制文字时先画一层模糊（`QPainterPath` + 模糊位图，或给每个文本控件挂 `QGraphicsDropShadowEffect`），代价是每处文本都要走这条路。 |
| E17 图片淡入 | (b) | QSS 没有过渡动画；自绘用 `QPropertyAnimation` 驱动 opacity（或 `QGraphicsOpacityEffect`）即可。 |
| E18 Material Tonal 分层 | (a) | 本质是「不同控件用不同的不透明纯色」，QSS 完全够用（M3 的色调阶梯就是几组固定色值）。 |
| E19 表面三档层级 | (a) | 三档 = 三套 `rgba` 背景值 + 三套圆角/描边开关，QSS 变量化即可。 |
| E20 通透模式 | (a) | 全局开关只改填充 alpha 的乘数；QSS 下重新生成样式表（或在 `QPalette` 里换 alpha）即可，无需自绘。 |
| E21 系统栏图标明暗 | (d) | 这是 Android `WindowInsetsController` 的平台行为；Widgets/QSS/QPainter/QML 四个栈都没有「系统状态栏图标明暗」的跨平台 API（Windows 要 DWM、Linux 各桌面不同、Android 要 JNI/平台接口）。 |
| E22 真·逐表面背景模糊 | (d) | 规格缺口，不是实现难度：主项目自己也没做到（`ui/components/Glass.kt:79-81` 明说 Compose 读不到已绘制内容）。QSS 无此概念；Widgets 的 QPainter **无法采样兄弟控件的绘制结果**（要做到得反转整个绘制架构，实际不可用）；QML 的 `ShaderEffectSource` 只能抓显式引用的 item，能做单层近似，表达不了「本层之下所有半透明兄弟层」的递归采样。 |
| E23 动态取色（Monet 角色推导） | (d) | 四个栈都没有「从壁纸推导一整套 M3 颜色角色」的 API：Windows 只给一个 DWM accent、Linux 走 portal/KDE 配色、Qt 只给 `QStyleHints::colorScheme()`（明暗）；要接近只能自己实现取色算法（那已经超出这四个栈）。 |

### 5.2 动画（A01–A19）

| 编号 | 判断 | 理由（一句） |
| --- | --- | --- |
| A01 表面按压缩放 spring | (c) | 关键不是缩放而是 `spring(dampingRatio = 0.55f, stiffness = 900f)` 这套**物理弹簧**语义：Widgets 只有解析曲线 `QEasingCurve`（`OutBack` 只能近似过冲），要真弹簧得自己写积分器；QML 的 `SpringAnimation`（`spring`/`damping`）是内建的。 |
| A02 胶囊吸附 spring | (c) | 同上：`spring(dampingRatio = 0.72f/1f, stiffness = 420f/300f)`，且要求拖动中 `snapTo` 跟手、松手吸附连续；QML 里是 `Behavior on x { SpringAnimation {} }` 一行，Widgets 要自己接手势 + 积分。 |
| A03 胶囊拖起放大 spring | (c) | 同 A01/A02 的弹簧语义（`dampingRatio = 0.45f` / `1f`，`stiffness = 900f` / `700f`）；缩放本身 (b) 能做，做出这个手感不行。 |
| A04 图标随权重放大 | (b) | 只是每帧算一个 scale 并应用变换，没有动画系统需求；自绘 `QTransform` 即可。 |
| A05 底栏连续染色 | (b) | 颜色是拖动位置的连续函数（`lerp(textSecondary, accent, weight)`）：自绘时每帧 `QColor` 插值即可；QSS 做不到（颜色是静态的）。 |
| A06 预测性返回跟手淡出 | (d) | 输入源不存在：桌面三平台都没有「系统级预测性返回手势 + 窗口预览进度」，Qt 的四个栈也没有对应 API（Android 上还是系统在窗口层面画的，见 `ui/JmNavHost.kt:404-417` 的说明）。淡出本身 (b) 能做，但那一项单独不存在。 |
| A07 骰子旋转 720° | (b) | `QVariantAnimation`（600ms）+ `QPainter::rotate` 就能做，Qt 侧还有对应的 `QEasingCurve::FastOutSlowIn` 可对（源码 `tween(600)` 未显式给 easing，用 Compose 默认 `FastOutSlowInEasing`）。 |
| A08 页面转场（淡入淡出 / 整屏横滑） | (c) | 整屏横滑是 pager 几何（两页一进一出首尾相接）+ 可交互拖动，QML 的 `SwipeView`/`StackView` 有内建交互式转场；Widgets 没有动画化 pager，只能抓前后页位图自绘或从零写。(b) 也能做到，只是代价明显更高，所以这里给 (c)。 |
| A09 共享元素（封面飞行） | (b) | 本质是「在一张覆盖层上把封面从矩形 A 插值画到矩形 B」（`QVariantAnimation` 插值 QRect + `drawPixmap(targetRect, pm)`），Widgets 的覆盖层/自绘正好擅长；QML 也可以，但没有内建共享元素，两边都要手写。 |
| A10 退出瞬隐 | (b) | 直接 `alpha = 0`，自绘里就是 `setVisible(false)` 或不画；没有动画成分。 |
| A11 通知条进出 | (b) | 透明度 + 垂直位移（220ms 进 / 120ms 出、位移 1/3 高）：`QPropertyAnimation` 配 `OutCubic`/`InCubic` 即可。 |
| A12 阅读器顶栏 | (b) | 一条垂直位移（`slideInVertically { -it }`）：自绘高度偏移 + 动画即可；若要 Compose 默认 spring 的那点回弹，参见 A01 的结论。 |
| A13 阅读器底栏 | (b) | 同 A12，方向相反（`{ it }`）。 |
| A14 页码指示条 | (b) | 同 A12（顶部 72dp 处的浮动胶囊滑入滑出）。 |
| A15 列表条目进出场/位移 | (c) | Qt Widgets 的 `QListView/QListWidget` **没有**条目级增删/位移动画（`QListView` 只支持滚动与滚动条动画），要做只能在自绘列表里自己算布局差量并逐条动画；QML `ListView` 的 `add`/`remove`/`move`/`displaced` transition 是内建的。 |
| A16 图片淡入 | (b) | `QPropertyAnimation` 驱动 opacity（或 `QGraphicsOpacityEffect`），Coil 的 crossfade 语义就是一个透明度过渡。 |
| A17 缩放/平移跟手 | (b) | 触摸/滚轮事件里改 `QTransform` 并重绘即可；Qt 的 `QScroller` 与 `QGraphicsView` 也能提供部分基础，但手势判定要自己接。 |
| A18 跳页/跳章滚动动画 | (b) | `QScroller::scrollTo(pos, ms)` 或对滚动条 `value` 做 `QPropertyAnimation`，正是 `animateScrollToPage/Item` 的对应物。 |
| A19 M3 组件内建动效（指示器/涟漪/滑块） | (b) | Qt 没有对应的现成控件与状态过渡动画（QSS 做不了动画），要自绘这些控件与它们的 state layer；纯工作量问题，不是能力问题。 |

### 5.3 「只有 Qt Quick 能做」的清单（严格口径）

只列 **Qt Widgets 侧即使全部自绘也不现实**的项。表里其它标 (c) 的项（A08）只是「Quick 更合适」，不在此列。

| 项 | 为什么非 Quick 不可 |
| --- | --- |
| A01 表面按压缩放（弹簧手感） | 需要 `spring(dampingRatio, stiffness)` 的物理参数化；Widgets 只有解析曲线，必须自写数值积分器（能写，但那已经不是「用 Qt 的动画系统」）。 |
| A02 胶囊吸附（弹簧手感） | 同上，且要求与拖动跟手、松手吸附共用一个连续量。 |
| A03 胶囊拖起放大（弹簧手感） | 同上（`dampingRatio = 0.45f, stiffness = 900f`）。 |
| A15 列表条目增删/位移动画 | Widgets 没有条目级布局动画框架；QML `ListView` 的 add/remove/move/displaced transition 是内建的，自绘列表从零做「布局差量 → 逐条动画 → 复用回收」等于重写一个列表控件。 |

### 5.4 「四个栈都做不到」的清单

| 项 | 为什么四个栈都做不到 |
| --- | --- |
| E22 真·逐表面背景模糊（backdrop-filter） | 主项目自己也没做到（`ui/components/Glass.kt:79-81` 自述 Compose 读不到已绘制内容）；QSS 无概念；Widgets 的 QWidget 无法采样兄弟控件的绘制结果；QML 的 `ShaderEffectSource` 只能抓显式引用的 item，做不了「本层之下所有半透明兄弟层」的递归采样。**可做的是它现在的近似**：模糊整块底图 + 半透明填充（= E01+E07），这部分 (b) 可行。 |
| E23 动态取色（Monet → M3 全套角色） | 这四个栈里没有「从壁纸推导一整套颜色角色」的 API；Qt 只给明暗 `QStyleHints::colorScheme()`，Windows 只给一个 accent 色，Linux 各桌面走 portal。 |
| E21 系统栏图标明暗 | 平台行为，需要 Android JNI / Windows DWM / 各 Linux 桌面接口；四个栈本身都不提供。 |
| A06 系统级预测性返回跟手 | 桌面三平台没有这套系统手势与窗口预览，Qt 的四个栈也没有对应 API（连「输入源」都不存在）。 |

**附注（不属于这四个栈，但要如实记一笔）**：真 OS Acrylic/Mica（Windows 11 的 DWM 亚克力，取窗口**之后**的其它窗口/桌面内容）
四个栈都做不到，需要 Win32 调用（`DwmSetWindowAttribute` / `SetWindowCompositionAttribute`）。
**但主项目用的不是它**：`ui/theme/Tokens.kt:10-18,85-99` 写得很清楚，视觉基调是从博客 CSS 移植的
Acrylic × MIUI **自绘**做法（半透明分层 + 环境渐变底 + 发丝描边 + 颗粒），
不需要读窗口背后的真实内容 —— 所以这条**不构成移植阻塞**，E01/E02/E06/E09 的组合就是它的等价物。

---

## 6. 未找到 / 死令牌 / 注释与代码不一致（如实记档）

| 项 | 结论 |
| --- | --- |
| `Glass.blurRadius = 40.dp` | **未找到读点**（`ui/theme/Tokens.kt:102`）。全 `app/src/` 只有声明。测试注释也承认过这件事（`app/src/test/kotlin/com/jmnext/ThemeStyleTest.kt` 的类注释：「以前 blur 令牌是死值，声明了 40dp 却没有任何地方读它」）。 |
| `Glass.innerHighlight = 1.dp` | **未找到读点**（`Tokens.kt:106`）。真正生效的是 `SurfaceSpec.innerHighlight`（布尔）与 `hairline`。 |
| `Glass.TINT_ANGLE_DEG = 120f` | **未找到读点**（`Tokens.kt:108`）。橙蓝薄层实际用 `Offset.Zero → Offset.Infinite` 的对角线性渐变近似（`ui/components/Glass.kt:141-146`）。 |
| `Sizing.hairline = 1.dp` | **未找到读点**（`Tokens.kt:63`）。描边宽度实际读 `SurfaceSpec.hairline`（`ui/components/Glass.kt:188-189`）。 |
| `ElevationBase`（sm 2dp / card 6dp / flyout 12dp） | **未找到读点**（`Tokens.kt:117-121`）。实际投影读 `SurfaceSpec.shadows`。 |
| `Motion.FADE_ONLY = 80` | **未找到读点**（`Tokens.kt:82`）。注释描述的「小于该时长只做淡入」降级规则没有实现。 |
| `JmEasing.decel`、`JmEasing.inOutQuad` | **未找到读点**（`Theme.kt:50,67`）。 |
| `Modifier.accentBar`（E15） | **未找到调用点**（`ui/components/Glass.kt:247`）。全 `app/src/main/` 只有定义。 |
| `MotionSpec.asPlasma()` 的注释值 | 注释写「时长取 Kirigami 的 longDuration = 200ms、veryLongDuration = 400ms」（`ui/theme/ThemeStyle.kt:184-186`），但代码给的是 `150 / 250 / 420`（`:189-191`）。**照抄代码**。 |
| `SurfaceSpec.backdropSaturate` 的文档值 | `Tokens.kt:94` 与 `ui/components/Glass.kt:74` 的注释都写「模糊加到 64dp」，但**代码里没有任何 64dp**：模糊只由用户的壁纸设置决定（`ui/components/AmbientBackdrop.kt:58,215-222`），风格只提供饱和度补偿。测试断言也明确「风格侧只允许提供补偿倍数，不允许提供模糊半径」。 |
| 各风格「模糊半径」 | **未找到**（除用户设置外）。风格表里**没有** blur 字段，这是 1.4.0 之后的**有意设计**：风格不得覆盖用户对壁纸的显式选择（`ui/components/AmbientBackdrop.kt:50-57`）。 |

---

## 7. 文件索引（相对本文档）

- [Theme.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/theme/Theme.kt)（`JmEasing`、`readabilityShadow`、`JmTheme` 入口、动态取色）
- [ThemeStyle.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/theme/ThemeStyle.kt)（`SurfaceSpec` / `RadiusScale` / `MotionSpec` / `Styles` 五套表）
- [Tokens.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/theme/Tokens.kt)（`Radius` / `Spacing` / `Sizing` / `Motion` / `Glass` / `Elevation` / 两套基础调色板）
- [Palettes.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/theme/Palettes.kt)（Translucent / FlatBlur / Miuix / `toJmPalette`）
- [Shapes.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/theme/Shapes.kt)（`SquircleShape` 超椭圆、`jmShape`）
- [Glass.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/components/Glass.kt)（`GlassSurface`：填充/描边/高光/染色/颗粒/投影/按压）
- [AmbientBackdrop.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/components/AmbientBackdrop.kt)（环境底、壁纸模糊与饱和、遮罩、莫奈层）
- [ItemMotion.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/components/ItemMotion.kt)、[FloatingBottomBar.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/components/FloatingBottomBar.kt)、[GlassTopBar.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/components/GlassTopBar.kt)
- [JmNavHost.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/JmNavHost.kt)（页面转场、共享元素布局、预测性返回）、[SharedTransition.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/SharedTransition.kt)、[Notices.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/Notices.kt)
- [ReaderScreen.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/screens/reader/ReaderScreen.kt)、[RandomFab.kt](../../../JMComic_Next/app/src/main/kotlin/com/jmnext/ui/screens/home/RandomFab.kt)
- [ThemeStyleTest.kt](../../../JMComic_Next/app/src/test/kotlin/com/jmnext/ThemeStyleTest.kt)（对上述参数的断言，可作数值对账的第二来源）
