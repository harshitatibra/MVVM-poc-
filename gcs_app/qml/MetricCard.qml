import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: card
    required property string title
    required property string value
    property string detail: ""
    property color accent: "#7bdac5"
    implicitHeight: 150
    radius: 14
    color: "#192631"
    border.color: "#2a3c49"
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 8
        Label { text: card.title; color: "#9bb0bf"; font.pixelSize: 13 }
        Label {
            text: card.value
            color: card.accent
            font.pixelSize: 29
            font.weight: Font.DemiBold
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
        Label { text: card.detail; color: "#9bb0bf"; font.pixelSize: 12 }
    }
}
