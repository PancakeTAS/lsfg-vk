import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    spacing: 4

    TextField {
        Layout.fillWidth: true;
        Layout.maximumWidth: 450;
    }

    Button {
        icon.name: "folder-open"
    }
}
