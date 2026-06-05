import QtQuick
import QtQuick.Controls
import ".."

CheckBox {
    id: control

    property string labelText: ""
    property string fontFamily: ""
    property color textColor: "#d9deea"
    property color dimColor: "#687086"
    property color inputColor: "#20232c"

    text: labelText
    spacing: 8
    implicitWidth: indicator.implicitWidth + spacing + textMetrics.advanceWidth
    implicitHeight: AppStyle.toolbarSelectHeight
    font.family: fontFamily
    font.pixelSize: 12
    font.bold: true

    indicator: Rectangle {
        implicitWidth: 14
        implicitHeight: 14
        x: 0
        y: parent.height / 2 - height / 2
        radius: 3
        color: control.checked ? "#7b8497" : control.inputColor
        border.color: control.checked ? "#9aa4b8" : "#485063"

        Text {
            anchors.centerIn: parent
            text: control.checked ? "✓" : ""
            color: "#171a22"
            font.pixelSize: 11
            font.bold: true
        }
    }

    contentItem: Text {
        text: control.text
        leftPadding: control.indicator.width + control.spacing
        color: control.enabled ? control.textColor : control.dimColor
        font.family: control.fontFamily
        font.pixelSize: 12
        font.bold: true
        verticalAlignment: Text.AlignVCenter
    }

    TextMetrics {
        id: textMetrics
        font.family: control.fontFamily
        font.pixelSize: 12
        font.bold: true
        text: control.text
    }
}
