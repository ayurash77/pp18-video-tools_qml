#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include "VideoFileModel.h"
#include "services/FfmpegBatchService.h"
#include "services/UpdateService.h"

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QUrl>
#include <QVariantMap>
#include <QVariantList>
#include <QVector>

class FfmpegPreviewService;
class QProcess;
class TelegramController;

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(VideoFileModel* files READ files CONSTANT)
    Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int progressValue READ progressValue NOTIFY progressValueChanged)
    Q_PROPERTY(bool removeDupes READ removeDupes WRITE setRemoveDupes NOTIFY removeDupesChanged)
    Q_PROPERTY(bool convertTo25Fps READ convertTo25Fps WRITE setConvertTo25Fps NOTIFY convertTo25FpsChanged)
    Q_PROPERTY(int duplicateModeIndex READ duplicateModeIndex WRITE setDuplicateModeIndex NOTIFY duplicateModeIndexChanged)
    Q_PROPERTY(int existingModeIndex READ existingModeIndex WRITE setExistingModeIndex NOTIFY existingModeIndexChanged)
    Q_PROPERTY(bool latestVersionsOnly READ latestVersionsOnly WRITE setLatestVersionsOnly NOTIFY latestVersionsOnlyChanged)
    Q_PROPERTY(bool allSrcPreviewEnabled READ allSrcPreviewEnabled NOTIFY actionSummaryChanged)
    Q_PROPERTY(bool allSrcTelegramEnabled READ allSrcTelegramEnabled NOTIFY actionSummaryChanged)
    Q_PROPERTY(bool allFixedFixesEnabled READ allFixedFixesEnabled NOTIFY actionSummaryChanged)
    Q_PROPERTY(bool allFixedPreviewEnabled READ allFixedPreviewEnabled NOTIFY actionSummaryChanged)
    Q_PROPERTY(bool allFixedTelegramEnabled READ allFixedTelegramEnabled NOTIFY actionSummaryChanged)
    Q_PROPERTY(bool settingsWindowOpen READ settingsWindowOpen WRITE setSettingsWindowOpen NOTIFY settingsWindowOpenChanged)
    Q_PROPERTY(bool logWindowOpen READ logWindowOpen WRITE setLogWindowOpen NOTIFY logWindowOpenChanged)
    Q_PROPERTY(bool playerWindowOpen READ playerWindowOpen WRITE setPlayerWindowOpen NOTIFY playerWindowOpenChanged)
    Q_PROPERTY(QString playerPath READ playerPath NOTIFY playerChanged)
    Q_PROPERTY(QString playerTitle READ playerTitle NOTIFY playerChanged)
    Q_PROPERTY(QUrl playerSource READ playerSource NOTIFY playerChanged)
    Q_PROPERTY(bool mediaCacheEnabled READ mediaCacheEnabled WRITE setMediaCacheEnabled NOTIFY mediaCacheChanged)
    Q_PROPERTY(bool mediaCacheRunning READ mediaCacheRunning NOTIFY mediaCacheChanged)
    Q_PROPERTY(bool mediaCacheComplete READ mediaCacheComplete NOTIFY mediaCacheChanged)
    Q_PROPERTY(QString mediaCacheRootPath READ mediaCacheRootPath WRITE setMediaCacheRootPath NOTIFY mediaCacheSettingsChanged)
    Q_PROPERTY(QString mediaCacheSizeText READ mediaCacheSizeText NOTIFY mediaCacheSettingsChanged)
    Q_PROPERTY(double mediaCacheMaxSizeGb READ mediaCacheMaxSizeGb WRITE setMediaCacheMaxSizeGb NOTIFY mediaCacheSettingsChanged)
    Q_PROPERTY(UpdateService* updates READ updates CONSTANT)
    Q_PROPERTY(QString telegramBotToken READ telegramBotToken NOTIFY telegramSettingsChanged)
    Q_PROPERTY(QString telegramChatId READ telegramChatId NOTIFY telegramSettingsChanged)
    Q_PROPERTY(QVariantList telegramRecipients READ telegramRecipients NOTIFY telegramSettingsChanged)
    Q_PROPERTY(QString activeTelegramRecipientId READ activeTelegramRecipientId NOTIFY telegramSettingsChanged)

