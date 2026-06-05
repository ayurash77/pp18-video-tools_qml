#include "NativeFolderDialog.h"

#include <memory>
#include <utility>

#include <QDir>
#include <QProcess>

void openNativeFolderDialog(const QString& title, const QString& startDir, FolderDialogCallback callback)
{
    auto* process = new QProcess;
    auto callbackPtr = std::make_shared<FolderDialogCallback>(std::move(callback));
    auto completed = std::make_shared<bool>(false);
    const QString scriptWithArgs = R"(
on run argv
    set promptText to item 1 of argv
    set defaultPath to item 2 of argv
    set pickedFolders to choose folder with prompt promptText default location (POSIX file defaultPath) with multiple selections allowed
    set outputText to ""
    repeat with pickedFolder in pickedFolders
        set outputText to outputText & POSIX path of pickedFolder & linefeed
    end repeat
    return outputText
end run
)";
    const QString simpleScript = "return POSIX path of (choose folder)";

    QObject::connect(process, &QProcess::finished, process,
        [process, callbackPtr, completed, simpleScript](int exitCode, QProcess::ExitStatus exitStatus) mutable {
            if (*completed)
                return;

            QString folder;
            QStringList folders;
            QString error;
            if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                *completed = true;
                const QString stdoutText = QString::fromUtf8(process->readAllStandardOutput());
                for (const QString& line : stdoutText.split('\n', Qt::SkipEmptyParts))
                    folders << line.trimmed();
                if (*callbackPtr)
                    (*callbackPtr)(folders, error);
                process->deleteLater();
                return;
            } else {
                const QString stderrText = QString::fromUtf8(process->readAllStandardError()).trimmed();
                error = QString("osascript завершился с кодом %1%2")
                    .arg(exitCode)
                    .arg(stderrText.isEmpty() ? QString() : QString(": %1").arg(stderrText));
            }

            if (process->property("fallbackStarted").toBool()) {
                *completed = true;
                const QString firstAttemptError = process->property("firstAttemptError").toString();
                if (*callbackPtr)
                    (*callbackPtr)({}, firstAttemptError.isEmpty() ? error : firstAttemptError + "; fallback: " + error);
                process->deleteLater();
                return;
            }

            process->setProperty("firstAttemptError", error);
            process->setProperty("fallbackStarted", true);
            process->start("/usr/bin/osascript", {"-e", simpleScript});
        });

    QObject::connect(process, &QProcess::errorOccurred, process,
        [process, callbackPtr, completed](QProcess::ProcessError) mutable {
            if (*completed)
                return;
            *completed = true;

            if (*callbackPtr)
                (*callbackPtr)({}, "Не удалось запустить /usr/bin/osascript: " + process->errorString());
            process->deleteLater();
        });

    process->start("/usr/bin/osascript", {"-e", scriptWithArgs, "--", title, QDir::toNativeSeparators(startDir)});
}
