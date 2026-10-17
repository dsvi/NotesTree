#ifndef NOTESTREEMODEL_H
#define NOTESTREEMODEL_H

#include "Note.h"

class NotesTreeModel;

class NoteInTree : public QObject
{
	Q_OBJECT
public:
	NoteInTree(std::weak_ptr<Note> n, QThread *viewThread);
	QString name;
	std::weak_ptr<Note> note;
	NotesTreeModel *model;
	bool    hasAttach;
	bool    isMarked = false;
	NoteInTree *parent;
	std::vector<std::shared_ptr<NoteInTree>> children;
	enum SearchType{
		WholePhrase,
		AllTheWords,
	};
	/// marks this note and all its subnotes which match the model's keywords.
	/// the note text arrives asynchronously, so marks keep landing after the
	/// call returns
	void markAll();
	/// clears the marks of this note and all its subnotes
	void unmarkAll();
	/// forgets the note text fetched for the previous search
	void dropTextCache();
private:
	bool    isLoadStarted = false;
	QString cachedTxt;
signals:
	void changeName(const QString &name);
	void adopt(const std::vector<std::weak_ptr<Note>> &list);
	void createSubnote(const QString &name);
	void deleteRecursively();
public slots:
	void addSubnote(std::shared_ptr<NoteInTree> n);
	void removeThis();
	void nameChanged(const QString &name);
	void gotAttach();
private:
	void mark();
};

Q_DECLARE_METATYPE(std::shared_ptr<NoteInTree>)

class NotesTreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	enum Roles {
		NotePtrRole = Qt::UserRole + 1,
		/// bool: the note matches the search currently running
		MatchRole,
	};

	explicit NotesTreeModel(QObject *parent = nullptr);
	void root(Note *root);

	// QAbstractItemModel interface
	QVariant data(const QModelIndex &index, int role) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

	/// ⭣ this is required for drag und drop support
	Qt::DropActions supportedDropActions() const override;
	QStringList mimeTypes() const override;
	QMimeData *mimeData(const QModelIndexList &indexes) const override;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
	/// ⭡ this is required for drag und drop support

	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QModelIndex index(int row, int column,
											const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;

	/// the live index of item, or an invalid one if item is the root or is gone
	QModelIndex indexOfItem(const NoteInTree *item) const;
	bool isSearching() const;

	NoteInTree *noteAt(const QModelIndex &);
	NoteInTree *noteAt(const QModelIndex &) const;
signals:
	/// emitted by setData() before the rename is handed to the note, so the
	/// editor can flush the note it is showing while the old name is still valid
	void aboutToRename(std::weak_ptr<Note> n);
	void deleteRecursively(const std::vector<std::weak_ptr<Note> > &notes);
public slots:
	void clear();
	/// if parentNote invalid, than add to root
	void addNote(const QModelIndex &parentNote, const QString &name);
	void removeNotes(const QModelIndexList &noteNdx);

	void searchFor(const QString &str, NoteInTree::SearchType);
	void endSearch();
private:
	std::unique_ptr<NoteInTree> root_;
	Note      *rootNote_;
	std::vector<QString> searchKeywords_;
	QString mimeType_ = "application/x.notes-tree";

	void noteMarked(const NoteInTree *item);
	void noteRenamed(const NoteInTree *item);

	friend class NoteInTree;
};


inline
NoteInTree *NotesTreeModel::noteAt(const QModelIndex &noteNdx)
{
	NoteInTree *note;
	if (noteNdx.isValid())
		note = static_cast<NoteInTree*>(noteNdx.internalPointer());
	else
		note = root_.get();
	return note;
}

inline
NoteInTree *NotesTreeModel::noteAt(const QModelIndex &noteNdx) const
{
	return mut_this->noteAt(noteNdx);
}

inline
bool NotesTreeModel::isSearching() const
{
	return !searchKeywords_.empty();
}

#endif // NOTESTREEMODEL_H
