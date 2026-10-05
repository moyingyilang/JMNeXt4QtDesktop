// jmnext4net —— 真实网络自检小工具（给真机用；容器里能跑通就算额外收获）。
//
//   jmnext4net get <url>              直接取一个 URL，打印状态码与正文开头
//   jmnext4net discover               跑真实主机发现，打印选中的主机
//   jmnext4net latest [--save 文件]   在发现到的主机上请求 latest?page=0，打印**解密后**的正文
//
// 为什么要它：真实响应格式在容器里取不到（无账号、无法确认可达性）。
// 这个工具让你在真机上一条命令把真响应导出来 —— 我拿到之后就能把 extractImages()
// 里那处"待核对的字段名"换成真字段，其余链路已经全部有测试覆盖。
#include "core/JmCore.h"
#include "core/JmApi.h"
#include "core/JmSession.h"
#include "qt/QtHttpClient.h"

#include <QCoreApplication>
#include <QDateTime>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
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
                     "  jmnext4net latest [--save 文件]\n  jmnext4net update [本地版本] [仓库]\n");
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

    if (cmd == "update") {
        // 更新检查：拉 GitHub 最新 release，按 core::UpdateCheck 的规则比较
        // （fix(n) 规则也在 core 里，已有单测；这里只做"取 tag → 比较 → 给出该下哪个包"）
        const std::string repo = (argc >= 4) ? argv[3] : "moyingyilang/JMNeXt4QtDesktop";
        const std::string local = (argc >= 3) ? argv[2] : "0.1.0";
        const std::string url = "https://api.github.com/repos/" + repo + "/releases/latest";
        // 受限环境（本容器的 GitHub HTTPS 不通）可用 --from-file 喂罐装响应，
        // 这样"取 tag → 比较 → 拼附件名"这段逻辑仍能被验证；在线那一步如实标注未验证。
        jmnext::net::HttpResponse resp;
        if (argc >= 6 && std::string(argv[4]) == "--from-file") {
            std::ifstream in(argv[5], std::ios::binary);
            std::stringstream ss; ss << in.rdbuf();
            resp = jmnext::net::HttpResponse{200, ss.str()};
            std::printf("（用本地文件代替网络：%s）\n", argv[5]);
        } else {
            resp = http.get(url, {"Accept: application/vnd.github+json"});
        }
        if (!resp.ok()) {
            std::printf("失败：HTTP %d %s\n", resp.status, http.lastError().c_str());
            return 1;
        }
        // 只取 tag_name（够用且不引入 JSON 依赖）
        std::string tag;
        if (const auto at = resp.body.find("\"tag_name\""); at != std::string::npos) {
            if (const auto q1 = resp.body.find('"', resp.body.find(':', at)); q1 != std::string::npos) {
                if (const auto q2 = resp.body.find('"', q1 + 1); q2 != std::string::npos)
                    tag = resp.body.substr(q1 + 1, q2 - q1 - 1);
            }
        }
        if (tag.empty()) { std::printf("失败：响应里没有 tag_name\n"); return 1; }
        std::printf("远端最新：%s\n", tag.c_str());
        if (jmnext::core::isNewer(tag, local)) {
            std::printf("有新版本：%s（本地 %s）\n", tag.c_str(), local.c_str());
            std::printf("桌面端附件名（统一包排第一）：");
            for (const auto& n : jmnext::core::desktopAssetNames(tag, "Linux", "aarch64"))
                std::printf(" %s", n.c_str());
            // 示例附件名按**当前 tag 的版本**拼，不写死（此前写死了 0.1.0，且我第一次"修"它时 perl 没生效）
            const std::string example = "JMNeXt4QtDesktop-" +
                                        jmnext::core::cleanVersion(tag) + "-linux-arm64.deb";
            std::printf("\n下载直链示例：%s\n", jmnext::core::assetUrl(tag, example, repo).c_str());
            return 0;
        }
        std::printf("已是最新（本地 %s 不旧于远端 %s）\n", local.c_str(), tag.c_str());
        return 0;
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
