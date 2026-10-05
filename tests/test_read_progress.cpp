// 阅读进度存取测试：写→读一致、CRLF 容忍、缺行/坏数字/文件不存在都明确返回"没有进度"。
#include "core/JmCore.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

using namespace jmnext::core;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}

int main() {
    const std::string path = std::string(std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp") +
                             "/jmnext-progress-test.txt";
    std::remove(path.c_str());

    // 文件不存在 → 没有进度
    check(!loadReadProgress(path).has_value(), "文件不存在 → nullopt");

    // 写 → 读一致
    const ReadProgress p{"209827", "209828", 11};
    check(saveReadProgress(path, p), "写入成功");
    auto back = loadReadProgress(path);
    check(back.has_value(), "读回成功");
    if (back) {
        check(back->aid == p.aid, "aid 一致");
        check(back->chapterId == p.chapterId, "章节 id 一致");
        check(back->page == p.page, "页码一致");
    }

    // 覆盖写
    check(saveReadProgress(path, ReadProgress{"1", "2", 0}), "覆盖写成功");
    if (auto q = loadReadProgress(path)) check(q->page == 0, "覆盖后页码为 0");

    // CRLF 容忍
    { std::ofstream out(path, std::ios::trunc); out << "5\r\n6\r\n7\r\n"; }
    if (auto q = loadReadProgress(path)) {
        check(q->chapterId == "6", "CRLF 被容忍（章节 id）");
        check(q->page == 7, "CRLF 被容忍（页码）");
    } else {
        check(false, "CRLF 文件应当能读");
    }

    // 缺一行 → nullopt
    { std::ofstream out(path, std::ios::trunc); out << "1\n2\n"; }
    check(!loadReadProgress(path).has_value(), "只有两行 → nullopt");

    // 页码不是数字 → nullopt
    { std::ofstream out(path, std::ios::trunc); out << "1\n2\nabc\n"; }
    check(!loadReadProgress(path).has_value(), "页码非数字 → nullopt");

    // 空 aid → nullopt
    { std::ofstream out(path, std::ios::trunc); out << "\n2\n3\n"; }
    check(!loadReadProgress(path).has_value(), "aid 为空 → nullopt");

    std::remove(path.c_str());
    if (failures == 0) std::printf("全部通过：阅读进度存取（写读一致、CRLF、坏文件一律不猜）\n");
    return failures == 0 ? 0 : 1;
}
