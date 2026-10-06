# 主项目主题 token 摘录（供 Qt 侧照抄数值，不自创）

来源（主项目工作区目录名是 JMComic_Next）：

- `JMComic_Next/app/src/main/kotlin/com/jmnext/ui/theme/Tokens.kt`：`JmPalette` 字段定义 + 基色板 `LightPalette`/`DarkPalette`（WindowGlass 用的那一份）
- `JMComic_Next/app/src/main/kotlin/com/jmnext/ui/theme/Palettes.kt`：另外四套（Translucent / FlatBlur / Miuix 各含浅色与深色；Material 直接用 M3 角色）
- `JMComic_Next/app/src/main/kotlin/com/jmnext/ui/theme/ThemeStyle.kt`：五套风格的**表面工艺**参数（圆角/描边/颗粒/投影/按压/行高）

摘录方式：下面是**原文件里的原始行**（带行号），只筛出调色板名与 `Color(0x...)` 数值行，
目的是照抄数值。可读性靠字段名，不靠重排。

## JmPalette 字段（Tokens.kt）

```
134:data class JmPalette(
135-    // 强调色
136-    val accent: Color,
137-    val accentHover: Color,
138-    val accentActive: Color,
139-    val accentFg: Color,
140-    val accentSoft: Color,
141-    val accentGlow: Color,
142-
143-    // 表面：数字越大越靠近用户
144-    val surfaceMica: Color,
145-    val surface1: Color,
146-    val surface2: Color,
147-    val surface3: Color,
148-    val surfaceSunken: Color,
149-    val surfaceHover: Color,
150-    val surfaceActive: Color,
151-
152-    // 描边
153-    val stroke: Color,
154-    val strokeStrong: Color,
155-    val strokeInner: Color,
156-
157-    // 文字
158-    val text: Color,
159-    val textSecondary: Color,
160-    val textTertiary: Color,
161-    val textOnAccent: Color,
162-
163-    // 语义色（博客未定义，按 M3 规范补齐，保证错误态可用）
164-    val error: Color,
165-    val errorFg: Color,
166-
167-    /** 顶栏玻璃上的 MIUI 橙→蓝渐变薄层两端色 */
168-    val tintWarm: Color,
169-    val tintCool: Color,
170-
171-    /** 环境渐变底：不提供壁纸，仅用这组渐层作为 Acrylic 的采样底 */
172-    val backdrop: List<Color>,
173-
174-    /**
```

## 配色数值（Tokens.kt）

```
186:val LightPalette = JmPalette(
187:    accent = Color(0xFF0F6CBD),
188:    accentHover = Color(0xFF115EA3),
189:    accentActive = Color(0xFF0C3B5E),
191:    accentSoft = Color(0x1A0F6CBD),
192:    accentGlow = Color(0x470F6CBD),
194:    surfaceMica = Color(0xB8F6F7FA),
195:    surface1 = Color(0x94FFFFFF),
196:    surface2 = Color(0xBCFFFFFF),
197:    surface3 = Color(0xDBFFFFFF),
198:    surfaceSunken = Color(0x090F172A),
199:    surfaceHover = Color(0x0D0F172A),
200:    surfaceActive = Color(0x140F172A),
202:    stroke = Color(0x170F172A),
203:    strokeStrong = Color(0x290F172A),
204:    strokeInner = Color(0xBFDFFFFF),
206:    text = Color(0xFF16181D),
207:    textSecondary = Color(0xFF4A4F5A),
208:    textTertiary = Color(0xFF767C88),
211:    error = Color(0xFFB3261E),
214:    tintWarm = Color(0x33F78736),
215:    tintCool = Color(0x33367DF7),
218:        Color(0xFFEEF3FC),
219:        Color(0xFFF6F2FC),
220:        Color(0xFFEAF6FB),
225:val DarkPalette = JmPalette(
226:    accent = Color(0xFF60CDFF),
227:    accentHover = Color(0xFF7FD8FF),
228:    accentActive = Color(0xFF9AE2FF),
229:    accentFg = Color(0xFF04263A),
230:    accentSoft = Color(0x2460CDFF),
231:    accentGlow = Color(0x5760CDFF),
233:    surfaceMica = Color(0xB816181E),
234:    surface1 = Color(0x0EFFFFFF),
235:    surface2 = Color(0xB8262931),
236:    surface3 = Color(0xE630343E),
237:    surfaceSunken = Color(0x3D000000),
238:    surfaceHover = Color(0x14FFFFFF),
239:    surfaceActive = Color(0x21FFFFFF),
241:    stroke = Color(0x1AFFFFFF),
242:    strokeStrong = Color(0x2EFFFFFF),
243:    strokeInner = Color(0x1FFFFFFF),
245:    text = Color(0xFFF3F4F7),
246:    textSecondary = Color(0xB8FFFFFF),
247:    textTertiary = Color(0x80FFFFFF),
248:    textOnAccent = Color(0xFF04263A),
250:    error = Color(0xFFFFB4AB),
251:    errorFg = Color(0xFF690005),
253:    tintWarm = Color(0x24F78736),
254:    tintCool = Color(0x2E367DF7),
257:        Color(0xFF0A0E1A),
258:        Color(0xFF141428),
259:        Color(0xFF0B1522),
```

