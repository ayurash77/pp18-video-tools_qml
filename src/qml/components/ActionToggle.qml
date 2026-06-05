import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

Rectangle {
    id: control

    property bool active: false
    property string label: ""
    property string glyph: ""
    property string iconSource: ""
    property string fontFamily: ""
    property color tone: "#08aeea"
    property color mutedColor: "#8d95aa"
    property color hoverFill: "#38404d"
    property bool fillActive: false

    signal clicked()

    width: 20
    height: 20
    radius: 4
    color: fillActive && active ? tone : mouseArea.containsMouse && enabled ? hoverFill : "transparent"
    border.color: fillActive && active ? tone : "transparent"
    opacity: enabled ? 1.0 : 0.35

    ColorImage {
        anchors.centerIn: parent
        width: 14
        height: 14
        visible: control.iconSource.length > 0
        source: control.iconSource
        color: control.fillActive && control.active ? "#edf5ff" : control.active ? control.tone : control.mutedColor
    }

    Text {
        anchors.centerIn: parent
        visible: control.iconSource.length === 0
        text: control.glyph
        color: control.fillActive && control.active ? "#edf5ff" : control.active ? control.tone : control.mutedColor
        font.family: control.fontFamily
        font.pixelSize: control.glyph.length > 1 ? 9 : 13
        font.bold: true
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        enabled: control.enabled
        onClicked: control.clicked()
    }

    ToolTip.text: control.label
    ToolTip.visible: mouseArea.containsMouse && control.label.length > 0
}
