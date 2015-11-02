#ifndef NOTESTREEMODEL_H
#define NOTESTREEMODEL_H

#include "Note.h"

class NotesTreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	enum Roles {
		NameRole = Qt::UserRole + 1,
	};

	explicit NotesTreeModel(QObject *parent = nullptr);

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

	Note *noteAt(const QModelIndex &);

signals:


public slots:
	void rootPath(QString path);
	/// if parentNote invalid, than add to root
	void addNote(const QModelIndex &parentNote, const QString &name);
	void removeNotes(const QModelIndexList &noteNdx);

private:
	Note root_;
	QString mimeType_ = "application/x.overnote-note";
};


inline
Note *NotesTreeModel::noteAt(const QModelIndex &noteNdx)
{
	Note *note = static_cast<Note*>(noteNdx.internalPointer());
	return note;
}

#endif // NOTESTREEMODEL_H