## 配色数值（Palettes.kt）

```
29:val TranslucentLight = LightPalette.copy(
30:    surfaceMica = Color(0x8CF6F7FA),
31:    surface1 = Color(0x5EFFFFFF),
32:    surface2 = Color(0x85FFFFFF),
33:    surface3 = Color(0xA6FFFFFF),
34:    surfaceSunken = Color(0x0B0F172A),
35:    surfaceHover = Color(0x100F172A),
36:    surfaceActive = Color(0x1A0F172A),
37:    stroke = Color(0x2E0F172A),
38:    strokeStrong = Color(0x420F172A),
39:    strokeInner = Color(0xD9FFFFFF),
40:    tintWarm = Color(0x40F78736),
41:    tintCool = Color(0x40367DF7),
44:val TranslucentDark = DarkPalette.copy(
45:    surfaceMica = Color(0x99101218),
46:    surface1 = Color(0x0AFFFFFF),
47:    surface2 = Color(0x7A262931),
48:    surface3 = Color(0xA330343E),
49:    surfaceSunken = Color(0x4D000000),
50:    surfaceHover = Color(0x1AFFFFFF),
51:    surfaceActive = Color(0x29FFFFFF),
52:    stroke = Color(0x24FFFFFF),
53:    strokeStrong = Color(0x3DFFFFFF),
54:    strokeInner = Color(0x2EFFFFFF),
55:    tintWarm = Color(0x2EF78736),
56:    tintCool = Color(0x3D367DF7),
67:val FlatBlurLight = LightPalette.copy(
68:    surfaceMica = Color(0xCCF7F8FA),
69:    surface1 = Color(0xB3FFFFFF),
70:    surface2 = Color(0xD9FFFFFF),
71:    surface3 = Color(0xF2FFFFFF),
72:    stroke = Color(0x00000000),
73:    strokeStrong = Color(0x140F172A),
74:    strokeInner = Color(0x00000000),
75:    tintWarm = Color(0x00000000),
76:    tintCool = Color(0x00000000),
79:val FlatBlurDark = DarkPalette.copy(
80:    surfaceMica = Color(0xCC14161C),
81:    surface1 = Color(0x991E2128),
82:    surface2 = Color(0xCC262A33),
83:    surface3 = Color(0xE62E323C),
84:    surfaceSunken = Color(0x4D000000),
85:    surfaceHover = Color(0x14FFFFFF),
86:    surfaceActive = Color(0x21FFFFFF),
87:    stroke = Color(0x00000000),
88:    strokeStrong = Color(0x1FFFFFFF),
89:    strokeInner = Color(0x00000000),
90:    tintWarm = Color(0x00000000),
91:    tintCool = Color(0x00000000),
103:val MiuixLight = LightPalette.copy(
104:    accent = Color(0xFF3482FF),
105:    accentHover = Color(0xFF2168E0),
106:    accentActive = Color(0xFF0F4FBD),
108:    accentSoft = Color(0x1F3482FF),
109:    accentGlow = Color(0x4D3482FF),
111:    surfaceMica = Color(0xFFF2F3F5),
112:    surface1 = Color(0xFFFFFFFF),
113:    surface2 = Color(0xFFFFFFFF),
114:    surface3 = Color(0xFFFFFFFF),
115:    surfaceSunken = Color(0x0F000000),
116:    surfaceHover = Color(0x0A000000),
117:    surfaceActive = Color(0x14000000),
119:    stroke = Color(0x00000000),
120:    strokeStrong = Color(0x14000000),
121:    strokeInner = Color(0x00000000),
123:    text = Color(0xFF0D0D0D),
124:    textSecondary = Color(0xFF666666),
125:    textTertiary = Color(0xFF999999),
127:    tintWarm = Color(0x00000000),
128:    tintCool = Color(0x00000000),
129:    backdrop = listOf(Color(0xFFF2F3F5), Color(0xFFF2F3F5), Color(0xFFF2F3F5)),
132:val MiuixDark = DarkPalette.copy(
133:    accent = Color(0xFF4C93FF),
134:    accentHover = Color(0xFF6BA6FF),
135:    accentActive = Color(0xFF8AB9FF),
136:    accentFg = Color(0xFF06203F),
137:    accentSoft = Color(0x294C93FF),
138:    accentGlow = Color(0x5C4C93FF),
140:    surfaceMica = Color(0xFF000000),
141:    surface1 = Color(0xFF1C1C1E),
142:    surface2 = Color(0xFF242426),
143:    surface3 = Color(0xFF2C2C2E),
144:    surfaceSunken = Color(0x59000000),
145:    surfaceHover = Color(0x14FFFFFF),
146:    surfaceActive = Color(0x21FFFFFF),
148:    stroke = Color(0x00000000),
149:    strokeStrong = Color(0x1FFFFFFF),
150:    strokeInner = Color(0x00000000),
152:    text = Color(0xFFFFFFFF),
153:    textSecondary = Color(0xFFB3B3B3),
154:    textTertiary = Color(0xFF808080),
156:    tintWarm = Color(0x00000000),
157:    tintCool = Color(0x00000000),
158:    backdrop = listOf(Color(0xFF000000), Color(0xFF000000), Color(0xFF000000)),
```

