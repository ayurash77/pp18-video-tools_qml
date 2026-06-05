#include "TelegramService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QTimer>

namespace {

QString telegramChatIdToString(const QJsonValue& value)
{
    if (value.isString())
        return value.toString().trimmed();
    if (value.isDouble())
        return QString::number(value.toInteger());
    return QString();
}

QString telegramChatLabel(const QJsonObject& chat)
{
    const QString username = chat.value("username").toString().trimmed();
    if (!username.isEmpty())
        return "@" + username;

    const QString firstName = chat.value("first_name").toString().trimmed();
    const QString lastName = chat.value("last_name").toString().trimmed();
    const QString fullName = QString("%1 %2").arg(firstName, lastName).trimmed();
    if (!fullName.isEmpty())
        return fullName;

    const QString chatId = telegramChatIdToString(chat.value("id"));
    return chatId.isEmpty() ? QString("Telegram user") : chatId;
}

QJsonObject chatObjectFromUpdate(const QJsonObject& update)
{
    const QStringList messageKeys = {"message", "edited_message"};
    for (const QString& key : messageKeys) {
        const QJsonObject message = update.value(key).toObject();
        const QJsonObject chat = message.value("chat").toObject();
        if (!chat.isEmpty())
            return chat;
    }

    const QStringList memberKeys = {"my_chat_member", "chat_member"};
    for (const QString& key : memberKeys) {
        const QJsonObject chat = update.value(key).toObject().value("chat").toObject();
        if (!chat.isEmpty())
            return chat;
    }

    return {};
}

} // namespace

TelegramService::TelegramService(QObject* parent)
    : QObject(parent)
{
}

void TelegramService::setCredentials(const QString& botToken, const QString& chatId)
{
    m_token = botToken;
    m_chatId = chatId;
}

QString TelegramService::apiUrl(const QString& method) const
{
    return QString("https://api.telegram.org/bot%1/%2").arg(m_token, method);
}

void TelegramService::sendFile(const QString& filePath,
                               const QString& caption,
                               const QString& copyMacPath,
                               const QString& copyWindowsPath)
{
    if (m_token.isEmpty() || m_chatId.isEmpty()) {
        emit log("Telegram: не задан TOKEN или CHAT_ID");
        emit fileSent(filePath, false, "No token/chat_id");
        return;
    }

    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile()) {
        emit log("Telegram: файл не найден: " + QDir::toNativeSeparators(filePath));
        emit fileSent(filePath, false, "File not found");
        return;
    }

    auto* multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart chatIdPart;
    chatIdPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"chat_id\""));
    chatIdPart.setBody(m_chatId.toUtf8());
    multi->append(chatIdPart);

    QHttpPart parseModePart;
    parseModePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"parse_mode\""));
    parseModePart.setBody("HTML");
    multi->append(parseModePart);

    const QString fullCaption = caption.isEmpty() ? QDir::toNativeSeparators(filePath) : caption;
    QHttpPart captionPart;
    captionPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"caption\""));
    captionPart.setBody(fullCaption.toUtf8());
    multi->append(captionPart);

    if (!copyMacPath.isEmpty() && !copyWindowsPath.isEmpty()) {
        QJsonObject macCopy;
        macCopy.insert("text", copyMacPath);
        QJsonObject macButton;
        macButton.insert("text", " MacOS path");
        macButton.insert("copy_text", macCopy);

        QJsonObject windowsCopy;
        windowsCopy.insert("text", copyWindowsPath);
        QJsonObject windowsButton;
        windowsButton.insert("text", "⊞ Win path");
        windowsButton.insert("copy_text", windowsCopy);

        QJsonArray row;
        row.append(macButton);
        row.append(windowsButton);

        QJsonArray keyboard;
        keyboard.append(row);

        QJsonObject replyMarkup;
        replyMarkup.insert("inline_keyboard", keyboard);

        QHttpPart replyMarkupPart;
        replyMarkupPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"reply_markup\""));
        replyMarkupPart.setBody(QJsonDocument(replyMarkup).toJson(QJsonDocument::Compact));
        multi->append(replyMarkupPart);
    }

    QHttpPart streamingPart;
    streamingPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"supports_streaming\""));
    streamingPart.setBody("true");
    multi->append(streamingPart);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QString("form-data; name=\"video\"; filename=\"%1\"").arg(fi.fileName())));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("video/mp4"));

    QFile* file = new QFile(filePath, multi);
    if (!file->open(QIODevice::ReadOnly)) {
        emit log("Telegram: не удалось открыть файл: " + QDir::toNativeSeparators(filePath));
        emit fileSent(filePath, false, "Open failed");
        multi->deleteLater();
        return;
    }
    filePart.setBodyDevice(file);
    multi->append(filePart);

    QNetworkRequest req(QUrl(apiUrl("sendVideo")));
    QNetworkReply* reply = m_net.post(req, multi);
    multi->setParent(reply);

    reply->setProperty("tg_filePath", filePath);
    connect(reply, &QNetworkReply::finished, this, &TelegramService::onUploadFinished);

    emit log("Отправка видео в Telegram: " + QDir::toNativeSeparators(filePath));
}

