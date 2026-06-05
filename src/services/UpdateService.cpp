#include "UpdateService.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSysInfo>

#include <algorithm>

namespace {
constexpr const char* LatestReleaseUrl = "https://api.github.com/repos/ayurash77/pp18-video-tools_qml/releases/latest";
constexpr const char* ReleasesPageUrl = "https://github.com/ayurash77/pp18-video-tools_qml/releases";

QString normalizedVersion(QString value)
{
    value = value.trimmed();
    if (value.startsWith('v') || value.startsWith('V'))
        value.remove(0, 1);
    return value;
}

QVector<int> versionParts(const QString& value)
{
    QVector<int> result;
    static const QRegularExpression separator("[^0-9]+");
    const QStringList parts = normalizedVersion(value).split(separator, Qt::SkipEmptyParts);
    for (const QString& part : parts)
        result.append(part.toInt());
    return result;
}

bool containsAny(const QString& value, std::initializer_list<const char*> needles)
{
    for (const char* needle : needles) {
        if (value.contains(QLatin1String(needle)))
            return true;
    }
    return false;
}

} // namespace

UpdateService::UpdateService(QObject* parent)
    : QObject(parent)
    , m_currentVersion(QCoreApplication::applicationVersion())
    , m_statusText(QStringLiteral("Проверка обновлений еще не выполнялась."))
{
}

bool UpdateService::checking() const
{
    return m_checking;
}

QString UpdateService::currentVersion() const
{
    return m_currentVersion;
}

QString UpdateService::latestVersion() const
{
    return m_latestVersion;
}

bool UpdateService::updateAvailable() const
{
    return m_updateAvailable;
}

QString UpdateService::statusText() const
{
    return m_statusText;
}

QString UpdateService::releaseName() const
{
    return m_releaseName;
}

QString UpdateService::releaseNotes() const
{
    return m_releaseNotes;
}

QString UpdateService::releaseUrl() const
{
    return m_releaseUrl.toString();
}

QString UpdateService::assetName() const
{
    return m_asset.name;
}

QString UpdateService::assetSizeText() const
{
    return formatSize(m_asset.size);
}

bool UpdateService::assetAvailable() const
{
    return m_asset.url.isValid();
}

void UpdateService::checkForUpdates()
{
    if (m_reply)
        return;

    resetReleaseState();
    setChecking(true);
    setStatusText(QStringLiteral("Проверяю GitHub Releases..."));

    QNetworkRequest request(QUrl(QString::fromLatin1(LatestReleaseUrl)));
    request.setHeader(QNetworkRequest::UserAgentHeader,
        QStringLiteral("PP18-VideoTools/%1").arg(m_currentVersion));
    request.setRawHeader("Accept", "application/vnd.github+json");

    m_reply = m_net.get(request);
    connect(m_reply, &QNetworkReply::finished, this, &UpdateService::onLatestReleaseFinished);
}

void UpdateService::openReleasePage() const
{
    if (m_releaseUrl.isValid())
        QDesktopServices::openUrl(m_releaseUrl);
    else
        openReleasesPage();
}

void UpdateService::openReleasesPage() const
{
    QDesktopServices::openUrl(QUrl(QString::fromLatin1(ReleasesPageUrl)));
}

void UpdateService::downloadLatestAsset() const
{
    if (m_asset.url.isValid())
        QDesktopServices::openUrl(m_asset.url);
    else
        openReleasePage();
}

