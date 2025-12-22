import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    property real from
    property real to
    id: root
    spacing: 4

    Slider {
        Layout.fillWidth: true;
        Layout.maximumWidth: 450;
    }

    Label {
        Layout.preferredWidth: 40;
        text: "0%"
        horizontalAlignment: Text.AlignHCenter
    }
}
