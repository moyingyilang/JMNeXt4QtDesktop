// 接口路径表 —— 从主项目 JMNeXt（我们自己的代码）移植。
//
// 规格来源：shared/src/main/kotlin/com/jmnext/data/remote/JmPaths.kt
// 生成方式：由该文件的 const val 逐项抽出（见本文件末尾的对照说明），保证与原表一字不差；
// 主项目增删路径时，这里要同步 —— 测试里有几条抽样断言盯着这件事。
#pragma once

namespace jmnext::core::paths {

inline constexpr char PROMOTE[] = "promote";
inline constexpr char LATEST[] = "latest";
inline constexpr char PROMOTE_LIST[] = "promote_list";
inline constexpr char SERIALIZATION[] = "serialization";
inline constexpr char NOTIFICATIONS[] = "notifications";
inline constexpr char NOTIFICATIONS_UNREAD[] = "notifications/unreadCount";
inline constexpr char DAILY[] = "daily";
inline constexpr char DAILY_CHECK[] = "daily_chk";
inline constexpr char DAILY_LIST[] = "daily_list";
inline constexpr char DAILY_LIST_FILTER[] = "daily_list/filter";
inline constexpr char SEARCH[] = "search";
inline constexpr char HOT_TAGS[] = "hot_tags";
inline constexpr char ALBUM[] = "album";
inline constexpr char COMIC_READ[] = "comic_read";
inline constexpr char CATEGORIES[] = "categories";
inline constexpr char CATEGORIES_FILTER[] = "categories/filter";
inline constexpr char SETTING[] = "setting";
inline constexpr char LOGIN[] = "login";
inline constexpr char REGISTER[] = "register";
inline constexpr char FORGOT[] = "forgot";
inline constexpr char LOGOUT[] = "logout";
inline constexpr char FAVORITE[] = "favorite";
inline constexpr char FAVORITE_FOLDER[] = "favorite_folder";
inline constexpr char LIKE[] = "like";
inline constexpr char WATCH_LIST[] = "watch_list";
inline constexpr char FORUM[] = "forum";
inline constexpr char WEEK[] = "week";
inline constexpr char WEEK_FILTER[] = "week/filter";
inline constexpr char RANDOM_RECOMMEND_LIST[] = "random_recommend";
inline constexpr char CREATOR_AUTHOR[] = "creator_author";
inline constexpr char CREATOR_WORK[] = "creator_work";
inline constexpr char CREATOR_WORK_DETAIL[] = "creator_work_detail";
inline constexpr char CREATOR_WORK_INFO[] = "creator_work_info";
inline constexpr char CREATOR_WORK_INFO_DETAIL[] = "creator_work_info_detail";
inline constexpr char ARTIST_ICON_TEMPLATE[] = "media/library/artists/%s/icon/%s";
inline constexpr char ARTIST_BANNER_TEMPLATE[] = "media/library/artists/%s/banner/%s";
inline constexpr char SERTRACKING[] = "album_sertracking";
inline constexpr char TRACKING_LIST[] = "album_tracking";
inline constexpr char TAGS_FAVORITE[] = "tags_favorite";
inline constexpr char TAGS_FAVORITE_UPDATE[] = "tags_favorite_update";
inline constexpr char ALBUM_DOWNLOAD[] = "album_download_2";
inline constexpr char COMMENT_SEND[] = "comment";
inline constexpr char COMMENT_DELETE[] = "comment_delete";
inline constexpr char COVER_TEMPLATE[] = "media/albums/%s_3x4.jpg";
inline constexpr char AVATAR_TEMPLATE[] = "media/users/%s";

}  // namespace jmnext::core::paths

// 对照说明：本表由 JmPaths.kt 的 const val 机械抽出，没有人工改名；
// 若主项目改动了路径，请重新生成并跑 tests/test_paths.cpp。
