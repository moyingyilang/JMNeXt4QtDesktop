#pragma once
// 本地收藏（对应主项目 shared 的 FavoriteStore）。
//
// 为什么本地存：收藏列表来自需要登录态的接口，cookie 失效或断网时一条都取不到；
// 而「收藏了哪些作品、分别放进哪个分组」是用户自己整理出来的数据，服务端丢了
// 也没法凭空重建。本地留一份，启动就能显示，登录成功后再由上层决定怎么合并。
//
// 为什么整表重写：收藏是用户点击级别的低频写入，上限 1000 条时整表也就几十 KB，
// 重写的耗时可以忽略；换来的是磁盘上永远只有一个完整的 JSON 数组，加载端不必
// 处理追加日志里的重复项和「进程被杀留下的半条记录」。落盘走 QSaveFile，
// 先写临时文件再原子替换，写到一半被杀时旧文件仍然完整，不会变成截断的 JSON。
//
// 读写都在 GUI 线程发生，数据量小，不加锁。
#include <algorithm>

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QString>
#include <QVector>

namespace jmnext::core {

struct FavoriteEntry {
    QString aid;
    QString title;
    QString folder;         // 分组名，空串表示未分组
    qint64 timestamp = 0;   // 最近一次收藏/更新的毫秒时间，用于排序与淘汰
};

class FavoriteStore {
public:
    static FavoriteStore& instance() {
        static FavoriteStore s;
        return s;
    }

    /// 时间倒序（最新在前），与磁盘上的顺序一致。
    QVector<FavoriteEntry> list() const { return m_entries; }

    /// 收藏或更新：同一 aid 只保留一条，重复调用刷新时间与标题。
    /// 空的 title/folder 视为「调用方没带这项信息」，保留已有值，避免从别的界面
    /// 重复收藏时把用户填过的分组抹掉。
    void remember(const QString& aid, const QString& title, const QString& folder) {
        if (aid.isEmpty()) return;
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        for (int i = 0; i < m_entries.size(); ++i) {
            FavoriteEntry& e = m_entries[i];
            if (e.aid != aid) continue;
            if (!title.isEmpty()) e.title = title;
            if (!folder.isEmpty()) e.folder = folder;
            e.timestamp = now;
            // 更新时间后该条应排到最前；排序而不是移动，系统时间回拨时也仍然有序。
            sortByTimeDesc(m_entries);
            save();
            return;
        }
        FavoriteEntry e;
        e.aid = aid;
        e.title = title;
        e.folder = folder;
        e.timestamp = now;
        m_entries.prepend(e);
        // 已按时间倒序，超出上限的尾部就是最旧的，直接截掉。
        if (m_entries.size() > kMax) m_entries.resize(kMax);
        save();
    }

    /// 取消收藏。返回是否真的删掉了一条，调用方可以据此决定要不要提示。
    bool remove(const QString& aid) {
        for (int i = 0; i < m_entries.size(); ++i) {
            if (m_entries[i].aid != aid) continue;
            m_entries.removeAt(i);
            save();
            return true;
        }
        return false;
    }

    void clear() {
        m_entries.clear();
        save();
    }

    bool contains(const QString& aid) const {
        if (aid.isEmpty()) return false;
        for (const FavoriteEntry& e : m_entries) {
            if (e.aid == aid) return true;
        }
        return false;
    }

    static constexpr int kMax = 1000;

private:
    FavoriteStore() { load(); }

    static QString filePath() {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        return dir + QStringLiteral("/favorites.json");
    }

    static void sortByTimeDesc(QVector<FavoriteEntry>& entries) {
        std::sort(entries.begin(), entries.end(),
                  [](const FavoriteEntry& a, const FavoriteEntry& b) { return a.timestamp > b.timestamp; });
    }

    void load() {
        QFile f(filePath());
        if (!f.open(QIODevice::ReadOnly)) return;
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        // 坏 JSON（或被手工改成非数组）不做局部抢救：收藏是低价值可重建的数据，
        // 与其拿半份内容覆盖，不如按空表启动，下一次写入自然修好文件。
        if (!doc.isArray()) return;
        const QJsonArray arr = doc.array();
        // 文件可能被手工编辑或由旧版本写出，同一 aid 出现多次时保留时间较新的一条。
        QHash<QString, int> index;
        for (const auto& v : arr) {
            const QJsonObject o = v.toObject();
            FavoriteEntry e;
            e.aid = o.value(QStringLiteral("aid")).toString();
            if (e.aid.isEmpty()) continue;
            e.title = o.value(QStringLiteral("title")).toString();
            e.folder = o.value(QStringLiteral("folder")).toString();
            e.timestamp = static_cast<qint64>(o.value(QStringLiteral("ts")).toDouble());
            auto it = index.find(e.aid);
            if (it != index.end()) {
                FavoriteEntry& kept = m_entries[it.value()];
                if (e.timestamp >= kept.timestamp) kept = e;
                continue;
            }
            index.insert(e.aid, m_entries.size());
            m_entries.append(e);
        }
        sortByTimeDesc(m_entries);
        // 早期版本可能没有上限，加载时也裁一次，保证内存里不超过 kMax。
        if (m_entries.size() > kMax) m_entries.resize(kMax);
    }

    void save() const {
        QJsonArray arr;
        for (const auto& e : m_entries) {
            QJsonObject o;
            o.insert(QStringLiteral("aid"), e.aid);
            o.insert(QStringLiteral("title"), e.title);
            o.insert(QStringLiteral("folder"), e.folder);
            o.insert(QStringLiteral("ts"), static_cast<double>(e.timestamp));
            arr.append(o);
        }
        const QByteArray payload = QJsonDocument(arr).toJson(QJsonDocument::Compact);
        QSaveFile f(filePath());
        if (!f.open(QIODevice::WriteOnly)) return;
        if (f.write(payload) != payload.size()) {
            f.cancelWriting();
            return;
        }
        f.commit();   // 提交失败时磁盘上仍是上一版，下次修改会再写
    }

    QVector<FavoriteEntry> m_entries;
};

} // namespace jmnext::core
