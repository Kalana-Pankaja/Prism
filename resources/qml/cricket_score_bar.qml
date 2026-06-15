import QtQuick 2.15

Item {
    width: 1280
    height: 720

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 88
        color: "#0d1117"

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 2
            color: "#00e5ff"
        }

        // Team A
        Rectangle {
            id: teamABar
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            width: 4
            height: 50
            radius: 2
            color: "#ff9800"
        }

        Column {
            id: teamACol
            anchors.left: teamABar.right
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Text {
                text: "IND"
                color: "#ff9800"
                font.pixelSize: 20
                font.bold: true
                font.letterSpacing: 3
            }
            Text {
                text: "287 / 6"
                color: "white"
                font.pixelSize: 26
                font.bold: true
                font.family: "Monospace"
            }
        }

        Column {
            id: oversCol
            anchors.left: teamACol.right
            anchors.leftMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Text {
                text: "OV"
                color: "#607d8b"
                font.pixelSize: 11
                font.bold: true
                font.letterSpacing: 2
            }
            Text {
                text: "47.2"
                color: "#90a4ae"
                font.pixelSize: 20
                font.family: "Monospace"
            }
        }

        Rectangle {
            id: divider1
            anchors.left: oversCol.right
            anchors.leftMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: 54
            color: "#263238"
        }

        Text {
            id: requiredText
            anchors.left: divider1.right
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            text: "Need  63  off  16.4 ov"
            color: "#b0bec5"
            font.pixelSize: 18
            font.family: "Monospace"
        }

        Rectangle {
            id: divider2
            anchors.left: requiredText.right
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: 54
            color: "#263238"
        }

        Column {
            id: teamBCol
            anchors.left: divider2.right
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Text {
                text: "AUS"
                color: "#00bcd4"
                font.pixelSize: 20
                font.bold: true
                font.letterSpacing: 3
            }
            Text {
                text: "224 / 10"
                color: "white"
                font.pixelSize: 26
                font.bold: true
                font.family: "Monospace"
            }
        }

        Rectangle {
            anchors.left: teamBCol.right
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            width: 4
            height: 50
            radius: 2
            color: "#00bcd4"
        }
    }
}
