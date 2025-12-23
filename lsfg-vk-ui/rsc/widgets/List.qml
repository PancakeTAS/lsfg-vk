import QtQuick
import QtQuick.Layouts

Rectangle {
    property var model
    property int selected
    signal select(int index)

    Layout.fillWidth: true
    Layout.fillHeight: true
    id: root
    color: palette.dark
    border.color: palette.light
    border.width: 1
    radius: 4
    clip: true

    ListView {
        anchors.fill: parent
        id: view

        model: root.model
        currentIndex: root.selected

        delegate: Rectangle {
            width: view.width
            height: 32
            radius: 4
            color: ListView.isCurrentItem ? palette.highlight : "transparent"

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.right: parent.right
                anchors.rightMargin: 10

                text: display
                color: ListView.isCurrentItem ? palette.highlightedText : palette.text
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.select(index)
            }
        }
    }
}
