#include "AppController.h"

#include "platform/NativeFileDialog.h"
#include "platform/NativeFolderDialog.h"
#include "services/FfmpegPreviewService.h"
#include "services/TelegramController.h"
#include "services/TelegramService.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTextDocument>
#include <QUrl>

#include <algorithm>
#include <cmath>
#include <optional>

namespace {
constexpr const char* MacWorkPrefix = "/Volumes/work";
constexpr double DefaultMediaCacheMaxSizeGb = 20.0;
constexpr qint64 BytesInGb = 1024LL * 1024LL * 1024LL;

double parseFrameRate(const QString& value)
{
    const QString trimmed = value.trimmed();
    const QStringList parts = trimmed.split('/');
    if (parts.size() == 2) {
        const double numerator = parts.at(0).toDouble();
        const double denominator = parts.at(1).toDouble();
        return denominator > 0.0 ? numerator / denominator : 0.0;
    }
    return trimmed.toDouble();
}

struct VideoMetadata {
    QString resolution = "?";
    QString frames = "?F";
    QString fps = "?";
};

VideoMetadata metadataFromFfprobeOutput(const QString& output)
{
    QMap<QString, QString> values;
    for (const QString& line : output.split('\n', Qt::SkipEmptyParts)) {
        const int sep = line.indexOf('=');
        if (sep <= 0)
            continue;
        values.insert(line.left(sep).trimmed(), line.mid(sep + 1).trimmed());
    }

    VideoMetadata metadata;
    const int width = values.value("width").toInt();
    const int height = values.value("height").toInt();
    if (width > 0 && height > 0)
        metadata.resolution = QString("%1x%2").arg(width).arg(height);

    double fps = parseFrameRate(values.value("avg_frame_rate"));
    if (fps <= 0.0)
        fps = parseFrameRate(values.value("r_frame_rate"));
    if (fps > 0.0)
        metadata.fps = QString::number(fps, 'f', fps >= 100.0 ? 0 : 2);

    qint64 frames = values.value("nb_frames").toLongLong();
    if (frames <= 0) {
        const double duration = values.value("duration").toDouble();
        if (duration > 0.0 && fps > 0.0)
            frames = static_cast<qint64>(std::llround(duration * fps));
    }
    if (frames > 0)
        metadata.frames = QString("%1F").arg(frames);

    return metadata;
}

struct VersionedFileInfo {
    QString baseStem;
    QString extension;
    int version = -1;
    QString label;
};

std::optional<VersionedFileInfo> parseVersionedFileName(const QString& fileName)
{
    const QFileInfo fileInfo(fileName);
    const QRegularExpression versionRegex("^(.*)_v(\\d+)(?:__.*)?$", QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = versionRegex.match(fileInfo.completeBaseName());
    if (!match.hasMatch())
        return std::nullopt;

    VersionedFileInfo info;
    info.baseStem = match.captured(1);
    info.extension = fileInfo.suffix().toLower();
    info.version = match.captured(2).toInt();
    info.label = "v" + match.captured(2);
    return info;
}

bool isPreviewFile(const QFileInfo& fileInfo)
{
    return fileInfo.completeBaseName().endsWith("__preview", Qt::CaseInsensitive);
}

QString sourcePathForPreview(const QFileInfo& fileInfo)
{
    QString stem = fileInfo.completeBaseName();
    if (!stem.endsWith("__preview", Qt::CaseInsensitive))
        return fileInfo.absoluteFilePath();

    stem.chop(QStringLiteral("__preview").size());
    const QString movPath = fileInfo.dir().absoluteFilePath(stem + ".mov");
    if (QFileInfo::exists(movPath))
        return movPath;

    static const QStringList sourceExtensions = {
        "mov", "mp4", "m4v", "mkv", "avi", "mxf", "webm"
    };
    for (const QString& extension : sourceExtensions) {
        const QString candidate = fileInfo.dir().absoluteFilePath(stem + "." + extension);
        if (QFileInfo::exists(candidate))
            return candidate;
    }

    return movPath;
}

bool removeDirectoryContents(const QString& path)
{
    QDir dir(path);
    if (!dir.exists())
        return true;

    bool ok = true;
    const QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System);
    for (const QFileInfo& entry : entries) {
        if (entry.isDir()) {
            ok = QDir(entry.absoluteFilePath()).removeRecursively() && ok;
        } else {
            ok = QFile::remove(entry.absoluteFilePath()) && ok;
        }
    }
    return ok;
}

}

AppController::AppController(QObject* parent)
    : QObject(parent)
    , m_fixes(new FfmpegBatchService(this))
    , m_preview(new FfmpegPreviewService(this))
    , m_telegram(new TelegramController(this))
{
    QSettings settings;
    m_mediaCacheRootPath = settings.value("cache/root", defaultAppCacheRoot()).toString();
    m_mediaCacheMaxSizeGb = settings.value("cache/max_gb", DefaultMediaCacheMaxSizeGb).toDouble();
    m_mediaCacheEnabled = settings.value("cache/enabled", true).toBool();
    configureMediaCacheDirs();

    setStatusText("Готово");

    connect(m_fixes, &FfmpegBatchService::log, this, &AppController::appendLog);
    connect(m_fixes, &FfmpegBatchService::batchStarted, this, &AppController::onFixBatchStarted);
    connect(m_fixes, &FfmpegBatchService::fileStarted, this, &AppController::onFixFileStarted);
    connect(m_fixes, &FfmpegBatchService::fileFinished, this, &AppController::onFixFileFinished);
    connect(m_fixes, &FfmpegBatchService::progress, this, &AppController::onFixProgress);
    connect(m_fixes, &FfmpegBatchService::finished, this, &AppController::onFixFinished);

    connect(m_preview, &FfmpegPreviewService::log, this, &AppController::appendLog);
    connect(m_preview, &FfmpegPreviewService::started, this, &AppController::onPreviewStarted);
    connect(m_preview, &FfmpegPreviewService::fileStarted, this, &AppController::onPreviewFileStarted);
    connect(m_preview, &FfmpegPreviewService::fileFinished, this, &AppController::onPreviewFileFinished);
    connect(m_preview, &FfmpegPreviewService::progress, this, &AppController::onPreviewProgress);
    connect(m_preview, &FfmpegPreviewService::finished, this, &AppController::onPreviewFinished);

    connect(m_telegram, &TelegramController::log, this, &AppController::appendLog);
    connect(m_telegram->service(), &TelegramService::fileSent, this, &AppController::onTelegramFileSent);
    connect(m_telegram->service(), &TelegramService::privateRecipientsImported, this,
        [this](const QVariantList& recipients, bool ok, const QString& details) {
            if (ok)
                emit telegramRecipientsImported(recipients, details);
            else
                emit telegramRecipientsImportFailed(details);
        });
    connect(&m_files, &VideoFileModel::dataChanged, this, &AppController::actionSummaryChanged);
    connect(&m_files, &VideoFileModel::rowsInserted, this, &AppController::actionSummaryChanged);
    connect(&m_files, &VideoFileModel::rowsRemoved, this, &AppController::actionSummaryChanged);
    connect(&m_files, &VideoFileModel::modelReset, this, &AppController::actionSummaryChanged);
    loadTelegramRecipients();
}

VideoFileModel* AppController::files()
{
    return &m_files;
}

QString AppController::logText() const
{
    return m_logText;
}

QString AppController::statusText() const
{
    return m_statusText;
}

bool AppController::running() const
{
    return m_running;
}

int AppController::progressValue() const
{
    return m_progressValue;
}

bool AppController::removeDupes() const
{
    return m_removeDupes;
}

void AppController::setRemoveDupes(bool value)
{
    if (m_removeDupes == value)
        return;
    m_removeDupes = value;
    emit removeDupesChanged();
}

bool AppController::convertTo25Fps() const
{
    return m_convertTo25Fps;
}

