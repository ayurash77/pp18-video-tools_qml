#ifndef HANDBRAKEPREVIEWSERVICE_H
#define HANDBRAKEPREVIEWSERVICE_H

#include <QObject>
#include <QProcess>
#include <QStringList>

class HandbrakePreviewService : public QObject {
    Q_OBJECT
public:
    explicit HandbrakePreviewService(QObject* parent = nullptr);

    QString toolPath() const { return m_toolPath; }
    static QString makeOutputPath(const QString& input);

    void runPreviews(const QStringList& files);
    void stop();
    bool isRunning() const { return m_proc != nullptr; }

signals:
    void started(int total);
    void log(const QString& line);
    void progress(int completed, int total);
    void fileStarted(int index, int total, const QString& input, const QString& output);
    void fileFinished(int index, int total, bool ok, const QString& input, const QString& output, const QString& details);
    void finished(bool ok);

private slots:
    void onReadyRead();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    QString findTool(const QString& baseName) const;
    QStringList buildArgs(const QString& input, const QString& output) const;
    void startNext();
    void finish(bool ok);

private:
    QString m_toolPath;
    QStringList m_queue;
    int m_index = -1;
    QProcess* m_proc = nullptr;
    bool m_stopRequested = false;
};

#endif // HANDBRAKEPREVIEWSERVICE_H
