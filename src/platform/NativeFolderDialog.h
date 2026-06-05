#ifndef NATIVEFOLDERDIALOG_H
#define NATIVEFOLDERDIALOG_H

#include <functional>

#include <QString>
#include <QStringList>

using FolderDialogCallback = std::function<void(const QStringList& folders, const QString& error)>;

void openNativeFolderDialog(const QString& title, const QString& startDir, FolderDialogCallback callback);

#endif // NATIVEFOLDERDIALOG_H
