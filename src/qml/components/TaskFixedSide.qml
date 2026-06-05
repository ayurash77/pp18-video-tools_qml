import QtQuick

TaskSide {
    id: fixedSide

    required property string fixedPath
    required property string fixedPreviewPath
    required property bool fixes
    required property bool fixedPreview
    required property bool fixedTelegram
    required property bool fixedExists
    required property bool fixedPreviewExists
    required property string fixedThumbnailUrl

    property string missingMeta: "---"

    readonly property bool fixedPlayable: fixedExists || fixedPreviewExists
    readonly property string fixedPlayablePath: fixedPreviewExists ? fixedPreviewPath : fixedPath

    signal openPlayerRequested(string path, string title)
    signal setActionRequested(string action, bool active)

    function baseName(pathValue) {
        if (!pathValue || pathValue.length === 0)
            return ""
        const normalized = String(pathValue).replace(/\\/g, "/")
        const index = normalized.lastIndexOf("/")
        return index >= 0 ? normalized.slice(index + 1) : normalized
    }

    function parentFolder(pathValue) {
        if (!pathValue || pathValue.length === 0)
            return ""
        const normalized = String(pathValue).replace(/\\/g, "/")
        const index = normalized.lastIndexOf("/")
        return index >= 0 ? normalized.slice(0, index) : ""
    }

    function openPlayer() {
        if (!fixedSide.fixedPlayable)
            return
        const title = fixedSide.fixedPreviewExists ? ("fix preview: " + fixedSide.baseName(fixedSide.fixedPreviewPath)) : ("fix: " + fixedSide.baseName(fixedSide.fixedPath))
        fixedSide.openPlayerRequested(fixedSide.fixedPlayablePath, title)
    }

    title: "FIX"
    showFixes: true
    fixesActive: fixes
    previewActive: fixedPreview
    telegramActive: fixedTelegram
    thumbUrl: fixedThumbnailUrl
    fallbackLabel: baseName(fixedPath)
    folderPath: parentFolder(fixedPath)
    primaryText: baseName(fixedPath)
    primaryExists: fixedExists
    primaryTone: fixesColor
    primaryRes: missingMeta
    primaryFrames: missingMeta
    primaryFps: missingMeta
    secondaryText: baseName(fixedPreviewPath)
    secondaryExists: fixedPreviewExists
    secondaryTone: previewColor
    secondaryRes: missingMeta
    secondaryFrames: missingMeta
    secondaryFps: missingMeta

    onPlay: openPlayer()
    onToggleFixes: {
        fixedSide.setActionRequested("fixes", !fixedSide.fixes)
        if (fixedSide.fixes && !fixedSide.fixedExists) {
            fixedSide.setActionRequested("fixedPreview", false)
            fixedSide.setActionRequested("fixedTelegram", false)
        }
    }
    onTogglePreview: {
        if (!fixedSide.fixedExists)
            fixedSide.setActionRequested("fixes", true)
        fixedSide.setActionRequested("fixedPreview", !fixedSide.fixedPreview)
        if (fixedSide.fixedPreview && !fixedSide.fixedPreviewExists)
            fixedSide.setActionRequested("fixedTelegram", false)
    }
    onToggleTelegram: {
        if (!fixedSide.fixedTelegram && !fixedSide.fixedExists)
            fixedSide.setActionRequested("fixes", true)
        fixedSide.setActionRequested("fixedTelegram", !fixedSide.fixedTelegram)
        if (!fixedSide.fixedTelegram && !fixedSide.fixedPreviewExists)
            fixedSide.setActionRequested("fixedPreview", true)
    }
}
