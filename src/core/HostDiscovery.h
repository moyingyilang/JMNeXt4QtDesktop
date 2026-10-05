// 主机发现 —— 语义逐条对照主项目 shared/.../data/remote/JmHostDiscovery.kt。
//
// 关键语义：
//  1. 两个入口（BytePlus 对象存储上的 .txt），响应体是**密文**，用固定种子解密（decryptHostPayload）；
//  2. 明文形如 {"Setting":[...],"Server":[...],"jm3_Server":[[主机,线路名],...]}；
//     **`Server` 是纯字符串数组**，`jm3_Server` 才是 [主机,线路名] 对 ——
//     主项目注释明确写了：源码里对 `Server` 做 [host,label] 解构是**失效代码**，移植时别照抄那个错；
//  3. 从 `Server` 里挑一个（默认随机，把流量摊到多个域名；测试可注入确定性实现）；
//  4. **隐私规则**：主机发现不带任何凭证 —— 向与业务无关的第三方要一份公开域名列表，
//     没理由把自己的 Token / Authorization 送出去。调用方传入的 HttpClient 应当是不带拦截器的那个。
#pragma once
#include "core/JmSession.h"
#include "net/HttpClient.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace jmnext::core {

/// 主机清单入口（主项目实测两个都可用且返回同一份清单）
inline constexpr const char* HOST_ENDPOINTS[] = {
    "https://rup4a04-c01.tos-ap-southeast-1.bytepluses.com/newsvr-2025.txt",
    "https://rup4a04-c02.tos-cn-hongkong.bytepluses.com/newsvr-2025.txt",
};

struct HostPayload {
    std::vector<std::string> servers;                              // "Server"：纯字符串数组
    std::vector<std::string> setting;                              // "Setting"
    std::vector<std::pair<std::string, std::string>> lines;         // "jm3_Server"：[主机, 线路名]
};

/// 只做"够用且受测试覆盖"的 JSON 提取：读出上面三个字段。解析不出来返回 nullopt。
std::optional<HostPayload> parseHostPayload(const std::string& json);

/// 挑一个候选主机；返回 nullopt 表示放弃这个入口
using HostPicker = std::function<std::optional<std::string>(const std::vector<std::string>&)>;

/// 默认挑法：随机（对应主项目的 it.randomOrNull()，目的是摊流量）
std::optional<std::string> pickRandom(const std::vector<std::string>& hosts);

/// 依次尝试各入口；成功时写入 session 并返回 base（含尾斜杠）；全部失败返回 nullopt。
std::optional<std::string> discoverHost(JmSession& session, jmnext::net::HttpClient& http,
                                       const HostPicker& pick = pickRandom);

}  // namespace jmnext::core
