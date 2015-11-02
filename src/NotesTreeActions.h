#ifndef NOTESTREEACTIONS_H
#define NOTESTREEACTIONS_H

#include "NotesTreeModel.h"


class NotesTreeActions : public QObject
{
	Q_OBJECT
public:
	/// \p treeToApplyTo shall have model set
	explicit
	NotesTreeActions(QAbstractItemView *treeToApplyTo);

	QAction *getAddNew();
	QAction *getRemoveSelected();

signals:

private slots:
	void addNew();
	void removeSelected();
	void selectionChanged();

private:
	QAbstractItemView *treeView_;
	std::unique_ptr<QAction> addNew_;
	std::unique_ptr<QAction> removeSelected_;

	NotesTreeModel *treeModel();


};

inline
NotesTreeModel *NotesTreeActions::treeModel()
{
	return static_cast<NotesTreeModel*>(treeView_->model());
}


#endif // NOTESTREEACTIONS_H
