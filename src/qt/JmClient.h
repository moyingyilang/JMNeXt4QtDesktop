// 界面要用的数据服务：把"会话 + Qt 网络 + 解析 + 解码 + 还原"包成几个直接可用的方法。
//
// 存在的意义：把协议细节关在这一层里，界面只看到 ListEntry / AlbumInfo / ChapterImages / QImage。
// 全部方法都是同步的（桌面端在后台线程调用；容器里用 --live 自检时直接在主线程调用即可）。
#pragma once
#include "core/JmParse.h"
#include "core/JmSession.h"
#include "qt/ImageCache.h"
#include "qt/QtHttpClient.h"

#include <QImage>

#include <optional>
#include <string>
#include <vector>

namespace jmnext::qt {

class JmClient {
public:
    /// 主机发现。返回是否成功；失败原因见 lastError()
    bool bootstrap();
    /// 主机缓存（省掉每次启动的"两个入口请求 + 解密"）
    QString cachedHostPath() const;
    QString cachedHost() const;
    void saveCachedHost(const QString& host) const;
    std::optional<std::vector<jmnext::core::ListEntry>> latest(int page = 0);
    /// 热门标签（分类浏览页的数据源；纯字符串数组）
    std::optional<std::vector<std::string>> hotTags();
    /// 搜索（参数名 search_query 是探出来的；条目字段与首页列表相同）
    std::optional<std::vector<jmnext::core::ListEntry>> search(const std::string& word, int page);

    /// 通用分页列表：把任意 (path, query) 当作"作品列表"请求并解析。
    /// 存在理由：多个接口（收藏/画师作品/标签/周更/随机）的响应形态与搜索相同，
    /// 用一个入口覆盖，避免每个接口都写一条四层切片。
    std::optional<std::vector<jmnext::core::ListEntry>> paged(const std::string& path, const std::string& query);

    /// 登录：POST login（表单 username/password），成功后把 JWT 记进会话（供后续请求带头）。
    /// 依据 shared/data/JmRepository.kt 的 login()：参数 username/password，返回体里取 jwt_token。
    bool login(const std::string& username, const std::string& password);
    void logout();

    /// 通用动作（POST 表单）：收藏/点赞/追更/签到/评论发送等共用。
    /// 依据 shared/data/JmRepository.kt：这些接口都是 remote.post(路径, mapOf(...))。
    bool action(const std::string& path, const std::string& form, std::string* message = nullptr);

    /// 按分类筛选作品（分类浏览页点标签后用）。
    ///  c 分类标识；**为空时整个 c 参数必须省略** —— 实测发 `c=` 会让服务端返回
    ///          `Could not connect to mysql!` 错误页（不是 JSON）；省略才是合法的"不筛选"语义。
    ///          依据：shared/data/JmRepository.kt 对 categoryFilter 的注释（实测结论）。
    std::optional<std::vector<jmnext::core::ListEntry>> categoryFilter(const std::string& c, int page);
    std::optional<jmnext::core::AlbumInfo> album(const std::string& id);
    std::optional<jmnext::core::ChapterImages> chapter(const std::string& id);
    ImageCache& cache() { return cache_; }
    const ImageCache& cache() const { return cache_; }

    /// 下载一页 → Qt 解码 → 需要则切片还原 → 返回 QImage（带内存与磁盘缓存）

    /// 取列表项的封面缩略图（3x4 模板，带缓存）—— 真实网络已验证该地址返回 JPEG 400x533
    std::optional<QImage> cover(const jmnext::core::ListEntry& entry);
    /// 该条目的封面 URL（与 cover() 内部使用的完全一致；QML 侧图片提供器要用它做缓存键）
    std::string coverUrlFor(const jmnext::core::ListEntry& entry) const;
    std::optional<QImage> pageImage(const std::string& url, int aid, int scrambleId);

    const std::string& lastError() const { return lastError_; }
    /// 最近一次成功响应的原文（仅用于排查未知响应形态；不做持久化）
    const std::string& lastRaw() const { return lastRaw_; }
    const std::string& host() const { return host_; }
    jmnext::core::JmSession& session() { return session_; }

private:
    jmnext::core::JmSession session_{"2.1.9", 0};
    QtHttpClient http_;
    ImageCache cache_;
    std::string host_;
    std::string lastError_;
    bool bootstrapped_ = false;
    std::string lastRaw_;
};

}  // namespace jmnext::qt
