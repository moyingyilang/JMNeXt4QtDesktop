#include "core/JmUrls.h"

namespace jmnext::core {
namespace {
bool startsWithHttp(const std::string& s) {
    return s.rfind("http://", 0) == 0 || s.rfind("https://", 0) == 0;
}
std::string trimEndSlash(std::string s) {
    while (!s.empty() && s.back() == '/') s.pop_back();
    return s;
}
std::string trimStartSlash(const std::string& s) {
    std::size_t i = 0;
    while (i < s.size() && s[i] == '/') ++i;
    return s.substr(i);
}
}  // namespace

std::optional<std::string> coverUrl(const std::string& id, const std::string& rawImage,
                                    const std::string& updateAt, const std::string& imageBase) {
    if (!rawImage.empty()) {
        if (startsWithHttp(rawImage)) return rawImage;
        if (imageBase.empty()) return std::nullopt;
        return trimEndSlash(imageBase) + "/" + trimStartSlash(rawImage);
    }
    if (imageBase.empty() || id.empty()) return std::nullopt;
    // 与主项目 JmPaths.COVER_TEMPLATE 一致：media/albums/<id>_3x4.jpg
    std::string url = trimEndSlash(imageBase) + "/media/albums/" + id + "_3x4.jpg";
    if (!updateAt.empty()) url += "?v=" + updateAt;
    return url;
}

}  // namespace jmnext::core