void AppController::setConvertTo25Fps(bool value)
{
    if (m_convertTo25Fps == value)
        return;
    m_convertTo25Fps = value;
    emit convertTo25FpsChanged();
}

int AppController::duplicateModeIndex() const
{
    return m_duplicateModeIndex;
}

void AppController::setDuplicateModeIndex(int value)
{
    const int bounded = qBound(0, value, 4);
    if (m_duplicateModeIndex == bounded)
        return;
    m_duplicateModeIndex = bounded;
    emit duplicateModeIndexChanged();
}

int AppController::existingModeIndex() const
{
    return m_existingModeIndex;
}

void AppController::setExistingModeIndex(int value)
{
    const int bounded = qBound(0, value, 1);
    if (m_existingModeIndex == bounded)
        return;
    m_existingModeIndex = bounded;
    emit existingModeIndexChanged();
}

bool AppController::latestVersionsOnly() const
{
    return m_latestVersionsOnly;
}

void AppController::setLatestVersionsOnly(bool value)
{
    if (m_latestVersionsOnly == value)
        return;

    m_latestVersionsOnly = value;
    emit latestVersionsOnlyChanged();
    reloadDisplayedFiles();
}

bool AppController::allSrcPreviewEnabled() const
{
    return allActionEnabled("srcPreview");
}

bool AppController::allSrcTelegramEnabled() const
{
    return allActionEnabled("srcTelegram");
}

bool AppController::allFixedFixesEnabled() const
{
    return allActionEnabled("fixes");
}

bool AppController::allFixedPreviewEnabled() const
{
    return allActionEnabled("fixedPreview");
}

bool AppController::allFixedTelegramEnabled() const
{
    return allActionEnabled("fixedTelegram");
}

bool AppController::settingsWindowOpen() const
{
    return m_settingsWindowOpen;
}

void AppController::setSettingsWindowOpen(bool value)
{
    if (m_settingsWindowOpen == value)
        return;
    m_settingsWindowOpen = value;
    emit settingsWindowOpenChanged();
}

bool AppController::logWindowOpen() const
{
    return m_logWindowOpen;
}

void AppController::setLogWindowOpen(bool value)
{
    if (m_logWindowOpen == value)
        return;
    m_logWindowOpen = value;
    emit logWindowOpenChanged();
}

bool AppController::playerWindowOpen() const
{
    return m_playerWindowOpen;
}

void AppController::setPlayerWindowOpen(bool value)
{
    if (m_playerWindowOpen == value)
        return;
    m_playerWindowOpen = value;
    emit playerWindowOpenChanged();
}

QString AppController::playerPath() const
{
    return m_playerPath;
}

QString AppController::playerTitle() const
{
    return m_playerTitle;
}

QUrl AppController::playerSource() const
{
    return m_playerPath.isEmpty() ? QUrl() : QUrl::fromLocalFile(m_playerPath);
}

bool AppController::mediaCacheEnabled() const
{
    return m_mediaCacheEnabled;
}

void AppController::setMediaCacheEnabled(bool enabled)
{
    if (m_mediaCacheEnabled == enabled)
        return;

    m_mediaCacheEnabled = enabled;
    QSettings().setValue("cache/enabled", m_mediaCacheEnabled);
    if (m_mediaCacheEnabled) {
        queueCachePreviews(filteredDisplayFiles(m_allSourcePaths));
    } else {
        stopCachePreviewGeneration();
        updateMediaCacheComplete();
    }
    emit mediaCacheChanged();
}

bool AppController::mediaCacheRunning() const
{
    return m_cachePreviewProcess != nullptr || !m_cachePreviewQueue.isEmpty();
}

bool AppController::mediaCacheComplete() const
{
    return m_mediaCacheComplete;
}

QString AppController::mediaCacheRootPath() const
{
    return appCacheRoot();
}

void AppController::setMediaCacheRootPath(const QString& path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty())
        return;

    const QString normalized = QFileInfo(trimmed).absoluteFilePath();
    if (QDir::cleanPath(m_mediaCacheRootPath) == QDir::cleanPath(normalized))
        return;

    stopThumbnailGeneration();
    stopCachePreviewGeneration();
    m_mediaCacheRootPath = normalized;
    configureMediaCacheDirs();
    QSettings().setValue("cache/root", m_mediaCacheRootPath);
    appendLog("Папка кеша: " + QDir::toNativeSeparators(m_mediaCacheRootPath));
    emit mediaCacheSettingsChanged();
    emit mediaCacheChanged();

    const QStringList displayFiles = filteredDisplayFiles(m_allSourcePaths);
    if (!displayFiles.isEmpty()) {
        queueSourceThumbnails(displayFiles);
        queueFixedThumbnails();
        if (m_mediaCacheEnabled)
            queueCachePreviews(displayFiles);
    }
    updateMediaCacheComplete();
}

QString AppController::mediaCacheSizeText() const
{
    return formatByteSize(mediaCacheSizeBytes());
}

double AppController::mediaCacheMaxSizeGb() const
{
    return m_mediaCacheMaxSizeGb;
}

void AppController::setMediaCacheMaxSizeGb(double value)
{
    const double normalized = std::isfinite(value) ? std::max(0.0, value) : DefaultMediaCacheMaxSizeGb;
    if (qFuzzyCompare(m_mediaCacheMaxSizeGb + 1.0, normalized + 1.0))
        return;

    m_mediaCacheMaxSizeGb = normalized;
    QSettings().setValue("cache/max_gb", m_mediaCacheMaxSizeGb);
    enforceMediaCacheLimit();
    emit mediaCacheSettingsChanged();
}

QString AppController::telegramBotToken() const
{
    return m_telegram->botToken();
}

QString AppController::telegramChatId() const
{
    return m_telegram->chatId();
}

QVariantList AppController::telegramRecipients() const
{
    return m_telegramRecipients;
}

QString AppController::activeTelegramRecipientId() const
{
    return m_activeTelegramRecipientId;
}

void AppController::chooseFiles()
{
    QSettings settings;
    const QString startDir = settings.value("qml/lastFileDialogDir", QDir::homePath()).toString();

    setStatusText("Открываю диалог выбора видеофайлов...");
    appendLog("Открываю диалог выбора видеофайлов...");

    openNativeFileDialog("Выбор видеофайлов", startDir,
        [this](const QStringList& selectedFiles, const QString& error) {
            if (!error.isEmpty()) {
                setStatusText("Ошибка выбора файлов");
                appendLog(error);
                return;
            }

            if (selectedFiles.isEmpty()) {
                setStatusText("Выбор файлов отменен");
                appendLog("Выбор файлов отменен.");
                return;
            }

            QSettings settings;
            settings.setValue("qml/lastFileDialogDir", QFileInfo(selectedFiles.first()).absolutePath());

            addSourcePaths(selectedFiles, "Добавлено из выбора файлов");
        });
}

void AppController::chooseFolders()
{
    QSettings settings;
    const QString startDir = settings.value("qml/lastFolderDialogDir",
        settings.value("qml/lastFileDialogDir", QDir::homePath())).toString();

    setStatusText("Открываю диалог выбора папок...");
    appendLog("Открываю диалог выбора папок...");

    openNativeFolderDialog("Выбор папок с видеофайлами", startDir,
        [this](const QStringList& selectedFolders, const QString& error) {
            if (!error.isEmpty()) {
                setStatusText("Ошибка выбора папок");
                appendLog(error);
                return;
            }

            if (selectedFolders.isEmpty()) {
                setStatusText("Выбор папок отменен");
                appendLog("Выбор папок отменен.");
                return;
            }

            QSettings settings;
            settings.setValue("qml/lastFolderDialogDir", selectedFolders.first());

            addSourcePaths(selectedFolders, "Добавлено из выбора папок");
        });
}

