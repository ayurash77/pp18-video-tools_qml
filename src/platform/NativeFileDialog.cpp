#include "NativeFileDialog.h"

void openNativeFileDialog(const QString&, const QString&, FileDialogCallback callback)
{
    if (callback)
        callback({}, "Native file dialog is not implemented for this platform in the QML-only build.");
}
