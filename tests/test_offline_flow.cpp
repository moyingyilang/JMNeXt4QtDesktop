// 端到端离线闭环（集成测试）：用一个假服务端把已测过的零件串成完整链路。
//
//   主机发现 → 首页列表 → 屏蔽过滤 → 详情 → 章节图片 → 取图片字节 → 切片还原 → 版本检查
//
// 全程不碰真实网络。**未验证**：真实服务端的响应字段名（这里用简化约定演示链路，
// 真实字段需回主项目读实 DTO 后再替换；替换点集中在 extractImages() 一处）。
#include "core/JmCore.h"
#include "net/HttpClient.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace jmnext::core;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static void eqStr(const std::string& got, const std::string& want, const char* what) {
    if (got != want) {
        std::printf("FAIL: %s\n  实际 %s\n  期望 %s\n", what, got.c_str(), want.c_str());
        ++failures;
    }
}

static std::string serverEncrypt(const std::string& plain, const std::string& keyHex) {
    uint8_t key[32];
    keyFromHexString(keyHex, key);
    std::vector<uint8_t> data(plain.begin(), plain.end());
    return base64Encode(aes256EcbEncrypt(data, key));
}

/// 假服务端：按 URL 关键字路由，并按"服务端自己的会话时间戳"加密业务响应
class FakeServer : public jmnext::net::HttpClient {
public:
    int64_t serverTime = 1700000000;
    std::vector<std::string> log;      // 记录收到的 URL，便于断言请求顺序

    jmnext::net::HttpResponse get(const std::string& url,
                                 const std::vector<std::string>& headers) override {
        log.push_back(url);
        if (url.find("bytepluses.com") != std::string::npos) {          // 主机清单（第三方对象存储）
            if (!headers.empty()) return {500, "主机发现不该带任何头"};
            const std::string hosts = R"({"Setting":[],"Server":["api.example.com"],"jm3_Server":[]})";
            return {200, serverEncrypt(hosts, md5Hex(HOST_SEED))};
        }
        if (url.find("/latest") != std::string::npos) {                  // 首页列表
            const std::string body =
                R"({"status":"ok","data":{"content":[)"
                R"({"id":"1","name":"NTR作品","author":"某作者"},)"
                R"({"id":"2","name":"纯爱作品","author":"某作者"}]}})";
            return {200, serverEncrypt(body, token())};
        }
        if (url.find("/album") != std::string::npos) {                   // 详情
            const std::string body =
                R"({"status":"ok","data":{"id":"2","name":"纯爱作品","tags":["纯爱","日常"],)"
                R"("images":["https://img.example.com/1.jpg","https://img.example.com/2.jpg"]}})";
            return {200, serverEncrypt(body, token())};
        }
        if (url.find("img.example.com") != std::string::npos) {          // 图片字节（不加密，走图床）
            return {200, imageBytes};
        }
        return {404, ""};
    }

    /// 假服务端用与客户端**同一个会话时间戳**派生密钥（真实服务端也是这么做的）
    std::string token() const { return md5Hex(std::to_string(serverTime) + std::string(TOKEN_SEED)); }

    std::string imageBytes;   // 图片原始字节（这里放 PPM 头 + 载荷，模拟"图床不加密"）
};

/// 从详情正文里取图片地址。**这是真实字段名待核对的唯一一处**（容器内无账号无法取真响应）。
static std::vector<std::string> extractImages(const std::string& json) {
    std::vector<std::string> out;
    const std::string key = "\"images\"";
    auto at = json.find(key);
    if (at == std::string::npos) return out;
    auto lb = json.find('[', at);
    auto rb = json.find(']', lb);
    if (lb == std::string::npos || rb == std::string::npos) return out;
    const std::string arr = json.substr(lb + 1, rb - lb - 1);
    std::size_t pos = 0;
    while (true) {
        auto q1 = arr.find('"', pos);
        if (q1 == std::string::npos) break;
        auto q2 = arr.find('"', q1 + 1);
        if (q2 == std::string::npos) break;
        out.push_back(arr.substr(q1 + 1, q2 - q1 - 1));
        pos = q2 + 1;
    }
    return out;
}

