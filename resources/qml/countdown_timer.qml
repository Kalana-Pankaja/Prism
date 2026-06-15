import QtQuick 2.15

Rectangle {
    width: 1280
    height: 720
    color: "#0d0d0d"

    property int totalSeconds: 600
    property int remaining: totalSeconds

    Column {
        anchors.centerIn: parent
        spacing: 20

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "COUNTDOWN"
            color: "#546e7a"
            font.pixelSize: 28
            font.bold: true
            font.letterSpacing: 12
        }

        Text {
            id: countdownText
            anchors.horizontalCenter: parent.horizontalCenter
            text: {
                var m = Math.floor(remaining / 60)
                var s = remaining % 60
                return (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s
            }
            color: remaining <= 60 ? "#ff5252" : (remaining <= 180 ? "#ffab40" : "#00e5ff")
            font.pixelSize: 160
            font.bold: true
            font.family: "Monospace"

            Behavior on color { ColorAnimation { duration: 300 } }
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 600
            height: 6
            radius: 3
            color: "#1e2a30"

            Rectangle {
                width: parent.width * (remaining / totalSeconds)
                height: parent.height
                radius: parent.radius
                color: remaining <= 60 ? "#ff5252" : (remaining <= 180 ? "#ffab40" : "#00e5ff")

                Behavior on width { NumberAnimation { duration: 900 } }
                Behavior on color { ColorAnimation { duration: 300 } }
            }
        }
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: if (remaining > 0) remaining--
    }
}
