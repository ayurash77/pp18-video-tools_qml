import QtQuick

QtObject {
    id: appFonts

    readonly property int h1: 32
    readonly property int h2: 24
    readonly property int h3: 19
    readonly property int h4: 16
    readonly property int content: 13
    readonly property int button: 11

    readonly property bool useSystemMonoFonts: Qt.platform.os === "windows"

    readonly property var titilliumRegular: FontLoader { source: "qrc:/fonts/titillium-regular" }
    readonly property var titilliumSemibold: FontLoader { source: "qrc:/fonts/titillium-semibold" }
    readonly property var titilliumBold: FontLoader { source: "qrc:/fonts/titillium-bold" }
    readonly property var jetBrains: FontLoader { source: appFonts.useSystemMonoFonts ? "" : "qrc:/fonts/jetbrains" }
    readonly property var sfRegular: FontLoader { source: "qrc:/fonts/sf-regular" }
    readonly property var sfSemibold: FontLoader { source: "qrc:/fonts/sf-semibold" }
    readonly property var sfBold: FontLoader { source: "qrc:/fonts/sf-bold" }

    readonly property string systemAppFamily: Qt.platform.os === "windows" ? "Segoe UI" : "Arial"
    readonly property string systemMonoFamily: Qt.platform.os === "windows" ? "Consolas" : "Monospace"
    readonly property string appFamily: titilliumRegular.name.length > 0 ? titilliumRegular.name : systemAppFamily
    readonly property string monoFamily: useSystemMonoFonts ? systemMonoFamily : (jetBrains.name.length > 0 ? jetBrains.name : systemMonoFamily)
    readonly property string titilliumRegularFamily: titilliumRegular.name
    readonly property string titilliumSemiboldFamily: titilliumSemibold.name
    readonly property string titilliumBoldFamily: titilliumBold.name
    readonly property string jetBrainsFamily: jetBrains.name
}
