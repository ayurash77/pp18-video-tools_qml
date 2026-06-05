#include "app/AppController.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#ifndef APP_VERSION
#define APP_VERSION QT_VERSION_STR
#endif

namespace {
void applyLoggingRules()
{
    constexpr const char* quietInfoRules = "qt.multimedia.ffmpeg.info=false";
    QByteArray rules = qgetenv("QT_LOGGING_RULES");
    if (!rules.isEmpty())
        rules += ';';
    rules += quietInfoRules;
    qputenv("QT_LOGGING_RULES", rules);
}

void applySystemDefaultFont(QGuiApplication& app)
{
#ifdef Q_OS_WIN
    QFont font(QStringLiteral("Segoe UI"));
    font.setPixelSize(13);
    app.setFont(font);
#else
    const QString fontPath = QStringLiteral(":/fonts/sf-regular");
    const int fontId = QFontDatabase::addApplicationFont(fontPath);
    if (fontId == -1) {
        qWarning() << "Failed to load font from" << fontPath;
    }
    const QStringList families = fontId == -1 ? QStringList() : QFontDatabase::applicationFontFamilies(fontId);
    QFont font(families.isEmpty() ? QStringLiteral("Arial") : families.first());
    font.setPixelSize(13);
    app.setFont(font);

    QFontDatabase::addApplicationFont(":/fonts/titillium-regular");
    QFontDatabase::addApplicationFont(":/fonts/titillium-semibold");
    QFontDatabase::addApplicationFont(":/fonts/titillium-bold");
    QFontDatabase::addApplicationFont(":/fonts/jetbrains");
    QFontDatabase::addApplicationFont(":/fonts/sf-semibold");
    QFontDatabase::addApplicationFont(":/fonts/sf-bold");
#endif
}

}

int main(int argc, char* argv[])
{
    applyLoggingRules();

    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName("PP18");
    QCoreApplication::setOrganizationDomain("pp18.local");
    QCoreApplication::setApplicationName("VideoTools");
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION));

    QQuickStyle::setStyle("Fusion");
    applySystemDefaultFont(app);

    AppController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appController", &controller);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("PP18.VideoTools", "Main");

    return app.exec();
}
