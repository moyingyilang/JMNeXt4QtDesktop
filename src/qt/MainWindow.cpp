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
#include <QComboBox>
#include <QEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QCoreApplication>
#include <QEventLoop>
#include <QIcon>
#include <QCoreApplication>
#include <QEventLoop>
#include <QIcon>
#include <QListWidget>
#include <QThread>
#include <QTimer>
#include <QThread>
#include <QTimer>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QSplitter>
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

MainWindow::~MainWindow() {
    if (workerThread_) { workerThread_->quit(); workerThread_->wait(3000); }
}

MainWindow::MainWindow() {
    setupUi();
    applyCjkFont();

    // 后台线程：JmClient 的方法都是同步的，放主线程会卡界面
    workerThread_ = new QThread(this);
    worker_ = new JmWorker();
    worker_->moveToThread(workerThread_);
    connect(workerThread_, &QThread::finished, worker_, &QObject::deleteLater);

    connect(worker_, &JmWorker::status, this, [this](const QString& s) { log(s); });
    connect(worker_, &JmWorker::cacheStats, this,
            [this](int ih, int im, int rh, int rm) { lastCacheStats_ = CacheStats{ih, im, rh, rm}; });
    connect(worker_, &JmWorker::failed, this, [this](const QString& s) { log(QStringLiteral("失败：") + s); });
    connect(worker_, &JmWorker::listReady, this,
            [this](const QStringList& titles, const QStringList& ids) {
                listView_->clear();
                for (int i = 0; i < titles.size(); ++i) {
                    auto* item = new QListWidgetItem(titles[i]);
                    item->setData(Qt::UserRole, ids.value(i));
                    listView_->addItem(item);
                }
                log(QStringLiteral("列表已加载：%1 条").arg(titles.size()));
                pendingListResult_ = static_cast<int>(titles.size());
            });
    connect(worker_, &JmWorker::coverReady, this, [this](int index, const QImage& img) {
        if (!listView_ || index < 0 || index >= listView_->count()) return;
        listView_->item(index)->setIcon(QIcon(QPixmap::fromImage(img).scaled(
            72, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        ++coverLoaded_;
    });
    connect(worker_, &JmWorker::pageReady, this, [this](const QImage& img, const QString& statusText) {
        lastPageImage_ = img;
        applyReaderImage(img);
        pageStatus_ = statusText;
        if (pageLabel_) pageLabel_->setText(statusText);
        pendingPageStatus_ = statusText;
        log(statusText);
    });
    connect(chapterList_, &QListWidget::itemClicked, this, [this](QListWidgetItem* it) {
        if (worker_ && it) worker_->openChapterId(it->data(Qt::UserRole).toString(), 0);
    });
    workerThread_->start();
    connect(worker_, &JmWorker::albumCoverReady, this, [this](const QImage& img) {
        lastCoverSize_ = img.size();
        coverLabel_->setPixmap(QPixmap::fromImage(img).scaled(coverLabel_->size(),
                                                              Qt::KeepAspectRatio, Qt::SmoothTransformation));
    });
    connect(worker_, &JmWorker::listAppended, this,
            [this](const QStringList& titles, const QStringList& ids) {
                for (int i = 0; i < titles.size(); ++i) {
                    auto* item = new QListWidgetItem(titles[i]);
                    item->setData(Qt::UserRole, ids.value(i));
                    listView_->addItem(item);
                }
                log(QStringLiteral("列表现已 %1 条").arg(listView_->count()));
                pendingListResult_ = listView_->count();
            });
    connect(worker_, &JmWorker::albumReady, this,
            [this](const QString& name, const QString& author, const QStringList& tags) {
                albumInfo_->setText(QStringLiteral("%1　——　%2\n标签：%3")
                                        .arg(name, author, tags.isEmpty() ? QStringLiteral("（无）")
                                                                          : tags.join(QStringLiteral("、"))));
                last_album_text_ = albumInfo_->text();
            });
    connect(worker_, &JmWorker::chaptersReady, this,
            [this](const QStringList& names, const QStringList& ids) {
                for (int i = 0; i < names.size(); ++i)
                chapterList_->clear();
                for (int i = 0; i < names.size(); ++i) {
                    auto* item = new QListWidgetItem(QStringLiteral("%1. %2").arg(i + 1).arg(names[i]));
                    item->setData(Qt::UserRole, ids.value(i));
                    chapterList_->addItem(item);
                }
                log(QStringLiteral("章节列表：%1 个").arg(names.size()));
            });
    blockEdit_->setPlaceholderText(QStringLiteral("屏蔽关键词（逗号分隔），例如 NTR"));
    connect(blockEdit_, &QLineEdit::editingFinished, this, [this] {
        if (!worker_) return;
        QStringList words;
        const auto parts = blockEdit_->text().split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const auto& w : parts) words << w.trimmed();
        worker_->setBlockWords(words);
        worker_->loadList();
    });
}

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("JMNeXt4QtDesktop %1 —— JMComic 桌面端（原生 C++/Qt，早期阶段）")
                       .arg(jmnext::core::APP_VERSION));
    auto* split = new QSplitter(Qt::Horizontal);
    auto* leftPanel = new QWidget();   auto* leftCol = new QVBoxLayout(leftPanel);
    auto* rightPanel = new QWidget();  auto* rightCol = new QVBoxLayout(rightPanel);
    leftCol->setContentsMargins(10, 8, 6, 8);    leftCol->setSpacing(6);
    rightCol->setContentsMargins(6, 8, 10, 8);   rightCol->setSpacing(6);
    split->addWidget(leftPanel);
    split->addWidget(rightPanel);
    split->setStretchFactor(0, 0);               // 左侧窄
    split->setStretchFactor(1, 1);               // 右侧主区域

    auto* views = new QHBoxLayout();
    coverLabel_ = new QLabel(QStringLiteral("（封面）"));
    coverLabel_->setFixedSize(160, 213);          // 3:4
    coverLabel_->setAlignment(Qt::AlignCenter);
    coverLabel_->setObjectName(QStringLiteral("cover"));   // 交给 QSS
    views->addWidget(coverLabel_);          // 必须在 coverLabel_ 创建之后加（此前插在前面，导致布局收到空指针）
    albumInfo_ = new QLabel(QStringLiteral("（尚未选择作品）"));
    albumInfo_->setWordWrap(true);
    rightCol->addWidget(albumInfo_);
    originalView_ = new QLabel(QStringLiteral("（未加载图片）"));
    restoredView_ = new QLabel(QStringLiteral("（未还原）"));
    restoredScroll_ = new QScrollArea();
    restoredScroll_->setWidget(restoredView_);
    restoredScroll_->setWidgetResizable(true);
    originalView_->setAlignment(Qt::AlignCenter);
    restoredView_->setAlignment(Qt::AlignCenter);
    for (QWidget* v : {static_cast<QWidget*>(originalView_), static_cast<QWidget*>(restoredScroll_)}) {
        v->setMinimumSize(240, 320);
        v->setObjectName(QStringLiteral("viewer"));   // 交给 QSS（内联样式会盖掉主题，所以改用 objectName）
        v->installEventFilter(this);          // 滚轮翻页（只作用于图片区）
        views->addWidget(v);
    }
    // 左侧：真实首页列表（双击进入阅读器）
    listView_ = new QListWidget();
    listView_->setMinimumWidth(300);           // 避免被压得过窄（列表面板的下限）
    listView_->setMinimumWidth(300);
    auto* listRow = new QHBoxLayout();
    listRow->addWidget(listView_);
    auto* loadListBtn = new QPushButton(QStringLiteral("加载真实首页列表"));
    auto* moreBtn = new QPushButton(QStringLiteral("加载更多"));
    listRow->addWidget(moreBtn);
    connect(moreBtn, &QPushButton::clicked, this, [this] { if (worker_) worker_->loadMore(); });
    blockEdit_ = new QLineEdit();
    chapterList_ = new QListWidget();
    chapterList_->setMinimumHeight(150);
    {
        auto* h = new QLabel(QStringLiteral("② 章节（点选切换）"));
        h->setStyleSheet(QStringLiteral("color:#8a8f98; padding-top:6px;"));
        leftCol->addWidget(h);
    }
    leftCol->addWidget(chapterList_, 1);   // 章节列表占一份
    listRow->addWidget(blockEdit_);
    blockEdit_->setPlaceholderText(QStringLiteral("toggle"));   // 文本在构造函数里设置（避免此处出现中文标点）
    listRow->addWidget(loadListBtn);
    {
        auto* h = new QLabel(QStringLiteral("① 作品列表（双击进入阅读）"));
        h->setStyleSheet(QStringLiteral("color:#8a8f98; padding-top:6px;"));
        leftCol->addWidget(h);
    }
    leftCol->addLayout(listRow);
    {
        auto* h = new QLabel(QStringLiteral("③ 阅读区（← → 翻页，滚轮可用，适应窗口 / 100% 可切换）"));
        h->setStyleSheet(QStringLiteral("color:#8a8f98; padding-top:6px;"));
        rightCol->addWidget(h);
    }
    pageLabel_ = new QLabel(QStringLiteral("（尚未打开章节）"));
    pageLabel_->setStyleSheet(QStringLiteral("color:#b9bcc2; padding:2px 0;"));
    rightCol->addWidget(pageLabel_);
    rightCol->addLayout(views);
    rightCol->setStretchFactor(views, 2);   // 阅读区占两份，让它成为主区域

    auto* form = new QFormLayout();
    aidEdit_ = new QLineEdit(QStringLiteral("1"));
    pageEdit_ = new QLineEdit(QStringLiteral("1"));
    form->addRow(QStringLiteral("作品号 (aid)"), aidEdit_);
    form->addRow(QStringLiteral("页码 (page，接口原样字符串)"), pageEdit_);
    rightCol->addLayout(form);

    auto* buttons = new QHBoxLayout();
    auto* buttons2 = new QHBoxLayout();      // 第二行按钮（避开一行塞 6 个）
    buttons2->setSpacing(6);
    buttons->setSpacing(6);                    // 8 个按钮挤在一行时更要留缝
    auto* openBtn = new QPushButton(QStringLiteral("打开图片…"));
    auto* zoomBtn = new QPushButton(QStringLiteral("适应窗口 / 100%"));
    buttons->addWidget(zoomBtn);
    auto* unscrambleBtn = new QPushButton(QStringLiteral("还原"));
    buttons2->addWidget(openBtn);
    buttons2->addWidget(unscrambleBtn);
    rightCol->addLayout(buttons);

    rightCol->addLayout(buttons2);            // 第二行：本地图片工具（与阅读控制分开）
    log(QStringLiteral("快捷键：← / PageUp 上一页，→ / PageDown / 空格 / 滚轮 下一页；章节用左侧列表点选切换"));
    logView_ = new QPlainTextEdit();
    logView_->setReadOnly(true);
    logView_->setMaximumHeight(120);
    rightCol->addWidget(logView_);

    setCentralWidget(split);                   // 根容器换成左右分栏

    connect(openBtn, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择漫画图片"));
        if (!path.isEmpty()) loadImage(path);
    });
    connect(unscrambleBtn, &QPushButton::clicked, this, [this] { unscrambleNow(); });

    // 阅读器控制：上一页 / 下一页（真实数据）
    auto* prevBtn = new QPushButton(QStringLiteral("上一页"));
    auto* nextBtn = new QPushButton(QStringLiteral("下一页"));
    auto* openChBtn = new QPushButton(QStringLiteral("载入真实章节（用上方 aid）"));
    buttons->addWidget(openChBtn);
    buttons->addWidget(prevBtn);
    buttons->addWidget(nextBtn);
    connect(openChBtn, &QPushButton::clicked, this, [this] {
        if (worker_) worker_->openChapter(aidEdit_->text(), 0);      // 异步：走后台线程
    });
    // 加载列表走**后台线程**（JmClient 的方法都是同步的，放主线程会卡界面）
    connect(loadListBtn, &QPushButton::clicked, this, [this] {
        if (worker_) worker_->loadList();      // 队列连接到工作线程
    });
    connect(listView_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* it) {
        // 双击某条 → 用它的 id 当 aid 打开（骨架阶段常见做法：列表即作品）
        aidEdit_->setText(it->data(Qt::UserRole).toString());
        openChapter(it->data(Qt::UserRole).toString(), 0);
    });
    connect(zoomBtn, &QPushButton::clicked, this, [this] {
        setZoomFit(!zoomFit_);
        if (!lastPageImage_.isNull()) applyReaderImage(lastPageImage_);   // 立刻按新模式重排
    });
    connect(prevBtn, &QPushButton::clicked, this, [this] { if (worker_) worker_->step(-1); });
    connect(nextBtn, &QPushButton::clicked, this, [this] { if (worker_) worker_->step(1); });
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

