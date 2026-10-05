#include "qt/ImageCodec.h"

#include <QImage>
#include <QImageReader>

namespace jmnext::qt {

std::optional<Image> loadImage(const std::string& path) {
    QImageReader reader(QString::fromStdString(path));
    reader.setAutoTransform(true);
    const std::string fmt = reader.format().toStdString();   // 必须在 read() 之前取
    QImage img = reader.read();
    if (img.isNull()) return std::nullopt;
    img = img.convertToFormat(QImage::Format_ARGB32);

    Image out;
    out.width = img.width();
    out.height = img.height();
    out.detectedFormat = fmt;
    out.pixels.resize(static_cast<std::size_t>(out.width) * static_cast<std::size_t>(out.height));
    for (int y = 0; y < out.height; ++y) {
        const auto* line = reinterpret_cast<const uint32_t*>(img.constScanLine(y));
        for (int x = 0; x < out.width; ++x)
            out.pixels[static_cast<std::size_t>(y) * out.width + x] = line[x];
    }
    return out;
}

bool savePpm(const std::string& path, const Image& image) {
    if (image.width <= 0 || image.height <= 0) return false;
    QImage img(image.width, image.height, QImage::Format_ARGB32);
    for (int y = 0; y < image.height; ++y) {
        auto* line = reinterpret_cast<uint32_t*>(img.scanLine(y));
        for (int x = 0; x < image.width; ++x)
            line[x] = image.pixels[static_cast<std::size_t>(y) * image.width + x];
    }
    return img.save(QString::fromStdString(path), "PPM");
}

}  // namespace jmnext::qt

namespace jmnext::qt {
std::optional<Image> loadImageFile(const std::string& path) { return loadImage(path); }
}  // namespace jmnext::qt
