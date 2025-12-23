import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    property real from
    property real to
    property real value
    signal update(real value)

    id: root
    spacing: 4

    Slider {
        Layout.fillWidth: true;

        value: root.value
        from: root.from
        to: root.to

        onMoved: root.update(value.toFixed(2))
        onValueChanged: label.text = (value * 100).toFixed(0) + "%"
    }

    Label {
        Layout.preferredWidth: 40;

        id: label
        text: "0%"

        horizontalAlignment: Text.AlignHCenter
    }
}