namespace jmnext::qt {

// 说明：JmClient 的方法是同步的（走真实网络）。骨架阶段直接在界面线程调用，
// 会有短暂卡顿；异步化（后台线程 + 信号）留到下一步。
bool MainWindow::openChapter(const QString& aid, int pageIndex) {
    if (!clientReady_) {
        log(QStringLiteral("正在发现主机…"));
        if (!client_.bootstrap()) {
            log(QStringLiteral("主机发现失败：%1").arg(QString::fromStdString(client_.lastError())));
            return false;
        }
        clientReady_ = true;
        log(QStringLiteral("主机：%1").arg(QString::fromStdString(client_.host())));
    }
    auto al = client_.album(aid.toStdString());
    if (!al) {
        log(QStringLiteral("详情失败：%1").arg(QString::fromStdString(client_.lastError())));
        return false;
    }
    log(QStringLiteral("详情：%1（标签 %2 个，章节 %3 个）")
            .arg(QString::fromStdString(al->name))
            .arg(al->tags.size())
            .arg(al->series.size()));
    if (al->series.empty()) return false;

    const std::string chapterId = al->series.front().id;
    auto ch = client_.chapter(chapterId);
    if (!ch) {
        log(QStringLiteral("章节失败：%1").arg(QString::fromStdString(client_.lastError())));
        return false;
    }
    chapter_ = *ch;
    chapterAid_ = aid.toStdString();
    log(QStringLiteral("章节：%1 共 %2 页，scramble_id=%3")
            .arg(QString::fromStdString(chapter_.id))
            .arg(chapter_.totalPage)
            .arg(chapter_.scrambleId));
    return showPage(pageIndex);
}

bool MainWindow::showPage(int index) {
    if (chapter_.images.empty()) { log(QStringLiteral("章节没有图片")); return false; }
    if (index < 0 || index >= static_cast<int>(chapter_.images.size())) {
        log(QStringLiteral("页码越界：%1（共 %2 页）").arg(index).arg(chapter_.images.size()));
        return false;
    }
    const auto& page = chapter_.images[static_cast<std::size_t>(index)];
    auto img = client_.pageImage(page.url, std::atoi(chapterAid_.c_str()), chapter_.scrambleId);
    if (!img) {
        log(QStringLiteral("取图失败：%1").arg(QString::fromStdString(client_.lastError())));
        return false;
    }
    pageIndex_ = index;
    lastPageImage_ = *img;
    applyReaderImage(*img);
    pageStatus_ = QStringLiteral("第 %1/%2 页 %3x%4")
                      .arg(index + 1)
                      .arg(chapter_.images.size())
                      .arg(img->width())
                      .arg(img->height());
    log(pageStatus_ + QStringLiteral("（aid=%1 page=%2）")
                           .arg(QString::fromStdString(chapterAid_))
                           .arg(QString::fromStdString(page.url)));
    return true;
}

bool MainWindow::nextPage() { return showPage(pageIndex_ + 1); }
bool MainWindow::prevPage() { return showPage(pageIndex_ - 1); }

}  // namespace jmnext::qt

