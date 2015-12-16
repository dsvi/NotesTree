#ifndef NOTESTREE_H
#define NOTESTREE_H

#include "ui_NotesTree.h"

#include "Note.h"
#include "NotesTreeModel.h"

class NotesTree : public QWidget
{
	Q_OBJECT

public:
	explicit NotesTree(QWidget *parent = 0);

	void root(Note *root);
signals:
	void noteActivated(std::weak_ptr<Note> n);

private slots:
	void addNew();
	void removeSelected();

private:
	Ui::NotesTree ui;
	NotesTreeModel  notesTreeModel_;
};

#endif // NOTESTREE_H
