import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    property string name
    default property alias content: inner.children

    Layout.fillWidth: true
    id: root
    topPadding: label.implicitHeight + 8
    label: Label {
        text: root.name
        anchors.horizontalCenter: parent.horizontalCenter

        anchors.top: parent.top
        anchors.topMargin: 4
        padding: 4
    }
    background: Rectangle {
        color: palette.alternateBase
        border.color: palette.light
        radius: 4
    }

    ColumnLayout {
        id: inner

        anchors.fill: parent
        spacing: 12
    }
}
