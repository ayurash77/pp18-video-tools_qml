#include "NativeFileDialog.h"

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

void openNativeFileDialog(const QString& title, const QString& startDir, FileDialogCallback callback)
{
#ifdef Q_OS_WIN
    auto* process = new QProcess;
    auto callbackPtr = std::make_shared<FileDialogCallback>(std::move(callback));
    const QString script = QString(R"(
$dialogTitle = %1
$initialDirectory = %2
Add-Type -AssemblyName System.Windows.Forms
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$dialog = New-Object System.Windows.Forms.OpenFileDialog
$dialog.Title = $dialogTitle
$dialog.Multiselect = $true
$dialog.CheckFileExists = $true
$dialog.Filter = 'Video files|*.3g2;*.3gp;*.avi;*.dv;*.flv;*.m2ts;*.m4v;*.mkv;*.mov;*.mp4;*.mpeg;*.mpg;*.mts;*.mxf;*.ogv;*.ts;*.vob;*.webm;*.wmv|All files|*.*'
if ([System.IO.Directory]::Exists($initialDirectory)) {
    $dialog.InitialDirectory = $initialDirectory
}
if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
    foreach ($fileName in $dialog.FileNames) {
        Write-Output $fileName
    }
}
)")
        .arg(powerShellStringLiteral(title),
             powerShellStringLiteral(QDir::toNativeSeparators(startDir)));

    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), process,
        [process, callbackPtr](int exitCode, QProcess::ExitStatus exitStatus) mutable {
            QString error;
            QStringList files;
            if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                files = linesFromProcessOutput(process);
            } else {
                const QString stderrText = QString::fromUtf8(process->readAllStandardError()).trimmed();
                error = QString("powershell завершился с кодом %1%2")
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
        callback({}, "Native file dialog is not implemented for this platform in the QML-only build.");
#endif
}
