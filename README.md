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
