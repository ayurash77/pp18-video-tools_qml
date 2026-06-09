import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import ".."

Rectangle {
    id: control

    property string monoFamily: "Monospace"
    property color panelColor: AppStyle.panelBg
    property color borderColor: AppStyle.border
    property color accentColor: AppStyle.accent
    property real position: 0
    property real duration: 1
    property bool playing: false
    property bool loopEnabled: true
    property var playbackSpeeds: []
    property int playbackSpeedIndex: 3
    property string zoomText: "Fit"
    property string zoomMode: "fit"
    property real zoom: 1.0
    property bool infoVisible: false
    property bool versionsVisible: true
    property bool cachePlaybackEnabled: true
    property bool cacheRunning: false
    property bool cacheReady: false
    property string timeText: ""

    signal seekRequested(real value)
    signal togglePlaybackRequested()
    signal stepFrameRequested(int direction)
    signal toggleLoopRequested()
    signal speedSelected(real speed)
    signal zoomStepRequested(int direction)
    signal zoomPresetRequested(var value)
    signal infoVisibleRequested(bool visible)
    signal versionsVisibleRequested(bool visible)
    signal cachePlaybackToggled()

    Layout.fillWidth: true
    Layout.preferredHeight: 72
    radius: AppStyle.radius
    color: panelColor
    border.color: borderColor

    function zoomPresetSelected(value) {
        if (value === "fit")
            return zoomMode === "fit"
        return zoomMode === "manual" && Math.round(zoom * 100) === Math.round(Number(value) * 100)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 8
        anchors.bottomMargin: 9
        spacing: 6

        Slider {
            Layout.fillWidth: true
            Layout.preferredHeight: 18
            from: 0
            to: Math.max(1, control.duration)
            value: control.position
            onMoved: control.seekRequested(value)
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ToolbarButton {
                variant: "icon"
                color: "primary"
                icon: control.playing ? "qrc:/icons/pause" : "qrc:/icons/play"
                label: control.playing ? "Пауза" : "Проиграть"
                onClicked: control.togglePlaybackRequested()
            }
            ToolbarButton {
                variant: "icon"
                color: "secondary"
                icon: "qrc:/icons/chevron-left"
                label: "Кадр назад"
                onClicked: control.stepFrameRequested(-1)
            }
            ToolbarButton {
                variant: "icon"
                color: "secondary"
                icon: "qrc:/icons/chevron-right"
                label: "Кадр вперед"
                onClicked: control.stepFrameRequested(1)
            }
            ToolbarButton {
                variant: "icon"
                color: control.loopEnabled ? "primary" : "secondary"
                icon: control.loopEnabled ? "qrc:/icons/repeat" : "qrc:/icons/repeat-off"
                label: control.loopEnabled ? "Зацикливание включено" : "Зацикливание выключено"
                active: control.loopEnabled
                onClicked: control.toggleLoopRequested()
            }
            PlayerCombo {
                fontFamily: control.monoFamily
                model: control.playbackSpeeds
                currentIndex: control.playbackSpeedIndex
                Layout.preferredWidth: 64
                Layout.preferredHeight: AppStyle.toolbarButtonSize
                onActivated: function(index) {
                    control.speedSelected(control.playbackSpeeds[index])
                }
            }
            ToolbarButton {
                variant: "icon"
                color: "secondary"
                icon: "qrc:/icons/zoom-out"
                label: "Уменьшить зум"
                onClicked: control.zoomStepRequested(-1)
            }
            ToolbarButton {
                id: zoomMenuButton
                text: control.zoomText
                variant: "outline"
                color: "secondary"
                fontFamily: control.monoFamily
                Layout.preferredWidth: control.zoomMode === "fit" ? 86 : 64
                Layout.preferredHeight: AppStyle.toolbarButtonSize
                onClicked: zoomPopup.open()

                Popup {
                    id: zoomPopup

                    x: -3
                    width: Math.max(zoomMenuButton.width, 92) + 6
                    height: zoomColumn.implicitHeight + 6
                    y: -height - 4
                    padding: 3
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                    contentItem: Column {
                        id: zoomColumn
                        spacing: 0

                        Repeater {
                            model: [
                                { label: "Fit", value: "fit" },
                                { label: "25%", value: 0.25 },
                                { label: "50%", value: 0.5 },
                                { label: "100%", value: 1.0 },
                                { label: "150%", value: 1.5 },
                                { label: "200%", value: 2.0 }
                            ]

                            delegate: Rectangle {
                                id: zoomDelegate

                                required property var modelData

                                width: zoomPopup.width - zoomPopup.padding * 2
                                height: AppStyle.toolbarSelectDelegateHeight
                                radius: AppStyle.compactRadius
                                color: zoomMouse.containsMouse ? "#146ecc" : "transparent"
                                border.color: zoomMouse.containsMouse ? Qt.lighter(color, 1.15) : "transparent"

                                readonly property bool selected: control.zoomPresetSelected(modelData.value)

                                Text {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    text: zoomDelegate.modelData.label
                                    color: zoomMouse.containsMouse
                                           ? Qt.lighter("#c0cfe7", 1.15)
                                           : zoomDelegate.selected ? control.accentColor : "#c0cfe7"
                                    font.family: control.monoFamily
                                    font.pixelSize: 12
                                    font.bold: true
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }

                                MouseArea {
                                    id: zoomMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        control.zoomPresetRequested(zoomDelegate.modelData.value)
                                        zoomPopup.close()
                                    }
                                }
                            }
                        }
                    }

                    background: Rectangle {
                        radius: AppStyle.radius + 1
                        color: "#292b33"
                        opacity: 0.96
                        border.color: Qt.lighter(color, 1.18)
                    }
                }
            }
            ToolbarButton {
                variant: "icon"
                color: "secondary"
                icon: "qrc:/icons/zoom-in"
                label: "Увеличить зум"
                onClicked: control.zoomStepRequested(1)
            }
            ToolbarButton {
                variant: "icon"
                color: control.infoVisible ? "primary" : "secondary"
                icon: "qrc:/icons/info"
                label: "Информация"
                active: control.infoVisible
                onClicked: control.infoVisibleRequested(!control.infoVisible)
            }
            ToolbarButton {
                variant: "icon"
                color: control.versionsVisible ? "primary" : "secondary"
                icon: "qrc:/icons/layers"
                label: "Слои версий"
                active: control.versionsVisible
                onClicked: control.versionsVisibleRequested(!control.versionsVisible)
            }
            ToolbarButton {
                variant: "icon"
                color: control.cachePlaybackEnabled ? "success" : "secondary"
                icon: !control.cachePlaybackEnabled
                      ? "qrc:/icons/monitor-down"
                      : control.cacheRunning
                        ? "qrc:/icons/monitor-pause"
                        : control.cacheReady
                          ? "qrc:/icons/monitor-check"
                          : "qrc:/icons/monitor-down"
                label: control.cachePlaybackEnabled ? "Проигрывать кеш preview" : "Проигрывать оригинал"
                active: control.cachePlaybackEnabled
                Layout.preferredWidth: AppStyle.toolbarButtonSize
                Layout.preferredHeight: AppStyle.toolbarButtonSize
                onClicked: control.cachePlaybackToggled()
            }

            Item { Layout.fillWidth: true }

            Label {
                text: control.timeText
                color: AppStyle.status
                font.family: control.monoFamily
                font.pixelSize: 12
                font.bold: true
            }
        }
    }
}
