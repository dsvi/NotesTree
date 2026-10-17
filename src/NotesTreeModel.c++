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
	if (role != Qt::DisplayRole && role != Qt::EditRole && role != MatchRole)
		return QVariant();
	NoteInTree *item = noteAt(index);
	if (role == MatchRole)
		return item->isMarked;
	auto ret = item->name;
	if (role == Qt::DisplayRole && item->hasAttach)
		ret.prepend("📎");
	if (role == Qt::DisplayRole && item->isMarked)
		ret.prepend("✅");
	return ret;
}

bool NotesTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
		return false;

	NoteInTree *item = noteAt(index);
	emit aboutToRename(item->note);
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
	if (action != Qt::MoveAction)
		return false;
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
	NoteInTree *parentItem = noteAt(parent);
	return createIndex(row, column, parentItem->children[row].get());
}

QModelIndex NotesTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();
	return indexOfItem(noteAt(index)->parent);
}

QModelIndex NotesTreeModel::indexOfItem(const NoteInTree *item) const
{
	if (!item || item == root_.get())
		return QModelIndex();
	auto &sib = item->parent->children;
	auto it = find_if(sib.begin(), sib.end(), [&](auto &p){ return p.get() == item; });
	if (it == sib.end())
		return QModelIndex();
	return createIndex(it - sib.begin(), 0, const_cast<NoteInTree*>(item));
}

int NotesTreeModel::rowCount(const QModelIndex &parent) const
{
	return noteAt(parent)->children.size();
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

void NotesTreeModel::searchFor(const QString &str, NoteInTree::SearchType t)
{
	if (str.isEmpty()){
		endSearch();
		return;
	}
	searchKeywords_.clear();
	if (t == NoteInTree::WholePhrase)
		searchKeywords_.emplace_back(str);
	else
		for (auto &w : str.split(" ", Qt::SkipEmptyParts))
			searchKeywords_.emplace_back(w);
	root_->unmarkAll();
	root_->markAll();
}

void NotesTreeModel::endSearch()
{
	searchKeywords_.clear();
	root_->unmarkAll();
	root_->dropTextCache();
}

NoteInTree::NoteInTree(std::weak_ptr<Note> n, QThread *viewThread) : QObject(nullptr)
{
	this->moveToThread(viewThread);
	auto ns = n.lock();
	ASSERT(ns);
	auto note = ns.get();
	this->name = note->name();
	this->hasAttach = note->hasAttach();
	this->note = n;
	connect(this, &NoteInTree::changeName, note, &Note::changeName);
	connect(note, &Note::nameChanged, this, &NoteInTree::nameChanged);

	connect(this, &NoteInTree::adopt, note, &Note::adopt);
	connect(this, &NoteInTree::createSubnote, note, &Note::createSubnote);

	// 'this' as context object: the connection dies with this NoteInTree, and
	// the lambda is queued into the view thread, so it can create the child
	// and call addSubnote() directly. The note lives in the io thread and may
	// already be gone by the time the queued call runs.
	connect(note, &Note::noteAdded, this, [this, viewThread](weak_ptr<Note> mn){
		auto sn = mn.lock();
		if (!sn)
			return;
		addSubnote(make_shared<NoteInTree>(sn, viewThread));
	});
	connect(note, &Note::noteRemoved, this, &NoteInTree::removeThis);
	connect(note, &Note::attachReady, this, &NoteInTree::gotAttach);
}

void NoteInTree::addSubnote(std::shared_ptr<NoteInTree> n)
{
	auto row = static_cast<int>(children.size());
	model->beginInsertRows(model->indexOfItem(this), row, row);
	children.emplace_back(n);
	n->model = model;
	n->parent = this;
	model->endInsertRows();
}

void NoteInTree::removeThis()
{
	if ( this == model->root_.get() )
		return;
	auto &l = parent->children;
	auto it = find_if(l.begin(), l.end(), [&](auto &p){return p.get() == this;});
	if (it == l.end())
		return;
	auto row = static_cast<int>(it - l.begin());
	auto self = *it; // keeps this alive while its own entry is erased
	model->beginRemoveRows(model->indexOfItem(parent), row, row);
	l.erase(it);
	model->endRemoveRows();
}

void NoteInTree::nameChanged(const QString &name)
{
	this->name = name;
	model->noteRenamed(this);
}

void NoteInTree::gotAttach()
{
	hasAttach = true;
}

void NoteInTree::unmarkAll()
{
	for (auto &c : children)
		c->unmarkAll();
	if (!isMarked)
		return;
	isMarked = false;
	model->noteMarked(this);
}

void NoteInTree::dropTextCache()
{
	isLoadStarted = false;
	cachedTxt.clear();
	for (auto &c : children)
		c->dropTextCache();
}

void NoteInTree::markAll()
{
	if (!isLoadStarted){
		isLoadStarted = true;
		auto n = note.lock();
		if (n){
			connect(n.get(), &Note::notePlainTextRdy, this, [=,this](const QString &txt){
				disconnect(n.get(), &Note::notePlainTextRdy, this, 0);
				cachedTxt = txt + name;
				mark();
			});
			QMetaObject::invokeMethod(n.get(),"getNotePlainTxt", Qt::QueuedConnection);
		}
	}
	mark();
	for (auto &c : children)
		c->markAll();
}

void NoteInTree::mark()
{
	auto &kwds = model->searchKeywords_;
	if (kwds.empty() || cachedTxt.isEmpty() || isMarked)
		return;
	size_t found = 0;
	for (auto &k :kwds){
		if (cachedTxt.contains(k, Qt::CaseInsensitive))
			found++;
	}
	if (kwds.size() != found)
		return;
	isMarked = true;
	model->noteMarked(this);
}

void NotesTreeModel::noteMarked(const NoteInTree *item)
{
	auto ndx = indexOfItem(item);
	if (!ndx.isValid())
		return;
	emit dataChanged(ndx, ndx, {Qt::DisplayRole, MatchRole});
}

void NotesTreeModel::noteRenamed(const NoteInTree *item)
{
	auto ndx = indexOfItem(item);
	if (!ndx.isValid())
		return;
	emit dataChanged(ndx, ndx, {Qt::DisplayRole, Qt::EditRole});
}




