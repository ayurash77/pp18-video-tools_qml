import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: control

    property string text: ""
    property bool exists: true
    property color tone: "#d9deea"
    property color mutedColor: "#8d95aa"
    property string fontFamily: Qt.application.font.family
    property string monoFamily: "Monospace"
    property string res: ""
    property string frames: ""
    property string fps: ""

    height: 19
    opacity: exists ? 1.0 : 0.36

    RowLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: control.text
            color: control.tone
            font.family: control.fontFamily
            font.pixelSize: 12
            elide: Text.ElideRight
            Layout.fillWidth: true
            verticalAlignment: Text.AlignVCenter
        }

        Label {
            text: control.res
            color: control.mutedColor
            font.family: control.monoFamily
            font.pixelSize: 12
            horizontalAlignment: Text.AlignRight
            Layout.preferredWidth: 60
            verticalAlignment: Text.AlignVCenter
        }

        Label {
            text: control.frames
            color: control.mutedColor
            font.family: control.monoFamily
            font.pixelSize: 12
            horizontalAlignment: Text.AlignRight
            Layout.preferredWidth: 30
            verticalAlignment: Text.AlignVCenter
        }

        Label {
            text: control.fps
            color: control.mutedColor
            font.family: control.monoFamily
            font.pixelSize: 12
            horizontalAlignment: Text.AlignRight
            Layout.preferredWidth: 30
            verticalAlignment: Text.AlignVCenter
        }
    }
}
