import QtQuick
import QtQuick.Controls.Basic

MenuSeparator {
    id: control

    width: parent ? parent.width : implicitWidth
    implicitWidth: 176
    implicitHeight: 7
    padding: 0
    topPadding: 3
    bottomPadding: 3

    contentItem: Rectangle {
        implicitHeight: 1
        color: "#3a4050"
        opacity: 0.65
    }
}
