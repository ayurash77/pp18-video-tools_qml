import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: settingsWindow
    width: 980
    height: 720
    minimumWidth: 760
    minimumHeight: 520
    title: "PP18 Video Tools - Настройки"
    color: "#1e2028"

    property string botToken: ""
    property string chatId: ""
    property var recipients: []
    property string activeRecipientId: ""
    property string mediaCacheRootPath: ""
    property string mediaCacheSizeText: ""
    property real mediaCacheMaxSizeGb: 20
    property string appFamily: ""
    property string monoFamily: "Monospace"
    property bool importRunning: false
    signal saveRequested(string botToken, var recipients, string activeRecipientId, real mediaCacheMaxSizeGb)
    signal importUsersRequested(string botToken)

    function resetDraft() {
        tokenField.text = botToken
        chatField.text = chatId
        recipientsModel.clear()
        for (let i = 0; i < recipients.length; ++i) {
            const recipient = recipients[i]
            recipientsModel.append({
                id: recipient.id || ("recipient-" + (i + 1)),
                label: recipient.label || recipient.chatId || "",
                chatId: recipient.chatId || "",
                active: (recipient.id || "") === activeRecipientId
            })
        }
        if (recipientsModel.count === 0 && chatId.length > 0) {
            recipientsModel.append({ id: "recipient-default", label: "PP18 OUT", chatId: chatId, active: true })
        }
        if (recipientsModel.count > 0 && activeId().length === 0)
            recipientsModel.setProperty(0, "active", true)
        cacheLimitField.text = Number(mediaCacheMaxSizeGb).toLocaleString(Qt.locale(), "f", mediaCacheMaxSizeGb % 1 === 0 ? 0 : 2)
        messageLabel.text = ""
    }

    function activeId() {
        for (let i = 0; i < recipientsModel.count; ++i) {
            const item = recipientsModel.get(i)
            if (item.active)
                return item.id
        }
        return activeRecipientId || (recipientsModel.count > 0 ? recipientsModel.get(0).id : "")
    }

    function draftRecipients() {
        const result = []
        for (let i = 0; i < recipientsModel.count; ++i) {
            const item = recipientsModel.get(i)
            result.push({ id: item.id, label: item.label, chatId: item.chatId })
        }
        return result
    }

    function markActive(index) {
        for (let i = 0; i < recipientsModel.count; ++i)
            recipientsModel.setProperty(i, "active", i === index)
    }

    function hasRecipientChatId(chatId) {
        const normalized = String(chatId || "").trim()
        for (let i = 0; i < recipientsModel.count; ++i) {
            if (String(recipientsModel.get(i).chatId || "").trim() === normalized)
                return true
        }
        return false
    }

    function addManualChatIdRecipient() {
        const currentChatId = chatField.text.trim()
        if (currentChatId.length === 0 || hasRecipientChatId(currentChatId))
            return false

        recipientsModel.append({
            id: recipientsModel.count === 0 ? "recipient-default" : ("recipient-" + Date.now()),
            label: recipientsModel.count === 0 ? "PP18 OUT" : currentChatId,
            chatId: currentChatId,
            active: recipientsModel.count === 0
        })
        return true
    }

    function applyImportedRecipients(discovered, message) {
        importRunning = false
        let added = 0
        for (let i = 0; i < discovered.length; ++i) {
            const recipient = discovered[i]
            const chatId = String(recipient.chatId || recipient.chat_id || "").trim()
            if (chatId.length === 0 || hasRecipientChatId(chatId))
                continue

            recipientsModel.append({
                id: recipient.id || ("tg-" + chatId),
                label: recipient.label || chatId,
                chatId: chatId,
                active: recipientsModel.count === 0
            })
            ++added
        }

        if (recipientsModel.count > 0 && activeId().length === 0)
            recipientsModel.setProperty(0, "active", true)

        messageLabel.text = added > 0 ? (message + ". Добавлено: " + added) : message
    }

    function importUsersFailed(message) {
        importRunning = false
        messageLabel.text = message
    }

    function cacheLimitDraftGb() {
        const normalized = String(cacheLimitField.text || "").replace(",", ".").trim()
        const value = Number(normalized)
        if (!isFinite(value) || value < 0)
            return mediaCacheMaxSizeGb
        return value
    }

    onVisibleChanged: {
        if (visible)
            resetDraft()
    }

    component Panel : Rectangle {
        id: panel
        default property alias content: panelBody.data
        property string title: ""

        radius: 8
        color: "#242730"
        border.color: "#343946"
        implicitHeight: panelBody.implicitHeight + 32

        ColumnLayout {
            id: panelBody
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Label {
                visible: panel.title.length > 0
                text: panel.title
                color: "#d9deea"
                font.family: settingsWindow.appFamily
                font.pixelSize: 22
                font.bold: true
                Layout.fillWidth: true
            }
        }
    }

    component FieldLabel : Label {
        color: "#d9deea"
        font.family: settingsWindow.appFamily
        font.pixelSize: 12
        font.bold: true
    }

    component TextInputBox : TextField {
        height: 34
        color: "#d9deea"
        placeholderTextColor: "#687086"
        selectionColor: "#af8f5d"
        selectedTextColor: "#12141a"
        font.family: settingsWindow.monoFamily
        font.pixelSize: 12
        background: Rectangle {
            radius: 6
            color: "#20232c"
            border.color: parent.activeFocus ? "#2f6cf2" : "#394050"
        }
    }

    ListModel {
        id: recipientsModel
    }

    Rectangle {
        anchors.fill: parent
        color: "#1e2028"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            ScrollView {
                id: settingsScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                leftPadding: 24
                rightPadding: 24
                topPadding: 24
                bottomPadding: 24

                ColumnLayout {
                    width: settingsScroll.availableWidth
                    spacing: 16

                    Panel {
                        title: "Основные"
                        Layout.fillWidth: true

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 16

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Label {
                                    text: "Обновления"
                                    color: "#d9deea"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                Label {
                                    text: "Проверка обновлений будет перенесена после базового QML-плеера и Telegram recipients."
                                    color: "#8d95aa"
                                    font.pixelSize: 12
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                            }

                            Button {
                                text: "История обновлений"
                                enabled: false
                            }
                            Button {
                                text: "Проверить"
                                enabled: false
                            }
                        }
                    }

                    Panel {
                        title: "Кеширование"
                        Layout.fillWidth: true

                        Label {
                            text: "Кеш используется для thumbnails и облегченных mp4 preview, которые плеер может проигрывать вместо тяжелых исходников."
                            color: "#8d95aa"
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 1
                            rowSpacing: 8

                            FieldLabel { text: "Папка кеша" }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                TextInputBox {
                                    text: settingsWindow.mediaCacheRootPath
                                    readOnly: true
                                    Layout.fillWidth: true
                                    selectByMouse: true
                                }

                                Button {
                                    text: "Изменить"
                                    onClicked: appController.chooseMediaCacheFolder()
                                }
                            }

                            FieldLabel { text: "Занято кешем" }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    text: settingsWindow.mediaCacheSizeText
                                    color: "#af8f5d"
                                    font.family: settingsWindow.monoFamily
                                    font.pixelSize: 13
                                    font.bold: true
                                    verticalAlignment: Text.AlignVCenter
                                    Layout.fillWidth: true
                                }

                                Button {
                                    text: "Обновить"
                                    onClicked: appController.refreshMediaCacheStats()
                                }

                                Button {
                                    text: "Очистить кеш"
                                    onClicked: appController.clearMediaCache()
                                }
                            }

                            FieldLabel { text: "Максимальный объем, GB" }
                            TextInputBox {
                                id: cacheLimitField
                                Layout.preferredWidth: 160
                                validator: DoubleValidator {
                                    bottom: 0
                                    decimals: 2
                                    notation: DoubleValidator.StandardNotation
                                }
                                placeholderText: "20"
                            }

                            Label {
                                text: "0 отключает ограничение. При превышении лимита удаляются самые старые файлы кеша."
                                color: "#8d95aa"
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }
                    }

                    Panel {
                        title: "Телеграмм"
                        Layout.fillWidth: true

                        Label {
                            text: "Токен бота и активный Chat ID используются для пересылки готовых preview в Telegram."
                            color: "#8d95aa"
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 86
                            radius: 6
                            color: "#20232c"
                            border.color: "#343946"

                            Label {
                                anchors.fill: parent
                                anchors.margins: 12
                                text: "Для новой установки возьми Bot TOKEN и Chat ID из файла:\nw:\\_Dev\\pp18_bot-token_chat-id.txt"
                                color: "#8d95aa"
                                font.pixelSize: 12
                                font.bold: true
                                wrapMode: Text.WordWrap
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 1
                            rowSpacing: 8

                            FieldLabel { text: "Bot TOKEN" }
                            TextInputBox {
                                id: tokenField
                                Layout.fillWidth: true
                                placeholderText: "123456:ABC..."
                                echoMode: TextInput.Password
                            }

                            FieldLabel { text: "Chat ID" }
                            TextInputBox {
                                id: chatField
                                Layout.fillWidth: true
                                placeholderText: "-100... или 123..."
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Button {
                                text: "Скопировать ссылку"
                                enabled: false
                            }

                            Button {
                                text: settingsWindow.importRunning ? "Обновляю..." : "Обновить пользователей"
                                enabled: !settingsWindow.importRunning && (tokenField.text.trim().length > 0 || chatField.text.trim().length > 0)
                                onClicked: {
                                    const addedManual = settingsWindow.addManualChatIdRecipient()
                                    if (tokenField.text.trim().length === 0) {
                                        messageLabel.text = addedManual ? "Текущий Chat ID добавлен. Для поиска пользователей нужен Bot TOKEN." : "Telegram: не задан TOKEN"
                                        return
                                    }

                                    settingsWindow.importRunning = true
                                    messageLabel.text = addedManual ? "Текущий Chat ID добавлен. Запрашиваю Telegram getUpdates..." : "Запрашиваю Telegram getUpdates..."
                                    settingsWindow.importUsersRequested(tokenField.text)
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Label {
                                text: "Получатели"
                                color: "#d9deea"
                                font.pixelSize: 14
                                font.bold: true
                                Layout.fillWidth: true
                            }
                            Button {
                                text: "Добавить"
                                onClicked: {
                                    recipientsModel.append({
                                        id: "recipient-" + Date.now(),
                                        label: "",
                                        chatId: "",
                                        active: recipientsModel.count === 0
                                    })
                                }
                            }
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.preferredHeight: recipientsModel.count > 0 ? Math.min(contentHeight, 180) : 0
                            clip: true
                            spacing: 8
                            model: recipientsModel
                            boundsBehavior: Flickable.StopAtBounds
                            interactive: contentHeight > height

                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 76
                                radius: 6
                                color: "#20232c"
                                border.color: active ? "#2f6cf2" : "#343946"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        FieldLabel { text: "Название" }
                                        TextInputBox {
                                            text: label
                                            Layout.fillWidth: true
                                            placeholderText: "PP18 OUT / Мне / Имя пользователя"
                                            onEditingFinished: recipientsModel.setProperty(index, "label", text)
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        FieldLabel { text: "Chat ID" }
                                        TextInputBox {
                                            text: chatId
                                            Layout.fillWidth: true
                                            placeholderText: "-100... или 123..."
                                            onEditingFinished: recipientsModel.setProperty(index, "chatId", text)
                                        }
                                    }

                                    CheckBox {
                                        checked: active
                                        text: "По умолчанию"
                                        onClicked: markActive(index)
                                    }

                                    Button {
                                        text: "×"
                                        enabled: recipientsModel.count > 1
                                        onClicked: recipientsModel.remove(index)
                                    }
                                }
                            }
                        }

                        Label {
                            visible: recipientsModel.count === 0
                            text: "Список пуст. Добавь хотя бы PP18 OUT с текущим chat id."
                            color: "#8d95aa"
                            font.pixelSize: 12
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }

                        Label {
                            text: "Импорт берет пользователей из Telegram getUpdates. Пользователь должен сначала открыть бота и нажать Start."
                            color: "#8d95aa"
                            font.pixelSize: 12
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            Label {
                id: messageLabel
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                color: "#af8f5d"
                font.pixelSize: 12
                elide: Text.ElideRight
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 58
                color: "#191b22"
                border.color: "#2b303a"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 24
                    anchors.rightMargin: 24
                    spacing: 10

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Отмена"
                        onClicked: settingsWindow.close()
                    }

                    Button {
                        text: "Сохранить"
                        onClicked: {
                            settingsWindow.saveRequested(tokenField.text, draftRecipients(), activeId(), cacheLimitDraftGb())
                            messageLabel.text = "Настройки сохранены"
                        }
                    }
                }
            }
        }
    }
}
