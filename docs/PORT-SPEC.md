# 移植规格清单（JMNeXt4QtDesktop）

**用途**：这是"一等一重实现"的**判据**。每一项都注明规格来源；实现后逐项勾选，全部勾完才有资格取代 Kotlin 桌面版。
规格来源以主项目 `JMNeXt` 仓库为准（`shared/`、`desktop/`、`docs/STATE.md`、测试）。

> 说明：本清单在写的时候**只依据我确实读过的代码与文档**；标「待核对」的是我知道存在、但还没逐字读过的细节，
> 实现前必须回到主仓库核对，不要凭印象写。

## A. 接口与鉴权（最高优先级，全项目的根）

- [ ] 端点表与主机发现（含主机发现接口的**固定密钥**）—— 来源：`shared/src/main/kotlin/com/jmnext/data/crypto/JmCrypto.kt`
- [ ] 请求头 `Tokenparam: "<秒级时间戳>,<客户端版本>"`、`Token: md5(时间戳 + TOKEN_SEED)` —— 同上
- [ ] **响应体 AES-256-ECB 解密**：密钥 = `md5(时间戳 + TOKEN_SEED)` 的 UTF-8 十六进制字符串（32 字节） —— 同上
- [ ] **例外**：广告接口的密钥不带时间戳（`md5(TOKEN_SEED)`） —— 同上
- [ ] 十六进制小写 MD5（与 npm `md5` 包输出一致） —— 同上
- [ ] 会话/登录：Cookie 与登录态保持 —— 来源：`shared/.../remote/JmSession.kt`（**待核对**：具体端点与字段）
- [ ] **怪癖**：未登录时服务端返回 **HTTP 200 + `{"status":"0","msg":"請先登入"}`** 这类，不能只按状态码判断失败 —— 来源：`docs/STATE.md` 与 `DetailScreen` 注释
- [ ] 错误映射：把服务端 `msg` 映射成用户可读文案（主项目多处如此处理）

## B. 图片管线（最大的性能看点）

- [ ] 切片还原的**几何公式**，与 `JMComic_SRC` 的 `utils/Function.js` 逐字对应：
      `num = md5(aid + page)` 推出的份数；`base = h / num`；`remainder = h % num`；
      第 i 条：源区间 `y = h - base*(i+1) - remainder`，目标起点 `py = base*i`；
      `i == 0` 时把 `remainder` **补进条高**（而不是目标位置），使所有条带正好铺满整页 ——
      来源：`shared/src/main/kotlin/com/jmnext/data/image/ImageUnscramble.kt`（`bands()`）
- [ ] `bands()` 的边界条件：`num <= 1`、`height < num`、`width/height <= 0` 时**返回空列表 = 不需要还原**；
      以及 `srcTop/srcBottom/py/room` 的裁剪逻辑 —— 同上
- [ ] 「是否需要还原」的判定入口 —— 来源：`JmRepository.needsUnscramble`（**待核对**）
- [ ] 解码路径与缓存边界：主项目用 Coil，**磁盘缓存存的是"未还原"的原始字节**，转换结果不落盘
      → 冷启动后第一次看到某页仍要重新解码 + 还原一次。**这是管线本身的边界，不是配置漏了** ——
      来源：`app/src/main/kotlin/com/jmnext/data/image/ScrambleTransformation.kt` 注释
- [ ] 缓存键必须包含页号（`scramble:<aid>:<page>`）：同话不同页的 md5 不同、还原方式也不同 —— 同上
- [ ] **原生实现的机会**：把「解码 + 还原」合并成一次操作，避免"解码成位图 → 拷进数组 → 搬移 → 拷回位图"的两次整页拷贝
      （主项目现在是 Kotlin + Skia，瓶颈在拷贝与内存带宽，不在算力）

## C. 行为语义（用户能感知的部分）

- [ ] 收藏 / 追更 / 点赞（接口返回的是**一句人话**，如「已追踪!」，主项目拿它当权威而**不做乐观翻转**） —— 来源：`app/.../detail/DetailScreen.kt` 注释
- [ ] 屏蔽规则：标签规则 + 关键词（作者）规则；`allowedOnce`（允许一次）；「已读过的标签」收敛流程
      —— 来源：`shared/.../TagBlockResolver.kt`、`app/.../home/HomeScreen.kt`、`RandomListScreen.kt`
