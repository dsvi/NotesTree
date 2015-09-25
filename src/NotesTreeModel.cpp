#include "NotesTreeModel.h"


NotesTreeModel::NotesTreeModel(QObject *parent)
	: QAbstractItemModel(parent)
{
	root.createHierarchyFromRoot(QDir{"/home/ds/OTest"});
}

//bool NotesTreeModel::setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)
//{
//	return true;
//}

QVariant NotesTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
			return QVariant();

	if (role != Qt::DisplayRole)
			return QVariant();

	Note *item = static_cast<Note*>(index.internalPointer());

	return item->name();
}

bool NotesTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
		return false;

	Note *item = static_cast<Note*>(index.internalPointer());
	item->name(value.toString());

	//emit dataChanged(index, index, QVector<int>{Qt::DisplayRole});
	emit dataChanged(index, index);
	return true;
}

bool NotesTreeModel::insertRows(int row, int count, const QModelIndex &parent)
{
	return true;
}

//QHash<int, QByteArray> NotesTreeModel::roleNames() const
//{
//	QHash<int, QByteArray> roles;
//	roles[NameRole] = "name";
//	return roles;
//}

//bool NotesTreeModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
//{

//}

Qt::ItemFlags NotesTreeModel::flags(const QModelIndex &index) const
{
//	if (!index.isValid())
//			return 0;

//	return Qt::ItemIsEnabled | Qt::ItemIsEditable | QAbstractItemModel::flags(index);

//	if (!index.isValid())
//			return 0;
	return
	Qt::ItemIsSelectable  |
	Qt::ItemIsEditable    |
	Qt::ItemIsDragEnabled |
	Qt::ItemIsDropEnabled |
	Qt::ItemIsEnabled     ;
	//return QAbstractItemModel::flags(index);
}


QModelIndex NotesTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
			return QModelIndex();

	const Note *parentItem;

	if (!parent.isValid())
			parentItem = &root;
	else
			parentItem = static_cast<Note*>(parent.internalPointer());

	const Note *childItem = parentItem->child(row);
	if (childItem)
			return createIndex(row, column, (void*)childItem);
	else
			return QModelIndex();
}

QModelIndex NotesTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
			return QModelIndex();

	Note *childItem = static_cast<Note*>(index.internalPointer());
	Note *parentItem = childItem->parent();

	if (parentItem == &root)
			return QModelIndex();

	return createIndex(parentItem->findIndexOf(childItem), 0, parentItem);
}

int NotesTreeModel::rowCount(const QModelIndex &parent) const
{
	const Note *parentItem;
	if (parent.column() > 0)
			return 0;

	if (!parent.isValid())
			parentItem = &root;
	else
			parentItem = static_cast<Note*>(parent.internalPointer());

	return parentItem->numChildren();
}

int NotesTreeModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return 1;
}

void NotesTreeModel::rootPath(QString path)
{
	//TODO: try/catch
	root.createHierarchyFromRoot(QDir{path});
	emit modelChanged();
}

