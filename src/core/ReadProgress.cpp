#include "core/ReadProgress.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace jmnext::core {
namespace {
bool allDigits(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) if (c < '0' || c > '9') return false;
    return true;
}
}  // namespace

std::string serializeReadProgress(const ReadProgress& progress) {
    return progress.aid + "\n" + progress.chapterId + "\n" + std::to_string(progress.page) + "\n";
}

bool saveReadProgress(const std::string& path, const ReadProgress& progress) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << serializeReadProgress(progress);
    return static_cast<bool>(out);
}

std::optional<ReadProgress> loadReadProgress(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;

    std::string lines[3];
    for (int i = 0; i < 3; ++i) {
        if (!std::getline(in, lines[i])) return std::nullopt;   // 少一行 → 没有进度
    }
    // 容忍 CRLF
    for (auto& l : lines) while (!l.empty() && (l.back() == '\r')) l.pop_back();
    if (lines[0].empty() || lines[1].empty()) return std::nullopt;
    if (!allDigits(lines[2])) return std::nullopt;
    if (!allDigits(lines[0]) && lines[0].size() > 64) return std::nullopt;   // aid 异常长 → 视为坏文件

    ReadProgress p;
    p.aid = lines[0];
    p.chapterId = lines[1];
    p.page = std::atoi(lines[2].c_str());
    if (p.page < 0) return std::nullopt;
    return p;
}

}  // namespace jmnext::core
