#include "FfmpegBatchService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

FfmpegBatchService::FfmpegBatchService(QObject* parent)
    : QObject(parent)
    , m_ffmpegPath(findTool("ffmpeg"))
{
}

QStringList FfmpegBatchService::discoverFiles(const ConvertOptions& options) const
{
    QDir dir(options.folder);
    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);

    QStringList files;
    for (const QFileInfo& fileInfo : entries) {
        if (fileInfo.completeBaseName().endsWith("__preview", Qt::CaseInsensitive))
            continue;
        files << fileInfo.absoluteFilePath();
    }
    return files;
}

void FfmpegBatchService::run(const ConvertOptions& options)
{
    if (m_proc)
        return;

    QFileInfo tool(m_ffmpegPath);
    if ((!tool.isFile() || !tool.isExecutable()) && m_ffmpegPath != "ffmpeg" && m_ffmpegPath != "ffmpeg.exe") {
        emit log("ffmpeg не найден или не исполняемый: " + m_ffmpegPath);
        emit finished(false);
        return;
    }

    m_options = options;
    m_queue = options.useExplicitFiles ? options.explicitFiles : discoverFiles(options);
    m_index = -1;
    m_stopRequested = false;

    emit batchStarted(m_queue.size());
    emit progress(0, m_queue.size());

    if (m_queue.isEmpty()) {
        emit log("Файлы для обработки не найдены.");
        emit finished(false);
        return;
    }

    emit log("ffmpeg: " + m_ffmpegPath);
    if (!options.folder.isEmpty())
        emit log("Рабочая папка: " + QDir::toNativeSeparators(options.folder));
    startNext();
}

void FfmpegBatchService::stop()
{
    m_stopRequested = true;
    if (m_proc)
        m_proc->kill();
}

void FfmpegBatchService::onReadyRead()
{
    if (!m_proc)
        return;

    const QString text = QString::fromUtf8(m_proc->readAllStandardOutput()).trimmed();
    if (!text.isEmpty())
        emit log(text);
}

void FfmpegBatchService::onProcessError(QProcess::ProcessError error)
{
    if (!m_proc || error != QProcess::FailedToStart)
        return;

    const QString input = m_queue.value(m_index);
    const QString output = makeOutputPath(input);
    emit log("Не удалось запустить ffmpeg: " + m_proc->errorString());

    m_proc->deleteLater();
    m_proc = nullptr;

    emit fileFinished(m_index, m_queue.size(), false, input, output, "Failed to start ffmpeg");
    finish(false);
}

void FfmpegBatchService::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_proc)
        return;

    const QString input = m_queue.value(m_index);
    const QString output = makeOutputPath(input);
    const QString tail = QString::fromUtf8(m_proc->readAllStandardOutput()).trimmed();
    const bool ok = (exitCode == 0 && status == QProcess::NormalExit && QFileInfo::exists(output));

    m_proc->deleteLater();
    m_proc = nullptr;

    if (!tail.isEmpty())
        emit log(tail);

    if (m_stopRequested) {
        emit fileFinished(m_index, m_queue.size(), false, input, output, "Остановлено пользователем");
        finish(false);
        return;
    }

    if (ok) {
        emit log("Готово: " + QDir::toNativeSeparators(output));
    } else {
        QFile::remove(output);
        emit log("Ошибка при обработке: " + QDir::toNativeSeparators(input));
    }

    emit fileFinished(m_index, m_queue.size(), ok, input, output, tail);
    emit progress(m_index + 1, m_queue.size());
    startNext();
}

QString FfmpegBatchService::findTool(const QString& baseName) const
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
        QFileInfo fi(candidate);
        if (fi.isFile() && fi.isExecutable())
            return fi.absoluteFilePath();
    }

#ifdef Q_OS_WIN
    return baseName + ".exe";
#else
    return baseName;
#endif
}

QString FfmpegBatchService::makeOutputPath(const QString& input) const
{
    QFileInfo fi(input);
    return fi.dir().absoluteFilePath("fixed/" + fi.completeBaseName() + ".mov");
}

QString FfmpegBatchService::duplicateFilter(DuplicateMode mode) const
{
    switch (mode) {
    case DuplicateMode::Soft:
        return "mpdecimate=hi=768:lo=320:frac=0.33";
    case DuplicateMode::Medium:
        return "mpdecimate=hi=1024:lo=512:frac=0.45";
    case DuplicateMode::VeryAggressive:
        return "hqdn3d=4:3:6:4,mpdecimate=hi=2048:lo=1024:frac=0.75";
    case DuplicateMode::Maximum:
        return "hqdn3d=4:3:6:4,mpdecimate=hi=4096:lo=2048:frac=0.85";
    case DuplicateMode::Aggressive:
    default:
        return "mpdecimate=hi=2048:lo=1024:frac=0.75";
    }
}

QString FfmpegBatchService::videoFilter(const ConvertOptions& options) const
{
    QStringList filters;
    if (options.removeDupes)
        filters << duplicateFilter(options.duplicateMode);

    if (options.convertTo25Fps)
        filters << "setpts=N/(25*TB)";
    else if (options.removeDupes)
        filters << "setpts=N/FRAME_RATE/TB";

    return filters.join(",");
}

QStringList FfmpegBatchService::buildArgs(const QString& input, const QString& output, const ConvertOptions& options) const
{
    QStringList args;
    args << "-y" << "-i" << input;

    const QString vf = videoFilter(options);
    if (!vf.isEmpty()) {
        args << "-vf" << vf;
        if (options.convertTo25Fps)
            args << "-r" << "25";
    }

    args << "-c:v" << "prores_ks"
         << "-profile:v" << "3"
         << "-c:a" << "pcm_s16le"
         << output;

    return args;
}

void FfmpegBatchService::startNext()
{
    if (m_stopRequested) {
        finish(false);
        return;
    }

    ++m_index;
    if (m_index >= m_queue.size()) {
        finish(true);
        return;
    }

    const QString input = m_queue.at(m_index);
    const QString output = makeOutputPath(input);
    QDir outputDir(QFileInfo(output).dir());
    if (!outputDir.exists() && !outputDir.mkpath(".")) {
        emit log("Не удалось создать папку: " + QDir::toNativeSeparators(outputDir.absolutePath()));
        emit fileFinished(m_index, m_queue.size(), false, input, output, "Failed to create output folder");
        emit progress(m_index + 1, m_queue.size());
        startNext();
        return;
    }

    if (QFileInfo::exists(output)) {
        if (m_options.existingMode == ExistingMode::Skip) {
            emit log("Пропущено, уже существует: " + QDir::toNativeSeparators(output));
            emit fileFinished(m_index, m_queue.size(), true, input, output, "Skipped existing output");
            emit progress(m_index + 1, m_queue.size());
            startNext();
            return;
        }
        QFile::remove(output);
    }

    emit fileStarted(m_index, m_queue.size(), input, output);
    emit log(QString("Processing %1/%2: %3")
                 .arg(m_index + 1)
                 .arg(m_queue.size())
                 .arg(QDir::toNativeSeparators(input)));

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &FfmpegBatchService::onReadyRead);
    connect(m_proc, &QProcess::errorOccurred, this, &FfmpegBatchService::onProcessError);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FfmpegBatchService::onProcessFinished);
    m_proc->start(m_ffmpegPath, buildArgs(input, output, m_options));
}

void FfmpegBatchService::finish(bool ok)
{
    if (ok)
        emit log("Готово.");
    else
        emit log("Операция завершена с ошибкой или была остановлена.");

    m_queue.clear();
    m_index = -1;
    m_stopRequested = false;
    emit finished(ok);
}