public:
    explicit AppController(QObject* parent = nullptr);

    VideoFileModel* files();
    QString logText() const;
    QString statusText() const;
    bool running() const;
    int progressValue() const;
    bool removeDupes() const;
    void setRemoveDupes(bool value);
    bool convertTo25Fps() const;
    void setConvertTo25Fps(bool value);
    int duplicateModeIndex() const;
    void setDuplicateModeIndex(int value);
    int existingModeIndex() const;
    void setExistingModeIndex(int value);
    bool latestVersionsOnly() const;
    void setLatestVersionsOnly(bool value);
    bool allSrcPreviewEnabled() const;
    bool allSrcTelegramEnabled() const;
    bool allFixedFixesEnabled() const;
    bool allFixedPreviewEnabled() const;
    bool allFixedTelegramEnabled() const;
    bool settingsWindowOpen() const;
    void setSettingsWindowOpen(bool value);
    bool logWindowOpen() const;
    void setLogWindowOpen(bool value);
    bool playerWindowOpen() const;
    void setPlayerWindowOpen(bool value);
    QString playerPath() const;
    QString playerTitle() const;
    QUrl playerSource() const;
    bool mediaCacheEnabled() const;
    void setMediaCacheEnabled(bool enabled);
    bool mediaCacheRunning() const;
    bool mediaCacheComplete() const;
    QString mediaCacheRootPath() const;
    void setMediaCacheRootPath(const QString& path);
    QString mediaCacheSizeText() const;
    double mediaCacheMaxSizeGb() const;
    void setMediaCacheMaxSizeGb(double value);
    UpdateService* updates();
    QString telegramBotToken() const;
    QString telegramChatId() const;
    QVariantList telegramRecipients() const;
    QString activeTelegramRecipientId() const;

    Q_INVOKABLE void chooseFiles();
    Q_INVOKABLE void chooseFolders();
    Q_INVOKABLE void addDroppedUrls(const QVariantList& urls);
    Q_INVOKABLE void startOrStop();
    Q_INVOKABLE void startTask(int row);
    Q_INVOKABLE void startTaskSide(int row, const QString& side);
    Q_INVOKABLE void refreshList();
    Q_INVOKABLE void clearFiles();
    Q_INVOKABLE void removeFile(int row);
    Q_INVOKABLE void setFileAction(int row, const QString& action, bool enabled);
    Q_INVOKABLE void toggleAllAction(const QString& action);
    Q_INVOKABLE void revealInFolder(const QString& path);
    Q_INVOKABLE void openFile(const QString& path);
    Q_INVOKABLE void openSettingsWindow();
    Q_INVOKABLE void openLogWindow();
    Q_INVOKABLE void openPlayer(const QString& path, const QString& title = QString());
    Q_INVOKABLE QString localFileUrl(const QString& path) const;
    Q_INVOKABLE QString cachedPlaybackPath(const QString& path) const;
    Q_INVOKABLE bool cachePreviewExists(const QString& path) const;
    Q_INVOKABLE void requestCachePreview(const QString& path);
    Q_INVOKABLE void chooseMediaCacheFolder();
    Q_INVOKABLE void clearMediaCache();
    Q_INVOKABLE void refreshMediaCacheStats();
    Q_INVOKABLE void saveTelegramSettings(const QString& botToken, const QVariantList& recipients, const QString& activeRecipientId);
    Q_INVOKABLE void importTelegramRecipients(const QString& botToken);
    Q_INVOKABLE void setActiveTelegramRecipient(const QString& recipientId);
    Q_INVOKABLE QVariantList versionedSiblingFiles(const QString& path);

signals:
    void logTextChanged();
    void statusTextChanged();
    void runningChanged();
    void progressValueChanged();
    void removeDupesChanged();
    void convertTo25FpsChanged();
    void duplicateModeIndexChanged();
    void existingModeIndexChanged();
    void latestVersionsOnlyChanged();
    void actionSummaryChanged();
    void settingsWindowOpenChanged();
    void logWindowOpenChanged();
    void playerWindowOpenChanged();
    void playerChanged();
    void mediaCacheChanged();
    void mediaCacheSettingsChanged();
    void telegramSettingsChanged();
    void telegramRecipientsImported(const QVariantList& recipients, const QString& message);
    void telegramRecipientsImportFailed(const QString& message);

private slots:
    void onFixBatchStarted(int total);
    void onFixFileStarted(int index, int total, const QString& input, const QString& output);
    void onFixFileFinished(int index, int total, bool ok, const QString& input, const QString& output, const QString& details);
    void onFixProgress(int completed, int total);
    void onFixFinished(bool ok);
    void onPreviewStarted(int total);
    void onPreviewFileStarted(int index, int total, const QString& input, const QString& output);
    void onPreviewFileFinished(int index, int total, bool ok, const QString& input, const QString& output, const QString& details);
    void onPreviewProgress(int completed, int total);
    void onPreviewFinished(bool ok);
    void onTelegramFileSent(const QString& filePath, bool ok, const QString& details);
    void onMetadataFinished(int exitCode, QProcess::ExitStatus status);
    void onThumbnailFinished(int exitCode, QProcess::ExitStatus status);
    void onCachePreviewFinished(int exitCode, QProcess::ExitStatus status);

