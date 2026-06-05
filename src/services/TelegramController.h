#ifndef TELEGRAMCONTROLLER_H
#define TELEGRAMCONTROLLER_H

#include <QObject>
#include <QString>

class TelegramService;

class TelegramController : public QObject {
    Q_OBJECT
public:
    explicit TelegramController(QObject* parent = nullptr);

    TelegramService* service() const { return m_service; }

    QString botToken() const;
    QString chatId() const;

    void setCredentials(const QString& token, const QString& chat);

signals:
    void log(const QString& msg);

private:
    void loadFromSettingsOrEnv();
    void saveToSettings();

private:
    TelegramService* m_service = nullptr;
    QString m_token;
    QString m_chat;
};

#endif // TELEGRAMCONTROLLER_H
