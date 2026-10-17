#ifndef NOTESEARCHPROXYMODEL_H
#define NOTESEARCHPROXYMODEL_H
#define NOTEREARCHPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QTimer>

class NotesTreeModel;

/// Hides the notes that do not match the search NotesTreeModel is running, and
/// orders the rest by name. The source model always exposes the whole tree.
class NoteSearchProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	explicit NoteSearchProxyModel(QObject *parent = nullptr);

	void setSourceModel(QAbstractItemModel *sourceModel) override;

private slots:
	void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
	                       const QVector<int> &roles);
	void refresh();

private:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
	bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
	bool markedInSubtree(const QModelIndex &sourceIndex) const;

	const NotesTreeModel *treeModel() const;

	enum Refresh {
		Filter = 1,
		Sort   = 2,
	};

	int    pending_ = 0;
	QTimer refresh_;
};

#endif // NOTESEARCHPROXYMODEL_H
