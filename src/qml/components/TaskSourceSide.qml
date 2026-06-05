import QtQuick

TaskSide {
    id: sourceSide

    required property string sourcePath
    required property string fileName
    required property string folder
    required property string sourcePreviewPath
    required property bool sourcePreview
    required property bool sourceTelegram
    required property bool sourcePreviewExists
    required property string resolution
    required property string frames
    required property string fps
    required property string thumbnailUrl

    property string missingMeta: "---"

    signal openPlayerRequested(string path, string title)
    signal setActionRequested(string action, bool active)

    function baseName(pathValue) {
        if (!pathValue || pathValue.length === 0)
            return ""
        const normalized = String(pathValue).replace(/\\/g, "/")
        const index = normalized.lastIndexOf("/")
        return index >= 0 ? normalized.slice(index + 1) : normalized
    }

    function frameCell(value) {
        const text = String(value || sourceSide.missingMeta)
        return text.replace(/F$/, "")
    }

    function openPlayer() {
        sourceSide.openPlayerRequested(sourceSide.sourcePath, "src: " + sourceSide.fileName)
    }

    title: "SRC"
    previewActive: sourcePreview
    telegramActive: sourceTelegram
    thumbUrl: thumbnailUrl
    fallbackLabel: fileName
    folderPath: folder
    primaryText: baseName(sourcePath)
    primaryExists: true
    primaryTone: textColor
    primaryRes: resolution
    primaryFrames: frameCell(frames)
    primaryFps: fps
    secondaryText: baseName(sourcePreviewPath)
    secondaryExists: sourcePreviewExists
    secondaryTone: previewColor
    secondaryRes: sourcePreviewExists ? resolution : missingMeta
    secondaryFrames: sourcePreviewExists ? frameCell(frames) : missingMeta
    secondaryFps: sourcePreviewExists ? fps : missingMeta

    onPlay: openPlayer()
    onTogglePreview: {
        sourceSide.setActionRequested("srcPreview", !sourceSide.sourcePreview)
        if (sourceSide.sourcePreview && !sourceSide.sourcePreviewExists)
            sourceSide.setActionRequested("srcTelegram", false)
    }
    onToggleTelegram: {
        sourceSide.setActionRequested("srcTelegram", !sourceSide.sourceTelegram)
        if (!sourceSide.sourceTelegram && !sourceSide.sourcePreviewExists)
            sourceSide.setActionRequested("srcPreview", true)
    }
}
