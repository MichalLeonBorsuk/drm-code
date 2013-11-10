import QtQuick 2.0

Rectangle {
    property alias text: textItem.text
    width: 156
    height: 35
    Text {
        width: 150
        height: 20
        text: qsTr("Hello World")
        id: textItem
    }}
