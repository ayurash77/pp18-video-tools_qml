#ifndef NATIVEFILEDIALOG_H
#define NATIVEFILEDIALOG_H

#include <functional>

#include <QString>
#include <QStringList>

using FileDialogCallback = std::function<void(const QStringList& files, const QString& error)>;

void openNativeFileDialog(const QString& title, const QString& startDir, FileDialogCallback callback);

#endif // NATIVEFILEDIALOG_H
