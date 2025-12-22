import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    property string title
    property string description

    id: root
    spacing: 12

    ColumnLayout {
        spacing: 0

        Label {
            text: root.title
            font.bold: true
        }

        Label {
            text: root.description
            color: Qt.rgba(
                palette.text.r,
                palette.text.g,
                palette.text.b,
                0.7
            )
        }
    }

    Item {
        Layout.fillWidth: true
    }
}
