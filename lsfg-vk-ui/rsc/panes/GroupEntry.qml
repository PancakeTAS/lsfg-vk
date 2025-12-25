import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    property string title
    property string description
    default property alias content: inner.children

    id: root
    spacing: 12

    ColumnLayout {
        clip: true

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
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
    }

    RowLayout {
        Item {
            Layout.fillWidth: true
        }

        ColumnLayout {
            spacing: 0
            id: inner
        }
    }


}
