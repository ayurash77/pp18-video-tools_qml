#include "NativeFolderDialog.h"

#include <memory>
#include <utility>

#include <QDir>
#include <QProcess>

namespace {
QStringList linesFromProcessOutput(QProcess* process)
{
    QStringList lines;
    const QString stdoutText = QString::fromUtf8(process->readAllStandardOutput());
    for (const QString& line : stdoutText.split('\n', Qt::SkipEmptyParts))
        lines << line.trimmed();
    return lines;
}

QString powerShellStringLiteral(QString value)
{
    value.replace("'", "''");
    return "'" + value + "'";
}
}

void openNativeFolderDialog(const QString& title, const QString& startDir, FolderDialogCallback callback)
{
#ifdef Q_OS_WIN
    auto* process = new QProcess;
    auto callbackPtr = std::make_shared<FolderDialogCallback>(std::move(callback));
    const QString script = QString(R"(
$dialogTitle = %1
$initialDirectory = %2
Add-Type -AssemblyName System.Windows.Forms
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$dialog = New-Object System.Windows.Forms.FolderBrowserDialog
$dialog.Description = $dialogTitle
$dialog.ShowNewFolderButton = $false
if ([System.IO.Directory]::Exists($initialDirectory)) {
    $dialog.SelectedPath = $initialDirectory
}
if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
    Write-Output $dialog.SelectedPath
}
)")
        .arg(powerShellStringLiteral(title),
             powerShellStringLiteral(QDir::toNativeSeparators(startDir)));

    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), process,
        [process, callbackPtr](int exitCode, QProcess::ExitStatus exitStatus) mutable {
            QString error;
            QStringList folders;
            if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                folders = linesFromProcessOutput(process);
            } else {
                const QString stderrText = QString::fromUtf8(process->readAllStandardError()).trimmed();
                error = QString("powershell завершился с кодом %1%2")
                    .arg(exitCode)
                    .arg(stderrText.isEmpty() ? QString() : QString(": %1").arg(stderrText));
            }

            if (*callbackPtr)
                (*callbackPtr)(folders, error);
            process->deleteLater();
        });

    QObject::connect(process, &QProcess::errorOccurred, process,
        [process, callbackPtr](QProcess::ProcessError) mutable {
            if (*callbackPtr)
                (*callbackPtr)({}, "Не удалось запустить powershell.exe: " + process->errorString());
            process->deleteLater();
        });

    process->start("powershell.exe", {
        "-NoProfile",
        "-STA",
        "-ExecutionPolicy", "Bypass",
        "-Command", script
    });
#else
    if (callback)
        callback({}, "Native folder dialog is not implemented for this platform in the QML-only build.");
#endif
}
