#include "NoteSearchProxyModel.h"
#include "NotesTreeModel.h"

NoteSearchProxyModel::NoteSearchProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent)
{
	// with false, rows inserted after sort() would keep source order
	setDynamicSortFilter(true);
	refresh_.setSingleShot(true);
	refresh_.setInterval(0);
	connect(&refresh_, &QTimer::timeout, this, &NoteSearchProxyModel::refresh);
}

void NoteSearchProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
	QSortFilterProxyModel::setSourceModel(sourceModel);
	if (sourceModel)
		connect(sourceModel, &QAbstractItemModel::dataChanged,
		        this, &NoteSearchProxyModel::sourceDataChanged);
}

void NoteSearchProxyModel::sourceDataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &roles)
{
	pending_ |= roles.contains(NotesTreeModel::MatchRole) ? Filter : Sort;
	refresh_.start();
}

void NoteSearchProxyModel::refresh()
{
	// a mark deep in the tree can uncover its ancestors, so the whole filter is
	// re-evaluated rather than only the rows that changed
	if (pending_ & Sort){
		invalidate();
	}else{
		beginFilterChange();
		endFilterChange(Direction::Rows);
	}
	pending_ = 0;
}

const NotesTreeModel *NoteSearchProxyModel::treeModel() const
{
	return static_cast<const NotesTreeModel*>(sourceModel());
}

bool NoteSearchProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	auto m = treeModel();
	if (!m || !m->isSearching())
		return true;
	return markedInSubtree(m->index(sourceRow, 0, sourceParent));
}

bool NoteSearchProxyModel::markedInSubtree(const QModelIndex &sourceIndex) const
{
	auto m = treeModel();
	if (m->data(sourceIndex, NotesTreeModel::MatchRole).toBool())
		return true;
	for (int r = 0; r < m->rowCount(sourceIndex); ++r)
		if (markedInSubtree(m->index(r, 0, sourceIndex)))
			return true;
	return false;
}

bool NoteSearchProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	// EditRole is the bare name, DisplayRole carries the 📎 and ✅ prefixes
	auto l = sourceModel()->data(left, Qt::EditRole).toString();
	auto r = sourceModel()->data(right, Qt::EditRole).toString();
	return l.compare(r, Qt::CaseInsensitive) < 0;
}
