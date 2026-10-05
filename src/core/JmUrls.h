// URL 组装（封面等）—— 规则对照主项目 JmRepository.coverUrl。
//
// 真实依据（主项目注释）：封面在接口里虽然是字段，但**实际常有缺失**，
// 而客户端用的是 `${img_host}/media/albums/${id}_3x4.jpg?v=${update_at}` 这条约定；
// update_at 作为版本号参与 URL，让客户端缓存自然失效。
//
// 本函数已用真实网络验证：.../media/albums/209827_3x4.jpg 返回 JPEG 400x533。
#pragma once
#include <optional>
#include <string>

namespace jmnext::core {

/// 拼封面地址。rawImage 是服务端下发的 image 字段（可能为空、可能是相对路径、也可能是完整 URL）。
/// imageBase 是图床主机（含尾斜杠）；为空时返回 nullopt。
std::optional<std::string> coverUrl(const std::string& id, const std::string& rawImage,
                                    const std::string& updateAt, const std::string& imageBase);

}  // namespace jmnext::core
