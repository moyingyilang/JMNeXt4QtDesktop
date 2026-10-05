// 封面地址组装的测试（规则来自主项目，且已用真实网络确认过地址形态）。
#include "core/JmCore.h"

#include <cstdio>
#include <string>

using namespace jmnext::core;

static int failures = 0;
static void eqStr(const std::string& got, const std::string& want, const char* what) {
    if (got != want) {
        std::printf("FAIL: %s\n  实际 %s\n  期望 %s\n", what, got.c_str(), want.c_str());
        ++failures;
    }
}
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}

int main() {
    const std::string base = "https://www.cdnhjk.net/";

    // 服务端给了完整 URL：原样用
    eqStr(*coverUrl("1", "https://cdn.example.com/a.jpg", "123", base), "https://cdn.example.com/a.jpg",
          "image 是完整 URL 时原样使用");
    // 服务端给了相对路径：补图床主机（并容忍多余斜杠）
    eqStr(*coverUrl("1", "/media/library/album/1/thumb/a.jpg", "123", base),
          "https://www.cdnhjk.net/media/library/album/1/thumb/a.jpg", "相对路径补图床主机");
    // image 为空：按模板拼，并带上 update_at 作为版本号
    eqStr(*coverUrl("209827", "", "1791191078", base),
          "https://www.cdnhjk.net/media/albums/209827_3x4.jpg?v=1791191078",
          "模板拼封面 + update_at 版本号（真实形态）");
    eqStr(*coverUrl("209827", "", "", base), "https://www.cdnhjk.net/media/albums/209827_3x4.jpg",
          "没有 update_at 时不带 ?v=");
    // 图床缺失 → 明确失败
    check(!coverUrl("209827", "", "1", "").has_value(), "图床主机缺失 → 返回空");
    check(!coverUrl("", "", "1", base).has_value(), "id 为空且 image 为空 → 返回空");

    if (failures == 0) std::printf("全部通过：封面地址组装（完整 URL/相对路径/模板/版本号）\n");
    return failures == 0 ? 0 : 1;
}
