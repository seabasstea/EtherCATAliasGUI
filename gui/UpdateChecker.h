#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

// Queries the GitHub releases API for a newer published release.
// /releases/latest excludes drafts and pre-releases by API contract.
class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);

public slots:
    // interactive=true also reports "up to date" / errors with a dialog
    // (used by the Help menu); the startup check stays silent on those.
    void check(bool interactive);

signals:
    void updateAvailable(const QString &latestVersion, const QUrl &releaseUrl, bool interactive);
    void upToDate(bool interactive);
    void logMessage(const QString &message);

private:
    QNetworkAccessManager m_nam;
    bool m_inFlight = false;
};
