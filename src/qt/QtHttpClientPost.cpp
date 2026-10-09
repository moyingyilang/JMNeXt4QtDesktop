// QtHttpClient::post 实现（单独成文件：追加到 QtHttpClient.cpp 末尾会落到命名空间之外）。
#include "QtHttpClient.h"

#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace jmnext::qt {

jmnext::net::HttpResponse QtHttpClient::post(const std::string& url, const std::string& body,
                                             const std::vector<std::string>& headers) {
    lastError_.clear();
    jmnext::net::HttpResponse out;

    QNetworkRequest req{QUrl(QString::fromStdString(url))};
    for (const auto& h : headers) {
        const auto pos = h.find(':');
        if (pos == std::string::npos) continue;
        std::string name = h.substr(0, pos);
        std::string value = h.substr(pos + 1);
        while (!value.empty() && value.front() == ' ') value.erase(value.begin());
        req.setRawHeader(QByteArray::fromStdString(name), QByteArray::fromStdString(value));
    }

    QNetworkReply* reply = manager_.post(req, QByteArray::fromStdString(body));
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(20000);
    loop.exec();

    if (!reply->isFinished()) {
        lastError_ = "timeout";
        reply->abort();
        reply->deleteLater();
        return out;
    }
    out.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    out.body = reply->readAll().toStdString();
    if (reply->error() != QNetworkReply::NoError && out.status == 0)
        lastError_ = reply->errorString().toStdString();
    reply->deleteLater();
    return out;
}

}  // namespace jmnext::qt