private:
    struct ThumbnailJob {
        QString input;
        int row = -1;
        bool fixed = false;
    };

    struct CachePreviewJob {
        QString input;
        QString output;
    };

    enum class TaskSideScope {
        All,
        Source,
        Fixed
    };

    void appendLog(const QString& line);
    void setStatusText(const QString& text);
    void setRunning(bool running);
    void setProgressValue(int value);
    void addSourcePaths(const QStringList& paths, const QString& logPrefix);
    QStringList expandVideoPaths(const QStringList& paths) const;
    QStringList filteredDisplayFiles(const QStringList& files) const;
    void reloadDisplayedFiles();
    bool allActionEnabled(const QString& action) const;
    ConvertOptions optionsFromState() const;
    bool executePreviewAndTelegram(const QVector<int>& rows = {}, bool showNoActionsMessage = true, TaskSideScope sideScope = TaskSideScope::All);
    QString makeCaptionHtml(const QString& input) const;
    QString macFullPath(const QString& filePath) const;
    QString windowsWorkPath(const QString& macPath) const;
    QString findMediaTool(const QString& baseName) const;
    void queueMetadata(const QStringList& files);
    void startNextMetadata();
    void stopMetadataLoading();
    void queueSourceThumbnails(const QStringList& files);
    void queueFixedThumbnails();
    void queueFixedThumbnail(int row);
    void startNextThumbnail();
    void stopThumbnailGeneration();
    QString appCacheRoot() const;
    QString defaultAppCacheRoot() const;
    QString cachePreviewPath(const QString& path) const;
    QString cacheKeyForPath(const QString& path) const;
    void configureMediaCacheDirs();
    void queueCachePreviews(const QStringList& files);
    void startNextCachePreview();
    void stopCachePreviewGeneration();
    void updateMediaCacheComplete();
    qint64 mediaCacheSizeBytes() const;
    QString formatByteSize(qint64 bytes) const;
    void enforceMediaCacheLimit();
    void loadTelegramRecipients();
    void persistTelegramRecipients();
    QString activeTelegramChatId() const;
    QVariantList normalizedTelegramRecipients(const QVariantList& recipients) const;

private:
    VideoFileModel m_files;
    FfmpegBatchService* m_fixes = nullptr;
    FfmpegPreviewService* m_preview = nullptr;
    TelegramController* m_telegram = nullptr;
    UpdateService* m_updates = nullptr;
    QString m_logText;
    QString m_statusText;
    bool m_running = false;
    int m_progressValue = 0;
    bool m_removeDupes = true;
    bool m_convertTo25Fps = true;
    int m_duplicateModeIndex = 2;
    int m_existingModeIndex = 0;
    bool m_latestVersionsOnly = true;
    bool m_runPreviewAfterFixes = false;
    QVector<int> m_postFixRows;
    TaskSideScope m_postFixSideScope = TaskSideScope::All;
    QStringList m_allSourcePaths;
    QVector<int> m_fixRows;
    QVector<int> m_previewRows;
    QHash<QString, int> m_previewInputRows;
    QHash<QString, QString> m_previewInputOriginals;
    QHash<QString, int> m_telegramRows;
    QSet<QString> m_sendAfterPreview;
    QStringList m_metadataQueue;
    QProcess* m_metadataProcess = nullptr;
    QVector<ThumbnailJob> m_thumbnailQueue;
    QString m_thumbnailDir;
    QProcess* m_thumbnailProcess = nullptr;
    QString m_mediaCacheRootPath;
    QString m_cachePreviewDir;
    QVector<CachePreviewJob> m_cachePreviewQueue;
    QProcess* m_cachePreviewProcess = nullptr;
    bool m_mediaCacheEnabled = true;
    bool m_mediaCacheComplete = false;
    double m_mediaCacheMaxSizeGb = 20.0;
    bool m_settingsWindowOpen = false;
    bool m_logWindowOpen = false;
    bool m_playerWindowOpen = false;
    QString m_playerPath;
    QString m_playerTitle;
    QVariantList m_telegramRecipients;
    QString m_activeTelegramRecipientId;
};

#endif // APPCONTROLLER_H
