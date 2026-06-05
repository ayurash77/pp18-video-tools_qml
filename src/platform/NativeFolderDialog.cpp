#include "NativeFolderDialog.h"

void openNativeFolderDialog(const QString&, const QString&, FolderDialogCallback callback)
{
    if (callback)
        callback({}, "Native folder dialog is not implemented for this platform in the QML-only build.");
}
