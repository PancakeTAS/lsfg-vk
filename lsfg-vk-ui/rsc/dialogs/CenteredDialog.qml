import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    property string name
    default property alias content: inner.children
    signal confirm()

    id: root
    title: name
    standardButtons: Dialog.Ok
    onAccepted: root.confirm()

    modal: true
    dim: true
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    contentItem: ColumnLayout {
        id: inner
        spacing: 8
    }
}
