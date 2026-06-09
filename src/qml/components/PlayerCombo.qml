pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import ".."

ComboBox {
    id: control

    property string fontFamily: ""
    property color textColor: "#c0cfe7"
    property color dimColor: "#56627a"
    property color mutedColor: "#9aa8c1"
    property color inputColor: "#464c5b"
    property color borderColor: "#6a7281"
    property color panelColor: "#292b33"

    height: AppStyle.toolbarSelectHeight
    implicitHeight: AppStyle.toolbarSelectHeight
    leftPadding: 8
    rightPadding: 8
    spacing: 5

    font.family: fontFamily
    font.pixelSize: 12
    font.bold: true

    function delegateText(index, modelData) {
        if ((typeof modelData === "string" || typeof modelData === "number")
                && String(modelData).length > 0) {
            return String(modelData)
        }
        return control.textAt(index)
    }

    delegate: ItemDelegate {
        id: delegate

        required property int index
        required property var modelData

        width: control.width
        height: AppStyle.toolbarSelectDelegateHeight
        implicitHeight: AppStyle.toolbarSelectDelegateHeight
        padding: 0
        topPadding: 0
        bottomPadding: 0
        leftPadding: 0
        rightPadding: 0
        highlighted: control.highlightedIndex === delegate.index

        contentItem: Text {
            anchors.fill: parent
            leftPadding: control.leftPadding
            rightPadding: control.rightPadding
            text: control.delegateText(delegate.index, delegate.modelData)
            color: delegate.hovered || delegate.highlighted ? Qt.lighter(control.textColor, 1.15) : control.textColor
            font: control.font
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: AppStyle.compactRadius
            color: delegate.hovered || delegate.highlighted ? "#146ecc" : "transparent"
            border.color: delegate.hovered || delegate.highlighted ? Qt.lighter(color, 1.15) : "transparent"
        }
    }

    indicator: Item {
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 10
        height: 14

        ColorImage {
            anchors.fill: parent
            source: "qrc:/icons/arr-up-down"
            fillMode: Image.PreserveAspectFit
            color: control.enabled ? control.mutedColor : control.dimColor
        }
    }

    contentItem: Text {
        leftPadding: control.leftPadding
        rightPadding: control.indicator.width + control.spacing + control.rightPadding
        text: control.displayText
        color: control.enabled ? control.textColor : control.dimColor
        font: control.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: AppStyle.compactRadius
        color: control.down
               ? Qt.darker(control.inputColor, 1.12)
               : control.enabled && control.hovered
                 ? Qt.lighter(control.inputColor, 1.1)
                 : control.inputColor
        opacity: control.enabled ? 0.62 : 0.36
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus ? "#146ecc" : control.borderColor

        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutQuad } }
        Behavior on border.color { ColorAnimation { duration: 120; easing.type: Easing.OutQuad } }
    }

    popup: Popup {
        id: popup

        x: -padding
        y: control.height + 4
        width: control.width + padding * 2
        height: Math.min(control.count * AppStyle.toolbarSelectDelegateHeight + padding * 2, 260)
        padding: 3

        contentItem: ListView {
            clip: true
            implicitHeight: control.count * AppStyle.toolbarSelectDelegateHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }

        background: Rectangle {
            radius: AppStyle.radius + 1
            color: control.panelColor
            opacity: 0.96
            border.color: Qt.lighter(color, 1.18)
        }
    }
}