namespace jmnext::qt {

int MainWindow::loadRealList() {
    if (!clientReady_) {
        if (!client_.bootstrap()) {
            log(QStringLiteral("主机发现失败：%1").arg(QString::fromStdString(client_.lastError())));
            return -1;
        }
        clientReady_ = true;
        log(QStringLiteral("主机：%1").arg(QString::fromStdString(client_.host())));
    }
    auto list = client_.latest(0);
    if (!list) {
        log(QStringLiteral("列表失败：%1").arg(QString::fromStdString(client_.lastError())));
        return -1;
    }
    listView_->clear();
    for (const auto& e : *list) {
        const QString text = QStringLiteral("%1\n   %2　[%3]")
                                 .arg(QString::fromStdString(e.name))
                                 .arg(QString::fromStdString(e.author))
                                 .arg(QString::fromStdString(e.categoryTitle));
        auto* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, QString::fromStdString(e.id));
        listView_->addItem(item);
    }
    log(QStringLiteral("列表已加载：%1 条，第一条 id=%2")
            .arg(list->size())
            .arg(QString::fromStdString(list->front().id)));
    return static_cast<int>(list->size());
}

int MainWindow::listCount() const { return listView_ ? listView_->count() : 0; }

QString MainWindow::listItemText(int i) const {
    if (!listView_ || i < 0 || i >= listView_->count()) return {};
    return listView_->item(i)->text();
}

}  // namespace jmnext::qt

