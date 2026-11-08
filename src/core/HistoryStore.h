#pragma once
// 本地阅读历史（对应主项目 shared 的 HistoryStore）。
//
// 为什么桌面端也要本地存：历史只在个别接口返回、粒度到作品，断网就没有；
// 本地存一份之后「我读过什么」永远可见。写入时机是打开作品时。
//
// 存储形态：单个 JSON 文件，整表重写（进程被杀不会留下半条记录），上限 1000 条按时间淘汰。
#include <algorithm>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QString>
#include <QVector>

namespace jmnext::core {

struct HistoryEntry {
    QString aid;
    QString title;
    qint64 timestamp = 0;   // 毫秒
};

class HistoryStore {
public:
    static HistoryStore& instance() {
        static HistoryStore s;
        return s;
    }

    QVector<HistoryEntry> list() const { return m_entries; }

    /// 记录一次打开：同一 aid 只保留最新一条（时间更新），超过上限淘汰最旧。
    void remember(const QString& aid, const QString& title) {
        if (aid.isEmpty()) return;
        for (int i = 0; i < m_entries.size(); ++i) {
            if (m_entries[i].aid == aid) {
                m_entries[i].title = title.isEmpty() ? m_entries[i].title : title;
                m_entries[i].timestamp = QDateTime::currentMSecsSinceEpoch();
                std::sort(m_entries.begin(), m_entries.end(),
                          [](const HistoryEntry& a, const HistoryEntry& b) { return a.timestamp > b.timestamp; });
                save();
                return;
            }
        }
        HistoryEntry e;
        e.aid = aid;
        e.title = title;
        e.timestamp = QDateTime::currentMSecsSinceEpoch();
        m_entries.prepend(e);
        while (m_entries.size() > kMax) m_entries.removeLast();
        save();
    }

    void clear() {
        m_entries.clear();
        save();
    }

    static constexpr int kMax = 1000;

private:
    HistoryStore() { load(); }

    static QString filePath() {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        return dir + QStringLiteral("/history.json");
    }

    void load() {
        QFile f(filePath());
        if (!f.open(QIODevice::ReadOnly)) return;
        const auto doc = QJsonDocument::fromJson(f.readAll());
        const auto arr = doc.array();
        for (const auto& v : arr) {
            const auto o = v.toObject();
            HistoryEntry e;
            e.aid = o.value(QStringLiteral("aid")).toString();
            e.title = o.value(QStringLiteral("title")).toString();
            e.timestamp = static_cast<qint64>(o.value(QStringLiteral("ts")).toDouble());
            if (!e.aid.isEmpty()) m_entries.append(e);
        }
        // 坏数据（空 aid）已被过滤；顺序按时间倒序
        std::sort(m_entries.begin(), m_entries.end(),
                  [](const HistoryEntry& a, const HistoryEntry& b) { return a.timestamp > b.timestamp; });
    }

    void save() const {
        QJsonArray arr;
        for (const auto& e : m_entries) {
            QJsonObject o;
            o.insert(QStringLiteral("aid"), e.aid);
            o.insert(QStringLiteral("title"), e.title);
            o.insert(QStringLiteral("ts"), static_cast<double>(e.timestamp));
            arr.append(o);
        }
        QFile f(filePath());
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    }

    QVector<HistoryEntry> m_entries;
};

} // namespace jmnext::core
