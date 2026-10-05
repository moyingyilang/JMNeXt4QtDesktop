// jmnext4net —— 真实网络自检小工具（给真机用；容器里能跑通就算额外收获）。
//
//   jmnext4net get <url>              直接取一个 URL，打印状态码与正文开头
//   jmnext4net discover               跑真实主机发现，打印选中的主机
//   jmnext4net latest [--save 文件]   在发现到的主机上请求 latest?page=0，打印**解密后**的正文
//
// 为什么要它：真实响应格式在容器里取不到（无账号、无法确认可达性）。
// 这个工具让你在真机上一条命令把真响应导出来 —— 我拿到之后就能把 extractImages()
// 里那处"待核对的字段名"换成真字段，其余链路已经全部有测试覆盖。
#include "core/HostDiscovery.h"
#include "core/JmApi.h"
#include "core/JmSession.h"
#include "qt/QtHttpClient.h"

#include <QCoreApplication>
#include <QDateTime>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

using namespace jmnext::core;

static void printHead(const std::string& s, std::size_t maxLen) {
    const std::size_t n = s.size() < maxLen ? s.size() : maxLen;
    std::printf("%.*s%s\n", static_cast<int>(n), s.data(), s.size() > n ? "\n…（已截断）" : "");
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) {
        std::fprintf(stderr,
                     "用法:\n  jmnext4net get <url>\n  jmnext4net discover\n"
                     "  jmnext4net latest [--save 文件]\n");
        return 2;
    }
    jmnext::qt::QtHttpClient http;
    const std::string cmd = argv[1];
    // 默认用**真实当前时间**：服务端会校验时间戳时效（此前硬编码 1700000000 导致解密失败）
    int64_t now = QDateTime::currentSecsSinceEpoch();
    for (int i = 2; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "--fixed-time") now = std::strtoll(argv[i + 1], nullptr, 10);
    std::printf("使用时间戳: %lld\n", static_cast<long long>(now));

    if (cmd == "get" && argc >= 3) {
        const auto resp = http.get(argv[2], {});
        std::printf("状态码: %d  正文 %zu 字节\n", resp.status, resp.body.size());
        if (!http.lastError().empty()) std::printf("错误: %s\n", http.lastError().c_str());
        if (argc == 5 && std::string(argv[3]) == "--save") {
            std::ofstream out(argv[4], std::ios::binary);
            out.write(resp.body.data(), static_cast<std::streamsize>(resp.body.size()));
            std::printf("已写入 %s\n", argv[4]);
        } else {
            printHead(resp.body, 800);
        }
        return resp.ok() ? 0 : 1;
    }

    if (cmd == "discover") {
        JmSession session("2.1.9", now);
        auto base = discoverHost(session, http, [](const std::vector<std::string>& hosts) {
            if (hosts.empty()) return std::optional<std::string>{};
            std::printf("候选主机 %zu 个，选第一个：%s\n", hosts.size(), hosts.front().c_str());
            return std::optional<std::string>(hosts.front());
        });
        if (!base) {
            std::printf("主机发现失败：%s\n", http.lastError().c_str());
            return 1;
        }
        std::printf("发现主机: %s\n", base->c_str());
        return 0;
    }

    if (cmd == "latest") {
        JmSession session("2.1.9", now);
        auto base = discoverHost(session, http, [](const std::vector<std::string>& hosts) {
            return hosts.empty() ? std::optional<std::string>{} : std::optional<std::string>(hosts.front());
        });
        if (!base) {
            std::printf("主机发现失败：%s\n", http.lastError().c_str());
            return 1;
        }
        std::printf("主机: %s\n", base->c_str());
        JmApi api(session, http);
        auto r = api.request("latest", "page=0");
        if (!r) {
            std::printf("请求失败：%s\n", api.lastError().c_str());
            // 诊断：把原始响应也打出来，看清服务端到底回了什么（而不是只看到"解密失败"）
            const auto raw = http.get(*base + "latest?page=0",
                                      {"Token: " + session.token(), "Tokenparam: " + session.tokenParam(),
                                       "Accept: application/json, text/plain, */*"});
            std::printf("原始响应: 状态码 %d，%zu 字节\n", raw.status, raw.body.size());
            printHead(raw.body, 600);
            return 1;
        }
        std::printf("HTTP %d，解密后 %zu 字节%s\n", r->status, r->text.size(),
                    r->retried ? "（解密失败后 refresh 重试过）" : "");
        if (argc == 4 && std::string(argv[2]) == "--save") {
            std::ofstream out(argv[3], std::ios::binary);
            out << r->text;
            std::printf("已写入 %s\n", argv[3]);
        } else {
            printHead(r->text, 2000);
        }
        return 0;
    }

    if (cmd == "api" && argc >= 3) {
        // 通用探测：jmnext4net api <path> [query] [--save 文件]
        JmSession session("2.1.9", now);
        auto base = discoverHost(session, http, [](const std::vector<std::string>& hosts) {
            return hosts.empty() ? std::optional<std::string>{} : std::optional<std::string>(hosts.front());
        });
        if (!base) { std::printf("主机发现失败\n"); return 1; }
        JmApi api(session, http);
        const std::string path = argv[2];
        const std::string query = (argc >= 4 && std::string(argv[3]) != "--save") ? argv[3] : "";
        auto r = api.request(path, query);
        if (!r) {
            std::printf("请求失败：%s\n", api.lastError().c_str());
            const auto raw = http.get(*base + path + (query.empty() ? "" : "?" + query),
                                      {"Token: " + session.token(), "Tokenparam: " + session.tokenParam(),
                                       "Accept: application/json, text/plain, */*"});
            std::printf("原始响应: 状态码 %d，%zu 字节\n", raw.status, raw.body.size());
            printHead(raw.body, 400);
            return 1;
        }
        std::printf("HTTP %d，解密后 %zu 字节\n", r->status, r->text.size());
        if (argc >= 5 && std::string(argv[3]) == "--save") {
            std::ofstream out(argv[4], std::ios::binary);
            out << r->text;
            std::printf("已写入 %s\n", argv[4]);
        } else if (argc >= 4 && std::string(argv[argc - 2]) == "--save") {
            std::ofstream out(argv[argc - 1], std::ios::binary);
            out << r->text;
            std::printf("已写入 %s\n", argv[argc - 1]);
        } else {
            printHead(r->text, 1200);
        }
        return 0;
    }

    std::fprintf(stderr, "参数不对\n");
    return 2;
}
