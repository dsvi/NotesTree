import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQml.Models 2.2
import ds.OverNote 1.0


Item{
	NotesTreeModel {
		id : notesListModel
	}

	TreeView {
		anchors.fill: parent
		TableViewColumn {
			role: "display"
		}
		//alternatingRowColors : false
		headerVisible : false
		model : notesListModel
		onPressAndHold :{
			//console.log(TreeView.view.model.data(index));
			console.log(notesListModel.data(index));
			notesListModel.setData(index, notesListModel.data(index) + " tada!");

		}

//		itemDelegate: Rectangle {
//			color: ( styleData.row % 2 == 0 ) ? "white" : "lightblue"
//			height: 20

//			Text {
//					anchors.verticalCenter: parent.verticalCenter
//					anchors.left: parent.left // by default x is set to 0 so this had no effect
//					text: styleData.value
//			}
//		selectionMode: SelectionMode.ExtendedSelection
//		selection: ItemSelectionModel{}
//		}
	}
}
