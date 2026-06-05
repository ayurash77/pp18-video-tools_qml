import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import "components"

Window {
    id: playerWindow
    width: 1320
    height: 790
    minimumWidth: 760
    minimumHeight: 520
    title: playerTitle.length > 0
           ? ("PP18 Video Tools - " + playerTitle + (activeCachedPlayback ? " (cached preview)" : ""))
           : "PP18 Video Tools - Player"
    color: "#1e2028"

    property string initialPath: ""
    property string playerTitle: ""
    property string appFamily: ""
    property string monoFamily: "Monospace"
    property var versions: []
    property string activePath: initialPath
    property string comparePath: ""
    property var hiddenVersionKeys: ({})
    property int fps: 25
    property real zoom: 1.0
    property string zoomMode: "fit"
    property bool infoVisible: false
    property bool versionsVisible: true
    property bool cachePlaybackEnabled: true
    readonly property bool activeCachedPlayback: cachePlaybackEnabled
                                                 && appController.mediaCacheSizeText.length >= 0
                                                 && appController.cachePreviewExists(activePath)
    property bool splitEnabled: false
    property real splitPercent: 50
    property real splitHandleYPercent: 50
    property int splitHandleWidth: 12
    property int splitCrossHeight: 160
    property int splitCrossWidth: 20
    property real panX: 0
    property real panY: 0
    property bool splitDragging: false
    property bool initialWindowSized: false
    property bool playbackWanted: true
    property int lastPrimaryPosition: 0
    property int pendingPrimaryPosition: 0
    property bool pendingPrimaryPlaying: false
    property bool pendingPrimaryRestore: false
    property bool primarySwitchingSource: false
    property bool primaryUsingAlternate: false
    property bool primaryPreparingAlternate: false
    property bool primaryAwaitingResumeAfterSeek: false
    property int primaryResumeAttempts: 0
    property int pendingComparePosition: 0
    property bool pendingComparePlaying: false
    property bool pendingCompareRestore: false
    property bool compareSwitchingSource: false
    property var playbackSpeeds: [0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75, 2]
    signal requestVersions(string path)

    function fileName(path) {
        if (!path)
            return ""
        const normalized = String(path).replace(/\\/g, "/")
        const index = normalized.lastIndexOf("/")
        return index >= 0 ? normalized.slice(index + 1) : normalized
    }

    function fileStem(path) {
        const name = fileName(path)
        const dot = name.lastIndexOf(".")
        const stem = dot > 0 ? name.slice(0, dot) : name
        const match = stem.match(/^(.*)_v(\d+)(?:__.*)?$/i)
        return match ? match[1] : stem
    }

    function versionLabel(path) {
        const name = fileName(path)
        const dot = name.lastIndexOf(".")
        const stem = dot > 0 ? name.slice(0, dot) : name
        const match = stem.match(/_v(\d+)(?:__.*)?$/i)
        return match ? ("v" + match[1]) : ""
    }

    function samePath(left, right) {
        return String(left || "").replace(/\\/g, "/").toLowerCase()
            === String(right || "").replace(/\\/g, "/").toLowerCase()
    }

    function pathKey(path) {
        return String(path || "").replace(/\\/g, "/").toLowerCase()
    }

    function isVersionHidden(path) {
        return hiddenVersionKeys[pathKey(path)] === true
    }

    function visibleVersionCount() {
        let count = 0
        for (let i = 0; i < versions.length; ++i) {
            if (!isVersionHidden(versions[i].path))
                ++count
        }
        return count
    }

    function firstVisibleVersionPath() {
        for (let i = 0; i < versions.length; ++i) {
            if (!isVersionHidden(versions[i].path))
                return versions[i].path
        }
        return activePath
    }

    function fileUrl(path) {
        if (!path)
            return ""
        return "file://" + encodeURI(path).replace(/#/g, "%23")
    }

    function playbackPath(path) {
        if (cachePlaybackEnabled && typeof appController !== "undefined")
            return appController.cachedPlaybackPath(path)
        return path
    }

    function isCachedPlayback(path) {
        return cachePlaybackEnabled
            && path
            && typeof appController !== "undefined"
            && appController.cachePreviewExists(path)
    }

    function playbackUrl(path) {
        return fileUrl(playbackPath(path))
    }

    function playbackSourceMatches(player, path) {
        return String(player.source || "") === playbackUrl(path)
    }

    function reloadPlaybackSources() {
        if (activePath.length > 0)
            setActivePath(activePath, false)
        if (splitEnabled && comparePath.length > 0)
            setComparePath(comparePath)
    }

    function mediaReady(status) {
        return status === MediaPlayer.LoadedMedia || status === MediaPlayer.BufferedMedia
    }

    function frameAt(positionMs) {
        return Math.max(0, Math.round((positionMs / 1000) * fps))
    }

    function totalFrames() {
        return frameAt(currentPrimaryPlayer().duration)
    }

    function padFrame(value) {
        const text = String(value)
        return text.length >= 3 ? text : ("000" + text).slice(-3)
    }

    function durationLabel(positionMs) {
        const seconds = Math.max(0, Math.round(positionMs / 1000))
        const minutes = Math.floor(seconds / 60)
        const rest = seconds % 60
        return minutes + ":" + (rest < 10 ? "0" : "") + rest
    }

    function currentPrimaryOutput() {
        return primaryUsingAlternate ? primaryVideoOutputB : primaryVideoOutputA
    }

    function videoSizeFromOutput(output) {
        if (output.sourceRect.width > 0 && output.sourceRect.height > 0)
            return { width: output.sourceRect.width, height: output.sourceRect.height }
        if (output.videoSink.videoSize.width > 0 && output.videoSink.videoSize.height > 0)
            return { width: output.videoSink.videoSize.width, height: output.videoSink.videoSize.height }
        return { width: Math.max(1, stage.width), height: Math.max(1, stage.height) }
    }

    function hasRealVideoSize(output) {
        return (output.sourceRect.width > 0 && output.sourceRect.height > 0)
            || (output.videoSink.videoSize.width > 0 && output.videoSink.videoSize.height > 0)
    }

    function videoPixelWidth() {
        return videoSizeFromOutput(currentPrimaryOutput()).width
    }

    function videoPixelHeight() {
        return videoSizeFromOutput(currentPrimaryOutput()).height
    }

    function videoDisplayWidth() {
        return videoPixelWidth() * zoom
    }

    function videoDisplayHeight() {
        return videoPixelHeight() * zoom
    }

    function fitZoom() {
        if (stage.width <= 0 || stage.height <= 0)
            return zoom
        return Math.max(0.05, Math.min(4.0, Math.min(stage.width / videoPixelWidth(), stage.height / videoPixelHeight())))
    }

    function applyFitZoom() {
        if (zoomMode === "fit") {
            zoom = fitZoom()
            panX = 0
            panY = 0
        }
    }

    function stageVideoX(clipX) {
        return -clipX + (stage.width - videoDisplayWidth()) / 2 + panX
    }

    function stageVideoY() {
        return (stage.height - videoDisplayHeight()) / 2 + panY
    }

    function setZoom(nextZoom) {
        zoomMode = "manual"
        zoom = Math.max(0.05, Math.min(4.0, nextZoom))
    }

    function setFitZoom() {
        zoomMode = "fit"
        zoom = fitZoom()
        panX = 0
        panY = 0
    }

    function zoomStep(direction) {
        const step = zoom < 1.0 ? 0.05 : 0.10
        setZoom(zoom + direction * step)
    }

    function zoomLabel() {
        const percent = Math.round(zoom * 100)
        return zoomMode === "fit" ? ("Fit " + percent + "%") : (percent + "%")
    }

    function splitMarginPercent() {
        if (stage.width <= 0)
            return 0
        return Math.min(50, (splitHandleWidth / 2) / stage.width * 100)
    }

    function clampSplitPercent(value) {
        const margin = splitMarginPercent()
        return Math.max(margin, Math.min(100 - margin, value))
    }

    function setSplitFromStageX(x) {
        if (stage.width <= 0)
            return
        splitPercent = clampSplitPercent(x / stage.width * 100)
    }

    function splitYMarginPercent() {
        if (stage.height <= 0)
            return 0
        return Math.min(50, (splitCrossHeight / 2) / stage.height * 100)
    }

    function clampSplitHandleYPercent(value) {
        const margin = splitYMarginPercent()
        return Math.max(margin, Math.min(100 - margin, value))
    }

    function setSplitFromStageY(y) {
        if (stage.height <= 0)
            return
        splitHandleYPercent = clampSplitHandleYPercent(y / stage.height * 100)
    }

    function splitHandleCenterY() {
        return stage.height * splitHandleYPercent / 100
    }

    function handlePrimaryVideoGeometryChanged() {
        resizeWindowForVideoDefault()
        applyFitZoom()
    }

    function resizeWindowForVideoDefault() {
        if (initialWindowSized || !visible)
            return

        if (!hasRealVideoSize(currentPrimaryOutput()) || stage.width <= 0 || stage.height <= 0) {
            initialWindowResizeRetry.restart()
            return
        }

        const videoW = videoPixelWidth()
        const videoH = videoPixelHeight()
        const chromeW = Math.max(0, width - stage.width)
        const chromeH = Math.max(0, height - stage.height)
        const requestedW = Math.ceil(videoW + chromeW)
        const requestedH = Math.ceil(videoH + chromeH)
        const available = screen ? screen.availableGeometry : Qt.rect(x, y, requestedW, requestedH)
        const maxW = Math.max(minimumWidth, available.width - 40)
        const maxH = Math.max(minimumHeight, available.height - 40)
        const nextWidth = Math.max(minimumWidth, Math.min(requestedW, maxW))
        const nextHeight = Math.max(minimumHeight, Math.min(requestedH, maxH))

        if (requestedW <= maxW && requestedH <= maxH) {
            width = nextWidth
            height = nextHeight
            zoomMode = "manual"
            zoom = 1.0
        } else {
            width = nextWidth
            height = nextHeight
            zoomMode = "fit"
            zoom = Math.max(0.05, Math.min(1.0, Math.min((nextWidth - chromeW) / videoW, (nextHeight - chromeH) / videoH)))
        }

        panX = 0
        panY = 0
        x = Math.round(available.x + (available.width - width) / 2)
        y = Math.round(available.y + (available.height - height) / 2)
        initialWindowSized = true
    }

    function versionIndex(path) {
        for (let i = 0; i < versions.length; ++i) {
            if (samePath(versions[i].path, path))
                return i
        }
        return -1
    }

    function versionAt(index) {
        return index >= 0 && index < versions.length ? versions[index] : null
    }

    function clampedPosition(player, position) {
        return Math.max(0, Math.min(player.duration > 0 ? player.duration : position, position))
    }

    function currentPrimaryPlayer() {
        return primaryUsingAlternate ? alternatePrimaryPlayer : primaryPlayer
    }

    function standbyPrimaryPlayer() {
        return primaryUsingAlternate ? primaryPlayer : alternatePrimaryPlayer
    }

    function restoringPrimaryPlayer() {
        return primaryPreparingAlternate ? standbyPrimaryPlayer() : currentPrimaryPlayer()
    }

    function finishPrimarySwitch() {
        if (!primaryPreparingAlternate)
            return

        const oldPlayer = currentPrimaryPlayer()
        primaryUsingAlternate = !primaryUsingAlternate
        primaryPreparingAlternate = false
        primarySwitchingSource = false
        primaryAwaitingResumeAfterSeek = false
        releasePrimarySwitchGuard.stop()
        lastPrimaryPosition = currentPrimaryPlayer().position
        oldPlayer.pause()
    }

    function handlePrimaryPositionChanged(player, position) {
        if (primaryPreparingAlternate && player !== standbyPrimaryPlayer())
            return

        if (primarySwitchingSource) {
            if (position >= Math.max(0, pendingPrimaryPosition - 40)) {
                primarySwitchingSource = false
                lastPrimaryPosition = position
            }
        } else if (!pendingPrimaryRestore && player === currentPrimaryPlayer()) {
            lastPrimaryPosition = position
        }

        if (primaryAwaitingResumeAfterSeek && player === restoringPrimaryPlayer()
                && position >= Math.max(0, pendingPrimaryPosition - 40)) {
            primaryAwaitingResumeAfterSeek = false
            primaryResumeAfterPlay.stop()
            if (playbackWanted && player.playbackState !== MediaPlayer.PlayingState)
                player.play()
            if (primaryPreparingAlternate)
                finishPrimarySwitch()
        }

        if (player === currentPrimaryPlayer() && splitEnabled && comparePath.length > 0
                && player.playbackState === MediaPlayer.PlayingState
                && Math.abs(comparePlayer.position - position) > 80) {
            comparePlayer.position = position
        }
    }

    function restorePrimary() {
        const player = restoringPrimaryPlayer()
        if (!pendingPrimaryRestore || !mediaReady(player.mediaStatus))
            return
        const target = clampedPosition(player, pendingPrimaryPosition)
        if (target > 0 && !player.seekable) {
            primaryRestoreTick.restart()
            return
        }
        pausePrimaryAfterDecode.stop()
        primaryResumeAfterPlay.stop()
        primaryResumeWatchdog.stop()
        primaryRestoreTick.stop()
        player.playbackRate = speedCombo.currentValue || 1
        pendingPrimaryRestore = false

        if (pendingPrimaryPlaying) {
            playbackWanted = true
            player.position = target
            lastPrimaryPosition = target
            primaryAwaitingResumeAfterSeek = target > 0
            primaryResumeAttempts = 0
            if (primaryAwaitingResumeAfterSeek)
                primaryResumeAfterPlay.restart()
            else {
                player.play()
                if (primaryPreparingAlternate)
                    finishPrimarySwitch()
            }
            primaryResumeWatchdog.restart()
        } else {
            primaryAwaitingResumeAfterSeek = false
            player.position = target
            lastPrimaryPosition = target
            playbackWanted = false
            player.play()
            pausePrimaryAfterDecode.restart()
        }
        releasePrimarySwitchGuard.restart()
    }

    function restoreCompare() {
        if (!pendingCompareRestore || !mediaReady(comparePlayer.mediaStatus))
            return
        pauseCompareAfterDecode.stop()
        compareRestoreTick.stop()
        const target = clampedPosition(comparePlayer, pendingComparePosition)
        comparePlayer.position = target
        comparePlayer.play()
        comparePlayer.playbackRate = currentPrimaryPlayer().playbackRate
        pendingCompareRestore = false

        if (!pendingComparePlaying)
            pauseCompareAfterDecode.restart()
        releaseCompareSwitchGuard.restart()
    }

    function setActivePath(path, resetVersions) {
        const currentPlayer = currentPrimaryPlayer()
        if (samePath(path, activePath) && currentPlayer.source.toString().length > 0
                && playbackSourceMatches(currentPlayer, path)) {
            if (resetVersions)
                requestVersions(path)
            return
        }

        pausePrimaryAfterDecode.stop()
        primaryResumeAfterPlay.stop()
        primaryResumeWatchdog.stop()
        primaryAwaitingResumeAfterSeek = false
        primaryRestoreTick.stop()
        const hasCurrentSource = currentPlayer.source.toString().length > 0
        pendingPrimaryPosition = hasCurrentSource ? (currentPlayer.position > 0 ? currentPlayer.position : lastPrimaryPosition) : 0
        pendingPrimaryPlaying = hasCurrentSource ? (playbackWanted || currentPlayer.playbackState === MediaPlayer.PlayingState) : true
        pendingPrimaryRestore = true
        primarySwitchingSource = true
        primaryPreparingAlternate = hasCurrentSource
        activePath = path
        const targetPlayer = hasCurrentSource ? standbyPrimaryPlayer() : currentPlayer
        targetPlayer.stop()
        targetPlayer.source = playbackUrl(path)
        if (!splitEnabled)
            comparePath = ""
        if (resetVersions)
            requestVersions(path)
    }

    function setComparePath(path) {
        if (samePath(path, comparePath) && comparePlayer.source.toString().length > 0
                && playbackSourceMatches(comparePlayer, path))
            return

        pauseCompareAfterDecode.stop()
        compareRestoreTick.stop()
        const currentPlayer = currentPrimaryPlayer()
        pendingComparePosition = currentPlayer.position
        pendingComparePlaying = splitEnabled && (playbackWanted || currentPlayer.playbackState === MediaPlayer.PlayingState)
        pendingCompareRestore = true
        compareSwitchingSource = true
        comparePath = path
        comparePlayer.source = playbackUrl(path)
    }

    function hideVersionsAbove(index) {
        const next = Object.assign({}, hiddenVersionKeys)
        for (let i = 0; i < versions.length; ++i) {
            if (i < index)
                next[pathKey(versions[i].path)] = true
        }
        if (index >= 0 && index < versions.length)
            delete next[pathKey(versions[index].path)]
        hiddenVersionKeys = next
    }

    function applyInitialVersionVisibility() {
        const index = versionIndex(activePath)
        const next = {}
        if (index >= 0) {
            for (let i = 0; i < index; ++i)
                next[pathKey(versions[i].path)] = true
        }
        hiddenVersionKeys = next
    }

    function activateVersion(path, updateHiddenVersions) {
        if (updateHiddenVersions)
            hideVersionsAbove(versionIndex(path))
        setActivePath(path, false)
    }

    function toggleVersionVisibility(path) {
        const key = pathKey(path)
        const hidden = hiddenVersionKeys[key] === true
        const next = Object.assign({}, hiddenVersionKeys)

        if (hidden) {
            delete next[key]
            hiddenVersionKeys = next
            setActivePath(path, false)
            return
        }

        if (visibleVersionCount() <= 1)
            return

        next[key] = true
        hiddenVersionKeys = next

        if (samePath(path, activePath))
            setActivePath(firstVisibleVersionPath(), false)
    }

    function reload(path) {
        splitEnabled = false
        comparePath = ""
        hiddenVersionKeys = ({})
        initialWindowSized = false
        panX = 0
        panY = 0
        zoomMode = "manual"
        zoom = 1.0
        setActivePath(path, true)
    }

    function togglePlayback() {
        const player = currentPrimaryPlayer()
        pausePrimaryAfterDecode.stop()
        pauseCompareAfterDecode.stop()
        primaryResumeAfterPlay.stop()
        primaryResumeWatchdog.stop()
        primaryAwaitingResumeAfterSeek = false
        primaryRestoreTick.stop()
        compareRestoreTick.stop()
        pendingPrimaryRestore = false
        pendingCompareRestore = false
        if (player.playbackState === MediaPlayer.PlayingState) {
            playbackWanted = false
            player.pause()
            comparePlayer.pause()
            return
        }
        playbackWanted = true
        if (splitEnabled && comparePath.length > 0) {
            comparePlayer.position = player.position
            comparePlayer.playbackRate = player.playbackRate
            comparePlayer.play()
        }
        player.play()
    }

    function seek(positionMs) {
        const player = currentPrimaryPlayer()
        const next = Math.max(0, Math.min(player.duration, positionMs))
        player.position = next
        lastPrimaryPosition = next
        if (splitEnabled && comparePath.length > 0)
            comparePlayer.position = next
    }

    function stepFrame(direction) {
        playbackWanted = false
        pausePrimaryAfterDecode.stop()
        pauseCompareAfterDecode.stop()
        primaryResumeAfterPlay.stop()
        primaryResumeWatchdog.stop()
        primaryAwaitingResumeAfterSeek = false
        primaryRestoreTick.stop()
        compareRestoreTick.stop()
        pendingPrimaryRestore = false
        pendingCompareRestore = false
        currentPrimaryPlayer().pause()
        comparePlayer.pause()
        seek(currentPrimaryPlayer().position + direction * (1000 / fps))
    }

    function toggleSplit() {
        splitEnabled = !splitEnabled
        if (splitEnabled && comparePath.length === 0)
            setComparePath(activePath)
        if (!splitEnabled)
            comparePlayer.pause()
    }

    Timer {
        id: pausePrimaryAfterDecode
        interval: 140
        repeat: false
        onTriggered: {
            if (!playbackWanted) {
                const player = restoringPrimaryPlayer()
                player.position = clampedPosition(player, pendingPrimaryPosition)
                player.pause()
                if (primaryPreparingAlternate)
                    finishPrimarySwitch()
                lastPrimaryPosition = currentPrimaryPlayer().position
                primarySwitchingSource = false
            }
        }
    }

    Timer {
        id: pauseCompareAfterDecode
        interval: 140
        repeat: false
        onTriggered: {
            if (!playbackWanted) {
                comparePlayer.position = clampedPosition(comparePlayer, pendingComparePosition)
                comparePlayer.pause()
                compareSwitchingSource = false
            }
        }
    }

    Timer {
        id: releasePrimarySwitchGuard
        interval: 320
        repeat: false
        onTriggered: {
            primarySwitchingSource = false
            const player = currentPrimaryPlayer()
            if (pendingPrimaryPosition <= 0 || player.position >= pendingPrimaryPosition - 40)
                lastPrimaryPosition = player.position
        }
    }

    Timer {
        id: primaryResumeAfterPlay
        interval: 35
        repeat: false
        onTriggered: {
            if (!playbackWanted)
                return

            primaryAwaitingResumeAfterSeek = false
            const player = restoringPrimaryPlayer()
            if (player.playbackState !== MediaPlayer.PlayingState)
                player.play()
        }
    }

    Timer {
        id: primaryResumeWatchdog
        interval: 120
        repeat: true
        onTriggered: {
            if (!playbackWanted) {
                stop()
                return
            }

            const player = primaryPreparingAlternate ? standbyPrimaryPlayer() : currentPrimaryPlayer()
            if (player.playbackState !== MediaPlayer.PlayingState)
                player.play()
            if (primaryPreparingAlternate && player.position >= Math.max(0, pendingPrimaryPosition - 40))
                finishPrimarySwitch()

            primaryResumeAttempts += 1
            if (primaryResumeAttempts >= 8) {
                if (primaryPreparingAlternate) {
                    player.position = clampedPosition(player, pendingPrimaryPosition)
                    finishPrimarySwitch()
                }
                stop()
            }
        }
    }

    Timer {
        id: releaseCompareSwitchGuard
        interval: 320
        repeat: false
        onTriggered: compareSwitchingSource = false
    }

    Timer {
        id: primaryRestoreTick
        interval: 80
        repeat: false
        onTriggered: {
            if (!pendingPrimaryRestore)
                return

            restorePrimary()
        }
    }

    Timer {
        id: compareRestoreTick
        interval: 80
        repeat: false
        onTriggered: {
            if (!pendingCompareRestore)
                return

            const target = clampedPosition(comparePlayer, pendingComparePosition)
            if (comparePlayer.playbackRate !== currentPrimaryPlayer().playbackRate)
                comparePlayer.playbackRate = currentPrimaryPlayer().playbackRate
            comparePlayer.position = target
            pendingCompareRestore = false
            if (pendingComparePlaying) {
                comparePlayer.play()
            } else {
                comparePlayer.pause()
            }
        }
    }

    Timer {
        interval: 120
        repeat: true
        running: playbackWanted && splitEnabled && comparePath.length > 0 && currentPrimaryPlayer().playbackState === MediaPlayer.PlayingState
        onTriggered: {
            const player = currentPrimaryPlayer()
            if (Math.abs(comparePlayer.position - player.position) > 60)
                comparePlayer.position = player.position
            if (comparePlayer.playbackState !== MediaPlayer.PlayingState)
                comparePlayer.play()
        }
    }

    onVisibleChanged: {
        if (visible && initialPath.length > 0) {
            reload(initialPath)
            initialWindowResizeRetry.restart()
        } else if (!visible) {
            primaryPlayer.stop()
            alternatePrimaryPlayer.stop()
            comparePlayer.stop()
            playbackWanted = false
            primaryRestoreTick.stop()
            compareRestoreTick.stop()
            primaryResumeAfterPlay.stop()
            primaryResumeWatchdog.stop()
            primaryAwaitingResumeAfterSeek = false
            releasePrimarySwitchGuard.stop()
            releaseCompareSwitchGuard.stop()
            pendingPrimaryRestore = false
            pendingCompareRestore = false
            primarySwitchingSource = false
            primaryPreparingAlternate = false
            compareSwitchingSource = false
        }
    }

    onInitialPathChanged: {
        if (visible && initialPath.length > 0)
            reload(initialPath)
    }

    onVersionsChanged: applyInitialVersionVisibility()
    onCachePlaybackEnabledChanged: {
        if (visible)
            reloadPlaybackSources()
    }

    Shortcut { sequence: "Space"; onActivated: togglePlayback() }
    Shortcut { sequence: "Left"; onActivated: stepFrame(-1) }
    Shortcut { sequence: "Right"; onActivated: stepFrame(1) }

    Timer {
        id: initialWindowResizeRetry
        interval: 80
        repeat: false
        onTriggered: resizeWindowForVideoDefault()
    }

    MediaPlayer {
        id: primaryPlayer
        audioOutput: AudioOutput { volume: 1.0 }
        videoOutput: primaryVideoOutputA
        loops: MediaPlayer.Infinite
        onMediaStatusChanged: restorePrimary()
        onSeekableChanged: restorePrimary()
        onPositionChanged: handlePrimaryPositionChanged(primaryPlayer, position)
    }

    MediaPlayer {
        id: alternatePrimaryPlayer
        audioOutput: AudioOutput { muted: true }
        videoOutput: primaryVideoOutputB
        loops: primaryPlayer.loops
        playbackRate: primaryPlayer.playbackRate
        onMediaStatusChanged: restorePrimary()
        onSeekableChanged: restorePrimary()
        onPositionChanged: handlePrimaryPositionChanged(alternatePrimaryPlayer, position)
    }

    MediaPlayer {
        id: comparePlayer
        audioOutput: AudioOutput { muted: true }
        videoOutput: compareVideoOutput
        loops: currentPrimaryPlayer().loops
        playbackRate: currentPrimaryPlayer().playbackRate
        onMediaStatusChanged: restoreCompare()
    }

    Rectangle {
        anchors.fill: parent
        color: "#1e2028"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 12

            Rectangle {
                visible: versionsVisible
                Layout.preferredWidth: 182
                Layout.fillHeight: true
                radius: 6
                color: "#24273080"
                border.color: "#343946"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Label {
                        text: "VERSIONS"
                        color: "#8d95aa"
                        font.family: appFamily
                        font.pixelSize: 11
                        font.bold: true
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        leftPadding: 12
                        verticalAlignment: Text.AlignVCenter
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: versions
                        delegate: Rectangle {
                            property bool hidden: isVersionHidden(modelData.path)
                            property bool active: samePath(modelData.path, activePath)
                            width: ListView.view.width - 16
                            height: 31
                            x: 8
                            radius: 4
                            opacity: hidden ? 0.45 : 1.0
                            color: active && !hidden ? "#26384d" : versionMouse.containsMouse ? "#303542" : "transparent"
                            border.color: active && !hidden ? "#3b82f6" : "#303541"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 7
                                spacing: 7

                                Label {
                                    text: "o"
                                    color: hidden ? "#687086" : "#08aeea"
                                    font.family: monoFamily
                                    font.pixelSize: 11
                                    font.bold: true
                                }

                                Label {
                                    text: fileStem(modelData.path)
                                    color: hidden ? "#687086" : "#8d95aa"
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Label {
                                    text: modelData.label || versionLabel(modelData.path)
                                    color: hidden ? "#8d95aa" : "#08aeea"
                                    font.family: monoFamily
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }

                            MouseArea {
                                id: versionMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: toggleVersionVisibility(modelData.path)
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: 6
                    color: "#24273080"
                    border.color: "#343946"

                    GridLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        columns: 3
                        columnSpacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                text: "A"
                                color: "#8d95aa"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24
                                background: Rectangle {
                                    radius: 3
                                    color: "#20232c"
                                    border.color: "#343946"
                                }
                            }

                            Label {
                                text: "---"
                                color: "#8d95aa"
                                font.family: monoFamily
                                font.pixelSize: 11
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                Layout.preferredWidth: 74
                                Layout.preferredHeight: 24
                                background: Rectangle {
                                    radius: 3
                                    color: "#20232c"
                                    border.color: "#303541"
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Label {
                                text: fileStem(activePath)
                                color: "#d9deea"
                                font.pixelSize: 13
                                font.bold: true
                                elide: Text.ElideLeft
                                horizontalAlignment: Text.AlignRight
                                Layout.maximumWidth: 240
                            }

                            PlayerCombo {
                                visible: versions.length > 1
                                fontFamily: monoFamily
                                model: versions
                                textRole: "label"
                                currentIndex: Math.max(0, versionIndex(activePath))
                                Layout.preferredWidth: 74
                                Layout.preferredHeight: 26
                                onActivated: {
                                    const version = versionAt(index)
                                    if (version)
                                        activateVersion(version.path, true)
                                }
                            }
                        }

                        PlayerButton {
                            // text: "A/B"
                            iconSource: "qrc:/icons/split-horizontal"
                            checkable: true
                            checked: splitEnabled
                            buttonWidth: 38
                            pill: false
                            onClicked: toggleSplit()
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            opacity: splitEnabled ? 1.0 : 0.35
                            spacing: 8

                            PlayerCombo {
                                enabled: splitEnabled && versions.length > 1
                                visible: versions.length > 1
                                fontFamily: monoFamily
                                model: versions
                                textRole: "label"
                                currentIndex: Math.max(0, versionIndex(comparePath.length > 0 ? comparePath : activePath))
                                Layout.preferredWidth: 74
                                Layout.preferredHeight: 26
                                onActivated: {
                                    const version = versionAt(index)
                                    if (version)
                                        setComparePath(version.path)
                                }
                            }

                            Label {
                                text: versionLabel(comparePath.length > 0 ? comparePath : activePath)
                                visible: versions.length <= 1
                                color: "#8d95aa"
                                font.family: monoFamily
                                Layout.preferredWidth: 50
                            }

                            Label {
                                text: fileStem(comparePath.length > 0 ? comparePath : activePath)
                                color: "#d9deea"
                                font.pixelSize: 13
                                font.bold: true
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Item { Layout.fillWidth: true }

                            Label {
                                text: "---"
                                color: "#8d95aa"
                                font.family: monoFamily
                                font.pixelSize: 11
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                Layout.preferredWidth: 74
                                Layout.preferredHeight: 24
                                background: Rectangle {
                                    radius: 3
                                    color: "#20232c"
                                    border.color: "#303541"
                                }
                            }

                            Label {
                                text: "B"
                                color: "#8d95aa"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24
                                background: Rectangle {
                                    radius: 3
                                    color: "#20232c"
                                    border.color: "#343946"
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    id: stage
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 6
                    color: "#07080c"
                    border.color: "#343946"
                    clip: true
                    onWidthChanged: {
                        splitPercent = clampSplitPercent(splitPercent)
                        splitHandleYPercent = clampSplitHandleYPercent(splitHandleYPercent)
                        applyFitZoom()
                    }
                    onHeightChanged: {
                        splitHandleYPercent = clampSplitHandleYPercent(splitHandleYPercent)
                        applyFitZoom()
                    }

                    Item {
                        id: leftLayer
                        x: 0
                        y: 0
                        width: splitEnabled ? stage.width * splitPercent / 100 : stage.width
                        height: stage.height
                        clip: true

                        VideoOutput {
                            id: primaryVideoOutputA
                            x: stageVideoX(leftLayer.x)
                            y: stageVideoY()
                            width: videoDisplayWidth()
                            height: videoDisplayHeight()
                            opacity: primaryUsingAlternate ? 0 : 1
                            visible: true
                            z: primaryUsingAlternate ? 0 : 1
                            fillMode: VideoOutput.PreserveAspectFit
                            endOfStreamPolicy: VideoOutput.KeepLastFrame
                            onSourceRectChanged: handlePrimaryVideoGeometryChanged()
                        }

                        VideoOutput {
                            id: primaryVideoOutputB
                            x: stageVideoX(leftLayer.x)
                            y: stageVideoY()
                            width: videoDisplayWidth()
                            height: videoDisplayHeight()
                            opacity: primaryUsingAlternate ? 1 : 0
                            visible: true
                            z: primaryUsingAlternate ? 1 : 0
                            fillMode: VideoOutput.PreserveAspectFit
                            endOfStreamPolicy: VideoOutput.KeepLastFrame
                            onSourceRectChanged: handlePrimaryVideoGeometryChanged()
                        }
                    }

                    Item {
                        id: rightLayer
                        visible: splitEnabled
                        x: leftLayer.width
                        y: 0
                        width: stage.width - leftLayer.width
                        height: stage.height
                        clip: true

                        VideoOutput {
                            id: compareVideoOutput
                            x: stageVideoX(rightLayer.x)
                            y: stageVideoY()
                            width: videoDisplayWidth()
                            height: videoDisplayHeight()
                            fillMode: VideoOutput.PreserveAspectFit
                            endOfStreamPolicy: VideoOutput.KeepLastFrame
                            onSourceRectChanged: applyFitZoom()
                        }
                    }

                    Connections {
                        target: primaryVideoOutputA.videoSink
                        function onVideoSizeChanged() {
                            handlePrimaryVideoGeometryChanged()
                        }
                    }

                    Connections {
                        target: primaryVideoOutputB.videoSink
                        function onVideoSizeChanged() {
                            handlePrimaryVideoGeometryChanged()
                        }
                    }

                    Rectangle {
                        visible: splitEnabled
                        z: 10
                        x: Math.max(0, Math.min(stage.width - width, leftLayer.width - width / 2))
                        y: 0
                        width: splitHandleWidth
                        height: parent.height
                        id: splitHandle
                        color: "transparent"

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            y: splitHandleCenterY() - height / 2
                            width: splitDragging ? 2 : 1
                            height: splitCrossHeight
                            radius: 1
                            color: splitDragging ? "#88ddff" : "#ffffff"
                        }

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            y: splitHandleCenterY() - height / 2
                            width: splitCrossWidth
                            height: 1
                            radius: 1
                            color: splitDragging ? "#88ddff" : "#ffffff"
                        }



                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.SizeAllCursor
                            onPressed: {
                                splitDragging = true
                                setSplitFromStageX(mouse.x + parent.x)
                                setSplitFromStageY(mouse.y)
                            }
                            onReleased: splitDragging = false
                            onPositionChanged: {
                                if (pressed) {
                                    setSplitFromStageX(mouse.x + parent.x)
                                    setSplitFromStageY(mouse.y)
                                }
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        z: 1
                        acceptedButtons: Qt.MiddleButton
                        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.ArrowCursor
                        property real startX: 0
                        property real startY: 0
                        property real baseX: 0
                        property real baseY: 0
                        onPressed: {
                            startX = mouse.x
                            startY = mouse.y
                            baseX = panX
                            baseY = panY
                        }
                        onPositionChanged: {
                            if (pressed) {
                                panX = baseX + mouse.x - startX
                                panY = baseY + mouse.y - startY
                            }
                        }
                        onWheel: function(wheel) {
                            zoomStep(wheel.angleDelta.y > 0 ? 1 : -1)
                            wheel.accepted = true
                        }
                    }

                    Rectangle {
                        visible: infoVisible
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: 12
                        width: Math.min(parent.width - 24, 560)
                        height: infoText.implicitHeight + 18
                        radius: 6
                        color: "#99000000"
                        border.color: "#22ffffff"
                        Label {
                            id: infoText
                            anchors.fill: parent
                            anchors.margins: 9
                            text: splitEnabled ? ("A: " + activePath + "\nB: " + comparePath) : activePath
                            color: "#ffffff"
                            font.family: monoFamily
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 72
                    radius: 6
                    color: "#24273080"
                    border.color: "#343946"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        anchors.topMargin: 8
                        anchors.bottomMargin: 9
                        spacing: 6

                        Slider {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 18
                            from: 0
                            to: Math.max(1, currentPrimaryPlayer().duration)
                            value: currentPrimaryPlayer().position
                            onMoved: seek(value)
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            PlayerButton {
                                text: currentPrimaryPlayer().playbackState === MediaPlayer.PlayingState ? "||" : ""
                                iconSource: currentPrimaryPlayer().playbackState === MediaPlayer.PlayingState ? "" : "qrc:/icons/play"
                                onClicked: togglePlayback()
                            }
                            PlayerButton { text: "<"; onClicked: stepFrame(-1) }
                            PlayerButton { text: ">"; onClicked: stepFrame(1) }
                            PlayerButton {
                                text: currentPrimaryPlayer().loops === MediaPlayer.Infinite ? "Loop" : "Once"
                                buttonWidth: 62
                                onClicked: {
                                    const nextLoops = currentPrimaryPlayer().loops === MediaPlayer.Infinite ? 1 : MediaPlayer.Infinite
                                    primaryPlayer.loops = nextLoops
                                    alternatePrimaryPlayer.loops = nextLoops
                                    comparePlayer.loops = nextLoops
                                }
                            }
                            PlayerCombo {
                                id: speedCombo
                                fontFamily: monoFamily
                                model: playbackSpeeds
                                currentIndex: 3
                                Layout.preferredWidth: 64
                                Layout.preferredHeight: 28
                                onActivated: {
                                    currentPrimaryPlayer().playbackRate = playbackSpeeds[index]
                                    standbyPrimaryPlayer().playbackRate = playbackSpeeds[index]
                                    comparePlayer.playbackRate = playbackSpeeds[index]
                                }
                            }
                            PlayerButton { text: "-"; onClicked: zoomStep(-1) }
                            PlayerButton {
                                id: zoomMenuButton
                                text: zoomLabel()
                                buttonWidth: zoomMode === "fit" ? 86 : 64
                                onClicked: zoomMenu.open()

                                Menu {
                                    id: zoomMenu

                                    MenuItem {
                                        text: "Fit"
                                        checkable: true
                                        checked: zoomMode === "fit"
                                        onTriggered: setFitZoom()
                                    }

                                    MenuItem {
                                        text: "25%"
                                        checkable: true
                                        checked: zoomMode === "manual" && Math.round(zoom * 100) === 25
                                        onTriggered: { setZoom(0.25); panX = 0; panY = 0 }
                                    }

                                    MenuItem {
                                        text: "50%"
                                        checkable: true
                                        checked: zoomMode === "manual" && Math.round(zoom * 100) === 50
                                        onTriggered: { setZoom(0.5); panX = 0; panY = 0 }
                                    }

                                    MenuItem {
                                        text: "100%"
                                        checkable: true
                                        checked: zoomMode === "manual" && Math.round(zoom * 100) === 100
                                        onTriggered: { setZoom(1.0); panX = 0; panY = 0 }
                                    }

                                    MenuItem {
                                        text: "150%"
                                        checkable: true
                                        checked: zoomMode === "manual" && Math.round(zoom * 100) === 150
                                        onTriggered: { setZoom(1.5); panX = 0; panY = 0 }
                                    }

                                    MenuItem {
                                        text: "200%"
                                        checkable: true
                                        checked: zoomMode === "manual" && Math.round(zoom * 100) === 200
                                        onTriggered: { setZoom(2.0); panX = 0; panY = 0 }
                                    }
                                }
                            }
                            PlayerButton { text: "+"; onClicked: zoomStep(1) }
                            PlayerButton { text: "i"; checkable: true; checked: infoVisible; onClicked: infoVisible = checked }
                            PlayerButton {
                                text: "Layers"
                                iconSource: versionsVisible ? "qrc:/icons/eye" : "qrc:/icons/eye-off"
                                checkable: true
                                checked: versionsVisible
                                buttonWidth: 82
                                onClicked: versionsVisible = checked
                            }
                            ToolbarButton {
                                variant: "icon"
                                color: cachePlaybackEnabled ? "success" : "secondary"
                                icon: !cachePlaybackEnabled
                                      ? "qrc:/icons/monitor-down"
                                      : appController.mediaCacheRunning
                                        ? "qrc:/icons/monitor-pause"
                                        : appController.cachePreviewExists(activePath)
                                          ? "qrc:/icons/monitor-check"
                                          : "qrc:/icons/monitor-down"
                                label: cachePlaybackEnabled ? "Проигрывать кеш preview" : "Проигрывать оригинал"
                                active: cachePlaybackEnabled
                                Layout.preferredWidth: AppStyle.toolbarButtonSize
                                Layout.preferredHeight: AppStyle.toolbarButtonSize
                                onClicked: {
                                    cachePlaybackEnabled = !cachePlaybackEnabled
                                    if (cachePlaybackEnabled) {
                                        if (!appController.mediaCacheEnabled)
                                            appController.mediaCacheEnabled = true
                                        appController.requestCachePreview(activePath)
                                        if (splitEnabled)
                                            appController.requestCachePreview(comparePath)
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Label {
                                text: durationLabel(currentPrimaryPlayer().position) + " " + padFrame(frameAt(currentPrimaryPlayer().position)) + "F / "
                                    + durationLabel(currentPrimaryPlayer().duration) + " " + padFrame(totalFrames()) + "F"
                                color: "#af8f5d"
                                font.family: monoFamily
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }
    }
}
