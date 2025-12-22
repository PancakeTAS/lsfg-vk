import QtQuick
import QtQuick.Layouts

Rectangle {
    default property alias content: inner.children

    id: root
    color: "transparent"

    ColumnLayout {
        id: inner
        anchors.fill: parent
        anchors.margins: 12
        spacing: 4
    }
}