void AppController::addDroppedUrls(const QVariantList& urls)
{
    QStringList paths;
    paths.reserve(urls.size());

    for (const QVariant& value : urls) {
        const QUrl url(value.toString());
        const QString path = url.isLocalFile() ? url.toLocalFile() : value.toString();
        if (!path.isEmpty())
            paths << path;
    }

    addSourcePaths(paths, "Добавлено через drag & drop");
}

void AppController::startOrStop()
{
    if (m_fixes->isRunning()) {
        m_runPreviewAfterFixes = false;
        m_postFixRows.clear();
        m_postFixSideScope = TaskSideScope::All;
        m_fixes->stop();
        return;
    }

    if (m_preview->isRunning()) {
        m_preview->stop();
        return;
    }

    if (m_files.count() == 0) {
        appendLog("Запуск отменен: файлы не выбраны.");
        setStatusText("Файлы не выбраны");
        return;
    }

    QVector<int> rows;
    const QStringList fixFiles = m_files.filesWithAction("fixes", &rows);
    const QStringList previewFiles = m_files.filesWithAction("preview");
    const QStringList telegramFiles = m_files.filesWithAction("telegram");

    if (fixFiles.isEmpty() && previewFiles.isEmpty() && telegramFiles.isEmpty()) {
        appendLog("Запуск отменен: действия в списке не выбраны.");
        setStatusText("Нет выбранных действий");
        return;
    }

    m_files.clearRuntimeStatuses();
    m_files.refreshExistence();
    setProgressValue(0);

    if (fixFiles.isEmpty()) {
        executePreviewAndTelegram();
        return;
    }

    ConvertOptions options = optionsFromState();
    options.explicitFiles = fixFiles;
    options.useExplicitFiles = true;
    options.folder = QFileInfo(fixFiles.first()).absolutePath();

    m_fixRows = rows;
    m_postFixRows.clear();
    m_postFixSideScope = TaskSideScope::All;
    m_runPreviewAfterFixes = !previewFiles.isEmpty() || !telegramFiles.isEmpty();
    setRunning(true);
    appendLog(QString("Запуск Fixes: %1 файл(ов).").arg(fixFiles.size()));
    m_fixes->run(options);
}

void AppController::startTask(int row)
{
    if (m_running || m_fixes->isRunning() || m_preview->isRunning())
        return;

    if (row < 0 || row >= m_files.count()) {
        appendLog(QString("Запуск задачи отменен: строка %1 не найдена.").arg(row));
        setStatusText("Задача не найдена");
        return;
    }

    const bool runFixes = m_files.actionAt(row, "fixes");
    const bool runPreview = m_files.actionAt(row, "srcPreview") || m_files.actionAt(row, "fixedPreview");
    const bool runTelegram = m_files.actionAt(row, "srcTelegram") || m_files.actionAt(row, "fixedTelegram");

    if (!runFixes && !runPreview && !runTelegram) {
        appendLog("Запуск задачи отменен: действия в строке не выбраны.");
        setStatusText("Нет выбранных действий");
        return;
    }

    m_files.setRuntimeStatus(row, QString());
    m_files.refreshExistence();
    setProgressValue(0);

    const QVector<int> rows{row};
    if (!runFixes) {
        executePreviewAndTelegram(rows, true, TaskSideScope::All);
        return;
    }

    const QString file = m_files.fileAt(row);
    ConvertOptions options = optionsFromState();
    options.explicitFiles = {file};
    options.useExplicitFiles = true;
    options.folder = QFileInfo(file).absolutePath();

    m_fixRows = rows;
    m_postFixRows = (runPreview || runTelegram) ? rows : QVector<int>{};
    m_postFixSideScope = TaskSideScope::All;
    m_runPreviewAfterFixes = !m_postFixRows.isEmpty();
    setRunning(true);
    appendLog("Запуск задачи Fixes: " + QDir::toNativeSeparators(file));
    m_fixes->run(options);
}

void AppController::startTaskSide(int row, const QString& side)
{
    if (m_running || m_fixes->isRunning() || m_preview->isRunning())
        return;

    if (row < 0 || row >= m_files.count()) {
        appendLog(QString("Запуск задачи отменен: строка %1 не найдена.").arg(row));
        setStatusText("Задача не найдена");
        return;
    }

    const TaskSideScope sideScope = side.compare("fixed", Qt::CaseInsensitive) == 0 ? TaskSideScope::Fixed : TaskSideScope::Source;
    const bool runFixes = sideScope == TaskSideScope::Fixed && m_files.actionAt(row, "fixes");
    const bool runPreview = sideScope == TaskSideScope::Fixed ? m_files.actionAt(row, "fixedPreview") : m_files.actionAt(row, "srcPreview");
    const bool runTelegram = sideScope == TaskSideScope::Fixed ? m_files.actionAt(row, "fixedTelegram") : m_files.actionAt(row, "srcTelegram");

    if (!runFixes && !runPreview && !runTelegram) {
        appendLog("Запуск задачи отменен: действия в выбранной части строки не выбраны.");
        setStatusText("Нет выбранных действий");
        return;
    }

    m_files.setRuntimeStatus(row, QString());
    m_files.refreshExistence();
    setProgressValue(0);

    const QVector<int> rows{row};
    if (!runFixes) {
        executePreviewAndTelegram(rows, true, sideScope);
        return;
    }

    const QString file = m_files.fileAt(row);
    ConvertOptions options = optionsFromState();
    options.explicitFiles = {file};
    options.useExplicitFiles = true;
    options.folder = QFileInfo(file).absolutePath();

    m_fixRows = rows;
    m_postFixRows = (runPreview || runTelegram) ? rows : QVector<int>{};
    m_postFixSideScope = sideScope;
    m_runPreviewAfterFixes = !m_postFixRows.isEmpty();
    setRunning(true);
    appendLog("Запуск задачи Fixes: " + QDir::toNativeSeparators(file));
    m_fixes->run(options);
}

void AppController::refreshList()
{
    reloadDisplayedFiles();
    appendLog(QString("Список обновлен: %1 файл(ов).").arg(m_files.count()));
}

void AppController::clearFiles()
{
    stopMetadataLoading();
    stopThumbnailGeneration();
    stopCachePreviewGeneration();
    m_allSourcePaths.clear();
    m_files.clear();
    updateMediaCacheComplete();
    setStatusText("Список очищен");
    appendLog("Список файлов очищен.");
}

void AppController::removeFile(int row)
{
    const QString file = m_files.fileAt(row);
    m_allSourcePaths.removeAll(file);
    m_files.removeAt(row);
    updateMediaCacheComplete();
    setStatusText(QString("Файлов в списке: %1").arg(m_files.count()));
}

void AppController::setFileAction(int row, const QString& action, bool enabled)
{
    m_files.setAction(row, action, enabled);
    emit actionSummaryChanged();
}

void AppController::toggleAllAction(const QString& action)
{
    if (m_files.count() == 0 || m_running)
        return;

    const bool next = !allActionEnabled(action);
    for (int row = 0; row < m_files.count(); ++row) {
        if (action == "fixedPreview" && next && !m_files.actionAt(row, "fixes"))
            m_files.setAction(row, "fixes", true);
        if (action == "fixedPreview" && !next && !QFileInfo::exists(m_files.fixedPreviewPathAt(row)))
            m_files.setAction(row, "fixedTelegram", false);
        if (action == "fixedTelegram" && next) {
            if (!m_files.actionAt(row, "fixes"))
                m_files.setAction(row, "fixes", true);
            if (!m_files.actionAt(row, "fixedPreview"))
                m_files.setAction(row, "fixedPreview", true);
        }
        if (action == "srcTelegram" && next && !m_files.actionAt(row, "srcPreview"))
            m_files.setAction(row, "srcPreview", true);
        if (action == "srcPreview" && !next && !QFileInfo::exists(m_files.sourcePreviewPathAt(row)))
            m_files.setAction(row, "srcTelegram", false);
        if (action == "fixes" && !next && !QFileInfo::exists(m_files.fixedPathAt(row))) {
            m_files.setAction(row, "fixedPreview", false);
            m_files.setAction(row, "fixedTelegram", false);
        }

        m_files.setAction(row, action, next);
    }
    emit actionSummaryChanged();
}

