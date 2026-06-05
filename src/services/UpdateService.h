#ifndef UPDATESERVICE_H
#define UPDATESERVICE_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkReply;

class UpdateService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool checking READ checking NOTIFY changed)
    Q_PROPERTY(QString currentVersion READ currentVersion NOTIFY changed)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY changed)
    Q_PROPERTY(QString statusText READ statusText NOTIFY changed)
    Q_PROPERTY(QString releaseName READ releaseName NOTIFY changed)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY changed)
    Q_PROPERTY(QString releaseUrl READ releaseUrl NOTIFY changed)
    Q_PROPERTY(QString assetName READ assetName NOTIFY changed)
    Q_PROPERTY(QString assetSizeText READ assetSizeText NOTIFY changed)
    Q_PROPERTY(bool assetAvailable READ assetAvailable NOTIFY changed)

public:
    explicit UpdateService(QObject* parent = nullptr);

    bool checking() const;
    QString currentVersion() const;
    QString latestVersion() const;
    bool updateAvailable() const;
    QString statusText() const;
    QString releaseName() const;
    QString releaseNotes() const;
    QString releaseUrl() const;
    QString assetName() const;
    QString assetSizeText() const;
    bool assetAvailable() const;

    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void openReleasePage() const;
    Q_INVOKABLE void openReleasesPage() const;
    Q_INVOKABLE void downloadLatestAsset() const;

signals:
    void changed();
    void log(const QString& line);

private slots:
    void onLatestReleaseFinished();

private:
    struct Asset {
        QString name;
        QUrl url;
        qint64 size = 0;
    };

    static int compareVersions(QString left, QString right);
    static QString formatSize(qint64 bytes);
    static int platformAssetScore(const QString& name);

    void resetReleaseState();
    void setChecking(bool checking);
    void setStatusText(const QString& statusText);

private:
    QNetworkAccessManager m_net;
    QNetworkReply* m_reply = nullptr;
    bool m_checking = false;
    QString m_currentVersion;
    QString m_latestVersion;
    bool m_updateAvailable = false;
    QString m_statusText;
    QString m_releaseName;
    QString m_releaseNotes;
    QUrl m_releaseUrl;
    Asset m_asset;
};

#endif // UPDATESERVICE_H
