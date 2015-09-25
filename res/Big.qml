import QtQuick 2.5
import QtQuick.Controls 1.4



SplitView {
	id : splitView
	orientation: Qt.Horizontal

	//property alias mouseArea: mouseArea

	Component.onCompleted: {
		splitView.addItem( notesList );
		splitView.addItem( note );
		notesList.width = 70*mm;
		note.width = root.width - notesList.width;
	}
}


