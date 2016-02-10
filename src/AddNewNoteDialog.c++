#include "AddNewNoteDialog.h"

AddNewNoteDialog::AddNewNoteDialog(QWidget *parent) :
  QDialog(parent)
{
	ui.setupUi(this);
}

void AddNewNoteDialog::hideWhereToAddSelection()
{
	ui.whereToAddGroup->hide();
}


void AddNewNoteDialog::accept()
{
	res_.name = ui.nameEdit->text();
	if (res_.name.isEmpty())
		QDialog::reject();
	if (!ui.whereToAddGroup->isHidden())
		res_.addToRoot = ui.addToRootRB->isChecked();
	QDialog::accept();
}
