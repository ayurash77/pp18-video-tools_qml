import QtQuick
import QtQuick.Controls.Basic
import ".."

MenuItem {
    id: control

    property string fontFamily: ""
    property color textColor: "#c0cfe7"
    property color dimColor: "#56627a"
    property color highlightColor: "#146ecc"

    width: parent ? parent.width : implicitWidth
    implicitWidth: 176
    implicitHeight: AppStyle.toolbarSelectDelegateHeight
    padding: 0
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    contentItem: Text {
        id: itemText

        anchors.fill: parent
        leftPadding: 8
        rightPadding: 8
        text: control.text
        color: !control.enabled
               ? control.dimColor
               : control.hovered || control.highlighted
                 ? Qt.lighter(control.textColor, 1.15)
                 : control.textColor
        font.family: control.fontFamily
        font.pixelSize: 12
        font.bold: false
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 4
        color: control.enabled && (control.hovered || control.highlighted) ? control.highlightColor : "transparent"
        border.color: control.enabled && (control.hovered || control.highlighted) ? Qt.lighter(color, 1.15) : "transparent"
    }
}