void UpdateService::onLatestReleaseFinished()
{
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || reply != m_reply)
        return;

    const QByteArray body = reply->readAll();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QNetworkReply::NetworkError networkError = reply->error();
    reply->deleteLater();
    m_reply = nullptr;
    setChecking(false);

    if (networkError != QNetworkReply::NoError) {
        setStatusText(QStringLiteral("Ошибка проверки обновлений: %1").arg(reply->errorString()));
        emit log(m_statusText);
        return;
    }

    if (httpStatus < 200 || httpStatus >= 300) {
        setStatusText(QStringLiteral("GitHub Releases вернул HTTP %1.").arg(httpStatus));
        emit log(m_statusText);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setStatusText(QStringLiteral("GitHub Releases: неверный JSON ответ."));
        emit log(m_statusText);
        return;
    }

    const QJsonObject root = document.object();
    const QString tagName = root.value("tag_name").toString().trimmed();
    if (tagName.isEmpty()) {
        setStatusText(QStringLiteral("GitHub Releases: у последнего релиза нет tag_name."));
        emit log(m_statusText);
        return;
    }

    m_latestVersion = tagName;
    m_releaseName = root.value("name").toString(tagName);
    m_releaseNotes = root.value("body").toString();
    m_releaseUrl = QUrl(root.value("html_url").toString());
    m_updateAvailable = compareVersions(m_currentVersion, tagName) < 0;

    Asset bestAsset;
    int bestScore = 0;
    const QJsonArray assets = root.value("assets").toArray();
    for (const QJsonValue& value : assets) {
        const QJsonObject assetObject = value.toObject();
        const QString name = assetObject.value("name").toString();
        const QUrl url(assetObject.value("browser_download_url").toString());
        const int score = platformAssetScore(name);
        if (!url.isValid() || score <= bestScore)
            continue;
        bestScore = score;
        bestAsset = {name, url, assetObject.value("size").toInteger()};
    }
    m_asset = bestAsset;

    if (m_updateAvailable) {
        const QString assetSuffix = m_asset.url.isValid()
            ? QStringLiteral(" Доступен пакет: %1 (%2).").arg(m_asset.name, assetSizeText())
            : QStringLiteral(" Подходящий пакет для этой платформы не найден.");
        setStatusText(QStringLiteral("Доступна версия %1. Текущая: %2.%3")
            .arg(m_latestVersion, m_currentVersion, assetSuffix));
    } else {
        setStatusText(QStringLiteral("Установлена актуальная версия %1.").arg(m_currentVersion));
    }

    emit log(m_statusText);
    emit changed();
}

int UpdateService::compareVersions(QString left, QString right)
{
    const QVector<int> leftParts = versionParts(left);
    const QVector<int> rightParts = versionParts(right);
    const int count = std::max(leftParts.size(), rightParts.size());
    for (int i = 0; i < count; ++i) {
        const int leftValue = i < leftParts.size() ? leftParts.at(i) : 0;
        const int rightValue = i < rightParts.size() ? rightParts.at(i) : 0;
        if (leftValue < rightValue)
            return -1;
        if (leftValue > rightValue)
            return 1;
    }
    return 0;
}

QString UpdateService::formatSize(qint64 bytes)
{
    if (bytes <= 0)
        return QString();
    const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    if (mb >= 1.0)
        return QStringLiteral("%1 MB").arg(QString::number(mb, 'f', mb >= 10.0 ? 1 : 2));
    const double kb = static_cast<double>(bytes) / 1024.0;
    return QStringLiteral("%1 KB").arg(QString::number(kb, 'f', 1));
}

int UpdateService::platformAssetScore(const QString& name)
{
    const QString lower = name.toLower();
    int score = 0;

#if defined(Q_OS_WIN)
    if (!containsAny(lower, {"win", "windows"}))
        return 0;
    score += 100;
    if (containsAny(lower, {"x64", "x86_64", "amd64"}))
        score += 20;
    if (lower.endsWith(".zip"))
        score += 8;
    if (lower.endsWith(".exe") || lower.endsWith(".msi"))
        score += 4;
#elif defined(Q_OS_MACOS)
    if (!containsAny(lower, {"mac", "macos", "darwin", "osx"}))
        return 0;
    score += 100;
    const QString arch = QSysInfo::currentCpuArchitecture().toLower();
    if (lower.contains("universal"))
        score += 18;
    if ((arch.contains("arm") || arch.contains("aarch64")) && containsAny(lower, {"arm64", "aarch64", "apple-silicon"}))
        score += 20;
    if ((arch.contains("x86_64") || arch.contains("amd64")) && containsAny(lower, {"x64", "x86_64", "intel"}))
        score += 20;
    if (lower.endsWith(".dmg"))
        score += 10;
    if (lower.endsWith(".zip"))
        score += 6;
#else
    if (!containsAny(lower, {"linux", "appimage"}))
        return 0;
    score += 100;
#endif

    return score;
}

void UpdateService::resetReleaseState()
{
    m_latestVersion.clear();
    m_updateAvailable = false;
    m_releaseName.clear();
    m_releaseNotes.clear();
    m_releaseUrl = QUrl();
    m_asset = {};
    emit changed();
}

void UpdateService::setChecking(bool checking)
{
    if (m_checking == checking)
        return;
    m_checking = checking;
    emit changed();
}

void UpdateService::setStatusText(const QString& statusText)
{
    if (m_statusText == statusText)
        return;
    m_statusText = statusText;
    emit changed();
}
