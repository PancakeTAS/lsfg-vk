import QtQuick
import QtQuick.Layouts

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    id: root
    color: palette.dark
    border.color: palette.light
    border.width: 1
    radius: 4

    ListView {
        anchors.fill: parent
    }
}
