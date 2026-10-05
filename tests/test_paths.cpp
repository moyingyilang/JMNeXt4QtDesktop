// 路径表的抽样断言：盯住"移植后路径没被改错/漏掉"。
// 判据取自主项目 JmPaths.kt 的实际内容（抽样覆盖读类、写类、登录类各一条）。
#include "core/JmPaths.h"

#include <cstdio>
#include <cstring>

using namespace jmnext::core::paths;

static int failures = 0;
static void eq(const char* got, const char* want, const char* what) {
    if (std::strcmp(got, want) != 0) {
        std::printf("FAIL: %s —— 实际 '%s' 期望 '%s'\n", what, got, want);
        ++failures;
    }
}

int main() {
    eq(LATEST, "latest", "首页最新");
    eq(ALBUM, "album", "作品详情");
    eq(COMIC_READ, "comic_read", "阅读（取章节图片）");
    eq(SEARCH, "search", "搜索");
    eq(CATEGORIES_FILTER, "categories/filter", "分类筛选");
    eq(LOGIN, "login", "登录");
    eq(LOGOUT, "logout", "登出");
    eq(DAILY_CHECK, "daily_chk", "每日签到");
    eq(NOTIFICATIONS_UNREAD, "notifications/unreadCount", "未读通知数");

    if (failures == 0) std::printf("全部通过：路径表抽样\n");
    return failures == 0 ? 0 : 1;
}
