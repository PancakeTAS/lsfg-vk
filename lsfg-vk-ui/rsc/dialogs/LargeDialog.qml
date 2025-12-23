import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    property string name
    default property alias content: inner.children
    signal confirm()

    id: root
    title: name
    onAccepted: root.confirm()

    modal: true
    dim: true
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    width: parent.width * 0.75
    height: parent.height * 0.75

    contentItem: ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true

        id: inner
        spacing: 8
    }
}
