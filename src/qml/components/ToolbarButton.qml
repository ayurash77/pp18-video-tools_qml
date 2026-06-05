import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import ".."

Control {
    id: control

    property string variant: "default"
    property string color: "secondary"
    property string text: ""
    property string label: ""
    property string icon: ""
    property string iconSource: ""
    property string fontFamily: Qt.application.font.family
    property bool active: false
    property bool danger: false

    property color fill: AppStyle.toolbarButtonBg(control.variant, control.effectiveColor)
    property color hoverFill: AppStyle.toolbarButtonHoverBg(control.variant, control.effectiveColor)
    property color foreground: AppStyle.toolbarButtonHoveredContent(control.variant,
                                                                    control.effectiveColor,
                                                                    control.iconOnly,
                                                                    control.active,
                                                                    control.hovered,
                                                                    control.activeFocus)
    property color activeBorder: AppStyle.toolbarButtonBase(control.effectiveColor)

    readonly property string effectiveColor: danger ? "danger" : color
    readonly property string effectiveIcon: icon.length > 0 ? icon : iconSource
    readonly property bool iconOnly: effectiveIcon.length > 0 && text.length === 0
    readonly property bool iconVariant: variant === "icon" && iconOnly
    readonly property string renderedIcon: effectiveIcon

    signal clicked()

    implicitWidth: AppStyle.toolbarButtonSize
    implicitHeight: AppStyle.toolbarButtonSize
    width: implicitWidth
    height: implicitHeight
    padding: 0
    spacing: 4
    opacity: enabled ? 1.0 : 0.45
    scale: tapHandler.pressed ? 0.97 : 1.0
    focusPolicy: Qt.TabFocus

    contentItem: Item {
        Row {
            anchors.centerIn: parent
            spacing: control.text.length > 0 && iconImage.visible ? control.spacing : 0

            ColorImage {
                id: iconImage
                anchors.verticalCenter: parent.verticalCenter
                width: AppStyle.toolbarButtonIconSize
                height: AppStyle.toolbarButtonIconSize
                visible: control.effectiveIcon.length > 0
                source: control.renderedIcon
                smooth: true
                fillMode: Image.PreserveAspectFit
                color: control.foreground
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                color: control.foreground
                font.family: control.fontFamily
                font.pixelSize: control.text.length > 1 ? 10 : 15
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    background: Rectangle {
        radius: AppStyle.compactRadius
        color: tapHandler.pressed && control.variant === "default"
               ? Qt.darker(control.fill, 1.12)
               : control.enabled && (control.hovered || control.active)
                 ? control.hoverFill
                 : control.fill
        border.width: control.variant === "icon" || (control.variant === "ghost" && !control.active && !control.activeFocus) ? 0 : 1
        border.color: AppStyle.toolbarButtonBorder(control.variant,
                                                   control.effectiveColor,
                                                   control.active,
                                                   control.hovered,
                                                   control.activeFocus)

        Rectangle {
            anchors.centerIn: parent
            width: parent.width + 6
            height: parent.height + 6
            radius: parent.radius + 3
            color: "transparent"
            border.width: 1
            border.color: control.activeBorder
            opacity: control.variant !== "icon" && control.activeFocus ? 0.65 : 0
        }

        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutQuad } }
        Behavior on border.color { ColorAnimation { duration: 120; easing.type: Easing.OutQuad } }
    }

    TapHandler {
        id: tapHandler
        enabled: control.enabled
        acceptedButtons: Qt.LeftButton
        onTapped: control.clicked()
    }

    HoverHandler {
        id: hoverHandler
        enabled: control.enabled
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            control.clicked()
            event.accepted = true
        }
    }

    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCirc } }

    ToolTip.text: label
    ToolTip.visible: control.hovered && label.length > 0
}
