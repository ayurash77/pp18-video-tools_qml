import QtQuick

QtObject {
    readonly property int h1: 32
    readonly property int h2: 24
    readonly property int h3: 19
    readonly property int h4: 16
    readonly property int content: 13
    readonly property int button: 11

    readonly property var titilliumRegular: FontLoader { source: "qrc:/fonts/titillium-regular" }
    readonly property var titilliumSemibold: FontLoader { source: "qrc:/fonts/titillium-semibold" }
    readonly property var titilliumBold: FontLoader { source: "qrc:/fonts/titillium-bold" }
    readonly property var jetBrains: FontLoader { source: "qrc:/fonts/jetbrains" }
    readonly property var sfRegular: FontLoader { source: "qrc:/fonts/sf-regular" }
    readonly property var sfSemibold: FontLoader { source: "qrc:/fonts/sf-semibold" }
    readonly property var sfBold: FontLoader { source: "qrc:/fonts/sf-bold" }

    readonly property string titilliumRegularFamily: titilliumRegular.name === "Titillium Web"
        ? "Titillium Web[RUS by Daymarius]"
        : titilliumRegular.name
    readonly property string titilliumSemiboldFamily: titilliumSemibold.name === "Titillium Web"
        ? "Titillium Web[RUS by Daymarius]"
        : titilliumSemibold.name
    readonly property string titilliumBoldFamily: titilliumBold.name === "Titillium Web"
        ? "Titillium Web[RUS by Daymarius]"
        : titilliumBold.name
    readonly property string jetBrainsFamily: jetBrains.name
}