## 表面工艺参数（ThemeStyle.kt，节选）

```
63:data class RadiusScale(
88: * [fillAlphaScale] 是**乘在调色板 alpha 上的倍数**而不是绝对值：调色板里
93:    val fillAlphaScale: Float,
95:    val accentTint: Float,
109:    val backdropSaturate: Float,
121:    val pressScale: Float = 1f,
137:    val lineHeightFactor: Float,
227:    val wallpaperScrim: Float,
236:    val backdropGlow: Float,
258:        radius = RadiusScale(xs = 2.dp, sm = 4.dp, md = 6.dp, lg = 8.dp, xl = 12.dp),
261:            fillAlphaScale = 1f,
262:            accentTint = 0f,
263:            hairline = 1.dp,
265:            backdropSaturate = Glass.SATURATION,
266:            noise = 0.5f,
271:            shadows = listOf(0.dp, 0.dp, 0.dp),
277:            bodyWeight = FontWeight.Normal, lineHeightFactor = 1.72f,
280:        wallpaperScrim = 0.34f,
281:        backdropGlow = 1f,
293:            fillAlphaScale = 0.58f,
294:            accentTint = 0.12f,
295:            backdropSaturate = Glass.SATURATION,
296:            noise = 0.2f,
298:            shadows = listOf(0.dp, 0.dp, 0.dp),
301:        wallpaperScrim = 0.58f,
302:        backdropGlow = 1f,
311:        radius = RadiusScale(xs = 4.dp, sm = 8.dp, md = 12.dp, lg = 16.dp, xl = 24.dp),
314:            fillAlphaScale = 1f,
315:            accentTint = 0f,
316:            hairline = 0.dp,
318:            backdropSaturate = 1f,
319:            noise = 0f,
321:            shadows = listOf(0.dp, 1.dp, 3.dp),
322:            pressScale = 0.97f,
328:            bodyWeight = FontWeight.Medium, lineHeightFactor = 1.45f,
331:        wallpaperScrim = 0.46f,
332:        backdropGlow = 0f,
346:        radius = RadiusScale(xs = 4.dp, sm = 8.dp, md = 12.dp, lg = 12.dp, xl = 28.dp),
349:            fillAlphaScale = 1f,
350:            accentTint = 0f,
```