int main() {
    // 造一张 8x12、每行同色的 "图片"（当作已解码的 ARGB 像素；Qt 解码路径另有测试覆盖）
    const int W = 8, H = 12;
    std::vector<uint32_t> sourcePixels(static_cast<std::size_t>(W) * H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) sourcePixels[static_cast<std::size_t>(y) * W + x] = static_cast<uint32_t>(y);

    FakeServer server;
    JmSession session("2.1.9", 1700000000);
    server.serverTime = session.time();

    // ---------- 1) 主机发现（不带任何凭证）----------
    {
        auto base = discoverHost(session, server, [](const std::vector<std::string>& h) {
            return h.empty() ? std::nullopt : std::optional<std::string>(h[0]);
        });
        check(base.has_value(), "主机发现成功");
        eqStr(*base, "https://api.example.com/", "选中的主机");
        check(server.log.size() == 1 && server.log[0].find("bytepluses.com") != std::string::npos,
              "第一个请求是主机清单（第三方存储）");
    }

    JmApi api(session, server);

    // ---------- 2) 首页列表 ----------
    auto latest = api.request("latest", "page=0");
    check(latest.has_value(), "拿到首页列表");
    if (latest) {
        check(latest->text.find("纯爱作品") != std::string::npos, "正文里有列表内容");
    }

    // ---------- 3) 屏蔽过滤（关键词命中的条目被挡掉）----------
    BlockRules rules;
    rules.words = {"NTR"};
    const auto items = std::vector<ListItem>{
        {"NTR作品", "某作者", "", ""},
        {"纯爱作品", "某作者", "", ""},
    };
    std::vector<ListItem> visible;
    for (const auto& it : items)
        if (!rules.hides(it)) visible.push_back(it);
    check(visible.size() == 1, "屏蔽后只剩一条可见");
    if (!visible.empty()) eqStr(visible[0].name, "纯爱作品", "留下的是没命中关键词的那条");

    // ---------- 4) 详情 ----------
    auto detail = api.request("album", "id=" + visible[0].name);   // 参数名待与真实接口核对
    check(detail.has_value(), "拿到详情");
    std::vector<std::string> images;
    if (detail) {
        images = extractImages(detail->text);
        check(images.size() == 2, "详情里解析出两个图片地址");
        check(!rules.matchesTags({"纯爱", "日常"}), "该作品标签未命中屏蔽名单");
    }

    // ---------- 5) 取图片字节（不加密，走图床）----------
    if (!images.empty()) {
        const auto img = server.get(images[0], {});
        check(img.ok(), "取到图片字节");
        eqStr(server.log.back(), images[0], "取图请求打的是图床地址");
    }

    // ---------- 6) 切片还原 ----------
    {
        const int aid = 1;
        const std::string page = "1";
        const int num = sliceCount(aid, page);
        const auto bandList = jmnext::core::bands(W, H, num);   // 变量名不能也叫 bands：会遮蔽同名函数
        const auto restored = applyBands(sourcePixels, W, H, bandList);
        check(restored.size() == sourcePixels.size(), "还原后尺寸不变（不重不漏）");
        // 独立复算：目标第 y 行应来自哪一行
        int mismatches = 0;
        for (int y = 0; y < H; ++y) {
            int want = -1;
            for (const auto& b : bandList)
                if (y >= b.dstY && y < b.dstY + b.height) want = b.srcY + (y - b.dstY);
            if (want >= 0 && restored[static_cast<std::size_t>(y) * W] != static_cast<uint32_t>(want)) ++mismatches;
        }
        check(mismatches == 0, "逐行独立复算：还原结果正确");
    }

    // ---------- 7) 版本检查（含 fix(n) 规则）----------
    {
        check(isNewer("v2.1.7.fix1", "2.1.7"), "修复版被判定为新版本");
        eqStr(androidAssetName("2.1.7.fix1.lite", true), "Android-lite-2.1.7.fix1.apk",
              "修复版 + lite 的附件名");
    }

    // ---------- 请求序列总览（便于人工核对）----------
    std::printf("请求序列（共 %zu 次）：\n", server.log.size());
    for (const auto& u : server.log) std::printf("  %s\n", u.c_str());

    if (failures == 0) std::printf("全部通过：离线端到端闭环（发现→列表→屏蔽→详情→取图→还原→版本检查）\n");
    return failures == 0 ? 0 : 1;
}