- [ ] **列表接口不下发标签**，因此屏蔽要靠**按 id 异步取详情拿标签**；并发上限 3；共享缓存避免重复请求 —— 同上
- [ ] 「只读可见条目」与「整批读」的取舍：只读可见 ⇒ 没滚到的条目永远没标签 ⇒ **用户要自己翻过去一遍才生效**（主项目踩过，见 issue #3）——
      现做法：**整批读当前一批，可见的排在最前** —— 来源：`RandomListScreen.kt`
- [ ] 首页：`latest` 分页是 **0-indexed**；分区（推荐位）与「最新上架」是两块数据，**两处都要过滤屏蔽** —— 来源：`HomeViewModel.kt`、`HomeScreen.kt`
- [ ] 阅读器：翻页/跳章/进度记录/缩放与手势、预取 —— 来源：`app/.../reader/`、`desktop/.../PagedReader.kt`（**待核对**）
- [ ] 搜索：排序/检索筛选、历史、`hidden` 计数提示、随机推荐 —— 来源：`SearchScreen.kt`（**待核对**）
- [ ] 下载/离线、壁纸、通知（追更未读）、RSS？—— 来源：`SerialNotify.kt`、`WallpaperStore.kt` 等（**待核对范围**）

## D. 更新检查（含两条血泪规则）

- [ ] 版本比较必须**归一化**：去掉 `v` 前缀、忽略变体后缀（`.lite`/`.debug`） —— 来源：`shared/.../UpdateCheck.kt`
- [ ] **`x.x.x.fix(n)` 规则（2026-10-05 新增）**：待修项的修复版命名为 `2.1.7.fix1` 形式；
      **必须把 `fixN` 当作第四段数字比较**（`[2,1,7,1] > [2,1,7]`），否则修复版会被判定为"已是最新"而**永远推不出去** —— 来源：`docs/STATE.md` 第三十三节
- [ ] 附件名归一化要**保留 `fixN`、只丢 `.lite`/`.debug`**（`2.1.7.fix1.lite` → `2.1.7.fix1`） —— 同上
- [ ] **发布页附件名里不含架构**（一个 APK 同时含多 ABI）→ 由**应用自己拼直链**，不要把用户丢到发布页让他自己挑 —— 来源：`docs/STATE.md` 第三十二节、issue #5
- [ ] 桌面端按 `os.name`/`os.arch` 选包，并**把"内含多架构运行时"的统一包排第一** —— 来源：`UpdateCheck.desktopAssetNames`

## E. 工程与交付

- [ ] 版本号规则：`x.x.x`（小功能 +001／大功能 +010）、`x.x.x.fix(n)` 用于待修项
- [ ] 发布矩阵（原生版可**大幅简化**）：Windows x64/arm64、Linux aarch64/x86_64；是否需要"统一包"视体积而定
- [ ] 打包：Windows 用 Qt 的部署工具（`windeployqt`），Linux 用 `linuxdeploy`/手写 tar.gz、deb、rpm、AppImage
- [ ] **验收脚本**：主项目用 `verify-apk.sh` 断言产物（APK 包名/版本/manifest/资源）；
      原生版应对每个平台产物有一份**断言式验收**（不能只看"文件存在、命令退出码为 0"——主项目为此吃过发布坏包的亏）
- [ ] 日志与诊断：崩溃/启动失败**必须留下可读日志**（主项目的 exe 曾因静默无输出而无法定位）

## F. 对齐验证（"一等一"的客观判据）

- [ ] 移植主项目 **56 个共享层测试**的语义（`shared/src/test`）：算法类（切片几何、版本比较、屏蔽匹配）**逐条对齐**
- [ ] 关键 UI 行为做**人工对照清单**（同一作品、同一账号、两台机器对比：列表顺序、屏蔽结果、阅读器交互）
- [ ] 声明对齐基线：本仓库 README 里写明"对应 JMNeXt <版本> 的功能集"

## 当前状态

- 本清单为**初稿**：A/B/D 三节依据我确实读过的代码与文档；C/E 的部分条目与 B 的两处标了「待核对」。
- 下一步（实现前）：把「待核对」逐条回主仓库读实，并补齐 C 节（阅读器、搜索、下载、壁纸、通知的真实范围）。
