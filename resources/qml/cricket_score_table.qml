import QtQuick 2.15

Rectangle {
    width: 1280
    height: 720
    color: "#0d0f14"

    Column {
        anchors.centerIn: parent
        width: 960
        spacing: 0

        // Title bar
        Rectangle {
            width: parent.width
            height: 52
            color: "#1565c0"

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 16
                text: "INDIA vs AUSTRALIA  —  1st ODI"
                color: "white"
                font.pixelSize: 20
                font.bold: true
                font.letterSpacing: 1
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 16
                text: "● LIVE"
                color: "#ff5252"
                font.pixelSize: 14
                font.bold: true
                font.letterSpacing: 3
            }
        }

        // Column header
        Rectangle {
            width: parent.width
            height: 36
            color: "#1c2733"
            Row {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 0
                Text { width: 260; text: "BATSMAN";   color: "#607d8b"; font.pixelSize: 13; font.bold: true; font.letterSpacing: 2 }
                Text { width: 80;  text: "R";          color: "#607d8b"; font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                Text { width: 80;  text: "B";          color: "#607d8b"; font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                Text { width: 70;  text: "4s";         color: "#607d8b"; font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                Text { width: 70;  text: "6s";         color: "#607d8b"; font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                Text { width: 90;  text: "SR";         color: "#607d8b"; font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                Text { width: 200; text: "DISMISSAL";  color: "#607d8b"; font.pixelSize: 13; font.bold: true }
            }
        }

        // Batting rows
        Repeater {
            model: [
                { name: "R. Sharma",    runs: 64,  balls: 58, fours: 8, sixes: 3, sr: "110.3", how: "c Maxwell b Starc",   batting: false },
                { name: "S. Dhawan",    runs: 32,  balls: 40, fours: 3, sixes: 1, sr: "80.0",  how: "b Cummins",           batting: false },
                { name: "V. Kohli *",   runs: 87,  balls: 92, fours: 9, sixes: 2, sr: "94.6",  how: "not out",             batting: true  },
                { name: "K.L. Rahul †", runs: 45,  balls: 61, fours: 4, sixes: 0, sr: "73.8",  how: "lbw b Hazlewood",     batting: false },
                { name: "H. Pandya",    runs: 28,  balls: 22, fours: 2, sixes: 1, sr: "127.3", how: "c Smith b Zampa",     batting: false },
                { name: "R. Jadeja",    runs: 18,  balls: 14, fours: 1, sixes: 1, sr: "128.6", how: "not out",             batting: true  },
            ]

            Rectangle {
                width: 960
                height: 42
                color: modelData.batting ? "#0f2030" : (index % 2 === 0 ? "#131a23" : "#161e29")

                Rectangle {
                    anchors.left: parent.left
                    width: 3
                    height: parent.height
                    color: modelData.batting ? "#00e5ff" : "transparent"
                }

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 0
                    Text { width: 260; text: modelData.name;  color: modelData.batting ? "white" : "#cfd8dc"; font.pixelSize: 15; font.bold: modelData.batting }
                    Text { width: 80;  text: modelData.runs;  color: "#ffcc02"; font.pixelSize: 16; font.bold: true; font.family: "Monospace"; horizontalAlignment: Text.AlignHCenter }
                    Text { width: 80;  text: modelData.balls; color: "#90a4ae"; font.pixelSize: 14; font.family: "Monospace"; horizontalAlignment: Text.AlignHCenter }
                    Text { width: 70;  text: modelData.fours; color: "#4fc3f7"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter }
                    Text { width: 70;  text: modelData.sixes; color: "#81c784"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter }
                    Text { width: 90;  text: modelData.sr;    color: "#90a4ae"; font.pixelSize: 14; font.family: "Monospace"; horizontalAlignment: Text.AlignHCenter }
                    Text { width: 200; text: modelData.how;   color: "#546e7a"; font.pixelSize: 13; elide: Text.ElideRight }
                }
            }
        }

        // Extras
        Rectangle {
            width: parent.width
            height: 36
            color: "#111920"
            Row {
                anchors.fill: parent
                anchors.leftMargin: 16
                spacing: 12
                Text { text: "Extras";                              color: "#546e7a"; font.pixelSize: 14 }
                Text { text: "13";                                  color: "#90a4ae"; font.pixelSize: 14; font.family: "Monospace" }
                Text { text: "(b 4, lb 5, wd 3, nb 1)";            color: "#37474f"; font.pixelSize: 12 }
            }
        }

        // Total
        Rectangle {
            width: parent.width
            height: 44
            color: "#1a2840"

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 16
                text: "IND  287 / 6  (47.2 Overs)"
                color: "#00e5ff"
                font.pixelSize: 18
                font.bold: true
                font.family: "Monospace"
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 16
                text: "CRR: 6.10   RRR: 3.83"
                color: "#607d8b"
                font.pixelSize: 14
                font.family: "Monospace"
            }
        }
    }
}
