#include "NativeFileDialog.h"

#include <memory>
#include <utility>

#include <QDir>
#include <QProcess>

namespace {
QString appleScriptListLiteral(const QStringList& values)
{
    QStringList escaped;
    escaped.reserve(values.size());
    for (QString value : values) {
        value.replace("\\", "\\\\");
        value.replace("\"", "\\\"");
        escaped << "\"" + value + "\"";
    }
    return "{" + escaped.join(", ") + "}";
}
}

void openNativeFileDialog(const QString& title, const QString& startDir, FileDialogCallback callback)
{
    auto* process = new QProcess;
    auto callbackPtr = std::make_shared<FileDialogCallback>(std::move(callback));
    const QStringList extensions{
        "3g2", "3gp", "avi", "dv", "flv", "m2ts", "m4v", "mkv", "mov", "mp4",
        "mpeg", "mpg", "mts", "mxf", "ogv", "ts", "vob", "webm", "wmv"
    };

    const QString script = QString(R"(
on run argv
    set promptText to item 1 of argv
    set defaultPath to item 2 of argv
    set allowedExtensions to %1
    set pickedFiles to choose file with prompt promptText default location (POSIX file defaultPath) of type allowedExtensions with multiple selections allowed
    set outputText to ""
    repeat with pickedFile in pickedFiles
        set outputText to outputText & POSIX path of pickedFile & linefeed
    end repeat
    return outputText
end run
)").arg(appleScriptListLiteral(extensions));

    QObject::connect(process, &QProcess::finished, process,
        [process, callbackPtr](int exitCode, QProcess::ExitStatus exitStatus) mutable {
            QStringList files;
            QString error;
            if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                const QString stdoutText = QString::fromUtf8(process->readAllStandardOutput());
                for (const QString& line : stdoutText.split('\n', Qt::SkipEmptyParts))
                    files << line.trimmed();
            } else {
                const QString stderrText = QString::fromUtf8(process->readAllStandardError()).trimmed();
                error = QString("osascript завершился с кодом %1%2")
                    .arg(exitCode)
                    .arg(stderrText.isEmpty() ? QString() : QString(": %1").arg(stderrText));
            }

            if (*callbackPtr)
                (*callbackPtr)(files, error);
            process->deleteLater();
        });

    QObject::connect(process, &QProcess::errorOccurred, process,
        [process, callbackPtr](QProcess::ProcessError) mutable {
            if (*callbackPtr)
                (*callbackPtr)({}, "Не удалось запустить /usr/bin/osascript: " + process->errorString());
            process->deleteLater();
        });

    process->start("/usr/bin/osascript", {"-e", script, "--", title, QDir::toNativeSeparators(startDir)});
}
