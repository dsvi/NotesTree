import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

Dialog {
		visible: true
		title: "Blue sky dialog"

		contentItem: Rectangle {
				color: "lightskyblue"
				implicitWidth: 400
				implicitHeight: 100
				Text {
						text: "Hello blue sky!"
						color: "navy"
						anchors.centerIn: parent
				}
		}
}
