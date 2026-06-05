#include "TelegramController.h"

#include "TelegramService.h"

#include <QSettings>

namespace {
constexpr const char* KeyBotToken = "telegram/bot_token";
constexpr const char* KeyChatId = "telegram/chat_id";
constexpr const char* EnvBotToken = "TG_PP18NOTIFIER_BOT_TOKEN";
constexpr const char* EnvChatId = "TG_PP18OUT_CHAT_ID";
} // namespace

TelegramController::TelegramController(QObject* parent)
    : QObject(parent)
    , m_service(new TelegramService(this))
{
    loadFromSettingsOrEnv();
    m_service->setCredentials(m_token, m_chat);
    connect(m_service, &TelegramService::log, this, &TelegramController::log);
}

QString TelegramController::botToken() const
{
    return m_token;
}

QString TelegramController::chatId() const
{
    return m_chat;
}

void TelegramController::setCredentials(const QString& token, const QString& chat)
{
    m_token = token;
    m_chat = chat;
    m_service->setCredentials(m_token, m_chat);
    saveToSettings();
}

void TelegramController::loadFromSettingsOrEnv()
{
    QSettings settings;
    m_token = settings.value(KeyBotToken).toString();
    m_chat = settings.value(KeyChatId).toString();

    if (m_token.isEmpty()) {
        const QString envToken = qEnvironmentVariable(EnvBotToken);
        if (!envToken.isEmpty()) {
            m_token = envToken;
            settings.setValue(KeyBotToken, m_token);
        }
    }

    if (m_chat.isEmpty()) {
        const QString envChat = qEnvironmentVariable(EnvChatId);
        if (!envChat.isEmpty()) {
            m_chat = envChat;
            settings.setValue(KeyChatId, m_chat);
        }
    }
}

void TelegramController::saveToSettings()
{
    QSettings settings;
    settings.setValue(KeyBotToken, m_token);
    settings.setValue(KeyChatId, m_chat);
}
