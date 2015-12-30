#include "NotesTreeModel.h"
#include "ByteArraySerializer.h"

using namespace std;

NotesTreeModel::NotesTreeModel(QObject *parent)
	: QAbstractItemModel(parent)
{
}

void NotesTreeModel::root(Note *root)
{
	root_ = make_unique<NoteInTree>(shared_ptr<Note>(root,[](auto){}), thread());
	root_->model = this;
	root_->parent = nullptr;
	rootNote_ = root;
	connect(rootNote_, &Note::clear, this, &NotesTreeModel::clear);
	connect(this, &NotesTreeModel::deleteRecursively, rootNote_, &Note::deleteRecursively);
}


QVariant NotesTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role != Qt::DisplayRole && role != Qt::EditRole)
		return QVariant();
	NoteInTree *item = noteAt(index);
	return item->name;
}

bool NotesTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
		return false;

	NoteInTree *item = noteAt(index);
	item->changeName(value.toString());

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
		NoteInTree *note = noteAt(ndx);
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
	Q_UNUSED(action)
	ASSERT(action == Qt::MoveAction);
	NoteInTree *parentNote = noteAt(parent);
	auto ba = data->data(mimeType_);
	ByteArrayDeSerializer bd(&ba);
	decltype(ba.size()) numNotes;
	bd.get(numNotes);
	std::vector<weak_ptr<Note>> list;
	for (int num = 0; num < numNotes; ++num){
		NoteInTree *n;
		bd.get(n);
		list.push_back(n->note);
	}
	parentNote->adopt(list);
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

	const NoteInTree *parentItem = noteAt(parent);

	ASSERT((size_t) row < parentItem->children.size());
	NoteInTree *childItem = parentItem->children[row].get();
	childItem->mdlNdx = createIndex(row, column, childItem);
	ASSERT(childItem->mdlNdx.isValid());
	return childItem->mdlNdx;
}

QModelIndex NotesTreeModel::parent(const QModelIndex &index) const
{
	NoteInTree *childItem = noteAt(index);
	if (childItem == root_.get())
		return QModelIndex();
	return childItem->parent->mdlNdx;
}

int NotesTreeModel::rowCount(const QModelIndex &parent) const
{
	const NoteInTree *parentItem = noteAt(parent);
	return parentItem->children.size();
}

int NotesTreeModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return 1;
}

void NotesTreeModel::clear()
{
	beginResetModel();
	root_->children.clear();
	endResetModel();
}

void NotesTreeModel::addNote(const QModelIndex &parentNote, const QString &name)
{
	NoteInTree *parent = noteAt(parentNote);
	parent->createSubnote(name);
}

void NotesTreeModel::removeNotes(const QModelIndexList &noteNdx)
{
	vector<std::weak_ptr<Note>> notes;
	for (const auto &ndx : noteNdx){
		ASSERT(ndx.isValid());
		notes.push_back(noteAt(ndx)->note);
	}
	emit deleteRecursively(notes);
}

NoteInTree::NoteInTree(std::weak_ptr<Note> n, QThread *viewThread) : QObject(nullptr)
{
	this->moveToThread(viewThread);
	auto ns = n.lock();
	ASSERT(ns);
	auto note = ns.get();
	this->name = note->name();
	this->note = n;
	connect(this, &NoteInTree::changeName, note, &Note::changeName);
	connect(note, &Note::nameChanged, this, &NoteInTree::nameChanged);

	connect(this, &NoteInTree::adopt, note, &Note::adopt);
	connect(this, &NoteInTree::createSubnote, note, &Note::createSubnote);

	auto c = connect(note, &Note::noteAdded, [=](weak_ptr<Note> mn){
		auto vn = make_shared<NoteInTree>(mn, viewThread);
		QMetaObject::invokeMethod(this, "addSubnote", Qt::QueuedConnection, Q_ARG(std::shared_ptr<NoteInTree>, vn));
	});
	connect(this, &NoteInTree::destroyed, [=](){
		this->disconnect(c);
	});

	connect(note, &Note::noteRemoved, this, &NoteInTree::removeThis);
}

void NoteInTree::addSubnote(std::shared_ptr<NoteInTree> n)
{
	model->layoutAboutToBeChanged();
	children.emplace_back(n);
	auto newNote = children.back().get();
	newNote->model = model;
	newNote->parent = this;
	sortKids();
	model->layoutChanged();
}

void NoteInTree::removeThis()
{
	if ( this == model->root_.get() )
		return;
	model->beginRemoveRows(mdlNdx.parent(), mdlNdx.row(), mdlNdx.row());
	auto &l = parent->children;
	auto it = find_if(l.begin(), l.end(), [&](auto &p){return p.get() == this;});
	ASSERT(it != l.end());
	auto tmp = *it;
	l.erase(it);
	model->endRemoveRows();
}

void NoteInTree::nameChanged(const QString &name)
{
	this->name = name;
	model->dataChanged(mdlNdx, mdlNdx, QVector<int>{Qt::DisplayRole});
	model->layoutAboutToBeChanged();
	parent->sortKids();
	model->layoutChanged();
}

void NoteInTree::sortKids()
{
	std::sort(children.begin(), children.end(), [](const auto &a, const auto &b){
		return a->name.compare(b->name, Qt::CaseInsensitive) < 0;
	});
}



