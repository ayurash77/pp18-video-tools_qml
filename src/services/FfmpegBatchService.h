#ifndef FFMPEGBATCHSERVICE_H
#define FFMPEGBATCHSERVICE_H

#include <QObject>
#include <QProcess>
#include <QStringList>

enum class DuplicateMode {
    Soft,
    Medium,
    Aggressive,
    VeryAggressive,
    Maximum
};

enum class ExistingMode {
    Skip,
    Overwrite
};

struct ConvertOptions {
    QString folder;
    QStringList explicitFiles;
    bool useExplicitFiles = false;
    bool removeDupes = true;
    DuplicateMode duplicateMode = DuplicateMode::Aggressive;
    bool convertTo25Fps = true;
    ExistingMode existingMode = ExistingMode::Skip;
};

class FfmpegBatchService : public QObject {
    Q_OBJECT
public:
    explicit FfmpegBatchService(QObject* parent = nullptr);

    QString ffmpegPath() const { return m_ffmpegPath; }
    QStringList discoverFiles(const ConvertOptions& options) const;

    void run(const ConvertOptions& options);
    void stop();
    bool isRunning() const { return m_proc != nullptr; }

signals:
    void log(const QString& line);
    void batchStarted(int total);
    void fileStarted(int index, int total, const QString& input, const QString& output);
    void fileFinished(int index, int total, bool ok, const QString& input, const QString& output, const QString& details);
    void progress(int completed, int total);
    void finished(bool ok);

private slots:
    void onReadyRead();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    QString findTool(const QString& baseName) const;
    QString makeOutputPath(const QString& input) const;
    QString duplicateFilter(DuplicateMode mode) const;
    QString videoFilter(const ConvertOptions& options) const;
    QStringList buildArgs(const QString& input, const QString& output, const ConvertOptions& options) const;
    void startNext();
    void finish(bool ok);

private:
    QString m_ffmpegPath;
    ConvertOptions m_options;
    QStringList m_queue;
    int m_index = -1;
    QProcess* m_proc = nullptr;
    bool m_stopRequested = false;
};

#endif // FFMPEGBATCHSERVICE_H
