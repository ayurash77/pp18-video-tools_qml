import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import ".."

Button {
    id: control

    property int buttonWidth: AppStyle.toolbarButtonSize
    property bool pill: false
    property string fontFamily: ""
    property string iconSource: ""
    property string colorRole: "primary"

    Layout.preferredWidth: buttonWidth
    Layout.preferredHeight: AppStyle.toolbarButtonSize
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
                width: AppStyle.toolbarButtonIconSize
                height: AppStyle.toolbarButtonIconSize
                visible: control.iconSource.length > 0
                source: control.iconSource
                fillMode: Image.PreserveAspectFit
                color: control.checked
                       ? AppStyle.toolbarButtonHoveredContent("icon", control.colorRole, true, true, control.hovered, control.activeFocus)
                       : "#b5bdcd"
                opacity: control.enabled ? 1 : 0.45
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text ?? ""
                color: control.checked
                       ? AppStyle.toolbarButtonHoveredContent("icon", control.colorRole, control.iconSource.length > 0, true, control.hovered, control.activeFocus)
                       : "#b5bdcd"
                font: control.font
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    background: Rectangle {
        radius: control.pill ? height / 2 : AppStyle.compactRadius
        color: control.down
               ? Qt.darker(AppStyle.toolBgHover, 1.1)
               : control.checked
                 ? AppStyle.toolbarButtonHoverBg("icon", control.colorRole)
                 : control.hovered
                   ? AppStyle.toolBgHover
                   : AppStyle.toolBg
        border.width: 1
        border.color: control.checked
                      ? AppStyle.toolbarButtonBorder("outline", control.colorRole, true, control.hovered, control.activeFocus)
                      : AppStyle.border

        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutQuad } }
        Behavior on border.color { ColorAnimation { duration: 120; easing.type: Easing.OutQuad } }
    }
}
