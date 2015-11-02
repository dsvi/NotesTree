#include "NotesTreeActions.h"
#include "AddNewNoteDialog.h"

NotesTreeActions::NotesTreeActions(QAbstractItemView *treeToApplyTo)
{
	treeView_ = treeToApplyTo;
	ASSERT(treeModel());
}

void NotesTreeActions::addNew()
{
	NotesTreeModel *mdl = treeModel();
	auto sm = treeView_->selectionModel();
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
	mdl->addNote(mdlNdx, r->name);
}

void NotesTreeActions::removeSelected()
{
	auto sm = treeView_->selectionModel();
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
	NotesTreeModel *mdl = treeModel();
	mdl->removeNotes(selected);
}

void NotesTreeActions::selectionChanged()
{
	auto selected = treeView_->selectionModel()->selection();
	if (selected.isEmpty())
		removeSelected_->setEnabled(false);
	else
		removeSelected_->setEnabled(true);
}

QAction *NotesTreeActions::getAddNew()
{
	if (!addNew_){
		addNew_ = std::make_unique<QAction>(QIcon(":/ico/add"), tr("Add"), this);
		addNew_->setShortcuts(QKeySequence::New);
		addNew_->setToolTip(tr("Create a new note. As a subnote to the selected note, or at root of the notes tree"));
		//addNew_->setStatusTip(tr("Create new note"));
		connect(addNew_.get(), &QAction::triggered, this, &NotesTreeActions::addNew);
	}
	return addNew_.get();
}

QAction *NotesTreeActions::getRemoveSelected()
{
	if (!removeSelected_){
		removeSelected_ = std::make_unique<QAction>(QIcon(":/ico/remove"), tr("Delete"), this);
		removeSelected_->setShortcuts(QKeySequence::Delete);
		removeSelected_->setToolTip(tr("Delete selected note and its subnotes"));
		removeSelected_->setEnabled(false);
		auto sm = treeView_->selectionModel();
		connect(sm, &QItemSelectionModel::selectionChanged, this, &NotesTreeActions::selectionChanged);
		connect(removeSelected_.get(), &QAction::triggered, this, &NotesTreeActions::removeSelected);
	}
	return removeSelected_.get();
}

