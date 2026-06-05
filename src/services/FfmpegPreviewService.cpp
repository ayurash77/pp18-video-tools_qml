#include "FfmpegPreviewService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

FfmpegPreviewService::FfmpegPreviewService(QObject* parent)
    : QObject(parent)
    , m_toolPath(findTool("ffmpeg"))
{
}

void FfmpegPreviewService::runPreviews(const QStringList& files)
{
    if (m_proc)
        return;

    QFileInfo tool(m_toolPath);
    if ((!tool.isFile() || !tool.isExecutable()) && m_toolPath != "ffmpeg" && m_toolPath != "ffmpeg.exe") {
        emit log("ffmpeg не найден или не исполняемый: " + m_toolPath);
        emit finished(false);
        return;
    }

    m_queue.clear();
    for (const QString& file : files) {
        if (QFileInfo::exists(file))
            m_queue << file;
        else
            emit log("Файл для превью не найден: " + QDir::toNativeSeparators(file));
    }

    m_index = -1;
    m_stopRequested = false;

    emit started(m_queue.size());
    emit progress(0, m_queue.size());

    if (m_queue.isEmpty()) {
        emit log("Нет файлов для превью.");
        emit finished(false);
        return;
    }

    emit log("ffmpeg preview: " + m_toolPath);
    startNext();
}

void FfmpegPreviewService::stop()
{
    m_stopRequested = true;
    if (m_proc)
        m_proc->kill();
}

QString FfmpegPreviewService::makeOutputPath(const QString& input)
{
    QFileInfo fi(input);
    return fi.dir().absoluteFilePath(fi.completeBaseName() + "__preview.mp4");
}

void FfmpegPreviewService::onReadyRead()
{
    if (!m_proc)
        return;

    const QString text = QString::fromUtf8(m_proc->readAllStandardOutput()).trimmed();
    if (!text.isEmpty())
        emit log(text);
}

void FfmpegPreviewService::onProcessError(QProcess::ProcessError error)
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

void FfmpegPreviewService::onProcessFinished(int exitCode, QProcess::ExitStatus status)
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
        emit log("Превью готово: " + QDir::toNativeSeparators(output));
    } else {
        QFile::remove(output);
        emit log("Ошибка при создании превью: " + QDir::toNativeSeparators(input));
    }

    emit fileFinished(m_index, m_queue.size(), ok, input, output, tail);
    emit progress(m_index + 1, m_queue.size());
    startNext();
}

QString FfmpegPreviewService::findTool(const QString& baseName) const
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

QStringList FfmpegPreviewService::buildArgs(const QString& input, const QString& output) const
{
    return {
        "-y",
        "-i", input,
        "-map", "0:v:0",
        "-map", "0:a?",
        "-c:v", "libx264",
        "-preset", "superfast",
        "-crf", "23",
        "-profile:v", "main",
        "-level", "4.0",
        "-pix_fmt", "yuv420p",
        "-c:a", "aac",
        "-b:a", "160k",
        "-movflags", "+faststart",
        output
    };
}

void FfmpegPreviewService::startNext()
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

    QFile::remove(output);

    emit fileStarted(m_index, m_queue.size(), input, output);
    emit log(QString("Превью %1/%2: %3")
                 .arg(m_index + 1)
                 .arg(m_queue.size())
                 .arg(QDir::toNativeSeparators(input)));

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &FfmpegPreviewService::onReadyRead);
    connect(m_proc, &QProcess::errorOccurred, this, &FfmpegPreviewService::onProcessError);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FfmpegPreviewService::onProcessFinished);
    m_proc->start(m_toolPath, buildArgs(input, output));
}

void FfmpegPreviewService::finish(bool ok)
{
    if (ok)
        emit log("Превью готовы.");
    else
        emit log("Создание превью завершено с ошибкой или было остановлено.");

    m_queue.clear();
    m_index = -1;
    m_stopRequested = false;
    emit finished(ok);
}
