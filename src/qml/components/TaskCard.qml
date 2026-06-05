import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts

Rectangle {
    id: card

    required property int index
    required property string path
    required property string fileName
    required property string folder
    required property string fixedPath
    required property string sourcePreviewPath
    required property string fixedPreviewPath
    required property bool fixes
    required property bool srcPreview
    required property bool srcTelegram
    required property bool fixedPreview
    required property bool fixedTelegram
    required property bool fixedExists
    required property bool sourcePreviewExists
    required property bool fixedPreviewExists
    required property string status
    required property string resolution
    required property string frames
    required property string fps
    required property string thumbnailUrl
    required property string fixedThumbnailUrl

    property bool running: false
    property string missingMeta: "---"
    property string appFamily: Qt.application.font.family
    property string monoFamily: "Monospace"
    property color panelColor: "#24273080"
    property color sideColor: "#292d37"
    property color sideHoverColor: "#2e333f"
    property color thumbnailColor: "#2d313b"
    property color borderColor: "#343946"
    property color borderSoftColor: "#303541"
    property color textColor: "#d9deea"
    property color mutedColor: "#8d95aa"
    property color fixesColor: "#43bf83"
    property color previewColor: "#08aeea"
    property color telegramColor: "#2f6cf2"
    property color accentColor: "#af8f5d"

    readonly property bool sourceRunnable: card.srcPreview || card.srcTelegram
    readonly property bool fixedRunnable: card.fixes || card.fixedPreview || card.fixedTelegram
    property string contextSide: ""

    signal openPlayerRequested(string path, string title)
    signal runRequested(int fileIndex, string side)
    signal revealInFolderRequested(string path)
    signal openFileRequested(string path)
    signal removeRequested(int fileIndex)
    signal setFileActionRequested(int fileIndex, string action, bool active)

    width: ListView.view ? ListView.view.width : implicitWidth
    height: 82
    radius: 8
    color: panelColor
    border.color: borderColor

    function runtimeStatus(text) {
        const normalized = String(text || "").replace(/\n/g, " · ")
        return /^(Fixes|SRC Preview|SRC TG|FIX Preview|FIX TG)( · (Fixes|SRC Preview|SRC TG|FIX Preview|FIX TG))*$/.test(normalized) ? "" : normalized
    }

    Menu {
        id: sourceContextMenu
        width: 184
        padding: 3
        onAboutToHide: card.contextSide = ""
        background: Item {
            Rectangle {
                id: sourceMenuShadow
                anchors.fill: parent
                radius: 7
                color: "#292b33"
                visible: false
            }

            MultiEffect {
                anchors.fill: sourceMenuShadow
                source: sourceMenuShadow
                shadowEnabled: true
                shadowBlur: 0.8
                shadowColor: "black"
                shadowOpacity: 0.32
                shadowVerticalOffset: 2
            }

            Rectangle {
                anchors.fill: parent
                radius: 7
                color: "#292b33"
                opacity: 0.96
                border.color: Qt.lighter(color, 1.18)
            }
        }
        ContextMenuItem {
            text: "Проиграть src"
            fontFamily: card.appFamily
            onTriggered: sourceSide.openPlayer()
        }
        ContextMenuSeparator {}
        ContextMenuItem {
            text: "Запустить задачу"
            fontFamily: card.appFamily
            enabled: !card.running && card.sourceRunnable
            onTriggered: card.runRequested(card.index, "source")
        }
        ContextMenuSeparator {}
        ContextMenuItem {
            text: "Показать в папке"
            fontFamily: card.appFamily
            onTriggered: card.revealInFolderRequested(card.path)
        }
        ContextMenuItem {
            text: "Открыть"
            fontFamily: card.appFamily
            onTriggered: card.openFileRequested(card.path)
        }
        ContextMenuSeparator {}
        ContextMenuItem {
            text: "Убрать из списка"
            fontFamily: card.appFamily
            enabled: !card.running
            onTriggered: card.removeRequested(card.index)
        }
    }

    Menu {
        id: fixedContextMenu
        width: 184
        padding: 3
        onAboutToHide: card.contextSide = ""
        background: Item {
            Rectangle {
                id: fixedMenuShadow
                anchors.fill: parent
                radius: 7
                color: "#292b33"
                visible: false
            }

            MultiEffect {
                anchors.fill: fixedMenuShadow
                source: fixedMenuShadow
                shadowEnabled: true
                shadowBlur: 0.8
                shadowColor: "black"
                shadowOpacity: 0.32
                shadowVerticalOffset: 2
            }

            Rectangle {
                anchors.fill: parent
                radius: 7
                color: "#292b33"
                opacity: 0.96
                border.color: Qt.lighter(color, 1.18)
            }
        }
        ContextMenuItem {
            text: "Проиграть fix"
            fontFamily: card.appFamily
            enabled: fixedSide.fixedPlayable
            onTriggered: fixedSide.openPlayer()
        }
        ContextMenuSeparator {}
        ContextMenuItem {
            text: "Запустить задачу"
            fontFamily: card.appFamily
            enabled: !card.running && card.fixedRunnable
            onTriggered: card.runRequested(card.index, "fixed")
        }
        ContextMenuSeparator {}
        ContextMenuItem {
            text: "Показать в папке"
            fontFamily: card.appFamily
            enabled: fixedSide.fixedPlayable
            onTriggered: card.revealInFolderRequested(fixedSide.fixedPlayablePath)
        }
        ContextMenuItem {
            text: "Открыть"
            fontFamily: card.appFamily
            enabled: fixedSide.fixedPlayable
            onTriggered: card.openFileRequested(fixedSide.fixedPlayablePath)
        }
        ContextMenuSeparator {}
        ContextMenuItem {
            text: "Убрать из списка"
            fontFamily: card.appFamily
            enabled: !card.running
            onTriggered: card.removeRequested(card.index)
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 3
        spacing: 8

        TaskSourceSide {
            id: sourceSide
            Layout.fillWidth: true
            Layout.fillHeight: true
            appFamily: card.appFamily
            monoFamily: card.monoFamily
            sideColor: card.sideColor
            sideHoverColor: card.sideHoverColor
            borderSoftColor: card.borderSoftColor
            thumbnailColor: card.thumbnailColor
            borderColor: card.borderColor
            textColor: card.textColor
            mutedColor: card.mutedColor
            fixesColor: card.fixesColor
            previewColor: card.previewColor
            telegramColor: card.telegramColor
            controlsEnabled: !card.running
            contextActive: card.contextSide === "source"
            missingMeta: card.missingMeta
            sourcePath: card.path
            fileName: card.fileName
            folder: card.folder
            sourcePreviewPath: card.sourcePreviewPath
            sourcePreview: card.srcPreview
            sourceTelegram: card.srcTelegram
            sourcePreviewExists: card.sourcePreviewExists
            resolution: card.resolution
            frames: card.frames
            fps: card.fps
            thumbnailUrl: card.thumbnailUrl
            onOpenPlayerRequested: function(path, title) {
                card.openPlayerRequested(path, title)
            }
            onSetActionRequested: function(action, active) {
                card.setFileActionRequested(card.index, action, active)
            }
        }

        TaskFixedSide {
            id: fixedSide
            Layout.fillWidth: true
            Layout.fillHeight: true
            appFamily: card.appFamily
            monoFamily: card.monoFamily
            sideColor: card.sideColor
            sideHoverColor: card.sideHoverColor
            borderSoftColor: card.borderSoftColor
            thumbnailColor: card.thumbnailColor
            borderColor: card.borderColor
            textColor: card.textColor
            mutedColor: card.mutedColor
            fixesColor: card.fixesColor
            previewColor: card.previewColor
            telegramColor: card.telegramColor
            controlsEnabled: !card.running
            contextActive: card.contextSide === "fixed"
            missingMeta: card.missingMeta
            fixedPath: card.fixedPath
            fixedPreviewPath: card.fixedPreviewPath
            fixes: card.fixes
            fixedPreview: card.fixedPreview
            fixedTelegram: card.fixedTelegram
            fixedExists: card.fixedExists
            fixedPreviewExists: card.fixedPreviewExists
            fixedThumbnailUrl: card.fixedThumbnailUrl
            onOpenPlayerRequested: function(path, title) {
                card.openPlayerRequested(path, title)
            }
            onSetActionRequested: function(action, active) {
                card.setFileActionRequested(card.index, action, active)
            }
        }
    }

    MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: parent.width / 2
        acceptedButtons: Qt.RightButton
        onClicked: {
            card.contextSide = "source"
            sourceContextMenu.popup()
        }
    }

    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: parent.width / 2
        acceptedButtons: Qt.RightButton
        onClicked: {
            card.contextSide = "fixed"
            fixedContextMenu.popup()
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 7
        width: statusLabel.implicitWidth + 12
        height: 21
        radius: 4
        visible: card.runtimeStatus(card.status).length > 0
        color: "#55292a34"
        border.color: card.borderSoftColor

        Label {
            id: statusLabel
            anchors.centerIn: parent
            text: card.runtimeStatus(card.status)
            color: card.accentColor
            font.pixelSize: 11
            font.bold: true
        }
    }
}
