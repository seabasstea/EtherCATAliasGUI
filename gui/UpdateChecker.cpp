#include "UpdateChecker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVersionNumber>

static const char kLatestReleaseUrl[] =
    "https://api.github.com/repos/seabasstea/EtherCATAliasGUI/releases/latest";

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent) {}

void UpdateChecker::check(bool interactive)
{
    if (m_inFlight)
        return;
    m_inFlight = true;

    QNetworkRequest request{QUrl(QLatin1String(kLatestReleaseUrl))};
    request.setRawHeader("Accept", "application/vnd.github+json");
    // GitHub rejects requests without a User-Agent
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("EtherCATAliasGUI/" APP_VERSION));
    request.setTransferTimeout(10000);

    QNetworkReply *reply = m_nam.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, interactive] {
        reply->deleteLater();
        m_inFlight = false;

        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 404) {
            // No published releases yet
            emit logMessage(QStringLiteral("Update check: no releases published."));
            emit upToDate(interactive);
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            emit logMessage(QStringLiteral("Update check failed: %1").arg(reply->errorString()));
            return;
        }

        const QJsonObject release = QJsonDocument::fromJson(reply->readAll()).object();
        QString tag = release.value(QStringLiteral("tag_name")).toString();
        const QUrl pageUrl(release.value(QStringLiteral("html_url")).toString());
        if (tag.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
            tag.remove(0, 1);

        const QVersionNumber latest  = QVersionNumber::fromString(tag);
        const QVersionNumber current = QVersionNumber::fromString(QStringLiteral(APP_VERSION));
        if (latest.isNull() || !pageUrl.isValid()) {
            emit logMessage(QStringLiteral("Update check: could not parse latest release."));
            return;
        }

        if (latest > current) {
            emit logMessage(QStringLiteral("Update available: %1 (installed: %2)")
                                .arg(tag, QStringLiteral(APP_VERSION)));
            emit updateAvailable(tag, pageUrl, interactive);
        } else {
            emit logMessage(QStringLiteral("Up to date (version %1).")
                                .arg(QStringLiteral(APP_VERSION)));
            emit upToDate(interactive);
        }
    });
}
