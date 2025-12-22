import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "panes"
import "widgets"

ApplicationWindow {
    title: "lsfg-vk Configuration Window"
    width: 900
    height: 475
    minimumWidth: 700
    minimumHeight: 400
    visible: true

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Pane {
            SplitView.minimumWidth: 200
            SplitView.preferredWidth: 250
            SplitView.maximumWidth: 300

            Label {
                text: "Profiles"
                Layout.fillWidth: true
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            ProfileList {

            }

            Button {
                text: "Create New Profile"
                Layout.fillWidth: true
            }
            Button {
                text: "Rename Profile"
                Layout.fillWidth: true
            }
            Button {
                text: "Delete Profile"
                Layout.fillWidth: true
            }
        }

        Pane {
            SplitView.fillWidth: true

            Group {
                name: "Global Settings"

                GroupEntry {
                    title: "Browse for Lossless.dll"
                    description: "Change the location of Lossless.dll"

                    FileEdit {}
                }

                GroupEntry {
                    title: "Allow half-precision"
                    description: "Allow acceleration through half-precision"

                    CheckBox {}
                }
            }

            Group {
                name: "Profile Settings"

                GroupEntry {
                    title: "Multiplier"
                    description: "Control the amount of generated frames"

                    SpinBox { from: 2; to: 100 }
                }

                GroupEntry {
                    title: "Flow Scale"
                    description: "Lower the internal motion estimation resolution"

                    FlowSlider { from: 0.25; to: 1.00 }
                }

                GroupEntry {
                    title: "Performance Mode"
                    description: "Use a significantly lighter frame generation modeln"

                    CheckBox {}
                }

                GroupEntry {
                    title: "Pacing Mode"
                    description: "Change how frames are presented to the display"

                    ComboBox { model: ["None"] }
                }

                GroupEntry {
                    title: "GPU"
                    description: "Select which GPU to use for frame generation"

                    ComboBox { model: ["Auto"] }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