void AppController::revealInFolder(const QString& path)
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        appendLog("Файл не найден: " + QDir::toNativeSeparators(path));
        return;
    }

#ifdef Q_OS_MAC
    QProcess::startDetached("/usr/bin/open", {"-R", fileInfo.absoluteFilePath()});
#elif defined(Q_OS_WIN)
    QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(fileInfo.absoluteFilePath())});
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
#endif
}

void AppController::openFile(const QString& path)
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        appendLog("Файл не найден: " + QDir::toNativeSeparators(path));
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
}

void AppController::openSettingsWindow()
{
    setSettingsWindowOpen(true);
}

void AppController::openLogWindow()
{
    setLogWindowOpen(true);
}

void AppController::openPlayer(const QString& path, const QString& title)
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        appendLog("Файл не найден: " + QDir::toNativeSeparators(path));
        setStatusText("Файл не найден");
        return;
    }

    const QString sourcePath = sourcePathForPreview(fileInfo);
    const QFileInfo sourceInfo(sourcePath);
    m_playerPath = sourceInfo.exists() ? sourceInfo.absoluteFilePath() : fileInfo.absoluteFilePath();
    if (m_mediaCacheEnabled)
        queueCachePreviews({m_playerPath});
    m_playerTitle = title.isEmpty() ? QFileInfo(m_playerPath).fileName() : title;
    emit playerChanged();
    setPlayerWindowOpen(true);
}

QString AppController::cachedPlaybackPath(const QString& path) const
{
    const QString sourcePath = sourcePathForPreview(QFileInfo(path));
    const QString previewPath = cachePreviewPath(sourcePath);
    return QFileInfo::exists(previewPath) ? previewPath : QFileInfo(sourcePath).absoluteFilePath();
}

bool AppController::cachePreviewExists(const QString& path) const
{
    return QFileInfo::exists(cachePreviewPath(sourcePathForPreview(QFileInfo(path))));
}

void AppController::requestCachePreview(const QString& path)
{
    if (path.trimmed().isEmpty())
        return;
    queueCachePreviews({path});
}

void AppController::chooseMediaCacheFolder()
{
    openNativeFolderDialog("Выбери папку кеша PP18 VideoTools", appCacheRoot(),
        [this](const QStringList& folders, const QString& error) {
            if (!error.isEmpty()) {
                appendLog("Выбор папки кеша отменен: " + error);
                return;
            }
            if (folders.isEmpty()) {
                appendLog("Выбор папки кеша отменен.");
                return;
            }
            setMediaCacheRootPath(folders.first());
        });
}

void AppController::clearMediaCache()
{
    stopThumbnailGeneration();
    stopCachePreviewGeneration();

    const bool thumbsOk = removeDirectoryContents(m_thumbnailDir);
    const bool previewsOk = removeDirectoryContents(m_cachePreviewDir);
    configureMediaCacheDirs();
    appendLog(thumbsOk && previewsOk ? "Кеш очищен." : "Кеш очищен частично: часть файлов удалить не удалось.");
    emit mediaCacheSettingsChanged();
    updateMediaCacheComplete();

    const QStringList displayFiles = filteredDisplayFiles(m_allSourcePaths);
    if (displayFiles.isEmpty())
        return;

    queueSourceThumbnails(displayFiles);
    queueFixedThumbnails();
    if (m_mediaCacheEnabled)
        queueCachePreviews(displayFiles);
}

void AppController::refreshMediaCacheStats()
{
    emit mediaCacheSettingsChanged();
}

void AppController::saveTelegramSettings(const QString& botToken, const QVariantList& recipients, const QString& activeRecipientId)
{
    m_telegramRecipients = normalizedTelegramRecipients(recipients);
    m_activeTelegramRecipientId = activeRecipientId.trimmed();
    if (m_activeTelegramRecipientId.isEmpty() && !m_telegramRecipients.isEmpty())
        m_activeTelegramRecipientId = m_telegramRecipients.first().toMap().value("id").toString();

    m_telegram->setCredentials(botToken.trimmed(), activeTelegramChatId());
    persistTelegramRecipients();
    appendLog("Настройки Telegram обновлены.");
    emit telegramSettingsChanged();
}

void AppController::importTelegramRecipients(const QString& botToken)
{
    m_telegram->service()->importPrivateRecipients(botToken);
}

void AppController::setActiveTelegramRecipient(const QString& recipientId)
{
    const QString id = recipientId.trimmed();
    if (id.isEmpty() || id == m_activeTelegramRecipientId)
        return;

    for (const QVariant& value : m_telegramRecipients) {
        const QVariantMap recipient = value.toMap();
        if (recipient.value("id").toString() != id)
            continue;

        m_activeTelegramRecipientId = id;
        m_telegram->setCredentials(m_telegram->botToken(), activeTelegramChatId());
        persistTelegramRecipients();
        emit telegramSettingsChanged();
        appendLog("Активный получатель Telegram: " + recipient.value("label", recipient.value("chatId")).toString());
        return;
    }
}

QVariantList AppController::versionedSiblingFiles(const QString& path)
{
    QVariantList result;
    const QFileInfo fileInfo(sourcePathForPreview(QFileInfo(path)));
    const auto parsed = parseVersionedFileName(fileInfo.fileName());
    if (!parsed)
        return result;

    const QDir dir = fileInfo.dir();
    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
    QVector<QVariantMap> versions;
    for (const QFileInfo& entry : entries) {
        const auto candidate = parseVersionedFileName(entry.fileName());
        if (!candidate)
            continue;
        if (candidate->baseStem != parsed->baseStem || candidate->extension != parsed->extension)
            continue;

        QVariantMap item;
        item.insert("path", entry.absoluteFilePath());
        item.insert("label", candidate->label);
        item.insert("version", candidate->version);
        versions.append(item);
    }

    std::sort(versions.begin(), versions.end(), [](const QVariantMap& left, const QVariantMap& right) {
        return left.value("version").toInt() > right.value("version").toInt();
    });
    QStringList versionPaths;
    for (const QVariantMap& item : std::as_const(versions)) {
        result << item;
        versionPaths << item.value("path").toString();
    }
    if (m_mediaCacheEnabled)
        queueCachePreviews(versionPaths);
    return result;
}

void AppController::appendLog(const QString& line)
{
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    if (!m_logText.isEmpty())
        m_logText += '\n';
    m_logText += QString("[%1] %2").arg(timestamp, line);
    emit logTextChanged();
}

void AppController::setStatusText(const QString& text)
{
    if (m_statusText == text)
        return;
    m_statusText = text;
    emit statusTextChanged();
}

void AppController::setRunning(bool running)
{
    if (m_running == running)
        return;
    m_running = running;
    emit runningChanged();
}

void AppController::setProgressValue(int value)
{
    const int bounded = qBound(0, value, 100);
    if (m_progressValue == bounded)
        return;
    m_progressValue = bounded;
    emit progressValueChanged();
}

