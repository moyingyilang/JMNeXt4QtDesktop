// HistoryStore / FavoriteStore 单元测试（新文件；不改 CMakeLists.txt，也不改 src 下任何现有文件）。
//
// 约定：
//   - 不引入 QtTest 或任何测试框架，只用普通函数 + 断言；
//   - 每个用例打印「用例名: PASS/FAIL」，任一断言失败时 main 返回非零；
//   - 断言失败时额外打印具体是哪一条断言、实际值是多少。
//
// 为什么必须打开 QStandardPaths 测试模式：
//   两个 store 都把 JSON 写在 QStandardPaths::AppDataLocation 下（history.json /
//   favorites.json）。不打开测试模式，用例会直接读写当前用户的真实数据目录，跑一次
//   测试就会覆盖掉用户的阅读历史和收藏。QStandardPaths::setTestModeEnabled(true)
//   把可写位置重定向到 ~/.qttest/...，测试进程从头到尾碰不到真实数据。
//   它必须在任何 writableLocation()/store 实例之前调用，否则 Qt 已经算出了旧路径。
//
// 为什么坏 JSON 用例要单独起进程：
//   HistoryStore 与 FavoriteStore 都是单例，load() 只在进程里第一次 instance() 时
//   执行。如果先 remember 再手工把文件写坏，list() 读的是内存，永远不会重新 load，
//   测不到「文件损坏后启动」这条路径。Unix 下用 fork + execvp 重新执行自己，让新进程
//   在加载前看到坏文件——这正好对应真实用户「文件被改坏后重启应用」。
//   非 Unix 没有 fork/exec，退化为在本进程首次加载前写坏文件：因为坏 JSON 用例排在
//   所有其他用例之前，这时 store 还没实例化，仍然是一次真实加载（只是不能重复）。
//
// 依赖：src/core/FavoriteStore.h 由并行任务提供，接口为
//   list() / remember(aid, title, folder) / remove(aid) / clear() / contains(aid)。
//   本文件只使用这些公开接口和 FavoriteEntry 的 aid/title/folder/timestamp 字段。
//
// 覆盖范围（共 13 个用例）见 main() 里的调用列表；未覆盖的部分写在 main() 末尾的注释。
// 需要链接 Qt6::Core；测试目标由维护者加入 CMakeLists.txt，本文件不自带构建逻辑。

#include "core/FavoriteStore.h"
#include "core/HistoryStore.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QString>
#include <QVector>

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <thread>

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

using jmnext::core::FavoriteEntry;
using jmnext::core::FavoriteStore;
using jmnext::core::HistoryEntry;
using jmnext::core::HistoryStore;

// 子进程场景参数：重新执行自己时用来区分「跑坏 JSON 用例」和「跑全部用例」。
static constexpr const char* kScenarioHistoryCorrupt = "--scenario=history-corrupt";
static constexpr const char* kScenarioFavoriteCorrupt = "--scenario=favorite-corrupt";

static constexpr const char* kCaseHistoryCorrupt = "用例「历史-坏 JSON 重新读取」";
static constexpr const char* kCaseFavoriteCorrupt = "用例「收藏-坏 JSON 重新读取」";

static char* g_argv0 = nullptr;   // 供 fork 后 execvp 重新执行自己
static int g_casesRun = 0;
static int g_casesFailed = 0;

// ---------------------------------------------------------------------------
// 最朴素的断言与用例结果输出
// ---------------------------------------------------------------------------

static bool check(bool cond, const char* what, const QString& detail = QString()) {
    if (!cond) {
        if (detail.isEmpty()) {
            std::printf("  断言失败：%s\n", what);
        } else {
            std::printf("  断言失败：%s（%s）\n", what, detail.toUtf8().constData());
        }
        std::fflush(stdout);
    }
    return cond;
}

// 只打印结果，不改计数（子进程里用；父进程用 caseDone 统一统计）。
static void printCase(const char* name, bool ok) {
    std::printf("%s: %s\n", name, ok ? "PASS" : "FAIL");
    std::fflush(stdout);
}

static void caseDone(const char* name, bool ok) {
    printCase(name, ok);
    ++g_casesRun;
    if (!ok) ++g_casesFailed;
}

static QString sizeText(int size) {
    return QStringLiteral("实际 size=%1").arg(size);
}

// ---------------------------------------------------------------------------
// 路径与文件辅助
// ---------------------------------------------------------------------------

