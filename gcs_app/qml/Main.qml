import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quadruped.Gcs

ApplicationWindow {
    id: window
    required property RobotViewModel viewModel
    visible: true
    width: 1100
    height: 830
    minimumWidth: 760
    minimumHeight: 650
    title: "Quadruped · Ground Control"
    color: "#101a23"
    palette.text: "#e5edf3"
    palette.windowText: "#e5edf3"
    palette.buttonText: "#e5edf3"
    palette.button: "#294454"
    palette.highlight: "#69d2bc"

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ColumnLayout {
            width: window.width - 64
            x: 32
            spacing: 20
            Item { Layout.preferredHeight: 8 }
            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    spacing: 5
                    Label { text: "QUADRUPED / GCS"; font.pixelSize: 25; font.weight: Font.DemiBold }
                    Label { text: "Two-laptop telemetry & command demonstration"; color: "#9bb0bf" }
                }
                Item { Layout.fillWidth: true }
                Rectangle {
                    implicitWidth: statusText.implicitWidth + 30
                    implicitHeight: 36
                    radius: 18
                    color: window.viewModel.telemetryFresh ? "#173d36" : "#493b25"
                    Label {
                        id: statusText
                        anchors.centerIn: parent
                        text: window.viewModel.linkStatus
                        color: window.viewModel.telemetryFresh ? "#7bdac5" : "#f0c77d"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }
            }
            Label {
                Layout.fillWidth: true
                text: !window.viewModel.hasTelemetry
                      ? "Start the simulator on the robot laptop. Discovery is automatic."
                      : window.viewModel.telemetryFresh
                        ? "Receiving live telemetry · 5 updates per second"
                        : "No recent telemetry. Values below are the last received readings."
                color: window.viewModel.telemetryFresh ? "#9bb0bf" : "#f0c77d"
                wrapMode: Text.WordWrap
            }
            GridLayout {
                Layout.fillWidth: true
                columns: window.width >= 1000 ? 4 : 2
                rowSpacing: 14
                columnSpacing: 14
                opacity: window.viewModel.telemetryFresh ? 1 : 0.55
                MetricCard {
                    Layout.fillWidth: true
                    title: "BATTERY"
                    value: window.viewModel.hasTelemetry ? window.viewModel.batteryPercent.toFixed(1) + " %" : "—"
                    detail: window.viewModel.hasTelemetry ? window.viewModel.batteryVoltage.toFixed(2) + " V · simulated pack" : "Waiting for data"
                    accent: window.viewModel.batteryPercent < 20 ? "#f0c77d" : "#7bdac5"
                }
                MetricCard {
                    Layout.fillWidth: true
                    title: "HEALTH"
                    value: window.viewModel.health
                    detail: "Warning below 20% battery"
                    accent: window.viewModel.health === "OK" ? "#7bdac5" : "#f0c77d"
                }
                MetricCard {
                    Layout.fillWidth: true
                    title: "CPU TEMPERATURE"
                    value: window.viewModel.hasTelemetry ? window.viewModel.cpuTemperature.toFixed(1) + " °C" : "—"
                    detail: "Simulated onboard computer"
                    accent: "#91bfff"
                }
                MetricCard {
                    Layout.fillWidth: true
                    title: "ROBOT POSTURE"
                    value: window.viewModel.mode
                    detail: "Changes after Stand / Sit"
                    accent: "#c2b0ff"
                }
            }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 100
                radius: 14
                color: "#192631"
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 24
                    ColumnLayout {
                        Label { text: "DUMMY SENSOR"; color: "#9bb0bf"; font.pixelSize: 12 }
                        Label { text: window.viewModel.hasTelemetry ? window.viewModel.dummyValue.toFixed(2) : "—"; font.pixelSize: 22 }
                    }
                    ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: window.viewModel.dummyValue
                        opacity: window.viewModel.telemetryFresh ? 1 : 0.4
                    }
                    ColumnLayout {
                        Label { text: "UPTIME / SEQUENCE"; color: "#9bb0bf"; font.pixelSize: 12 }
                        Label { text: window.viewModel.hasTelemetry ? window.viewModel.uptimeSec.toFixed(1) + " s / #" + window.viewModel.sequence : "—" }
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 12
                RowLayout {
                    Label { text: "COMMANDS"; font.pixelSize: 14; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Label { text: window.viewModel.serviceAvailable ? "Service discovered" : "Waiting for command service"; color: "#9bb0bf" }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Repeater {
                        model: [
                            { label: "Ping", command: "ping" },
                            { label: "Stand", command: "stand" },
                            { label: "Sit", command: "sit" },
                            { label: "Low battery", command: "low_battery" },
                            { label: "Reset", command: "reset" }
                        ]
                        delegate: Button {
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: 42
                            text: modelData.label
                            enabled: window.viewModel.canSendCommand
                            onClicked: window.viewModel.sendCommand(modelData.command)
                        }
                    }
                }
                Label {
                    Layout.fillWidth: true
                    text: window.viewModel.commandStatus
                    color: window.viewModel.commandPending ? "#f0c77d" : "#b8ccd9"
                    wrapMode: Text.WordWrap
                }
            }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 190
                radius: 14
                color: "#0b141c"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    Label { text: "EVENT LOG"; color: "#9bb0bf"; font.pixelSize: 12; font.bold: true }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 8
                        model: window.viewModel.eventLog
                        ScrollBar.vertical: ScrollBar {}
                        delegate: Label {
                            required property string modelData
                            width: ListView.view.width - 18
                            text: modelData
                            font.family: "monospace"
                            font.pixelSize: 12
                            color: "#b8ccd9"
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
            Label { text: "SIMULATOR ONLY · Commands update dummy state"; color: "#748b9b"; font.pixelSize: 11 }
            Item { Layout.preferredHeight: 12 }
        }
    }
}