void AppController::addSourcePaths(const QStringList& paths, const QString& logPrefix)
{
    const QStringList videoFiles = expandVideoPaths(paths);
    if (videoFiles.isEmpty()) {
        setStatusText("Видео не найдены");
        appendLog(logPrefix + ": поддерживаемые видео не найдены.");
        return;
    }

    QSet<QString> seen;
    for (const QString& existing : std::as_const(m_allSourcePaths))
        seen.insert(existing);

    QStringList addedFiles;
    for (const QString& file : videoFiles) {
        const QString absolute = QFileInfo(file).absoluteFilePath();
        if (seen.contains(absolute))
            continue;
        seen.insert(absolute);
        m_allSourcePaths << absolute;
        addedFiles << absolute;
    }

    if (addedFiles.isEmpty()) {
        setStatusText(QString("Файлов в списке: %1").arg(m_files.count()));
        appendLog(logPrefix + ": новых видеофайлов нет.");
        return;
    }

    reloadDisplayedFiles();
    setStatusText(QString("Файлов в списке: %1").arg(m_files.count()));
    appendLog(QString("%1: %2 файл(ов).").arg(logPrefix).arg(addedFiles.size()));
}

QStringList AppController::expandVideoPaths(const QStringList& paths) const
{
    QStringList videoFiles;
    for (const QString& path : paths) {
        const QFileInfo fileInfo(path);
        if (fileInfo.isDir()) {
            QDirIterator it(fileInfo.absoluteFilePath(), QDir::Files | QDir::Readable,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString filePath = it.next();
                if (VideoFileModel::isVideoPath(filePath))
                    videoFiles << filePath;
            }
            continue;
        }

        if (VideoFileModel::isVideoPath(path))
            videoFiles << fileInfo.absoluteFilePath();
    }
    return videoFiles;
}

QStringList AppController::filteredDisplayFiles(const QStringList& files) const
{
    if (!m_latestVersionsOnly)
        return files;

    struct Candidate {
        QString path;
        int version = -1;
    };

    QHash<QString, Candidate> bestByKey;
    const QRegularExpression versionRegex("^(.*)_v(\\d+)$", QRegularExpression::CaseInsensitiveOption);

    for (const QString& file : files) {
        const QFileInfo fileInfo(file);
        const QRegularExpressionMatch match = versionRegex.match(fileInfo.completeBaseName());
        const bool versioned = match.hasMatch();
        const QString base = versioned ? match.captured(1).toLower() : fileInfo.completeBaseName().toLower();
        const QString key = fileInfo.absolutePath().toLower() + "/" + base + "." + fileInfo.suffix().toLower();
        const int version = versioned ? match.captured(2).toInt() : -1;

        const Candidate current{fileInfo.absoluteFilePath(), version};
        if (!bestByKey.contains(key) || current.version > bestByKey.value(key).version)
            bestByKey.insert(key, current);
    }

    QStringList result;
    result.reserve(bestByKey.size());
    for (const Candidate& candidate : bestByKey)
        result << candidate.path;
    result.sort(Qt::CaseInsensitive);
    return result;
}

void AppController::reloadDisplayedFiles()
{
    const QStringList displayFiles = filteredDisplayFiles(m_allSourcePaths);
    stopMetadataLoading();
    stopThumbnailGeneration();
    m_files.setFiles(displayFiles);
    queueMetadata(displayFiles);
    queueSourceThumbnails(displayFiles);
    queueFixedThumbnails();
    if (m_mediaCacheEnabled)
        queueCachePreviews(displayFiles);
    else
        updateMediaCacheComplete();
    setStatusText(QString("Файлов в списке: %1").arg(m_files.count()));
    emit actionSummaryChanged();
}

bool AppController::allActionEnabled(const QString& action) const
{
    if (m_files.count() == 0)
        return false;

    for (int row = 0; row < m_files.count(); ++row) {
        if (!m_files.actionAt(row, action))
            return false;
    }
    return true;
}

ConvertOptions AppController::optionsFromState() const
{
    ConvertOptions options;
    options.removeDupes = m_removeDupes;
    switch (m_duplicateModeIndex) {
    case 0:
        options.duplicateMode = DuplicateMode::Soft;
        break;
    case 1:
        options.duplicateMode = DuplicateMode::Medium;
        break;
    case 3:
        options.duplicateMode = DuplicateMode::VeryAggressive;
        break;
    case 4:
        options.duplicateMode = DuplicateMode::Maximum;
        break;
    case 2:
    default:
        options.duplicateMode = DuplicateMode::Aggressive;
        break;
    }
    options.convertTo25Fps = m_convertTo25Fps;
    options.existingMode = m_existingModeIndex == 1 ? ExistingMode::Overwrite : ExistingMode::Skip;
    return options;
}

bool AppController::executePreviewAndTelegram(const QVector<int>& rows, bool showNoActionsMessage, TaskSideScope sideScope)
{
    QSet<QString> previewInputs;
    QSet<QString> convertSet;
    m_sendAfterPreview.clear();
    m_previewInputRows.clear();
    m_previewInputOriginals.clear();
    m_telegramRows.clear();

    const auto addPreviewConversion = [this, &convertSet, &previewInputs](int row, const QString& previewInput) {
        convertSet.insert(previewInput);
        previewInputs.insert(previewInput);
        m_previewInputRows.insert(previewInput, row);
        m_previewInputOriginals.insert(previewInput, m_files.fileAt(row));
    };

    const auto shouldProcessRow = [&rows](int row) {
        return rows.isEmpty() || rows.contains(row);
    };

    for (int row = 0; row < m_files.count(); ++row) {
        if (!shouldProcessRow(row))
            continue;
        if (sideScope != TaskSideScope::Fixed && m_files.actionAt(row, "srcPreview"))
            addPreviewConversion(row, m_files.fileAt(row));
        if (sideScope != TaskSideScope::Source && m_files.actionAt(row, "fixedPreview"))
            addPreviewConversion(row, m_files.fixedPathAt(row));
    }

    int sentExisting = 0;
    int missingExisting = 0;
    const auto handleTelegram = [this, &previewInputs, &sentExisting, &missingExisting](int row, const QString& previewInput) {
        const QString previewOutput = VideoFileModel::makePreviewPath(previewInput);
        m_previewInputRows.insert(previewInput, row);
        m_previewInputOriginals.insert(previewInput, m_files.fileAt(row));

        if (previewInputs.contains(previewInput)) {
            m_sendAfterPreview.insert(previewInput);
            return;
        }

        if (QFileInfo::exists(previewOutput)) {
            m_files.setRuntimeStatus(row, "Отправка TG");
            m_telegramRows.insert(previewOutput, row);
            const QString original = m_files.fileAt(row);
            m_telegram->service()->sendFile(previewOutput, makeCaptionHtml(original));
            ++sentExisting;
        } else {
            m_files.setRuntimeStatus(row, "Нет preview");
            appendLog("TG пропущен, preview не найден: " + QDir::toNativeSeparators(previewOutput));
            ++missingExisting;
        }
    };

    for (int row = 0; row < m_files.count(); ++row) {
        if (!shouldProcessRow(row))
            continue;
        if (sideScope != TaskSideScope::Fixed && m_files.actionAt(row, "srcTelegram"))
            handleTelegram(row, m_files.fileAt(row));
        if (sideScope != TaskSideScope::Source && m_files.actionAt(row, "fixedTelegram"))
            handleTelegram(row, m_files.fixedPathAt(row));
    }

    QStringList toConvert = convertSet.values();
    toConvert.sort(Qt::CaseInsensitive);

    m_previewRows.clear();
    for (const QString& file : toConvert)
        m_previewRows << m_previewInputRows.value(file, m_files.rowForPath(file));

    appendLog(QString("Preview/TG: создать preview %1, отправить после preview %2, отправить готовые %3, без preview %4.")
                  .arg(toConvert.size())
                  .arg(m_sendAfterPreview.size())
                  .arg(sentExisting)
                  .arg(missingExisting));

    if (toConvert.isEmpty()) {
        if (sentExisting == 0 && missingExisting == 0 && showNoActionsMessage) {
            appendLog("Запуск отменен: действия в задаче не выбраны.");
            setStatusText("Нет выбранных действий");
            return false;
        }
        setRunning(false);
        setStatusText(sentExisting > 0 ? QString("TG: отправка готовых preview: %1").arg(sentExisting) : "Готово");
        return sentExisting > 0 || missingExisting > 0;
    }

    setRunning(true);
    m_preview->runPreviews(toConvert);
    return true;
}

