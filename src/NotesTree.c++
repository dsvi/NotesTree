#include "NotesTree.h"
#include "AddNewNoteDialog.h"

NotesTree::NotesTree(QWidget *parent) :
  QWidget(parent)
{
	ui.setupUi(this);
}

void NotesTree::root(Note *root)
{
	notesTreeModel_.root(root);
	auto treeView = ui.notesView;
	treeView->setModel(&notesTreeModel_);
	connect(ui.notesView->selectionModel(), &QItemSelectionModel::currentChanged, [this](const QModelIndex & index){
		emit noteActivated(notesTreeModel_.noteAt(index)->note);
	});

	QAction *addNew = new QAction(QIcon(":/ico/add"), QString(), this);
	//QAction *addNew = new QAction(QIcon::fromTheme("list-add"), QString(), this);
	addNew->setShortcuts(QKeySequence::New);
	addNew->setToolTip(tr("Create a new note. As a subnote to the selected note, or at root of the notes tree"));
	connect(addNew, &QAction::triggered, this, &NotesTree::addNew);
	app->addToolButton(this, ui.toolBoxLayout, addNew);

	QAction	*removeSelected = new QAction(QIcon(":/ico/remove"), QString(), this);
	//QAction	*removeSelected = new QAction(QIcon::fromTheme("list-remove"), QString(), this);
	removeSelected->setShortcuts(QKeySequence::Delete);
	removeSelected->setToolTip(tr("Delete selected note and its subnotes"));
	removeSelected->setEnabled(false);
	auto sm = treeView->selectionModel();
	connect(sm, &QItemSelectionModel::selectionChanged, [=](){
		auto selected = treeView->selectionModel()->selection();
		if (selected.isEmpty())
			removeSelected->setEnabled(false);
		else
			removeSelected->setEnabled(true);
	});
	connect(removeSelected, &QAction::triggered, this, &NotesTree::removeSelected);
	app->addToolButton(this, ui.toolBoxLayout, removeSelected);

	ui.toolBoxLayout->addStretch();
}

void NotesTree::addNew()
{
	auto sm = ui.notesView->selectionModel();
	AddNewNoteDialog dialog(app->mainWindow());
	auto selected = sm->selectedIndexes();
	if (selected.size() != 1)
		dialog.hideWhereToAddSelection();
	dialog.adjustSize();
	if ( dialog.exec() == QDialog::Rejected )
		return;
	auto r = dialog.result();
	QModelIndex mdlNdx;
	if (selected.size() == 1 && !r->addToRoot)
		mdlNdx = selected[0];
	notesTreeModel_.addNote(mdlNdx, r->name);
}

void NotesTree::removeSelected()
{
	auto sm = ui.notesView->selectionModel();
	auto selected = sm->selectedIndexes();
	if (selected.isEmpty())
		return;
	QMessageBox dialog(app->mainWindow());
	if (selected.size() == 1)
		dialog.setText(tr("Delete the note and all it's subnotes?"));
	else
		dialog.setText(tr(
			"Delete the %1 selected notes and all their subnotes?", "number of selected notes")
			.arg(selected.size()));
	dialog.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	dialog.setDefaultButton(QMessageBox::No);
	if ( dialog.exec() != QMessageBox::Yes )
		return;
	notesTreeModel_.removeNotes(selected);
}
