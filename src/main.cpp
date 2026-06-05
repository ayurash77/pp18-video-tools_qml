#include "app/AppController.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

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
    const int fontId = QFontDatabase::addApplicationFont(":/fonts/sf-regular");
    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    QFont font(families.isEmpty() ? QString() : families.first());
    font.setPixelSize(13);
    app.setFont(font);
}

}

int main(int argc, char* argv[])
{
    applyLoggingRules();

    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName("PP18");
    QCoreApplication::setOrganizationDomain("pp18.local");
    QCoreApplication::setApplicationName("VideoTools");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QQuickStyle::setStyle("Fusion");
    applySystemDefaultFont(app);

    AppController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appController", &controller);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    return app.exec();
}