QString AppController::makeCaptionHtml(const QString& input) const
{
    const QString windowsPath = windowsWorkPath(macFullPath(input));
    QString normalized = windowsPath;
    normalized.replace('/', '\\');

    const int separator = normalized.lastIndexOf('\\');
    const QString parent = separator >= 0 ? normalized.left(separator) : QString();
    const QString fileName = separator >= 0 ? normalized.mid(separator + 1) : normalized;

    return "<code>" + parent.toHtmlEscaped() + "\\</code>\n<b>" + fileName.toHtmlEscaped() + "</b>";
}

QString AppController::macFullPath(const QString& filePath) const
{
    return QFileInfo(filePath).absoluteFilePath();
}

QString AppController::windowsWorkPath(const QString& macPath) const
{
    QString normalizedWindows = macPath;
    normalizedWindows.replace('/', '\\');
    const QString lowerWindows = normalizedWindows.toLower();
    if (lowerWindows == "w:" || lowerWindows == "w:\\")
        return QStringLiteral("w:\\");
    if (lowerWindows.startsWith(QStringLiteral("w:\\"))) {
        const QString rest = normalizedWindows.mid(QStringLiteral("w:\\").size()).trimmed();
        return rest.isEmpty() ? QStringLiteral("w:\\") : QStringLiteral("w:\\") + rest;
    }

    QString rest;
    if (macPath == MacWorkPrefix)
        rest.clear();
    else if (macPath.startsWith(QString(MacWorkPrefix) + '/'))
        rest = macPath.mid(QString(MacWorkPrefix).size() + 1);
    else
        return QDir::toNativeSeparators(macPath);

    rest.replace('/', '\\');

    QString winPath = QStringLiteral("w:\\");
    if (!rest.isEmpty())
        winPath += rest;
    return winPath;
}

QString AppController::findMediaTool(const QString& baseName) const
{
    const QString appDir = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
    const QString exe = baseName + ".exe";
    const QStringList candidates{
        appDir + "/bin/" + exe,
        appDir + "/" + exe,
        appDir + "/../bin/" + exe,
        appDir + "/../../pp18-video-tools_cli/bin/" + exe
    };
#elif defined(Q_OS_MAC)
    const QString exe = baseName;
    const QStringList candidates{
        appDir + "/../Resources/bin/" + exe,
        appDir + "/bin/" + exe,
        appDir + "/../bin/" + exe,
        appDir + "/../../../../../pp18-video-tools_cli/bin/" + exe,
        appDir + "/../../pp18-video-tools_cli/bin/" + exe
    };
#else
    const QString exe = baseName;
    const QStringList candidates{
        appDir + "/bin/" + exe,
        appDir + "/../bin/" + exe,
        appDir + "/../../pp18-video-tools_cli/bin/" + exe
    };
#endif

    for (const QString& candidate : candidates) {
        const QFileInfo fileInfo(candidate);
        if (fileInfo.isFile() && fileInfo.isExecutable())
            return fileInfo.absoluteFilePath();
    }

#ifdef Q_OS_WIN
    return baseName + ".exe";
#else
    return baseName;
#endif
}

void AppController::queueMetadata(const QStringList& files)
{
    stopMetadataLoading();
    m_metadataQueue = files;
    startNextMetadata();
}

void AppController::startNextMetadata()
{
    if (m_metadataProcess || m_metadataQueue.isEmpty())
        return;

    const QString input = m_metadataQueue.takeFirst();
    const int row = m_files.rowForPath(input);
    if (row < 0) {
        startNextMetadata();
        return;
    }

    auto* process = new QProcess(this);
    process->setProperty("metadata_input", input);
    process->setProcessChannelMode(QProcess::MergedChannels);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AppController::onMetadataFinished);
    m_metadataProcess = process;

    process->start(findMediaTool("ffprobe"), {
        "-v", "error",
        "-select_streams", "v:0",
        "-show_entries", "stream=width,height,avg_frame_rate,r_frame_rate,nb_frames,duration",
        "-of", "default=noprint_wrappers=1",
        input
    });
}

void AppController::stopMetadataLoading()
{
    m_metadataQueue.clear();
    if (!m_metadataProcess)
        return;

    m_metadataProcess->kill();
    m_metadataProcess->deleteLater();
    m_metadataProcess = nullptr;
}

void AppController::queueSourceThumbnails(const QStringList& files)
{
    stopThumbnailGeneration();
    m_thumbnailQueue.clear();
    m_thumbnailQueue.reserve(files.size());
    for (const QString& file : files) {
        const int row = m_files.rowForPath(file);
        if (row >= 0)
            m_thumbnailQueue.append({file, row, false});
    }
    startNextThumbnail();
}

void AppController::queueFixedThumbnails()
{
    for (int row = 0; row < m_files.count(); ++row) {
        if (QFileInfo::exists(m_files.fixedPathAt(row)))
            m_thumbnailQueue.append({m_files.fixedPathAt(row), row, true});
    }
    startNextThumbnail();
}

void AppController::queueFixedThumbnail(int row)
{
    if (row < 0 || row >= m_files.count())
        return;

    const QString fixedPath = m_files.fixedPathAt(row);
    if (!QFileInfo::exists(fixedPath))
        return;

    m_thumbnailQueue.append({fixedPath, row, true});
    startNextThumbnail();
}

void AppController::startNextThumbnail()
{
    if (m_thumbnailProcess || m_thumbnailQueue.isEmpty())
        return;

    const ThumbnailJob job = m_thumbnailQueue.takeFirst();
    if (job.row < 0 || job.row >= m_files.count() || !QFileInfo::exists(job.input)) {
        startNextThumbnail();
        return;
    }

    const QByteArray hashInput = (job.input + QFileInfo(job.input).lastModified().toString(Qt::ISODateWithMs)).toUtf8();
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(hashInput, QCryptographicHash::Md5).toHex());
    const QString output = QDir(m_thumbnailDir).absoluteFilePath(hash + ".jpg");

    if (QFileInfo::exists(output)) {
        if (job.fixed)
            m_files.setFixedThumbnailUrl(job.row, QUrl::fromLocalFile(output).toString());
        else
            m_files.setThumbnailUrl(job.row, QUrl::fromLocalFile(output).toString());
        startNextThumbnail();
        return;
    }

    auto* process = new QProcess(this);
    process->setProperty("thumb_input", job.input);
    process->setProperty("thumb_output", output);
    process->setProperty("thumb_row", job.row);
    process->setProperty("thumb_fixed", job.fixed);
    process->setProcessChannelMode(QProcess::MergedChannels);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AppController::onThumbnailFinished);
    m_thumbnailProcess = process;

    process->start(findMediaTool("ffmpeg"), {
        "-y",
        "-ss", "00:00:01",
        "-i", job.input,
        "-frames:v", "1",
        "-vf", "scale=288:162:force_original_aspect_ratio=increase,crop=288:162",
        "-q:v", "3",
        output
    });
}

void AppController::stopThumbnailGeneration()
{
    m_thumbnailQueue.clear();
    if (!m_thumbnailProcess)
        return;

    m_thumbnailProcess->kill();
    m_thumbnailProcess->deleteLater();
    m_thumbnailProcess = nullptr;
}

QString AppController::appCacheRoot() const
{
    const QString root = m_mediaCacheRootPath.trimmed().isEmpty() ? defaultAppCacheRoot() : m_mediaCacheRootPath;
    QDir().mkpath(root);
    return root;
}

QString AppController::defaultAppCacheRoot() const
{
    const QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const QString root = home.isEmpty()
        ? QDir(QDir::tempPath()).absoluteFilePath("PP18_VideoTools/Cache")
        : QDir(home).absoluteFilePath("PP18_VideoTools/Cache");
    return root;
}