static QString dataDir() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

static QString historyPath() {
    return dataDir() + QStringLiteral("/history.json");
}

static QString favoritePath() {
    return dataDir() + QStringLiteral("/favorites.json");
}

static void removeFile(const QString& path) {
    QFile::remove(path);
}

static bool writeFile(const QString& path, const QByteArray& bytes) {
    QDir().mkpath(dataDir());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return f.write(bytes) == bytes.size();
}

// HistoryEntry 与 FavoriteEntry 都有 aid，按 aid 找一条（返回指向局部拷贝内部的指针，
// 调用方只在 entries 还活着时使用）。
template <typename Entry>
static const Entry* findEntry(const QVector<Entry>& entries, const QString& aid) {
    for (const Entry& e : entries) {
        if (e.aid == aid) return &e;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// 坏 JSON 场景（子进程里执行；非 Unix 时在父进程首次加载前执行）
// ---------------------------------------------------------------------------

static bool scenarioHistoryCorrupt() {
    bool ok = true;
    const QString path = historyPath();
    removeFile(path);
    ok = check(writeFile(path, QByteArray("{not json")),
               "应能把坏 JSON 写进 history.json") && ok;
    if (!ok) {
        printCase(kCaseHistoryCorrupt, false);
        return false;
    }

    // 这里才第一次构造单例，load() 读到的就是上面写的坏文件。
    HistoryStore& store = HistoryStore::instance();
    const QVector<HistoryEntry> entries = store.list();
    ok = check(entries.isEmpty(), "坏 JSON 加载后 list() 应退化为空",
               sizeText(static_cast<int>(entries.size()))) && ok;

    store.clear();   // 坏文件下 clear() 不能崩溃
    ok = check(store.list().isEmpty(), "坏 JSON 后 clear() 不应崩溃且列表为空") && ok;

    store.remember(QStringLiteral("after-corrupt"), QStringLiteral("修复后仍可写"));
    ok = check(store.list().size() == 1, "坏 JSON 被 clear() 覆盖后仍能 remember") && ok;

    store.clear();
    removeFile(path);
    printCase(kCaseHistoryCorrupt, ok);
    return ok;
}

static bool scenarioFavoriteCorrupt() {
    bool ok = true;
    const QString path = favoritePath();
    removeFile(path);
    ok = check(writeFile(path, QByteArray("{not json")),
               "应能把坏 JSON 写进 favorites.json") && ok;
    if (!ok) {
        printCase(kCaseFavoriteCorrupt, false);
        return false;
    }

    FavoriteStore& store = FavoriteStore::instance();
    const QVector<FavoriteEntry> entries = store.list();
    ok = check(entries.isEmpty(), "坏 JSON 加载后 list() 应退化为空",
               sizeText(static_cast<int>(entries.size()))) && ok;
    ok = check(!store.contains(QStringLiteral("any")),
               "坏 JSON 加载后 contains() 应为 false") && ok;

    store.clear();   // 坏文件下 clear() 不能崩溃
    ok = check(store.list().isEmpty(), "坏 JSON 后 clear() 不应崩溃且列表为空") && ok;

    store.remember(QStringLiteral("after-corrupt"), QStringLiteral("标题"),
                   QStringLiteral("分组"));
    ok = check(store.contains(QStringLiteral("after-corrupt")),
               "坏 JSON 被覆盖后仍能 remember") && ok;

    store.clear();
    removeFile(path);
    printCase(kCaseFavoriteCorrupt, ok);
    return ok;
}

// Unix：fork + execvp 自己，新进程里单例尚未加载，才能真的重新读一次坏文件。
// 非 Unix：没有 fork，退化为在本进程首次加载前验证（调用点保证排在其他用例之前）。
static bool runChildCase(const char* scenario, const char* caseName) {
#ifdef Q_OS_UNIX
    std::fflush(stdout);   // 避免 fork 把父进程未刷出的输出复制一份
    const pid_t pid = fork();
    if (pid < 0) {
        std::printf("%s: FAIL（fork 失败：%s）\n", caseName, std::strerror(errno));
        ++g_casesRun;
        ++g_casesFailed;
        return false;
    }
    if (pid == 0) {
        char* const argv[] = { g_argv0, const_cast<char*>(scenario), nullptr };
        execvp(g_argv0, argv);
        std::perror("execvp");
        _exit(127);
    }

    int status = 0;
    pid_t waited = 0;
    do {
        waited = waitpid(pid, &status, 0);
    } while (waited < 0 && errno == EINTR);

    if (waited < 0) {
        std::printf("%s: FAIL（waitpid 失败：%s）\n", caseName, std::strerror(errno));
        ++g_casesRun;
        ++g_casesFailed;
        return false;
    }

    ++g_casesRun;
    if (WIFEXITED(status)) {
        const int code = WEXITSTATUS(status);
        if (code == 0) return true;
        ++g_casesFailed;
        if (code != 1) {
            // 子进程退出码 1 表示它自己已经打印过 FAIL 和具体断言；其它退出码
            // （例如 execvp 失败 127）没打印过用例名，这里补一行。
            std::printf("%s: FAIL（子进程异常退出，退出码 %d）\n", caseName, code);
        }
        return false;
    }

    ++g_casesFailed;
    const int sig = WIFSIGNALED(status) ? WTERMSIG(status) : -1;
    std::printf("%s: FAIL（子进程被信号 %d 终止）\n", caseName, sig);
    return false;
#else
    (void)caseName;
    bool ok = false;
    if (std::strcmp(scenario, kScenarioHistoryCorrupt) == 0) {
        ok = scenarioHistoryCorrupt();
    } else if (std::strcmp(scenario, kScenarioFavoriteCorrupt) == 0) {
        ok = scenarioFavoriteCorrupt();
    }
    ++g_casesRun;
    if (!ok) ++g_casesFailed;
    return ok;
#endif
}

// ---------------------------------------------------------------------------
// HistoryStore 用例
// ---------------------------------------------------------------------------

static bool caseHistoryEmpty() {
    bool ok = true;
    // 先删文件再取单例：父进程里第一次 instance() 应当走「文件不存在」的分支。
    removeFile(historyPath());
    HistoryStore& store = HistoryStore::instance();
    const QVector<HistoryEntry> entries = store.list();
    ok = check(entries.isEmpty(), "空表时 list() 应为空",
               sizeText(static_cast<int>(entries.size()))) && ok;
    return ok;
}

static bool caseHistoryRemember() {
    bool ok = true;
    HistoryStore& store = HistoryStore::instance();
    store.clear();
    store.remember(QStringLiteral("1001"), QStringLiteral("标题一"));

    const QVector<HistoryEntry> entries = store.list();
    ok = check(entries.size() == 1, "remember 后应只有一条记录",
               sizeText(static_cast<int>(entries.size()))) && ok;
    const HistoryEntry* e = findEntry(entries, QStringLiteral("1001"));
    ok = check(e != nullptr, "remember 后应能按 aid 找到该条") && ok;
    if (e) {
        ok = check(e->title == QStringLiteral("标题一"),
                   "标题应与 remember 传入的一致", e->title) && ok;
        ok = check(e->timestamp > 0, "时间戳应大于 0",
                   QStringLiteral("实际 timestamp=%1").arg(e->timestamp)) && ok;
    }
    return ok;
}

static bool caseHistoryDuplicate() {
    bool ok = true;
    HistoryStore& store = HistoryStore::instance();
    store.clear();
    store.remember(QStringLiteral("dup"), QStringLiteral("旧标题"));
    const QVector<HistoryEntry> before = store.list();
    const qint64 firstTs = before.isEmpty() ? 0 : before.first().timestamp;

    // 隔几毫秒，让「时间更新」在毫秒精度下也能观察到；相等仍接受（见下面的断言）。
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    store.remember(QStringLiteral("dup"), QStringLiteral("新标题"));

    const QVector<HistoryEntry> entries = store.list();
    ok = check(entries.size() == 1, "同一 aid 重复 remember 不应产生两条",
               sizeText(static_cast<int>(entries.size()))) && ok;
    if (!entries.isEmpty()) {
        ok = check(entries.first().aid == QStringLiteral("dup"),
                   "去重后 aid 仍为 dup", entries.first().aid) && ok;
        ok = check(entries.first().title == QStringLiteral("新标题"),
                   "重复 remember 应更新标题", entries.first().title) && ok;
        ok = check(entries.first().timestamp >= firstTs,
                   "重复 remember 后时间戳不应倒退",
                   QStringLiteral("旧=%1 新=%2")
                       .arg(firstTs)
                       .arg(entries.first().timestamp)) && ok;
    }
    return ok;
}

static bool caseHistoryOverflow() {
    bool ok = true;
    HistoryStore& store = HistoryStore::instance();
    store.clear();

    // 只比上限多 5 条：能验证「淘汰最旧」即可，避免用例太慢。
    // 每次 remember 都会整表落盘，插入 1005 条会有约 1005 次写文件，属正常现象。
    constexpr int kExtra = 5;
    const int total = HistoryStore::kMax + kExtra;   // 1005
    std::printf("  （正在插入 %d 条历史，每次都会整表落盘，稍慢）\n", total);
    std::fflush(stdout);
    for (int i = 1; i <= total; ++i) {
        store.remember(QString::number(i), QStringLiteral("标题%1").arg(i));
    }

    const QVector<HistoryEntry> entries = store.list();
    ok = check(entries.size() <= HistoryStore::kMax, "超过上限后条数不得超过 kMax",
               sizeText(static_cast<int>(entries.size()))) && ok;
    ok = check(entries.size() == HistoryStore::kMax, "超过上限后应恰好保留 kMax 条",
               sizeText(static_cast<int>(entries.size()))) && ok;
    if (!entries.isEmpty()) {
        ok = check(entries.first().aid == QString::number(total),
                   "首条应是最新插入的一条", entries.first().aid) && ok;
        ok = check(entries.last().aid == QString::number(kExtra + 1),
                   "尾部应是最旧的保留条目（最早的 5 条被淘汰）",
                   entries.last().aid) && ok;
        ok = check(findEntry(entries, QString::number(1)) == nullptr,
                   "最早插入的条目应已被淘汰") && ok;
    }
    return ok;
}

static bool caseHistoryClear() {
    bool ok = true;
    HistoryStore& store = HistoryStore::instance();
    store.clear();
    store.remember(QStringLiteral("to-clear"), QStringLiteral("待清空"));
    ok = check(store.list().size() == 1, "clear() 前应有一条记录") && ok;

    store.clear();
    const QVector<HistoryEntry> entries = store.list();
    ok = check(entries.isEmpty(), "clear() 后 list() 应为空",
               sizeText(static_cast<int>(entries.size()))) && ok;
    return ok;
}

// ---------------------------------------------------------------------------
// FavoriteStore 用例
// ---------------------------------------------------------------------------

static bool caseFavoriteEmpty() {
    bool ok = true;
    // 先删文件再取单例：父进程里第一次 instance() 应当走「文件不存在」的分支。
    removeFile(favoritePath());
    FavoriteStore& store = FavoriteStore::instance();
    const QVector<FavoriteEntry> entries = store.list();
    ok = check(entries.isEmpty(), "空表时 list() 应为空",
               sizeText(static_cast<int>(entries.size()))) && ok;
    ok = check(!store.contains(QStringLiteral("any")), "空表时 contains() 应为 false") && ok;
    return ok;
}

static bool caseFavoriteRemember() {
    bool ok = true;
    FavoriteStore& store = FavoriteStore::instance();
    store.clear();
    store.remember(QStringLiteral("2001"), QStringLiteral("收藏A"), QStringLiteral("分组甲"));

    const QVector<FavoriteEntry> entries = store.list();
    ok = check(entries.size() == 1, "remember 后应只有一条记录",
               sizeText(static_cast<int>(entries.size()))) && ok;
    const FavoriteEntry* e = findEntry(entries, QStringLiteral("2001"));
    ok = check(e != nullptr, "remember 后应能按 aid 找到该条") && ok;
    if (e) {
        ok = check(e->title == QStringLiteral("收藏A"),
                   "标题应与 remember 传入的一致", e->title) && ok;
        ok = check(e->folder == QStringLiteral("分组甲"),
                   "分组应与 remember 传入的一致", e->folder) && ok;
        ok = check(e->timestamp > 0, "时间戳应大于 0",
                   QStringLiteral("实际 timestamp=%1").arg(e->timestamp)) && ok;
    }
    ok = check(store.contains(QStringLiteral("2001")),
               "remember 后 contains() 应为 true") && ok;
    ok = check(!store.contains(QString()), "contains(空 aid) 应为 false") && ok;
    return ok;
}

static bool caseFavoriteDuplicate() {
    bool ok = true;
    FavoriteStore& store = FavoriteStore::instance();
    store.clear();
    store.remember(QStringLiteral("dup"), QStringLiteral("旧标题"), QStringLiteral("旧分组"));
    const QVector<FavoriteEntry> before = store.list();
    const qint64 firstTs = before.isEmpty() ? 0 : before.first().timestamp;

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    // 空的 title/folder 表示「调用方没带这项信息」，应保留已有值（见 FavoriteStore.h）。
    store.remember(QStringLiteral("dup"), QStringLiteral("新标题"), QString());

    const QVector<FavoriteEntry> entries = store.list();
    ok = check(entries.size() == 1, "同一 aid 重复 remember 不应产生两条",
               sizeText(static_cast<int>(entries.size()))) && ok;
    if (!entries.isEmpty()) {
        ok = check(entries.first().aid == QStringLiteral("dup"),
                   "去重后 aid 仍为 dup", entries.first().aid) && ok;
        ok = check(entries.first().title == QStringLiteral("新标题"),
                   "重复 remember 应更新标题", entries.first().title) && ok;
        ok = check(entries.first().folder == QStringLiteral("旧分组"),
                   "重复 remember 传空分组时应保留原分组", entries.first().folder) && ok;
        ok = check(entries.first().timestamp >= firstTs,
                   "重复 remember 后时间戳不应倒退",
                   QStringLiteral("旧=%1 新=%2")
                       .arg(firstTs)
                       .arg(entries.first().timestamp)) && ok;
    }
    return ok;
}

static bool caseFavoriteRemove() {
    bool ok = true;
    FavoriteStore& store = FavoriteStore::instance();
    store.clear();
    store.remember(QStringLiteral("2001"), QStringLiteral("甲"), QStringLiteral("g1"));
    store.remember(QStringLiteral("2002"), QStringLiteral("乙"), QStringLiteral("g2"));

    ok = check(store.contains(QStringLiteral("2001")), "remove 前 contains(2001) 应为 true") && ok;
    ok = check(store.remove(QStringLiteral("2001")), "remove 已存在的 aid 应返回 true") && ok;
    ok = check(!store.contains(QStringLiteral("2001")),
               "remove 后 contains(2001) 应为 false") && ok;
    ok = check(store.contains(QStringLiteral("2002")),
               "remove 不应误删其它条目") && ok;
    ok = check(!store.remove(QStringLiteral("2001")),
               "remove 不存在的 aid 应返回 false") && ok;
    ok = check(!store.remove(QString()), "remove 空 aid 应返回 false") && ok;

    const QVector<FavoriteEntry> entries = store.list();
    ok = check(entries.size() == 1, "remove 后应只剩一条",
               sizeText(static_cast<int>(entries.size()))) && ok;
    return ok;
}

static bool caseFavoriteOverflow() {
    bool ok = true;
    FavoriteStore& store = FavoriteStore::instance();
    store.clear();

    constexpr int kExtra = 5;
    const int total = FavoriteStore::kMax + kExtra;   // 1005
    std::printf("  （正在插入 %d 条收藏，每次都会整表落盘，稍慢）\n", total);
    std::fflush(stdout);
    for (int i = 1; i <= total; ++i) {
        store.remember(QString::number(i), QStringLiteral("收藏%1").arg(i),
                       QStringLiteral("分组"));
    }

    const QVector<FavoriteEntry> entries = store.list();
    ok = check(entries.size() <= FavoriteStore::kMax, "超过上限后条数不得超过 kMax",
               sizeText(static_cast<int>(entries.size()))) && ok;
    ok = check(entries.size() == FavoriteStore::kMax, "超过上限后应恰好保留 kMax 条",
               sizeText(static_cast<int>(entries.size()))) && ok;
    if (!entries.isEmpty()) {
        ok = check(entries.first().aid == QString::number(total),
                   "首条应是最新插入的一条", entries.first().aid) && ok;
        ok = check(entries.last().aid == QString::number(kExtra + 1),
                   "尾部应是最旧的保留条目（最早的 5 条被淘汰）",
                   entries.last().aid) && ok;
        ok = check(findEntry(entries, QString::number(1)) == nullptr,
                   "最早插入的条目应已被淘汰") && ok;
    }
    return ok;
}

static bool caseFavoriteClear() {
    bool ok = true;
    FavoriteStore& store = FavoriteStore::instance();
    store.clear();
    store.remember(QStringLiteral("3001"), QStringLiteral("待清空"), QStringLiteral("分组"));
    ok = check(store.contains(QStringLiteral("3001")), "clear() 前 contains() 应为 true") && ok;

    store.clear();
    const QVector<FavoriteEntry> entries = store.list();
    ok = check(entries.isEmpty(), "clear() 后 list() 应为空",
               sizeText(static_cast<int>(entries.size()))) && ok;
    ok = check(!store.contains(QStringLiteral("3001")),
               "clear() 后 contains() 应为 false") && ok;
    return ok;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

static void runCase(const char* name, bool (*fn)()) {
    caseDone(name, fn());
}

int main(int argc, char** argv) {
    g_argv0 = argv[0];

    // 不构造 QCoreApplication（两个 store 不需要事件循环），但 org/app 名会影响
    // AppDataLocation，先定成固定值，保证父进程与子进程算出的路径一致。
    QCoreApplication::setOrganizationName(QStringLiteral("JmNext"));
    QCoreApplication::setApplicationName(QStringLiteral("JMNeXt4QtDesktop-store-tests"));

    // 必须在任何 writableLocation()/store 实例之前打开：把可写目录重定向到
    // ~/.qttest/...，避免污染真实用户的 history.json / favorites.json。
    QStandardPaths::setTestModeEnabled(true);

    // 算不出目录时宁可不跑：否则 history.json 会落到根目录或当前目录。
    if (dataDir().isEmpty()) {
        std::printf("无法确定 AppDataLocation（HOME 未设置？），为安全起见中止。\n");
        return 1;
    }

    // 子进程模式：只跑一个坏 JSON 场景，跑完直接退出。
    if (argc > 1) {
        if (std::strcmp(argv[1], kScenarioHistoryCorrupt) == 0) {
            return scenarioHistoryCorrupt() ? 0 : 1;
        }
        if (std::strcmp(argv[1], kScenarioFavoriteCorrupt) == 0) {
            return scenarioFavoriteCorrupt() ? 0 : 1;
        }
        std::printf("未知子进程场景：%s\n", argv[1]);
        return 2;
    }

    std::printf("本地存储单元测试：HistoryStore / FavoriteStore\n");
    std::printf("测试数据目录：%s\n", dataDir().toUtf8().constData());
    std::printf("QStandardPaths 测试模式：已开启（不接触真实用户目录）\n\n");

    // 坏 JSON 用例必须排在所有进程内用例之前：保证对应单例还没有 load 过。
    runChildCase(kScenarioHistoryCorrupt, kCaseHistoryCorrupt);
    runChildCase(kScenarioFavoriteCorrupt, kCaseFavoriteCorrupt);

    runCase("用例「历史-空表」", caseHistoryEmpty);
    runCase("用例「历史-remember 后可见」", caseHistoryRemember);
    runCase("用例「历史-同 aid 去重且时间更新」", caseHistoryDuplicate);
    runCase("用例「历史-超过上限淘汰最旧」", caseHistoryOverflow);
    runCase("用例「历史-clear 清空」", caseHistoryClear);

    runCase("用例「收藏-空表」", caseFavoriteEmpty);
    runCase("用例「收藏-remember 后可见（含分组）」", caseFavoriteRemember);
    runCase("用例「收藏-同 aid 去重且保留分组」", caseFavoriteDuplicate);
    runCase("用例「收藏-remove 删除」", caseFavoriteRemove);
    runCase("用例「收藏-超过上限淘汰最旧」", caseFavoriteOverflow);
    runCase("用例「收藏-clear 清空」", caseFavoriteClear);

    // 收尾：清掉测试目录里的 json，下次运行仍从未加载状态开始。
    removeFile(historyPath());
    removeFile(favoritePath());

    std::fflush(stdout);
    std::printf("\n共 %d 个用例：通过 %d，失败 %d\n", g_casesRun,
                g_casesRun - g_casesFailed, g_casesFailed);

    // 未覆盖：正常数据「重启后从磁盘读回」、JSON 结构合法但字段缺失/类型错误、
    // 文件里同一 aid 重复出现时加载端的去重、加载时的时间排序与超限裁剪、并发写入。
    // 这些要么需要直接调用私有 load()，要么需要再写一个生产进程，超出本次只用公开
    // 接口的范围；坏 JSON 已经覆盖了「加载失败不崩溃」这一条最重要的容错路径。
    return g_casesFailed == 0 ? 0 : 1;
}
