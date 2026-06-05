import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

ApplicationWindow {
    id: root
    AppFonts {
        id: appFonts
    }

    width: 1365
    height: 768
    minimumWidth: 980
    minimumHeight: 620
    visible: true
    title: "PP18 VideoTools"
    color: "#1e2028"

    readonly property color toolbarColor: "#1a1c23"
    readonly property color panelColor: "#242730"
    readonly property color sideColor: "#292d37"
    readonly property color sideHoverColor: "#2e333f"
    readonly property color thumbnailColor: "#2d313b"
    readonly property color inputColor: "#20232c"
    readonly property color borderColor: "#343946"
    readonly property color borderSoftColor: "#303541"
    readonly property color textColor: "#d9deea"
    readonly property color mutedColor: "#8d95aa"
    readonly property color dimColor: "#687086"
    readonly property color fixesColor: "#43bf83"
    readonly property color previewColor: "#08aeea"
    readonly property color telegramColor: "#2f6cf2"
    readonly property color accentColor: "#af8f5d"
    readonly property color warningColor: "#e0184f"
    readonly property string missingMeta: "---"
    readonly property string monoFamily: appFonts.jetBrainsFamily.length > 0 ? appFonts.jetBrainsFamily : "Monospace"
    readonly property string appFamily: appFonts.titilliumRegularFamily.length > 0 ? appFonts.titilliumRegularFamily : ""

    font.family: root.appFamily

    function fileName(path) {
        if (!path || path.length === 0)
            return ""
        const normalized = String(path).replace(/\\/g, "/")
        const index = normalized.lastIndexOf("/")
        return index >= 0 ? normalized.slice(index + 1) : normalized
    }

    function folderName(path) {
        if (!path || path.length === 0)
            return ""
        const normalized = String(path).replace(/\\/g, "/")
        const index = normalized.lastIndexOf("/")
        return index >= 0 ? normalized.slice(0, index) : ""
    }

    function runtimeStatus(text) {
        const normalized = String(text || "").replace(/\n/g, " · ")
        return /^(Fixes|SRC Preview|SRC TG|FIX Preview|FIX TG)( · (Fixes|SRC Preview|SRC TG|FIX Preview|FIX TG))*$/.test(normalized) ? "" : normalized
    }

    function latestLog(text) {
        const value = String(text || "")
        if (value.length === 0)
            return appController.statusText
        const lines = value.split("\n")
        return lines.length > 0 ? lines[lines.length - 1] : appController.statusText
    }

    function frameCell(value) {
        const text = String(value || root.missingMeta)
        return text.replace(/F$/, "")
    }

    function activeTelegramLabel() {
        const recipients = appController.telegramRecipients
        for (let i = 0; i < recipients.length; ++i) {
            const recipient = recipients[i]
            if (recipient.id === appController.activeTelegramRecipientId)
                return recipient.label || recipient.chatId || "TG получатель"
        }
        return recipients.length > 0 ? (recipients[0].label || recipients[0].chatId) : "TG получатель"
    }

    function telegramRecipientLabels() {
        const recipients = appController.telegramRecipients
        const labels = []
        for (let i = 0; i < recipients.length; ++i) {
            const recipient = recipients[i]
            labels.push(recipient.label || recipient.chatId || "TG получатель")
        }
        return labels.length > 0 ? labels : ["TG получатель"]
    }

    function activeTelegramIndex() {
        const recipients = appController.telegramRecipients
        for (let i = 0; i < recipients.length; ++i) {
            if (recipients[i].id === appController.activeTelegramRecipientId)
                return i
        }
        return 0
    }

    function telegramRecipientIdAt(index) {
        const recipients = appController.telegramRecipients
        return index >= 0 && index < recipients.length ? recipients[index].id : ""
    }

    header: Rectangle {
        height: 52
        color: root.toolbarColor
        border.color: "#2c313c"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 8

            ToolbarButton {
                variant: "icon"
                color: "secondary"
                icon: "qrc:/icons/cog"
                label: "Настройки"
                onClicked: appController.openSettingsWindow()
            }

            ToolbarSelect {
                model: root.telegramRecipientLabels()
                currentIndex: root.activeTelegramIndex()
                controlWidth: AppStyle.toolbarSelectWidth
                label: "Получатель Telegram"
                enabled: !appController.running && appController.telegramRecipients.length > 0
                onActivated: {
                    const recipientId = root.telegramRecipientIdAt(currentIndex)
                    if (recipientId.length > 0)
                        appController.setActiveTelegramRecipient(recipientId)
                }
            }

            ToolbarCheck {
                labelText: "Удалять дублирующиеся кадры"
                checked: appController.removeDupes
                enabled: !appController.running
                onToggled: appController.removeDupes = checked
            }

            ToolbarSelect {
                model: ["Очень мягко", "Мягко", "Умеренно", "Агрессивно", "Максимально (есть риск)"]
                currentIndex: appController.duplicateModeIndex
                controlWidth: AppStyle.toolbarSelectWidth
                label: "Режим удаления дублей"
                enabled: !appController.running && appController.removeDupes
                onActivated: appController.duplicateModeIndex = currentIndex
            }

            ToolbarCheck {
                labelText: "Фиксировать 25 fps"
                checked: appController.convertTo25Fps
                enabled: !appController.running
                onToggled: appController.convertTo25Fps = checked
            }

            ToolbarSelect {
                model: ["Пропускать существующие", "Перезаписывать существующие"]
                currentIndex: appController.existingModeIndex
                controlWidth: AppStyle.toolbarSelectWideWidth
                label: "Режим существующих файлов"
                enabled: !appController.running
                onActivated: appController.existingModeIndex = currentIndex
            }

            Item {
                Layout.fillWidth: true
            }

            ToolbarButton {
                variant: "icon"
                color: appController.mediaCacheEnabled ? "success" : "secondary"
                icon: appController.mediaCacheRunning
                      ? "qrc:/icons/monitor-pause"
                      : appController.mediaCacheEnabled && appController.mediaCacheComplete
                        ? "qrc:/icons/monitor-check"
                        : "qrc:/icons/monitor-down"
                label: appController.mediaCacheEnabled ? "Отключить кеширование" : "Включить кеширование"
                active: appController.mediaCacheEnabled
                enabled: appController.files.count > 0 && !appController.running
                onClicked: appController.mediaCacheEnabled = !appController.mediaCacheEnabled
            }

            ToolbarButton {
                variant: "icon"
                color: "secondary"
                icon: "qrc:/icons/refresh-cw"
                label: "Обновить список"
                enabled: appController.files.count > 0 && !appController.running
                onClicked: appController.refreshList()
            }

            ToolbarButton {
                variant: "icon"
                color: "danger"
                icon: "qrc:/icons/trash"
                label: "Очистить список"
                enabled: appController.files.count > 0 && !appController.running
                onClicked: appController.clearFiles()
            }

            ToolbarButton {
                variant: "icon"
                color: appController.latestVersionsOnly ? "primary" : "secondary"
                icon: "qrc:/icons/funnel"
                label: "Последние версии"
                active: appController.latestVersionsOnly
                enabled: !appController.running
                onClicked: appController.latestVersionsOnly = !appController.latestVersionsOnly
            }

            ToolbarButton {
                variant: "icon"
                color: "secondary"
                icon: "qrc:/icons/file-plus"
                label: "Выбрать файлы"
                enabled: !appController.running
                onClicked: appController.chooseFiles()
            }

            ToolbarButton {
                variant: "icon"
                color: "secondary"
                icon: "qrc:/icons/folder-plus"
                label: "Выбрать папки"
                enabled: !appController.running
                onClicked: appController.chooseFolders()
            }

            ToolbarButton {
                variant: "icon"
                color: "warning"
                text: appController.running ? "■" : ""
                icon: appController.running ? "" : "qrc:/icons/play"
                label: appController.running ? "Остановить" : "Запустить задачи"
                enabled: appController.files.count > 0 || appController.running
                onClicked: appController.startOrStop()
            }
        }
    }

    Rectangle {
        id: listPanel
        anchors.fill: parent
        color: root.color

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 6

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    radius: 8
                    color: "#242730"
                    border.color: root.borderColor

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 8

                        Repeater {
                            model: ["SRC", "FIX"]

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 6
                                color: root.panelColor
                                border.color: root.borderSoftColor

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 10
                                    spacing: 8

                                    Label {
                                        text: modelData
                                        color: root.textColor
                                        font.pixelSize: 10
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        Layout.preferredWidth: 30
                                    }

                                    Row {
                                        spacing: 5
                                        Layout.preferredWidth: 110
                                        Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

                                        ActionToggle {
                                            visible: modelData === "FIX"
                                            active: appController.allFixedFixesEnabled
                                            enabled: !appController.running && appController.files.count > 0
                                            iconSource: "qrc:/icons/bug-off"
                                            label: "FIX: toggles fixes for all rows"
                                            tone: root.fixesColor
                                            fillActive: true
                                            onClicked: appController.toggleAllAction("fixes")
                                        }
                                        ActionToggle {
                                            active: modelData === "SRC" ? appController.allSrcPreviewEnabled : appController.allFixedPreviewEnabled
                                            enabled: !appController.running && appController.files.count > 0
                                            iconSource: "qrc:/icons/eye"
                                            label: modelData + ": toggles preview for all rows"
                                            tone: root.previewColor
                                            fillActive: true
                                            onClicked: appController.toggleAllAction(modelData === "SRC" ? "srcPreview" : "fixedPreview")
                                        }
                                        ActionToggle {
                                            active: modelData === "SRC" ? appController.allSrcTelegramEnabled : appController.allFixedTelegramEnabled
                                            enabled: !appController.running && appController.files.count > 0
                                            iconSource: "qrc:/icons/send"
                                            label: modelData + ": toggles Telegram for all rows"
                                            tone: root.telegramColor
                                            fillActive: true
                                            onClicked: appController.toggleAllAction(modelData === "SRC" ? "srcTelegram" : "fixedTelegram")
                                        }
                                    }

                                    Label {
                                        text: "Path"
                                        color: root.mutedColor
                                        font.pixelSize: 10
                                        font.bold: true
                                        Layout.fillWidth: true
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    Label {
                                        text: "Res"
                                        color: root.mutedColor
                                        font.pixelSize: 10
                                        font.bold: true
                                        horizontalAlignment: Text.AlignRight
                                        Layout.preferredWidth: 60
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    Label {
                                        text: "Frames"
                                        color: root.mutedColor
                                        font.pixelSize: 10
                                        font.bold: true
                                        horizontalAlignment: Text.AlignRight
                                        Layout.preferredWidth: 30
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    Label {
                                        text: "FPS"
                                        color: root.mutedColor
                                        font.pixelSize: 10
                                        font.bold: true
                                        horizontalAlignment: Text.AlignRight
                                        Layout.preferredWidth: 30
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }
                        }
                    }
                }

                ListView {
                    id: fileList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 6
                    clip: true
                    model: appController.files
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: TaskCard {
                        id: card
                        width: ListView.view.width
                        running: appController.running
                        missingMeta: root.missingMeta
                        appFamily: root.appFamily
                        monoFamily: root.monoFamily
                        panelColor: "#22242d"
                        sideColor: root.sideColor
                        sideHoverColor: root.sideHoverColor
                        thumbnailColor: root.thumbnailColor
                        borderColor: root.borderColor
                        borderSoftColor: root.borderSoftColor
                        textColor: root.textColor
                        mutedColor: root.mutedColor
                        fixesColor: root.fixesColor
                        previewColor: root.previewColor
                        telegramColor: root.telegramColor
                        accentColor: root.accentColor
                        onOpenPlayerRequested: function(path, title) {
                            appController.openPlayer(path, title)
                        }
                        onRunRequested: function(fileIndex, side) {
                            appController.startTaskSide(fileIndex, side)
                        }
                        onRevealInFolderRequested: function(path) {
                            appController.revealInFolder(path)
                        }
                        onOpenFileRequested: function(path) {
                            appController.openFile(path)
                        }
                        onRemoveRequested: function(fileIndex) {
                            appController.removeFile(fileIndex)
                        }
                        onSetFileActionRequested: function(fileIndex, action, active) {
                            appController.setFileAction(fileIndex, action, active)
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: appController.files.count === 0
                        text: "Файлы не выбраны"
                        color: root.mutedColor
                        font.pixelSize: 18
                    }
                }
            }

            DropArea {
                id: dropArea
                anchors.fill: parent
                onDropped: function(drop) {
                    appController.addDroppedUrls(drop.urls)
                    drop.acceptProposedAction()
                }
            }

            Rectangle {
                anchors.fill: parent
                visible: dropArea.containsDrag
                color: "#66262b34"
                border.color: root.accentColor
                border.width: 2

                Label {
                    anchors.centerIn: parent
                    text: "Отпустите файлы или папки"
                    color: root.textColor
                    font.pixelSize: 22
                    font.bold: true
                }
            }
        }

    footer: Rectangle {
        height: 40
        color: "#191b22"
        border.color: "#2b303a"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 12

            Label {
                text: root.latestLog(appController.logText)
                color: root.mutedColor
                font.pixelSize: 11
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: appController.running || appController.progressValue > 0 ? ("Прогресс: " + appController.progressValue + "%") : ""
                color: root.mutedColor
                font.pixelSize: 11
                font.bold: true
                horizontalAlignment: Text.AlignRight
                Layout.preferredWidth: 92
            }

            ProgressBar {
                from: 0
                to: 100
                value: appController.progressValue
                Layout.preferredWidth: 112
                Layout.preferredHeight: 8
                background: Rectangle {
                    radius: 4
                    color: "#222632"
                }
                contentItem: Item {
                    Rectangle {
                        width: parent.width * appController.progressValue / 100
                        height: parent.height
                        radius: 4
                        color: "#386fe8"
                    }
                }
            }

            ToolbarButton {
                variant: "icon"
                color: "secondary"
                icon: "qrc:/icons/logs"
                label: "Логи"
                onClicked: appController.openLogWindow()
            }
        }
    }

    SettingsWindow {
        id: settingsWindow
        visible: appController.settingsWindowOpen
        botToken: appController.telegramBotToken
        chatId: appController.telegramChatId
        recipients: appController.telegramRecipients
        activeRecipientId: appController.activeTelegramRecipientId
        appFamily: root.appFamily
        monoFamily: root.monoFamily
        onClosing: appController.settingsWindowOpen = false
        onSaveRequested: function(botToken, recipients, activeRecipientId) {
            appController.saveTelegramSettings(botToken, recipients, activeRecipientId)
            settingsWindow.close()
        }
        onImportUsersRequested: function(botToken) {
            appController.importTelegramRecipients(botToken)
        }

        Connections {
            target: appController
            function onTelegramRecipientsImported(recipients, message) {
                settingsWindow.applyImportedRecipients(recipients, message)
            }
            function onTelegramRecipientsImportFailed(message) {
                settingsWindow.importUsersFailed(message)
            }
        }
    }

    LogWindow {
        visible: appController.logWindowOpen
        logText: appController.logText
        monoFamily: root.monoFamily
        onClosing: appController.logWindowOpen = false
    }

    PlayerWindow {
        id: qmlPlayerWindow
        visible: appController.playerWindowOpen
        initialPath: appController.playerPath
        playerTitle: appController.playerTitle
        appFamily: root.appFamily
        monoFamily: root.monoFamily
        cachePlaybackEnabled: appController.mediaCacheEnabled
        onRequestVersions: function(path) {
            qmlPlayerWindow.versions = appController.versionedSiblingFiles(path)
        }
        onClosing: appController.playerWindowOpen = false
    }
}
