#include "NotesTreeModel.h"
#include "ByteArraySerializer.h"

NotesTreeModel::NotesTreeModel(QObject *parent)
	: QAbstractItemModel(parent)
{

}


QVariant NotesTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole && role != Qt::EditRole)
		return QVariant();

	Note *item = static_cast<Note*>(index.internalPointer());

	return item->name();
}

bool NotesTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
		return false;

	try{
		Note *item = static_cast<Note*>(index.internalPointer());
		item->name(value.toString());
	}
	catch(...){
		emit app->fatal(std::current_exception());
	}

	emit dataChanged(index, index, QVector<int>{Qt::DisplayRole});
	return true;
}

Qt::DropActions NotesTreeModel::supportedDropActions() const
{
	return Qt::MoveAction;
}

QStringList NotesTreeModel::mimeTypes() const
{
	return QStringList{mimeType_};
}

QMimeData *NotesTreeModel::mimeData(const QModelIndexList &indexes) const
{
	QByteArray ba;
	ByteArraySerializer bs(&ba);
	bs.add(indexes.size());
	for (auto &ndx : indexes){
		Note *note = static_cast<Note*>(ndx.internalPointer());
		bs.add(note);
	}
	QMimeData *ret = new QMimeData();
	ret->setData(mimeType_, ba);
	return ret;
}

bool NotesTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	Q_UNUSED(row)
	Q_UNUSED(column)
	ASSERT(action == Qt::MoveAction);
	try{
		layoutAboutToBeChanged();
		Note *parentNote;
		if (!parent.isValid())
			parentNote = &root_;
		else
			parentNote = static_cast<Note*>(parent.internalPointer());
		auto ba = data->data(mimeType_);
		ByteArrayDeSerializer bd(&ba);
		decltype(ba.size()) numNotes;
		bd.get(numNotes);
		std::vector<Note*> list;
		std::set<QString> existanceCheck;
		for (int num = 0; num < numNotes; ++num){
			Note *n;
			bd.get(n);
			QString name = n->name();
			if (parentNote->exist(name) || existanceCheck.find(name) != existanceCheck.end()){
				app->showErrorDilog(make_exception_ptr(Exception(
					"notes moving",
					"Note names have to be unique in the subnotes list. Can't move '%1'\n"
					"Skipped.\n"
					"FYI the skipped one was originally a subnote of\n"
					"'%2'.\n"
					"... and might still be.") % name % n->makePathName() ));
				continue;
			}
			existanceCheck.insert(name);
			list.push_back(n);
		}
		parentNote->adopt(std::move(list));
		layoutChanged();
	}
	catch(...){
		app->fatal(std::current_exception());
	}
	return true;
}

Qt::ItemFlags NotesTreeModel::flags(const QModelIndex &) const
{
	return
	Qt::ItemIsSelectable  |
	Qt::ItemIsEditable    |
	Qt::ItemIsDragEnabled |
	Qt::ItemIsDropEnabled |
	Qt::ItemIsEnabled     ;
}


QModelIndex NotesTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	const Note *parentItem;

	if (!parent.isValid())
		parentItem = &root_;
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

	if (parentItem == &root_)
		return QModelIndex();

	return createIndex(parentItem->findIndexOf(childItem), 0, parentItem);
}

int NotesTreeModel::rowCount(const QModelIndex &parent) const
{
	const Note *parentItem;

	if (!parent.isValid())
		parentItem = &root_;
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
	try{
		root_.createHierarchyFromRoot(QDir{path});
	}
	catch(...){
		emit app->showErrorDilog(std::current_exception());
	}
	emit modelChanged();
}



