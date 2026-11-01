import QtQuick

Rectangle {
    width: 480
    height: 320
    color: "#14251e"
    property int score: 0
    Text {
        anchors.centerIn: parent
        color: "#d9f99d"
        text: "YoungLion: " + parent.score + "\nTap to score"
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 28
    }
    MouseArea { anchors.fill: parent; onClicked: parent.score += 1 }
}
