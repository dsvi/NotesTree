#ifndef ADDNEWNOTEDIALOG_H
#define ADDNEWNOTEDIALOG_H

#include "ui_AddNewNoteDialog.h"

class AddNewNoteDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AddNewNoteDialog(QWidget *parent = 0);
	void hideWhereToAddSelection();

	struct Result{
		QString name;
		bool addToRoot = true;
	};
	Result *result();

	// QDialog interface
public slots:
	void accept() override;

private:
	Ui::AddNewNoteDialog ui;
	Result res_;

};

inline
AddNewNoteDialog::Result *AddNewNoteDialog::result()
{
	return &res_;
}

#endif // ADDNEWNOTEDIALOG_H