namespace jmnext::qt {

int MainWindow::loadCoversFirst(int n) {
    if (!clientReady_ || !listView_) return 0;
    // 重新拉一次列表数据（这里只需要每条对应的封面地址参数；列表本身已在界面上）
    auto list = client_.latest(0);
    if (!list) return 0;
    int loaded = 0;
    const int limit = std::min<int>(n, static_cast<int>(list->size()));
    for (int i = 0; i < limit; ++i) {
        auto img = client_.cover((*list)[static_cast<std::size_t>(i)]);
        if (!img) { log(QStringLiteral("第 %1 项封面失败：%2").arg(i + 1).arg(QString::fromStdString(client_.lastError()))); continue; }
        QPixmap pm = QPixmap::fromImage(*img).scaled(72, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        listView_->item(i)->setIcon(QIcon(pm));
        ++loaded;
    }
    coverLoaded_ = loaded;
    log(QStringLiteral("已为前 %1 项加载封面：成功 %2 张").arg(limit).arg(loaded));
    return loaded;
}

}  // namespace jmnext::qt

namespace jmnext::qt {
namespace {
/// 跑事件循环直到条件满足或超时（自检用；不引入自定义类型）
template <typename Predicate>
bool pumpUntil(QEventLoop& loop, QTimer& timer, Predicate done) {
    while (!done()) {
        if (!loop.isRunning()) return false;
        loop.processEvents(QEventLoop::AllEvents, 50);
        if (timer.remainingTime() < 0) return false;
    }
    return true;
}
}  // namespace

int MainWindow::requestListAndWait(int timeoutMs) {
    if (!worker_) return -1;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    pumpList_ = true;
    pendingListResult_ = -1;
    worker_->loadList();                      // 队列连接到工作线程
    // 等到 listReady 或超时
    while (pendingListResult_ < 0 && loop.isRunning()) {
        loop.processEvents(QEventLoop::AllEvents, 100);
        if (timer.remainingTime() < 0) break;
    }
    pumpList_ = false;
    return pendingListResult_;
}

int MainWindow::requestCoversAndWait(int n, int timeoutMs) {
    if (!worker_) return 0;
    coverLoaded_ = 0;
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(timeoutMs);
    worker_->loadCovers(n);
    while (coverLoaded_ < n && timer.remainingTime() > 0) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    return coverLoaded_;
}

}  // namespace jmnext::qt

namespace jmnext::qt {

int MainWindow::requestReaderAndWait(const QString& aid, int page, int steps, int timeoutMs) {
    if (!worker_) return -1;
    int pages = 0;
    pendingPageStatus_.clear();
    auto conn = connect(worker_, &JmWorker::pageReady, this, [&pages](const QImage&, const QString&) { ++pages; });
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(timeoutMs);
    worker_->openChapter(aid, page);
    while (pages < 1 && timer.remainingTime() > 0) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    for (int i = 0; i < steps && timer.remainingTime() > 0; ++i) {
        const int before = pages;
        worker_->step(1);
        while (pages == before && timer.remainingTime() > 0)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    disconnect(conn);
    return pages;
}

}  // namespace jmnext::qt

namespace jmnext::qt {

int MainWindow::requestChapterPickAndWait(const QString& aid, int index, int timeoutMs) {
    if (!worker_) return -1;
    int chapterCount = 0, pages = 0;
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(timeoutMs);
    auto c1 = connect(worker_, &JmWorker::chaptersReady, this,
                      [&chapterCount](const QStringList& names, const QStringList&) { chapterCount = names.size(); });
    auto c2 = connect(worker_, &JmWorker::pageReady, this,
                      [&pages](const QImage&, const QString&) { ++pages; });
    worker_->openChapter(aid, 0);
    while (chapterCount == 0 && timer.remainingTime() > 0)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    if (chapterCount == 0) { disconnect(c1); disconnect(c2); return -1; }
    if (index >= 0 && index < chapterCount) {
        const int before = pages;
        if (!chapterList_ || !chapterList_->item(index)) return -1;   // 列表未就绪：明确失败，别解引用空指针
        worker_->openChapterId(chapterList_->item(index)->data(Qt::UserRole).toString(), 0);
        while (pages == before && timer.remainingTime() > 0)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    disconnect(c1);
    disconnect(c2);
    return pages > 0 ? chapterCount : -1;
}

}  // namespace jmnext::qt

namespace jmnext::qt {

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (!worker_) { QMainWindow::keyPressEvent(event); return; }
    switch (event->key()) {
        case Qt::Key_Left:
        case Qt::Key_PageUp:
        case Qt::Key_Backspace:
            worker_->step(-1);
            return;
        case Qt::Key_Right:
        case Qt::Key_PageDown:
        case Qt::Key_Space:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            worker_->step(1);
            return;
        default:
            break;
    }
    QMainWindow::keyPressEvent(event);
}

}  // namespace jmnext::qt

namespace jmnext::qt {

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    // 只处理两个图片区上的滚轮；列表的滚动仍归列表自己
    if (worker_ && (watched == originalView_ || watched == restoredView_) &&
        event->type() == QEvent::Wheel) {
        auto* wheel = static_cast<QWheelEvent*>(event);
        const int dy = wheel->angleDelta().y();
        if (dy != 0) {
            worker_->step(dy > 0 ? -1 : 1);      // 上滚 → 上一页；下滚 → 下一页
            return true;                          // 吃掉事件，避免再滚动父级
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

}  // namespace jmnext::qt

namespace jmnext::qt {

void MainWindow::requestCacheStatsAndWait(int timeoutMs) {
    if (!worker_) return;
    bool got = false;
    auto conn = connect(worker_, &JmWorker::cacheStats, this, [&got](int, int, int, int) { got = true; });
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(timeoutMs);
    worker_->reportCacheStats();
    while (!got && timer.remainingTime() > 0) QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    disconnect(conn);
}

}  // namespace jmnext::qt

namespace jmnext::qt {

int MainWindow::requestLoadMoreAndWait(int timeoutMs) {
    if (!worker_) return -1;
    const int before = listView_ ? listView_->count() : 0;
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(timeoutMs);
    worker_->loadMore();
    while (listView_ && listView_->count() == before && timer.remainingTime() > 0)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    return listView_ ? listView_->count() : -1;
}

}  // namespace jmnext::qt

namespace jmnext::qt {

void MainWindow::applyReaderImage(const QImage& image) {
    if (image.isNull()) return;
    QPixmap pm = QPixmap::fromImage(image);
    if (zoomFit_) {
        const QSize target = restoredScroll_->viewport()->size();
        pm = pm.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    restoredView_->setPixmap(pm);
    restoredView_->resize(pm.size());          // 100% 模式下让滚动区域能滚起来
    displayedSize_ = pm.size();
}

void MainWindow::setZoomFit(bool fit) {
    zoomFit_ = fit;
    log(fit ? QStringLiteral("缩放：适应窗口") : QStringLiteral("缩放：原始尺寸 100%（可滚动）"));
}

}  // namespace jmnext::qt

namespace jmnext::qt {

QSize MainWindow::repaintReaderAndSize() {
    if (!lastPageImage_.isNull()) applyReaderImage(lastPageImage_);
    return displayedSize_;
}

}  // namespace jmnext::qt

namespace jmnext::qt {

int MainWindow::chapterListCount() const { return chapterList_ ? chapterList_->count() : 0; }

}  // namespace jmnext::qt