QString AppController::cacheKeyForPath(const QString& path) const
{
    const QFileInfo fileInfo(path);
    const QByteArray hashInput = (fileInfo.absoluteFilePath()
        + fileInfo.lastModified().toString(Qt::ISODateWithMs)).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(hashInput, QCryptographicHash::Md5).toHex());
}

QString AppController::cachePreviewPath(const QString& path) const
{
    if (path.trimmed().isEmpty())
        return QString();
    return QDir(m_cachePreviewDir).absoluteFilePath(cacheKeyForPath(path) + ".mp4");
}

void AppController::configureMediaCacheDirs()
{
    const QString root = appCacheRoot();
    m_thumbnailDir = QDir(root).absoluteFilePath("Thumbnails");
    m_cachePreviewDir = QDir(root).absoluteFilePath("Previews");
    QDir().mkpath(m_thumbnailDir);
    QDir().mkpath(m_cachePreviewDir);
}

void AppController::queueCachePreviews(const QStringList& files)
{
    if (!m_mediaCacheEnabled)
        return;

    QDir().mkpath(m_cachePreviewDir);
    QSet<QString> queuedOutputs;
    for (const CachePreviewJob& job : std::as_const(m_cachePreviewQueue))
        queuedOutputs.insert(job.output);
    if (m_cachePreviewProcess)
        queuedOutputs.insert(m_cachePreviewProcess->property("cache_output").toString());

    bool added = false;
    for (const QString& file : files) {
        const QString input = sourcePathForPreview(QFileInfo(file));
        const QFileInfo inputInfo(input);
        if (!inputInfo.exists() || !inputInfo.isFile())
            continue;

        const QString output = cachePreviewPath(inputInfo.absoluteFilePath());
        if (QFileInfo::exists(output) || queuedOutputs.contains(output))
            continue;

        m_cachePreviewQueue.append({inputInfo.absoluteFilePath(), output});
        queuedOutputs.insert(output);
        added = true;
    }

    if (added) {
        m_mediaCacheComplete = false;
        emit mediaCacheChanged();
    }
    startNextCachePreview();
    updateMediaCacheComplete();
}

void AppController::startNextCachePreview()
{
    if (!m_mediaCacheEnabled || m_cachePreviewProcess || m_cachePreviewQueue.isEmpty())
        return;

    const CachePreviewJob job = m_cachePreviewQueue.takeFirst();
    if (!QFileInfo::exists(job.input)) {
        startNextCachePreview();
        return;
    }

    auto* process = new QProcess(this);
    process->setProperty("cache_input", job.input);
    process->setProperty("cache_output", job.output);
    process->setProcessChannelMode(QProcess::MergedChannels);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AppController::onCachePreviewFinished);
    m_cachePreviewProcess = process;
    emit mediaCacheChanged();

    appendLog("Cache preview: " + QDir::toNativeSeparators(job.input));
    process->start(findMediaTool("ffmpeg"), {
        "-y",
        "-i", job.input,
        "-map", "0:v:0",
        "-map", "0:a?",
        "-vf", "scale='if(gte(iw,ih),min(1280,iw),-2)':'if(gt(ih,iw),min(1280,ih),-2)'",
        "-c:v", "libx264",
        "-preset", "veryfast",
        "-crf", "22",
        "-pix_fmt", "yuv420p",
        "-c:a", "aac",
        "-b:a", "128k",
        "-movflags", "+faststart",
        job.output
    });
}

void AppController::stopCachePreviewGeneration()
{
    m_cachePreviewQueue.clear();
    if (!m_cachePreviewProcess)
        return;

    m_cachePreviewProcess->kill();
    m_cachePreviewProcess->deleteLater();
    m_cachePreviewProcess = nullptr;
}

void AppController::updateMediaCacheComplete()
{
    bool complete = m_mediaCacheEnabled && !mediaCacheRunning();
    if (complete) {
        const QStringList displayFiles = filteredDisplayFiles(m_allSourcePaths);
        if (displayFiles.isEmpty())
            complete = false;
        for (const QString& file : displayFiles) {
            const QString input = sourcePathForPreview(QFileInfo(file));
            if (!QFileInfo::exists(cachePreviewPath(input))) {
                complete = false;
                break;
            }
        }
    }

    if (m_mediaCacheComplete == complete)
        return;
    m_mediaCacheComplete = complete;
    emit mediaCacheChanged();
}

qint64 AppController::mediaCacheSizeBytes() const
{
    qint64 total = 0;
    const QStringList roots{m_thumbnailDir, m_cachePreviewDir};
    for (const QString& root : roots) {
        QDirIterator it(root, QDir::Files | QDir::Readable | QDir::Hidden | QDir::System,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            total += it.fileInfo().size();
        }
    }
    return total;
}

QString AppController::formatByteSize(qint64 bytes) const
{
    const double gb = static_cast<double>(bytes) / static_cast<double>(BytesInGb);
    if (gb >= 1.0)
        return QString::number(gb, 'f', gb >= 10.0 ? 1 : 2) + " GB";

    const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    if (mb >= 1.0)
        return QString::number(mb, 'f', mb >= 10.0 ? 1 : 2) + " MB";

    const double kb = static_cast<double>(bytes) / 1024.0;
    if (kb >= 1.0)
        return QString::number(kb, 'f', 1) + " KB";

    return QString::number(bytes) + " B";
}

void AppController::enforceMediaCacheLimit()
{
    if (m_mediaCacheMaxSizeGb <= 0.0)
        return;

    const qint64 limitBytes = static_cast<qint64>(m_mediaCacheMaxSizeGb * static_cast<double>(BytesInGb));
    qint64 currentBytes = mediaCacheSizeBytes();
    if (currentBytes <= limitBytes)
        return;

    QFileInfoList files;
    const QStringList roots{m_thumbnailDir, m_cachePreviewDir};
    for (const QString& root : roots) {
        QDirIterator it(root, QDir::Files | QDir::Readable | QDir::Hidden | QDir::System,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            files << it.fileInfo();
        }
    }

    std::sort(files.begin(), files.end(), [](const QFileInfo& left, const QFileInfo& right) {
        return left.lastModified() < right.lastModified();
    });

    for (const QFileInfo& file : std::as_const(files)) {
        if (currentBytes <= limitBytes)
            break;
        const qint64 fileSize = file.size();
        if (QFile::remove(file.absoluteFilePath()))
            currentBytes -= fileSize;
    }
}

void AppController::loadTelegramRecipients()
{
    QSettings settings;
    m_telegramRecipients = normalizedTelegramRecipients(settings.value("telegram/recipients").toList());
    m_activeTelegramRecipientId = settings.value("telegram/active_recipient_id").toString();

    if (m_telegramRecipients.isEmpty() && !m_telegram->chatId().isEmpty()) {
        QVariantMap legacy;
        legacy.insert("id", "recipient-default");
        legacy.insert("label", "PP18 OUT");
        legacy.insert("chatId", m_telegram->chatId());
        m_telegramRecipients << legacy;
        m_activeTelegramRecipientId = legacy.value("id").toString();
        persistTelegramRecipients();
    }

    if (m_activeTelegramRecipientId.isEmpty() && !m_telegramRecipients.isEmpty())
        m_activeTelegramRecipientId = m_telegramRecipients.first().toMap().value("id").toString();

    const QString activeChat = activeTelegramChatId();
    if (!activeChat.isEmpty() && activeChat != m_telegram->chatId())
        m_telegram->setCredentials(m_telegram->botToken(), activeChat);

    emit telegramSettingsChanged();
}

void AppController::persistTelegramRecipients()
{
    QSettings settings;
    settings.setValue("telegram/recipients", m_telegramRecipients);
    settings.setValue("telegram/active_recipient_id", m_activeTelegramRecipientId);
}

