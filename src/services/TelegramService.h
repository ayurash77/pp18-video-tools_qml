#ifndef TELEGRAMSERVICE_H
#define TELEGRAMSERVICE_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QVariantList>

class TelegramService : public QObject {
    Q_OBJECT
public:
    explicit TelegramService(QObject* parent = nullptr);

    void setCredentials(const QString& botToken, const QString& chatId);
    void sendFile(const QString& filePath,
                  const QString& caption = QString(),
                  const QString& copyMacPath = QString(),
                  const QString& copyWindowsPath = QString());
    void importPrivateRecipients(const QString& botToken);

signals:
    void log(const QString& line);
    void fileSent(const QString& filePath, bool ok, const QString& details);
    void privateRecipientsImported(const QVariantList& recipients, bool ok, const QString& details);

private slots:
    void onUploadFinished();
    void onImportRecipientsFinished();

private:
    QString apiUrl(const QString& method) const;

private:
    QNetworkAccessManager m_net;
    QString m_token;
    QString m_chatId;
};

#endif // TELEGRAMSERVICE_H
