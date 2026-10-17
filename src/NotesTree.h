#ifndef NOTESTREE_H
#define NOTESTREE_H

#include "ui_NotesTree.h"

#include "Note.h"
#include "NotesTreeModel.h"
#include "NoteSearchProxyModel.h"

class PathsCollector : public QObject{
	Q_OBJECT
public:
	explicit PathsCollector(QObject *parent = nullptr) : QObject(parent) {}
	void startCollectingFor(const std::vector<std::weak_ptr<Note>> &notes);
public slots:
	void collect(const std::vector<QString> &);
signals:
	void collected(const std::vector<QString> &uris);
	void givePaths();
private:
	std::vector<QString> uris_;
	int                  total_ = 0;
};

class NotesTree : public QWidget
{
	Q_OBJECT

public:
	explicit NotesTree(QWidget *parent = 0);
	/// should be called only once
	void root(Note *root);
signals:
	void noteActivated(std::weak_ptr<Note> n);
	/// a rename is about to start for that note; see NoteEditor::aboutToRenameNote()
	void noteAboutToRename(std::weak_ptr<Note> n);
private slots:
	void addNew();
	void removeSelected();
	void copySelectedPathsToClipboard();

	void searchFor(const QString &str, NoteInTree::SearchType t);
	void endSearch();

private:
	Ui::NotesTree ui;
	NotesTreeModel    notesTreeModel_;
	NoteSearchProxyModel notesProxy_;
	std::vector<QMetaObject::Connection> attachConnections_;

	/// view indexes belong to notesProxy_; the model speaks source indexes
	NoteInTree *noteAt(const QModelIndex &viewNdx) const;
	QModelIndex toSource(const QModelIndex &viewNdx) const;
	QModelIndexList toSource(const QModelIndexList &viewNdxs) const;
};



#endif // NOTESTREE_H
