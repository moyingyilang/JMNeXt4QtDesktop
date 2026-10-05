#include "qt/MainWindow.h"

#include "core/JmCrypto.h"
#include "core/UnscrambleApply.h"
#include "qt/ImageCodec.h"

#include <QApplication>
#include <QChar>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QStringList>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <string>
#include <vector>

namespace jmnext::qt {
namespace {

/// ARGB 缓冲 → QImage（显示用）
QImage toQImage(const std::vector<uint32_t>& px, int w, int h) {
    QImage img(w, h, QImage::Format_ARGB32);
    for (int y = 0; y < h; ++y) {
        auto* line = reinterpret_cast<uint32_t*>(img.scanLine(y));
        for (int x = 0; x < w; ++x) line[x] = px[static_cast<std::size_t>(y) * w + x];
    }
    return img;
}

}  // namespace

MainWindow::MainWindow() {
    setupUi();
    applyCjkFont();
}

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("JMNeXt4QtDesktop —— 能看图（早期骨架）"));
    auto* central = new QWidget(this);
    auto* outer = new QVBoxLayout(central);

    auto* views = new QHBoxLayout();
    originalView_ = new QLabel(QStringLiteral("（未加载图片）"));
    restoredView_ = new QLabel(QStringLiteral("（未还原）"));
    for (QLabel* v : {originalView_, restoredView_}) {
        v->setMinimumSize(240, 320);
        v->setAlignment(Qt::AlignCenter);
        v->setStyleSheet(QStringLiteral("border: 1px solid #888;"));
        views->addWidget(v);
    }
    outer->addLayout(views);

    auto* form = new QFormLayout();
    aidEdit_ = new QLineEdit(QStringLiteral("1"));
    pageEdit_ = new QLineEdit(QStringLiteral("1"));
    form->addRow(QStringLiteral("作品号 (aid)"), aidEdit_);
    form->addRow(QStringLiteral("页码 (page，接口原样字符串)"), pageEdit_);
    outer->addLayout(form);

    auto* buttons = new QHBoxLayout();
    auto* openBtn = new QPushButton(QStringLiteral("打开图片…"));
    auto* unscrambleBtn = new QPushButton(QStringLiteral("还原"));
    buttons->addWidget(openBtn);
    buttons->addWidget(unscrambleBtn);
    outer->addLayout(buttons);

    logView_ = new QPlainTextEdit();
    logView_->setReadOnly(true);
    logView_->setMaximumHeight(120);
    outer->addWidget(logView_);

    setCentralWidget(central);

    connect(openBtn, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择漫画图片"));
        if (!path.isEmpty()) loadImage(path);
    });
    connect(unscrambleBtn, &QPushButton::clicked, this, [this] { unscrambleNow(); });
}

void MainWindow::applyCjkFont() {
    // 主项目在 Windows 上踩过"默认字体不含汉字 → 方框/被回退成日文字形"的坑。
    // 这里显式挑一个含中日韩字形的族，并用 inFont() 验证它真的能画「汉」。
    const QStringList candidates = {
        QStringLiteral("Noto Sans CJK SC"), QStringLiteral("Noto Sans CJK JP"),
        QStringLiteral("Source Han Sans SC"), QStringLiteral("WenQuanYi Zen Hei"),
        QStringLiteral("Noto Sans SC"), QStringLiteral("Microsoft YaHei"),
        QStringLiteral("PingFang SC"), QStringLiteral("Hiragino Sans"),
    };
    const QStringList families = QFontDatabase::families();
    QFont font = QApplication::font();
    for (const QString& cand : candidates) {
        if (!families.contains(cand)) continue;
        QFont probe(cand);
        if (QFontMetricsF(probe).inFont(QChar(0x6C49))) {   // 「汉」
            font.setFamily(cand);
            fontFamily_ = cand;
            break;
        }
    }
    if (fontFamily_.isEmpty()) {
        QFont probe = QApplication::font();
        if (QFontMetricsF(probe).inFont(QChar(0x6C49))) {
            fontFamily_ = probe.family();
            font = probe;
        } else {
            fontFamily_ = QStringLiteral("(未找到含汉字的字体)");
        }
    }
    QApplication::setFont(font);
    log(QStringLiteral("界面字体：%1").arg(fontFamily_));
}

bool MainWindow::loadImage(const QString& path) {
    auto img = loadImageFile(path.toStdString());
    if (!img) {
        log(QStringLiteral("解码失败：%1").arg(path));
        return false;
    }
    loadedPath_ = path;
    width_ = img->width;
    height_ = img->height;
    pixels_ = img->pixels;
    originalView_->setPixmap(QPixmap::fromImage(toQImage(pixels_, width_, height_))
                                 .scaled(originalView_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    log(QStringLiteral("已加载 %1（%2x%3，格式=%4）")
            .arg(path)
            .arg(width_)
            .arg(height_)
            .arg(QString::fromStdString(img->detectedFormat)));
    unscrambleNow();
    return true;
}

void MainWindow::unscrambleNow() {
    if (pixels_.empty()) {
        log(QStringLiteral("还没有图片可还原"));
        return;
    }
    const int aid = aidEdit_->text().toInt();
    const std::string page = pageEdit_->text().toStdString();
    const int num = jmnext::core::sliceCount(aid, page);
    const auto bands = jmnext::core::bands(width_, height_, num);
    const auto out = jmnext::core::applyBands(pixels_, width_, height_, bands);
    if (out.empty()) {
        log(QStringLiteral("还原失败（尺寸或条带不合法）"));
        return;
    }
    restoredView_->setPixmap(QPixmap::fromImage(toQImage(out, width_, height_))
                                 .scaled(restoredView_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    // 打印一个校验和，便于无显示环境核对（自检会用到）
    unsigned long long sum = 0;
    for (uint32_t v : out) sum = sum * 131 + v;
    log(QStringLiteral("已还原：aid=%1 page=%2 份数=%3 条带=%4 校验和=%5")
            .arg(aid)
            .arg(QString::fromStdString(page))
            .arg(num)
            .arg(bands.size())
            .arg(sum));
}

void MainWindow::log(const QString& line) {
    if (logView_) logView_->appendPlainText(line);
    std::printf("%s\n", line.toUtf8().constData());
    std::fflush(stdout);
}

}  // namespace jmnext::qt