void TelegramService::importPrivateRecipients(const QString& botToken)
{
    const QString token = botToken.trimmed().isEmpty() ? m_token : botToken.trimmed();
    if (token.isEmpty()) {
        const QString message = "Telegram: не задан TOKEN";
        emit log(message);
        emit privateRecipientsImported({}, false, message);
        return;
    }

    QNetworkRequest request(QUrl(QString("https://api.telegram.org/bot%1/getUpdates").arg(token)));
    request.setTransferTimeout(15000);
    QNetworkReply* reply = m_net.get(request);
    auto* timeout = new QTimer(reply);
    timeout->setSingleShot(true);
    timeout->setInterval(15000);
    connect(timeout, &QTimer::timeout, reply, [reply]() {
        if (!reply->isFinished())
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, timeout, &QTimer::stop);
    connect(reply, &QNetworkReply::finished, this, &TelegramService::onImportRecipientsFinished);
    timeout->start();
    emit log("Telegram: обновляю список пользователей...");
}

void TelegramService::onUploadFinished()
{
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;

    const QString filePath = reply->property("tg_filePath").toString();
    const QByteArray body = reply->readAll();
    const bool ok = reply->error() == QNetworkReply::NoError;

    if (ok) {
        emit log("Telegram отправил: " + QDir::toNativeSeparators(filePath));
        emit fileSent(filePath, true, QString());
    } else {
        emit log("Ошибка Telegram: " + reply->errorString());
        emit log("Ответ Telegram: " + QString::fromUtf8(body));
        emit fileSent(filePath, false, reply->errorString());
    }

    reply->deleteLater();
}

void TelegramService::onImportRecipientsFinished()
{
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;

    const QByteArray body = reply->readAll();
    const bool networkOk = reply->error() == QNetworkReply::NoError;
    if (!networkOk) {
        const QString message = "Telegram getUpdates: " + reply->errorString();
        emit log(message);
        emit privateRecipientsImported({}, false, message);
        reply->deleteLater();
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        const QString message = QString("Telegram getUpdates: неверный JSON ответ: %1; %2")
            .arg(parseError.errorString(), QString::fromUtf8(body));
        emit log(message);
        emit privateRecipientsImported({}, false, message);
        reply->deleteLater();
        return;
    }

    const QJsonObject root = document.object();
    if (!root.value("ok").toBool(false)) {
        const QString message = "Telegram getUpdates: " + QString::fromUtf8(body);
        emit log(message);
        emit privateRecipientsImported({}, false, message);
        reply->deleteLater();
        return;
    }

    QVariantList recipients;
    QSet<QString> seenChatIds;
    const QJsonArray updates = root.value("result").toArray();
    for (const QJsonValue& updateValue : updates) {
        const QJsonObject update = updateValue.toObject();
        const QJsonObject chat = chatObjectFromUpdate(update);
        if (chat.value("type").toString() != "private")
            continue;

        const QString chatId = telegramChatIdToString(chat.value("id"));
        if (chatId.isEmpty() || seenChatIds.contains(chatId))
            continue;

        seenChatIds.insert(chatId);
        QVariantMap recipient;
        recipient.insert("id", "tg-" + chatId);
        recipient.insert("label", telegramChatLabel(chat));
        recipient.insert("chatId", chatId);
        recipients << recipient;
    }

    const QString message = recipients.isEmpty()
        ? QString("Telegram пользователи не найдены. Попроси пользователя нажать Start у бота.")
        : QString("Telegram пользователи найдены: %1").arg(recipients.size());
    emit log(message);
    emit privateRecipientsImported(recipients, true, message);
    reply->deleteLater();
}
