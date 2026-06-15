import QtQuick 2.15

Rectangle {
    width: 1280
    height: 720
    color: "#0d0d0d"

    Column {
        anchors.centerIn: parent
        spacing: 16

        Text {
            id: timeText
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatTime(new Date(), "hh:mm:ss")
            color: "#00e5ff"
            font.pixelSize: 140
            font.bold: true
            font.family: "Monospace"
        }

        Text {
            id: dateText
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatDate(new Date(), "dddd, MMMM d yyyy")
            color: "#607d8b"
            font.pixelSize: 32
            font.family: "Sans Serif"
        }
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            var now = new Date()
            timeText.text = Qt.formatTime(now, "hh:mm:ss")
            dateText.text = Qt.formatDate(now, "dddd, MMMM d yyyy")
        }
    }
}
