#include "app/AppController.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStringList>
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

QString loadFontFamily(const QString& path)
{
    const int fontId = QFontDatabase::addApplicationFont(path);
    if (fontId == -1) {
        qWarning() << "Failed to load font from" << path;
        return {};
    }

    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    return families.isEmpty() ? QString() : families.first();
}

void applySystemDefaultFont(QGuiApplication& app)
{
    const QString titilliumFamily = loadFontFamily(QStringLiteral(":/fonts/titillium-regular"));
    QFont font(titilliumFamily.isEmpty() ? QStringLiteral("Arial") : titilliumFamily);
    font.setPixelSize(13);
    app.setFont(font);

    QFontDatabase::addApplicationFont(":/fonts/titillium-semibold");
    QFontDatabase::addApplicationFont(":/fonts/titillium-bold");

#ifndef Q_OS_WIN
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
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app-icon")));

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