QString AppController::activeTelegramChatId() const
{
    for (const QVariant& value : m_telegramRecipients) {
        const QVariantMap recipient = value.toMap();
        if (recipient.value("id").toString() == m_activeTelegramRecipientId)
            return recipient.value("chatId").toString().trimmed();
    }
    return m_telegramRecipients.isEmpty() ? QString() : m_telegramRecipients.first().toMap().value("chatId").toString().trimmed();
}

QVariantList AppController::normalizedTelegramRecipients(const QVariantList& recipients) const
{
    QVariantList normalized;
    QSet<QString> seenIds;

    for (int i = 0; i < recipients.size(); ++i) {
        QVariantMap source = recipients.at(i).toMap();
        QString chatId = source.value("chatId", source.value("chat_id")).toString().trimmed();
        if (chatId.isEmpty())
            continue;

        QString id = source.value("id").toString().trimmed();
        if (id.isEmpty())
            id = QString("recipient-%1").arg(i + 1);
        if (seenIds.contains(id))
            id = QString("%1-%2").arg(id).arg(i + 1);
        seenIds.insert(id);

        QString label = source.value("label").toString().trimmed();
        if (label.isEmpty())
            label = chatId;

        QVariantMap recipient;
        recipient.insert("id", id);
        recipient.insert("label", label);
        recipient.insert("chatId", chatId);
        normalized << recipient;
    }

    return normalized;
}

void AppController::onFixBatchStarted(int total)
{
    setProgressValue(0);
    setStatusText(QString("Fixes в очереди: %1").arg(total));
}

void AppController::onFixFileStarted(int index, int, const QString&, const QString&)
{
    const int row = m_fixRows.value(index, index);
    m_files.setRuntimeStatus(row, "Fixes");
}

void AppController::onFixFileFinished(int index, int, bool ok, const QString&, const QString&, const QString& details)
{
    const int row = m_fixRows.value(index, index);
    m_files.setRuntimeStatus(row, ok ? (details == "Skipped existing output" ? "Fixes skipped" : "Fixes ready") : "Fixes error");
    m_files.refreshExistence();
    if (ok)
        queueFixedThumbnail(row);
}

void AppController::onFixProgress(int completed, int total)
{
    const int percent = total > 0 ? (completed * 100 / total) : 0;
    setProgressValue(percent);
    setStatusText(QString("Fixes %1/%2").arg(completed).arg(total));
}

void AppController::onFixFinished(bool ok)
{
    m_fixRows.clear();
    m_files.refreshExistence();
    setStatusText(ok ? "Fixes готовы" : "Fixes завершены не полностью");

    const bool runNext = ok && m_runPreviewAfterFixes;
    m_runPreviewAfterFixes = false;
    if (runNext) {
        const QVector<int> rows = m_postFixRows;
        const TaskSideScope sideScope = m_postFixSideScope;
        m_postFixRows.clear();
        m_postFixSideScope = TaskSideScope::All;
        executePreviewAndTelegram(rows, false, sideScope);
        return;
    }

    m_postFixRows.clear();
    m_postFixSideScope = TaskSideScope::All;
    setRunning(false);
}

void AppController::onPreviewStarted(int total)
{
    setProgressValue(0);
    setStatusText(QString("Preview в очереди: %1").arg(total));
}

void AppController::onPreviewFileStarted(int index, int, const QString& input, const QString&)
{
    const int row = m_previewRows.value(index, m_previewInputRows.value(input, -1));
    m_files.setRuntimeStatus(row, "Make Preview");
}

void AppController::onPreviewFileFinished(int index, int, bool ok, const QString& input, const QString& output, const QString& details)
{
    const int row = m_previewRows.value(index, m_previewInputRows.value(input, -1));
    m_files.setRuntimeStatus(row, ok ? "Preview ready" : "Preview error");
    m_files.refreshExistence();

    if (!ok) {
        if (!details.isEmpty())
            appendLog("Ошибка preview: " + details);
        return;
    }

    if (m_sendAfterPreview.contains(input)) {
        m_files.setRuntimeStatus(row, "Отправка TG");
        m_telegramRows.insert(output, row);
        const QString original = m_previewInputOriginals.value(input, input);
        m_telegram->service()->sendFile(output, makeCaptionHtml(original));
    }
}

void AppController::onPreviewProgress(int completed, int total)
{
    const int percent = total > 0 ? (completed * 100 / total) : 0;
    setProgressValue(percent);
    setStatusText(QString("Preview %1/%2").arg(completed).arg(total));
}

void AppController::onPreviewFinished(bool ok)
{
    m_previewRows.clear();
    m_sendAfterPreview.clear();
    m_previewInputRows.clear();
    m_previewInputOriginals.clear();
    m_files.refreshExistence();
    setStatusText(ok ? "Preview готовы" : "Preview завершены не полностью");
    m_postFixSideScope = TaskSideScope::All;
    setRunning(false);
}

void AppController::onTelegramFileSent(const QString& filePath, bool ok, const QString& details)
{
    Q_UNUSED(details)

    if (!m_telegramRows.contains(filePath))
        return;

    const int row = m_telegramRows.take(filePath);
    if (row >= 0)
        m_files.setRuntimeStatus(row, ok ? "TG sent" : "TG error");
}

void AppController::onMetadataFinished(int exitCode, QProcess::ExitStatus status)
{
    auto* process = qobject_cast<QProcess*>(sender());
    if (!process || process != m_metadataProcess)
        return;

    const QString input = process->property("metadata_input").toString();
    const QString output = QString::fromUtf8(process->readAllStandardOutput());
    const int row = m_files.rowForPath(input);

    if (row >= 0 && exitCode == 0 && status == QProcess::NormalExit) {
        const VideoMetadata metadata = metadataFromFfprobeOutput(output);
        m_files.setMetadata(row, metadata.resolution, metadata.frames, metadata.fps);
    }

    process->deleteLater();
    m_metadataProcess = nullptr;
    startNextMetadata();
}

void AppController::onThumbnailFinished(int exitCode, QProcess::ExitStatus status)
{
    auto* process = qobject_cast<QProcess*>(sender());
    if (!process || process != m_thumbnailProcess)
        return;

    const QString input = process->property("thumb_input").toString();
    const QString output = process->property("thumb_output").toString();
    const int row = process->property("thumb_row").toInt();
    const bool fixed = process->property("thumb_fixed").toBool();

    if (row >= 0 && exitCode == 0 && status == QProcess::NormalExit && QFileInfo::exists(output)) {
        if (fixed)
            m_files.setFixedThumbnailUrl(row, QUrl::fromLocalFile(output).toString());
        else
            m_files.setThumbnailUrl(row, QUrl::fromLocalFile(output).toString());
    }

    process->deleteLater();
    m_thumbnailProcess = nullptr;
    enforceMediaCacheLimit();
    emit mediaCacheSettingsChanged();
    startNextThumbnail();
}

void AppController::onCachePreviewFinished(int exitCode, QProcess::ExitStatus status)
{
    auto* process = qobject_cast<QProcess*>(sender());
    if (!process || process != m_cachePreviewProcess)
        return;

    const QString input = process->property("cache_input").toString();
    const QString output = process->property("cache_output").toString();
    const QString details = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
    const bool ok = exitCode == 0 && status == QProcess::NormalExit && QFileInfo::exists(output);

    if (ok) {
        appendLog("Cache preview готов: " + QDir::toNativeSeparators(output));
    } else {
        QFile::remove(output);
        appendLog("Ошибка cache preview: " + QDir::toNativeSeparators(input));
        if (!details.isEmpty())
            appendLog(details.right(600));
    }

    process->deleteLater();
    m_cachePreviewProcess = nullptr;
    enforceMediaCacheLimit();
    emit mediaCacheChanged();
    emit mediaCacheSettingsChanged();
    startNextCachePreview();
    updateMediaCacheComplete();
}
