import QtQuick
import QtQuick.Controls

ComboBox {
    id: combo

    property string fontFamily: "Monospace"

    font.family: fontFamily
    font.pixelSize: 11

    contentItem: Text {
        text: combo.displayText
        color: "#08aeea"
        font: combo.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 4
        color: "#20232c"
        border.color: combo.hovered ? "#3d4657" : "#303541"
    }
}
