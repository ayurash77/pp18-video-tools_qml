import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

Button {
    id: control

    property int buttonWidth: 30
    property bool pill: true
    property string fontFamily: Qt.application.font.family
    property string iconSource: ""

    Layout.preferredWidth: buttonWidth
    Layout.preferredHeight: 28
    padding: 0
    font.family: fontFamily
    font.pixelSize: 11
    font.bold: true

    contentItem: Item {
        Row {
            anchors.centerIn: parent
            spacing: control.text.length > 0 && iconImage.visible ? 5 : 0

            ColorImage {
                id: iconImage
                anchors.verticalCenter: parent.verticalCenter
                width: 14
                height: 14
                visible: control.iconSource.length > 0
                source: control.iconSource
                fillMode: Image.PreserveAspectFit
                color: control.checked ? "#ffffff" : "#b5bdcd"
                opacity: control.enabled ? 1 : 0.45
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text ?? ""
                color: control.checked ? "#ffffff" : "#b5bdcd"
                font: control.font
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    background: Rectangle {
        radius: control.pill ? height / 2 : 4
        color: control.down || control.checked ? "#303542" : control.hovered ? "#2b303b" : "#252a34"
        border.color: "#343946"
    }
}
