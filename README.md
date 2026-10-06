# JMNeXt4QtDesktop

JMComic 客户端的**桌面端原生重实现**（C++ / Qt），与主项目 [JMNeXt](https://github.com/moyingyilang/JMNeXt) **并行**存在。

## 为什么另开一个项目

主项目的桌面端是 Compose Multiplatform（JVM）：包 70–90MB、冷启动约 1 秒、内存 150–300MB，
而且必须自带 JRE —— 于是打包链要 jlink/jpackage/NSIS 自解压那一整套，Windows 上还容易被杀软误伤。
桌面**没有 ART 这类限制**，用原生实现可以直接拿到：体积约 5–20MB、启动约 50ms、内存约 50MB、无 JVM。

Android 端**继续用 Kotlin/Compose 移植**（ART 侧的性能问题用 Baseline Profiles 与 Compose 优化解，不需要换语言）。

## 两条桌面线的关系

| 项目 | 技术 | 定位 |
| --- | --- | --- |
| JMNeXt（主仓库） | Kotlin + Compose Multiplatform | **稳定线**，继续发布与维护 |
| **JMNeXt4QtDesktop**（本仓库） | **C++ + Qt** | **原生线**，按 [docs/PORT-SPEC.md](docs/PORT-SPEC.md) 逐项对齐功能 |

原则：**本仓库声明自己对齐的功能基线**（例如"对应 JMNeXt 2.1.7 的功能集"），而不是承诺与主项目永远同步 ——
两个仓库各自版本、各自发布，互不阻塞。

## 技术选型

| 项 | 选择 | 理由 |
| --- | --- | --- |
| 语言 | **C++20** | 桌面无运行时限制；与 Qt 结合最顺 |
| UI | **Qt 6**（QML 或 Widgets，见下） | **CJK 文本/断行/字体回退最成熟** —— 主项目这周刚在字体上踩过坑，自绘或轻量框架要重走一遍 |
| 构建 | CMake | Qt 官方支持最好 |
| 版本号 | 与主项目同规则：`x.x.x`（小功能 +001／大功能 +010）、待修项用 `x.x.x.fix(n)` | 规则见主项目 docs/STATE.md 第三十三节 |

**UI 形态（QML 还是 Widgets）尚未定**，等移植规格定稿后再决定：阅读器需要流畅滚动与图片缩放，
QML 的动画/手势更顺，Widgets 的控件与文本更稳。

## 当前状态

**骨架阶段**：本仓库目前只有本文档、`docs/PORT-SPEC.md`（移植规格清单）与一个最小 Qt 程序骨架。
尚未安装工具链（`cmake` + `qt6-base-dev`），**因此还没编译验证过** —— 这一步在装好依赖后进行。

## 相关项目

- **[JMNeXt](https://github.com/moyingyilang/JMNeXt)** —— 主项目：Kotlin + Compose Multiplatform 实现，
  覆盖 **Android**（以此为准，移植路线）与**桌面**（JVM，稳定发布线）。
  本仓库是它的**桌面原生重实现**（C++ / Qt），功能对齐的判据见 [docs/PORT-SPEC.md](docs/PORT-SPEC.md)；
  在逐项对齐完成之前，桌面端请优先使用主项目的发布包。

  共享的领域知识（接口怪癖、切片还原几何、屏蔽语义、版本与更新规则、打包踩坑）都在主仓库的
  `docs/STATE.md` 与 `tools/buildkit/`（工具仓库）里 —— 本仓库实现时应以那些记录为准，不要凭印象重写。

## 运行期依赖（真实数据验证过，缺了会坏）

真实漫画图是 **WebP**。Debian 的 `qt6-base` 只带 gif/ico/jpeg 三个图片插件，
**必须额外安装 `qt6-image-formats-plugins`**，否则真实图片会全部解不开
（实测：安装前解码失败，安装后 852x1280 的 WebP 解码成功）。

## 真实数据验证情况（截至 2026-10-05）

已用**真实服务端**跑通（不再只有假响应）：

| 环节 | 状态 |
| --- | --- |
| 主机发现（两个 BytePlus 入口 → 解密 → 解析 → 归一化） | 真实网络验证通过（4 个候选主机） |
| 业务请求（三个请求头 → {code,data} 信封 → AES 解密） | 真实网络验证通过（真实漫画列表 JSON） |
| 章节图片列表（comic_read） | 真实网络验证通过（含 scramble_id、total_page、逐页 {page,image}） |
| 图片下载 + 解码 + 切片还原 | 真实图片验证通过（852x1280 WebP → 还原写出 PPM） |

真实数据纠正过的假设（都已在代码注释里注明）：响应是 {code,data} **信封**、密文里的 / 被 JSON 转义成 \/、
请求头需要 Accept、列表是**顶层数组**、主机清单是**裸密文**、图片是 **WebP**。

**尚未验证**：账号登录（Authorization: Bearer <jwt>）与需登录的接口；界面观感；Windows 端。

## 怎么构建与运行

### 依赖

```sh
# Debian 12（容器或本机）
apt-get install -y cmake g++ qt6-base-dev qt6-image-formats-plugins
```

`qt6-image-formats-plugins` **不能省**：真实漫画图是 **WebP**，Debian 的 `qt6-base` 只带
gif/ico/jpeg 三个图片插件，缺了它真实图片会全部解不开（已用真实数据验证过：安装前"解码失败"，
安装后 852x1280 解码成功）。

### 构建

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build          # 13 个测试，全部不依赖网络
```

### 运行

```sh
./run.sh                        # 起窗口；默认自动加载真实首页列表
./run.sh --list                 # 显式加载列表
./run.sh --chapters 209827      # 打开某作品并载入第一话
```

无显示环境（CI / 容器）也能验证渲染，用 Xvfb：

```sh
xvfb-run -a -s "-screen 0 1280x800x24" ./build/jmnext4desktop --screenshot /tmp/shot.png --list
```

### 附带的小工具

| 工具 | 用途 |
| --- | --- |
| `jmnext4net` | 真实网络自检：`discover`（主机发现）/ `latest`（真实列表）/ `api <path> [query]`（通用接口探测）/ `get <url> --save 文件` |
| `jmnext4img` | 真实图片 → 解码 → 切片还原 → 写出 PPM（用于验证图片管线） |
| `jmnext4cli` | 纯计算链路：`decrypt <base64> <时间戳>` / `unscramble <raw> <w> <h> <aid> <page> <out.ppm>` |
| `--live` / `--list` / `--reader` / `--reader-async` / `--chapters` / `--screenshot` | 主程序的自检模式，无显示环境可跑，且走的是与界面相同的代码路径 |

## 现在能做什么（已用真实服务端验证）

| 能力 | 状态 |
| --- | --- |
| 主机发现（两个入口 → 解密 → 解析 → 归一化） | 可用（真实网络） |
| 首页列表（80 条真实数据 + 封面缩略图） | 可用 |
| 作品详情（标签、章节表 159 个） | 可用（数据层；界面仅日志输出） |
| 章节选择 | 可用（下拉切换） |
| 阅读器（逐页下载 → 解码 → 切片还原 → 显示 → 翻页） | 可用 |
| 图片缓存（内存 LRU + 磁盘原始字节，跨进程命中） | 可用 |
| 屏蔽规则（关键词子串 / 标签与分类精确匹配） | 可用（已接进列表） |
| 后台线程（列表、章节、翻页都不阻塞界面） | 可用 |
| 账号登录与需登录的接口 | **未做** |
| 详情页/搜索页等完整界面 | **未做** |

## 尚未验证

- ~~**界面观感**~~ —— **2026-10-05 用户已在 VNC 里实际查看，反馈"感觉还行"**，
  即：窗口能起、真实列表与封面能显示、中文字形正常、翻页可用（这是本项目的第一次视觉确认）；
  更细的观感问题（排版细节、动画、长时使用的稳定性）仍待后续反馈；
- 账号登录与一切需要登录的接口；
- **Windows 端**（尤其要确认 webp 插件随包分发，否则真实图片打不开）；
- macOS / 其它发行版。

## 怎么打包（Linux）

```sh
./package-linux.sh            # 产出 dist/JMNeXt4QtDesktop-<版本>-linux-<架构>.{tar.gz,deb}
VERSION=0.1.1 ./package-linux.sh   # 指定版本
```

当前做法与取舍（如实）：

- **不捆绑 Qt**：捆绑要 linuxdeploy/AppImage 那一套，留到正式发版；现在依赖由系统包提供，
  deb 里声明 `Depends: libqt6widgets6, libqt6gui6, libqt6network6, qt6-image-formats-plugins`；
- deb 的依赖名是**在 Debian 12 里 `dpkg` 实测查出来的**，不是猜的；
- 打包产物已在容器里做过"**装 → 跑真实数据 → 卸**"的验证（`dpkg -i` → 用 Xvfb 跑 `--list`
  取到真实 80 条并"加载更多"到 160 条 → `dpkg -r` 干净卸载）。

**未验证**：deb 在其它发行版上的安装；Windows / macOS 包。

## 自 0.1.0 以来新增（尚未发版）

以下改动已在仓库里，但**不在 `v0.1.0` 的包里**（下一次发版会带上）：

| 能力 | 说明 |
| --- | --- |
| **章节列表** | 左侧列表填入全部章节（实测某作品 **159 话**），点选即切换；原先的下拉框已删除 |
| **更新检查工具** | `jmnext4net update [本地版本] [仓库] [--from-file <json>]`：拉 GitHub `releases/latest` → 取 tag → 按核心 `UpdateCheck` 比较 → 给出附件名与直链。**在线那一步未验证**（本容器的 GitHub HTTPS 不通，与主项目情况相同），已用罐装响应验证「取 tag → 比较 → 拼名字」这段逻辑，其中 **`0.1.1` vs `v0.1.1.fix1` 判为新版本**（即 `fix(n)` 规则在真实调用路径上生效） |
| 界面清理 | 删掉废弃的章节下拉框（-8 行）；修掉一处"布局收到空指针"的 silent defect |
| `--help` | 程序内列出全部启动模式与依赖说明（此前 README 提了但代码里没有，属于文档先于代码，已修正） |
| Linux 打包 | `package-linux.sh` → tar.gz + deb；已在 Debian 12 做过「装 → 跑真实数据 → 卸」验证 |

## 已知问题 / 待办

- **x86_64 包**未提供（需要 Qt 交叉工具链；当前容器是 arm64）；
- **未捆绑 Qt**：依赖由系统包提供（`qt6-base` 与 `qt6-image-formats-plugins`，后者缺了真实图片打不开）；
- 界面布局偏挤（控件集中在少数几行），章节列表的实际排版尚未由人确认；
- **账号登录**与一切需登录接口未做（搜索、收藏、追更、下载等随之也无法用）。

## 关于「检查更新」

`jmnext4net update` 会访问 **GitHub API**（`api.github.com`）。如果你的网络访问不到 GitHub，
这一步会失败（程序会打印 `HTTP 0` 与错误原因，不会静默）。

本项目的开发环境就属于这种情况（容器里到 GitHub 的 HTTPS 不通），所以：

- **在线那一步未经真实验证**；
- 已验证的部分是「取 tag → 比较 → 拼附件名/直链」这段逻辑（用罐装响应的 `--from-file` 模式验证过，
  其中 `0.1.1` vs `v0.1.1.fix1` 会被判为新版本，即 `fix(n)` 规则在真实调用路径上生效）。

版本号的单一来源是 `src/core/Version.h`（`APP_VERSION`），主程序 `--version` 与打包脚本都从它取。

## 阅读进度

- 每翻一页都会把「作品 id / 章节 id / 页码」写进 `AppDataLocation/progress.txt`；
- 再次打开**同一作品**时会**接着上次的位置读**（日志里会打印「恢复上次进度：第 N 页」）；
- 存储格式是三行文本（便于人工查看），任何一行不合法就当作"没有进度"而不是猜
  —— 这条规则有单元测试覆盖（`tests/test_read_progress.cpp`，7 组情形：写读一致、覆盖写、
  CRLF 容忍、只有两行、页码非数字、aid 为空、文件不存在）。

## 已知限制：截图模式的时序

`--screenshot` 是**定时**拍照（带子参数时 45 秒）。对 `--reader` 来说这是 **race**：
真实漫画页走 CDN，慢的时候 45 秒也未必到，拍到的会是空窗口。

正确做法是**事件驱动**（页面到达后再拍），尚未实现。因此：

- 判断一张阅读页截图是否有效，**看字节数**：空窗口约 37 KB，真实阅读页约 135 KB；
- 如果拿到的是小文件，重拍一次即可，不要当成"渲染坏了"。

## 运行状态与偏好存在哪里

全部位于 `QStandardPaths::AppDataLocation`（Linux 上实测为 `~/.local/share/jmnext4desktop/`），
都是**纯文本单行文件**，便于查看与排错：

| 文件 | 内容 | 谁写 |
| --- | --- | --- |
| `host.txt` | 上次发现的主机（如 `https://www.cdnhjk.net/`） | 启动时发现成功后写入；下次启动直接用它，省一轮往返（实测 1588 ms -> 16 ms） |
| `progress.txt` | 阅读进度：三行 = 作品 id / 章节 id / 页码 | 每翻一页写入；再次打开同一作品会接着上次读 |
| `style.txt` | 界面风格名：`windowGlass` / `translucent` / `flatBlur` / `miuix` | 命令行 `--theme` 或界面下拉框 |
| `dark.txt` | `dark` 或 `light` | 命令行 `--light` 或界面按钮 |

**重置办法**：删掉对应文件即可。例如：

```sh
rm ~/.local/share/jmnext4desktop/host.txt      # 下次启动重新做主机发现
rm ~/.local/share/jmnext4desktop/progress.txt  # 忘掉阅读进度
rm ~/.local/share/jmnext4desktop/style.txt ~/.local/share/jmnext4desktop/dark.txt   # 回到默认外观
```

**容错约定**：这四个文件读不出来、内容为空或名字不认识时，一律**回落到默认值**
（重新发现主机 / 没有进度 / `windowGlass` / 深色），不因为一个脏值让程序表现异常。
图片缓存在另一个位置（由 `JmClient` 的缓存目录管理，存的是**未还原的原始字节**）。
