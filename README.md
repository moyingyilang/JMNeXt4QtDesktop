# JMNeXt4QtDesktop

**faster, lighter, smaller 的 JMNeXt 桌面版。** 本项目是 [JMNeXt](https://github.com/moyingyilang/JMNeXt) 的
**桌面端原生重实现**（C++20 / Qt 6），与主项目并行开发，目标是在相同功能集下获得更小的体积、
更快的启动速度与更低的内存占用。

> **项目状态**：早期阶段，已发布两个测试版（最新 `v0.1.1`，仅 Linux arm64）。
> 核心链路（主机发现、列表、详情、章节、阅读器、缓存、屏蔽、阅读进度）已使用**真实服务端与真实图片**
> 验证通过，16 个单元测试全部通过。**账号登录与视觉对齐尚未实现。**
> 在逐项对齐完成之前，桌面端建议优先使用主项目 JMNeXt 的发布包。

## 项目背景

主项目的桌面端基于 Compose Multiplatform（JVM）：包体积 70–90MB、冷启动约 1 秒、内存 150–300MB，
且必须自带 JRE，因此打包链需要 jlink / jpackage / NSIS 自解压一整套流程，在 Windows 上也容易被杀软误报。

桌面端没有 ART 一类运行时限制，改用原生实现可直接获得约 5–20MB 体积、约 50ms 启动、
约 50MB 内存，且无需 JVM。

### 不提供 Android 版本的原因

**本仓库仅面向桌面端，不产出 Android 包**，原因是技术链差异，具体如下。

1. **Android 的性能瓶颈不在这一层**：Android 端的问题是 ART 运行时的编译与启动开销，
   主项目通过 Baseline Profiles 与 Compose 优化即可解决（`full` / `lite` 两种裁剪亦出于同一思路），
   改用 C++ / Qt 并不能绕开 ART；
2. **现有代码按桌面栈编写**：Widgets 与 Qt Quick 两个前端、`QStandardPaths` 偏好目录、
   tar.gz / deb 打包链与 deb 依赖声明均属桌面栈；迁移到 Android 需要将平台适配、打包与生命周期
   整套重做；
3. **两端各有一套实现意味着双份维护**：主项目的 Android 端本就以 Kotlin / Compose 维护，
   再增加一份 Qt / Android 实现会使同一协议存在两处实现。

因此 Android 端继续由主项目 [JMNeXt](https://github.com/moyingyilang/JMNeXt) 提供（Kotlin / Compose），
需要 Android 版本时请使用该仓库的发布包。

## 与主项目的关系

| 项目 | 技术 | 定位 |
| --- | --- | --- |
| [JMNeXt](https://github.com/moyingyilang/JMNeXt)（主仓库） | Kotlin + Compose Multiplatform | **稳定线**，继续发布与维护 |
| **JMNeXt4QtDesktop**（本仓库） | **C++20 + Qt 6** | **原生线**，按 [docs/PORT-SPEC.md](docs/PORT-SPEC.md) 逐项对齐功能 |

两个仓库各自确定版本、各自发布，互不阻塞。多源方向由独立项目
[TriComiX](https://github.com/moyingyilang/TriComiX) 推进，**Qt 线是否支持多源尚未决定**。

## 下载

发布页：[github.com/moyingyilang/JMNeXt4QtDesktop/releases](https://github.com/moyingyilang/JMNeXt4QtDesktop/releases)。

| 平台 | 格式 | 附件名 | 状态 |
| --- | --- | --- | --- |
| Linux arm64 | deb | `JMNeXt4QtDesktop-0.1.1-linux-arm64.deb` | 已在 Debian 12 完成「安装、运行真实数据、卸载」验证 |
| Linux arm64 | 便携 tar.gz | `JMNeXt4QtDesktop-0.1.1-linux-arm64.tar.gz` | 同上 |
| Linux x86_64 | 无 | 无 | **未提供**（需要 Qt 交叉工具链） |
| Windows / macOS | 无 | 无 | **未实现** |

**不捆绑 Qt**：依赖由系统包提供，deb 中声明的依赖为
`Depends: libqt6widgets6, libqt6gui6, libqt6network6, qt6-image-formats-plugins`
（依赖名经 Debian 12 的 `dpkg` 实测确认）。

## 功能与验证方式

| 能力 | 验证方式 |
| --- | --- |
| 端点表（45 条，由主项目常量机械抽取） | 抽样断言 |
| MD5 / AES-256-ECB / PKCS#7 / Base64 | RFC 1321 向量、NIST SP800-38A 向量、openssl 交叉验证 |
| 响应解密（`{code,data}` 信封、`\/` 还原、多 seed） | 单元测试与**真实网络** |
| 主机发现（两个入口、固定种子、裸密文、不带凭据） | 单元测试与**真实网络**（4 个候选主机） |
| 首页列表、封面与分页 | **真实网络**（80 至 160 条，封面 400x533） |
| 详情（标题 / 作者 / 标签 / 封面） | **真实网络**（标签 5 个、章节 159 个） |
| 章节列表点选切换 | **真实网络**（159 项） |
| 阅读器：逐页下载、WebP 解码、切片还原、显示；键盘与滚轮翻页；适应窗口 / 100%；单页与双页 | **真实网络**（某章节 285 页，真实图 852x1280） |
| 图片缓存（内存 LRU 与磁盘原始字节） | 跨进程命中实测 |
| 翻页预取 | 缓存统计（10 次取图命中 4 次） |
| 屏蔽规则（关键词子串、标签与分类精确匹配） | 单元测试与**真实数据**（80 降至 64） |
| 阅读进度（写入与重新打开后继续） | 单元测试 7 组与两次运行实测 |
| 全链路后台线程（列表、章节、翻页不阻塞界面） | 异步自检（走 worker 代码路径） |
| 搜索 | CLI 与应用内两条路径：`test` 返回 45 条、中文「巨乳」返回 80 条 |
| 打包（tar.gz 与 deb） | Debian 12 上完成「安装、运行真实数据、卸载」 |
| 更新检查（含 `fix(n)` 规则） | 单元测试与罐装响应；**在线步骤未验证** |

**尚未实现**：账号登录及所有需登录接口（收藏、追更、下载、屏蔽设置页）；界面布局整理；
x86_64 与 Windows / macOS 包；视觉系统完整对齐；未捆绑 Qt。阅读进度仅记录**最后一次**
（主项目按作品记录多条）。

## 快速开始

### 依赖

```sh
# Debian 12（容器或本机）
apt-get install -y cmake g++ qt6-base-dev qt6-image-formats-plugins
```

`qt6-image-formats-plugins` **不可省略**：真实漫画图为 **WebP**，Debian 的 `qt6-base` 仅包含
gif / ico / jpeg 三个图片插件，缺少该包会导致真实图片全部无法解码
（实测：安装前解码失败，安装后 852x1280 的 WebP 解码成功）。

### 构建与测试

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build          # 16 个测试，均不依赖网络
```

建议使用 Ninja 生成器（`-G Ninja`）：Makefile 生成器每次都会重编 QML 缓存并重新链接，
无改动增量构建需 13–25 秒，Ninja 为 0.04–0.20 秒。CMakeLists 检测到 `ccache` 时会自动启用
（清空重建由 72.5 秒降至 13 秒），如需关闭可传入 `-DCMAKE_CXX_COMPILER_LAUNCHER=`。

### 运行

```sh
./run.sh                        # 启动窗口，默认自动加载真实首页列表
./run.sh --list                 # 显式加载列表
./run.sh --chapters 209827      # 打开指定作品并载入章节列表
```

无显示环境（CI / 容器）下同样可以验证，可使用 `QT_QPA_PLATFORM=offscreen` 或 Xvfb：

```sh
QT_QPA_PLATFORM=offscreen ./build/jmnext4desktop --list
xvfb-run -a -s "-screen 0 1280x800x24" ./build/jmnext4desktop --screenshot /tmp/shot.png --list
```

### 环境自检

```sh
./build/jmnext4net discover                  # 主机发现：预期输出一个 https:// 主机
./build/jmnext4desktop --list                # 预期「列表已加载：80 条」与「加载更多之后：160 条」
./build/jmnext4desktop --chapters 209827     # 预期「章节列表项数：159」
```

### 附带工具

| 工具 | 用途 |
| --- | --- |
| `jmnext4net` | 真实网络自检：`discover` / `latest` / `api <path> [query]` / `get <url> --save 文件` / `update [本地版本] [仓库] [--from-file <json>]` |
| `jmnext4img` | 真实图片解码、切片还原并输出 PPM（用于验证图片管线） |
| `jmnext4cli` | 纯计算链路：`decrypt <base64> <时间戳>` / `unscramble <raw> <w> <h> <aid> <page> <out.ppm>` |
| `jmnext4qml` | QML 前端（见下节），支持 `--selftest` / `--shot <png>` 等自检模式 |
| 主程序自检模式 | `--live` / `--list` / `--reader` / `--reader-async` / `--chapters` / `--search` / `--zoom` / `--screenshot` / `--page <本地图>` |

以上自检模式**使用与界面相同的代码路径**。注意**模式参数必须位于第一个参数位置**
（`--selftest` / `--shot <png>` / `--list` 等），`--theme` / `--light` / `--verbose` 可置于其后。

## 界面

| 实现 | 状态 |
| --- | --- |
| **Widgets**（`jmnext4desktop`，默认） | 左右分栏：搜索框与列表、章节列表、详情区、阅读区（页码与百分比、单页与双页）。用户已在 VNC 中查看，反馈为「感觉还行」 |
| **QML**（`jmnext4qml`） | 目标是与 Kotlin（Compose）版**一对一复刻**。已完成：Qt Quick 运行验证（P0）、共用 Qt 层抽取、后端桥（P1b，实测取得列表 80 条与真实一页图）、图片提供器（P1c，跨实例命中磁盘缓存）、四屏（首页 / 详情 / 阅读器 / 搜索）、主题 token 接入与按压弹性。**尚未发版** |

QML 侧阅读器的渲染路径已通过**离线通道**验证（`--page <本地图>`：绘制 784x518，
截图由 224348 字节增至 348687 字节）；布局修复后使用真实网络页面复测时，因 CDN 波动未能一次性
取得全部证据，该项列为未验证。

`docs/ui-inventory/01..04` 是对主项目界面的清点文档（24 个逻辑屏幕、105 个 `@Composable`、
23 项效果与 19 项动画、桌面模块 49 个文件 / 8703 行），是 1:1 复刻的依据。

## 主题支持范围

界面支持四套具名风格（`windowGlass` / `translucent` / `flatBlur` / `miuix`，与主项目同名），
配色与圆角**照搬主项目数值**（由 `tools/gen-theme-tokens.sh` 从主项目生成，
结果提交于 `src/qt/ThemeTokens.h`，构建时不依赖主项目）。可通过命令行 `--theme <名称>`、`--light`
或界面切换，两种方式都会被持久化。

主项目的五套风格是五套**表面工艺**而非五套配色，当前技术栈无法完全复刻，逐项说明如下。

| 主项目要素 | 本实现 |
| --- | --- |
| 配色 / 圆角尺度 / 描边 | 支持（来自生成表） |
| 按下反馈 | 支持（QSS `:pressed` 与 QML 侧按压弹性；Widgets 侧无弹性缩放） |
| 投影（多级 shadows） | Widgets 侧仅为封面与阅读区各挂一层（取保守值，未按主项目校准） |
| 颗粒 / Acrylic noise | **不支持**（需自绘噪声纹理） |
| 高斯模糊 / 背景透明（flatBlur、translucent 的关键效果） | Widgets 侧**不支持**（无 backdrop blur）；QML 侧容器为 Qt 6.4，缺少 `MultiEffect`，需 compat 或自写 shader |
| 行高（lineHeightFactor） | **不支持**（QSS 无 line-height） |
| 壁纸压暗 / 背光 | **不支持**（本实现也没有壁纸功能） |
| Material You 3（第五套风格） | **未接入** |

因此当前实现为「**四套具名风格加可落地的子集**」。不做成五个仅名称不同、观感一致的主题是有意为之，
主项目注释中亦曾指出该做法的问题。

## 运行状态与偏好文件

全部位于 `QStandardPaths::AppDataLocation`（Linux 上实测为 `~/.local/share/jmnext4desktop/`），
均为**纯文本单行文件**，便于查看与排查。

| 文件 | 内容 | 写入方 |
| --- | --- | --- |
| `host.txt` | 上次发现的主机 | 发现成功后写入；下次直接使用，节省一轮往返（实测 1588 ms 降至 16 ms） |
| `progress.txt` | 阅读进度：三行，依次为作品 id、章节 id、页码 | 每翻一页写入；重新打开同一作品时继续上次位置 |
| `style.txt` | 界面风格名 | 命令行 `--theme` 或界面切换 |
| `dark.txt` | `dark` 或 `light` | 命令行 `--light` 或界面切换 |

**容错约定**：上述文件无法读取、内容为空或取值未知时，一律回落至默认值
（重新发现主机 / 无进度 / `windowGlass` / 深色），不会因单个异常值导致行为异常。

```sh
rm ~/.local/share/jmnext4desktop/host.txt      # 下次启动重新执行主机发现
rm ~/.local/share/jmnext4desktop/progress.txt  # 清除阅读进度
rm ~/.local/share/jmnext4desktop/style.txt ~/.local/share/jmnext4desktop/dark.txt   # 恢复默认外观
```

图片缓存位于另一路径（由 `JmClient` 的缓存目录管理，存储的是**未还原的原始字节**）。

## 性能基线

以下为本机实测值，`[ms]` 取自程序日志时间戳。

| 场景 | 耗时 | 说明 |
| --- | --- | --- |
| 首屏至第一页（无任何缓存） | 3685 ms | 主机发现、详情、章节、首图，共四次串行请求 |
| 首屏至第一页（主机缓存命中、图片磁盘缓存温） | 1091 ms | 节省约 2.6 秒 |
| 主机发现（有缓存与无缓存） | 16 ms 与 1588 ms | `AppDataLocation/host.txt` |
| 翻至下一页（预取命中） | 59–177 ms | 预取在后台线程完成，不阻塞界面 |

**测量边界**：数据来自同一台机器、同一网络环境，更换网络或机器会得到不同结果。
首屏剩余耗时主要来自「详情 → 章节 → 首图」三次串行请求（章节 id 依赖详情，无法并行）。

## 打包

```sh
./package-linux.sh                 # 产出 dist/JMNeXt4QtDesktop-<版本>-linux-<架构>.{tar.gz,deb}
VERSION=0.1.2 ./package-linux.sh   # 指定版本
```

版本号单一来源为 `src/core/Version.h` 中的 `APP_VERSION`，主程序 `--version` 与打包脚本均从该处读取。
**已知问题**：`CMakeLists.txt` 的 `project(... VERSION ...)` 仍为 0.1.0，发版前应一并更新。

## 已知问题与待办

按建议处理顺序排列：

1. **账号登录**及所有需登录接口，需要真实账号；
2. **界面布局整理**：控件集中在少数几行，建议先截图确认再调整；
3. **x86_64 包**：当前仅产出 linux-arm64，需要 Qt 交叉工具链；
4. **视觉系统对齐**：当前为「四套具名风格加可落地的子集」，与主项目差距最大的一项；
5. **未捆绑 Qt**：依赖系统包，正式发版时应考虑 AppImage 或捆绑方案；
6. `--screenshot` 为**定时截图**（带子参数时等待 45 秒），对阅读页存在竞态：
   空窗口约 37 KB，真实阅读页 135 KB 以上，**取到较小文件时重新截图即可**，不应判断为渲染故障；
7. **在线更新检查未验证**：开发环境无法访问 `api.github.com` 的 HTTPS（`HTTP 0`）。
   已验证的部分为「取 tag、比较版本、拼接附件名与直链」的逻辑（使用罐装响应验证，
   其中 `0.1.1` 对 `v0.1.1.fix1` 判定为新版本，即 `fix(n)` 规则在真实调用路径上生效）。

**尚未验证**：账号登录链路；Windows / macOS / 其它发行版的包；x86_64；
QML 侧使用真实网络页面复测（离线通道已排除渲染侧问题）；QML 界面上的点击路径（无头环境无法操作）。

## 文档

| 文件 | 内容 |
| --- | --- |
| [`STATUS.md`](STATUS.md) | **新接手者优先阅读**：现状、逐项验证方式、性能基线、已记录问题、QML 推进状态 |
| [`docs/PORT-SPEC.md`](docs/PORT-SPEC.md) | 移植规格清单：需要对齐的行为与功能基线 |
| [`docs/THEME-TOKENS.md`](docs/THEME-TOKENS.md) | 主题 token 的搬运方式与逐项对照 |
| [`docs/ui-inventory/`](docs/ui-inventory/) | 主项目界面清点：屏幕、组件、效果与动画、桌面模块 |

## 致谢

- **[moyingyilang](https://github.com/moyingyilang) 的 [JMNeXt](https://github.com/moyingyilang/JMNeXt)**：
  本项目的规格来源与功能基线。协议常量、切片还原几何、屏蔽语义与主题 token 均取自该项目
  （同一作者，AGPL-3.0，见 [LICENSE](LICENSE)）。
- **JMNeXt 致谢中的全部项目同样适用**：**[tiann](https://github.com/tiann) 的
  [KernelSU](https://github.com/tiann/KernelSU)**（悬浮底栏与四栏切换的几何）、
  **[raoxwup](https://github.com/raoxwup) 的 [haka_comic](https://github.com/raoxwup/haka_comic)**
  （屏蔽设计）、**[moyingyilang](https://github.com/moyingyilang) 的
  [moyingyilang.github.io](https://moyingyilang.github.io)**（UI 设计语言与设计令牌），
  以及所有提交 issue 与在真机上验证修复的使用者。

## 许可

本项目以 **GNU Affero General Public License v3.0（AGPL-3.0）** 发布，全文见 [LICENSE](LICENSE)。

与主项目 JMNeXt 使用同一许可证。AGPL 要求派生作品同样以 AGPL 开放，
因此任何基于本项目的分发（包括以网络服务形式提供）都必须附带完整源码。

本项目是 JMComic 客户端的第三方重实现，与官方无关；禁漫、JMComic 及相关内容的权利归其权利人所有。
