import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts

pragma ComponentBehavior: Bound

Rectangle {
    id: taskSide

    property string title: "SRC"
    property bool showFixes: false
    property bool fixesActive: false
    property bool previewActive: false
    property bool telegramActive: false
    property bool controlsEnabled: true
    property string thumbUrl: ""
    property string fallbackLabel: ""
    property string folderPath: ""
    property string primaryText: ""
    property bool primaryExists: true
    property color primaryTone: textColor
    property string primaryRes: ""
    property string primaryFrames: ""
    property string primaryFps: ""
    property string secondaryText: ""
    property bool secondaryExists: false
    property color secondaryTone: previewColor
    property string secondaryRes: ""
    property string secondaryFrames: ""
    property string secondaryFps: ""
    property bool contextActive: false

    property color sideColor: "#292d37"
    property color sideHoverColor: "#2e333f"
    property color borderSoftColor: "#303541"
    property color thumbnailColor: "#2d313b"
    property color borderColor: "#343946"
    property color textColor: "#d9deea"
    property color mutedColor: "#8d95aa"
    property color fixesColor: "#43bf83"
    property color previewColor: "#08aeea"
    property color telegramColor: "#2f6cf2"
    property string appFamily: ""
    property string monoFamily: "Monospace"

    signal play()

    signal toggleFixes()

    signal togglePreview()

    signal toggleTelegram()

    radius: 6
    color: sideMouse.containsMouse ? sideHoverColor : sideColor
    border.color: contextActive ? telegramColor : borderSoftColor
    clip: true

    MouseArea {
        id: sideMouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onDoubleClicked: taskSide.play()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 3
        anchors.rightMargin: 8
        anchors.topMargin: 3
        anchors.bottomMargin: 3
        spacing: 8

        Item {
            Layout.preferredWidth: 30
            Layout.fillHeight: true

            Column {
                anchors.centerIn: parent
                spacing: taskSide.showFixes ? 4 : 6

                ActionToggle {
                    visible: taskSide.showFixes
                    active: taskSide.fixesActive
                    enabled: taskSide.controlsEnabled
                    iconSource: "qrc:/icons/bug-off"
                    label: "Fixes"
                    tone: taskSide.fixesColor
                    onClicked: taskSide.toggleFixes()
                }

                ActionToggle {
                    active: taskSide.previewActive
                    enabled: taskSide.controlsEnabled
                    iconSource: "qrc:/icons/eye"
                    label: "Preview"
                    tone: taskSide.previewColor
                    onClicked: taskSide.togglePreview()
                }

                ActionToggle {
                    active: taskSide.telegramActive
                    enabled: taskSide.controlsEnabled
                    iconSource: "qrc:/icons/send"
                    label: "Telegram"
                    tone: taskSide.telegramColor
                    onClicked: taskSide.toggleTelegram()
                }
            }
        }

        Rectangle {
            id: thumbnailFrame
            Layout.preferredWidth: 110
            Layout.preferredHeight: 64
            Layout.alignment: Qt.AlignVCenter
            radius: 6
            color: taskSide.thumbnailColor

            Rectangle {
                id: thumbnailMask
                anchors.fill: thumbnailContent
                radius: thumbnailFrame.radius
                visible: false
                color: "white"
                layer.enabled: true
                antialiasing: true
            }

            Item {
                id: thumbnailContent
                property Item maskItem: thumbnailMask

                anchors.fill: parent
                anchors.margins: 0.5
                visible: taskSide.thumbUrl.length > 0
                layer.enabled: true
                layer.smooth: true
                layer.samples: 4
                layer.effect: MultiEffect {
                    maskEnabled: true
                    maskSource: thumbnailContent.maskItem
                }

                Image {
                    anchors.fill: parent
                    source: taskSide.thumbUrl
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    cache: true
                    smooth: true
                    mipmap: true
                }
            }

            Rectangle {
                anchors.centerIn: parent
                width: 32
                height: 32
                radius: 16
                visible: taskSide.thumbUrl.length === 0
                color: "#252934"
                border.color: "#252934"

                Label {
                    anchors.centerIn: parent
                    text: taskSide.fallbackLabel.slice(0, 2).toUpperCase()
                    color: taskSide.mutedColor
                    font.family: taskSide.monoFamily
                    font.pixelSize: 12
                    font.bold: true
                }
            }

            Rectangle {
                anchors.fill: parent
                radius: thumbnailFrame.radius
                color: "transparent"
                border.color: Qt.lighter(taskSide.borderColor, 1.18)
                border.width: 1
                border.pixelAligned: false
                antialiasing: true
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Label {
                    text: taskSide.folderPath
                    color: taskSide.mutedColor
                    font.family: taskSide.appFamily
                    font.pixelSize: 12
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                    Layout.preferredHeight: 16
                    verticalAlignment: Text.AlignVCenter
                }

                TaskLine {
                    text: taskSide.primaryText
                    exists: taskSide.primaryExists
                    tone: taskSide.primaryTone
                    mutedColor: taskSide.mutedColor
                    fontFamily: taskSide.appFamily
                    monoFamily: taskSide.monoFamily
                    res: taskSide.primaryRes
                    frames: taskSide.primaryFrames
                    fps: taskSide.primaryFps
                    Layout.fillWidth: true
                }

                TaskLine {
                    text: taskSide.secondaryText
                    exists: taskSide.secondaryExists
                    tone: taskSide.secondaryTone
                    mutedColor: taskSide.mutedColor
                    fontFamily: taskSide.appFamily
                    monoFamily: taskSide.monoFamily
                    res: taskSide.secondaryRes
                    frames: taskSide.secondaryFrames
                    fps: taskSide.secondaryFps
                    Layout.fillWidth: true
                }
            }
        }
    }
}
