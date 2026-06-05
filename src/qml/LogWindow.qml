import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: logWindow
    width: 760
    height: 420
    minimumWidth: 520
    minimumHeight: 260
    title: "PP18 Video Tools Log"
    color: "#191b22"

    property string logText: ""
    property string monoFamily: "Monospace"

    Rectangle {
        anchors.fill: parent
        color: "#191b22"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: "PP18 Video Tools Log"
                    color: "#d9deea"
                    font.pixelSize: 16
                    font.bold: true
                    Layout.fillWidth: true
                }

                Label {
                    text: logText.length === 0 ? "0" : String(logText.split("\n").length)
                    color: "#8d95aa"
                    font.family: monoFamily
                    font.pixelSize: 12
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                TextArea {
                    id: logArea
                    text: logText.length === 0 ? "Лог пуст" : logText
                    readOnly: true
                    wrapMode: TextEdit.NoWrap
                    color: "#d9deea"
                    selectedTextColor: "#12141a"
                    selectionColor: "#af8f5d"
                    font.family: monoFamily
                    font.pixelSize: 12
                    background: Rectangle {
                        color: "#12141a"
                        radius: 8
                        border.color: "#343946"
                    }
                    onTextChanged: cursorPosition = length
                }
            }
        }
    }
}
