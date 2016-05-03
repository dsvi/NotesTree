#ifndef NOTESTREE_H
#define NOTESTREE_H

#include "ui_NotesTree.h"

#include "Note.h"
#include "NotesTreeModel.h"

class PathsCollector : public QObject{
	Q_OBJECT
public:
	void startCollectingFor(const std::vector<std::weak_ptr<Note>> &notes);
public slots:
	void collect(const std::vector<QString> &);
signals:
	void collected(const std::vector<QString> &uris);
private:
	std::vector<QString> uris_;
	int                  total_;
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
private slots:
	void addNew();
	void removeSelected();
	void copySelectedPathsToClipboard();

	void searchFor(const QString &str, NoteInTree::SearchType t);
	void endSearch();

private:
	Ui::NotesTree ui;
	NotesTreeModel  notesTreeModel_;
	std::vector<QMetaObject::Connection> attachConnections_;
	std::unique_ptr<PathsCollector> pathsCollector_;
};



#endif // NOTESTREE_H
