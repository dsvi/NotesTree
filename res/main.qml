import QtQuick 2.5
import QtQuick.Window 2.2

Window {
	visible: true

	property real mm: Screen.pixelDensity
	property var notesList : NotesList {}
	property var note: Note {}

	width : 150*mm
	height: 70*mm

	Item{
		anchors.fill: parent

		Loader {
			id: root
			anchors.fill: parent
		}

		onWidthChanged: {
			if ( width > 100*mm ){
				if ( state != "BIG" ){
					console.log("Loading Big");
					state = "BIG";
				}
			}
			else{
				if ( state != "SMALL" ){
					console.log("Loading Small");
					state = "SMALL";
				}
			}
		}

		state: ""

		states: [
				State {
						name: "BIG"
						PropertyChanges { target: root; source : "Big.qml"}
				},
				State {
						name: "SMALL"
						PropertyChanges { target: root; source : "Small.qml"}
				}
		]
	}
	//	MainForm {
//		anchors.fill: parent
//		//		mouseArea.onClicked: {
//		//			Qt.quit();
//		//		}
//	}
}

