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

    Dialog {
        id: dialog_name
        title: "(...)"
        standardButtons: Dialog.Ok | Dialog.Cancel

        modal: true
        dim: true
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        contentItem: ColumnLayout {
            spacing: 8

            TextField {
                Layout.fillWidth: true

                id: nameField
                placeholderText: "Choose a profile name"
                selectByMouse: true
                focus: true
            }
        }
    }

    Dialog {
        id: dialog_confirm
        title: "Confirm Deletion"
        standardButtons: Dialog.Ok | Dialog.Cancel

        modal: true
        dim: true
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        contentItem: ColumnLayout {
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: "Are you sure you want to delete the selected profile?"
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

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
                model: backend.profiles
                selected: backend.profile_index
                onSelect: (index) => backend.profile_index = index
            }

            Button {
                Layout.fillWidth: true
                text: "Create New Profile"
                onClicked: {
                    dialog_name.title = "Create New Profile"
                    nameField.text = ""

                    dialog_name.open()
                }
            }
            Button {
                Layout.fillWidth: true
                text: "Rename Profile"
                onClicked: {
                    dialog_name.title = "Rename Profile"
                    nameField.text = "(...)"

                    dialog_name.open()
                }
            }
            Button {
                Layout.fillWidth: true
                text: "Delete Profile"
                onClicked: {
                    dialog_confirm.open()
                }
            }
        }

        Pane {
            SplitView.fillWidth: true

            Group {
                name: "Global Settings"

                GroupEntry {
                    title: "Path to Lossless Scaling"
                    description: "Change the location of Lossless.dll"

                    FileEdit {
                        title: "Select Lossless.dll"
                        filter: "Dynamic Link Library Files (*.dll)"

                        text: backend.dll
                        onUpdate: (text) => backend.dll = text
                    }
                }

                GroupEntry {
                    title: "Allow half-precision"
                    description: "Allow acceleration through half-precision"

                    CheckBox {
                        checked: backend.allow_fp16
                        onToggled: backend.allow_fp16 = checked
                    }
                }
            }

            Group {
                name: "Profile Settings"
                enabled: backend.available

                GroupEntry {
                    title: "Multiplier"
                    description: "Control the amount of generated frames"

                    SpinBox {
                        from: 2
                        to: 100

                        value: backend.multiplier
                        onValueModified: backend.multiplier = value
                    }
                }

                GroupEntry {
                    title: "Flow Scale"
                    description: "Lower the internal motion estimation resolution"

                    FlowSlider {
                        from: 0.25
                        to: 1.00

                        value: backend.flow_scale
                        onUpdate: (value) => backend.flow_scale = value
                    }
                }

                GroupEntry {
                    title: "Performance Mode"
                    description: "Use a significantly lighter frame generation modeln"

                    CheckBox {
                        checked: backend.performance_mode
                        onToggled: backend.performance_mode = checked
                    }
                }

                GroupEntry {
                    title: "Pacing Mode"
                    description: "Change how frames are presented to the display"

                    ComboBox {
                        model: ["None"]

                        currentValue: backend.pacing_mode
                        onActivated: (index) => backend.pacing_mode = model[index]
                    }
                }

                GroupEntry {
                    title: "GPU"
                    description: "Select which GPU to use for frame generation"

                    ComboBox {
                        model: ["Default"]

                        currentValue: backend.gpu
                        onActivated: (index) => backend.gpu = model[index]
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
