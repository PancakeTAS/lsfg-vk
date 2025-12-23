import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

RowLayout {
    property string title
    property string filter
    property string text
    signal update(string text)

    id: root
    spacing: 4

    TextField {
        Layout.fillWidth: true;

        text: root.text
        onEditingFinished: root.update(text)
    }

    Button {
        icon.name: "folder-open"
        onClicked: picker.open()
    }

    FileDialog {
        id: picker
        title: root.title
        nameFilters: [root.filter, "All Files (*)"]
        onAccepted: root.update(selectedFile.toString().replace("file://", ""))
    }
}
