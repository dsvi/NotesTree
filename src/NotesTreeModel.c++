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
	auto ret = item->name;
	if (role == Qt::DisplayRole && item->isMarked)
		ret.prepend("✅");
	return ret;
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

	ASSERT((size_t) row < parentItem->shown.size());
	NoteInTree *childItem = parentItem->shown[row];
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
	return parentItem->shown.size();
}

int NotesTreeModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return 1;
}

void NotesTreeModel::clear()
{
	beginResetModel();
	root_->shown.clear();
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
	root_->markAll(str, t);
}

void NotesTreeModel::endSearch()
{
	root_->showAndResetMarksAll();
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

	auto c = connect(note, &Note::noteAdded, [=](weak_ptr<Note> mn){
		auto vn = make_shared<NoteInTree>(mn, viewThread);
		QMetaObject::invokeMethod(this, "addSubnote", Qt::QueuedConnection, Q_ARG(std::shared_ptr<NoteInTree>, vn));
	});
	connect(this, &NoteInTree::destroyed, [=](){
		this->disconnect(c);
	});

	connect(note, &Note::noteRemoved, this, &NoteInTree::removeThis);
	connect(note, &Note::attachReady, this, &NoteInTree::gotAttach);
}

void NoteInTree::addSubnote(std::shared_ptr<NoteInTree> n)
{
	model->layoutAboutToBeChanged();
	children.emplace_back(n);
	shown.emplace_back(n.get());
	auto newNote = children.back().get();
	newNote->model = model;
	newNote->parent = this;
	sortShown();
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
	auto &sl = parent->shown;
	auto sit = find_if(sl.begin(), sl.end(), [&](auto &p){return p == this;});
	if (sit != sl.end())
		sl.erase(sit);
	model->endRemoveRows();
}

void NoteInTree::nameChanged(const QString &name)
{
	this->name = name;
	model->dataChanged(mdlNdx, mdlNdx, QVector<int>{Qt::DisplayRole});
	model->layoutAboutToBeChanged();
	parent->sortShown();
	model->layoutChanged();
}

void NoteInTree::gotAttach()
{
	hasAttach = true;
}

void NoteInTree::showAndResetMarksAll()
{
	if (!parent && keywords_.empty())
		return;
	model->beginResetModel();
	keywords_.clear();
	isLoadStarted = false;
	cachedTxt.clear();
	shown.clear();
	isMarked = false;
	for (auto &c : children){
		shown.push_back(c.get());
		c->showAndResetMarksAll();
	}
	sortShown();
	model->endResetModel();
}

void NoteInTree::markAll(const QString &str, SearchType t)
{
	if (str.isEmpty()){
		showAndResetMarksAll();
		return;
	}
	model->beginResetModel();
	hideAndResetMarksAll();
	model->endResetModel();
	keywords_.clear();
	ASSERT( t == WholePhrase || t == AllTheWords);
	if (t == WholePhrase){
		keywords_.emplace_back(str);
	}
	if (t == AllTheWords){
		auto l = str.split(" ", QString::SkipEmptyParts);
		for (auto &s:l)
			keywords_.emplace_back(move(s));
	}
	markAll();
}

void NoteInTree::markAll()
{
	if (!isLoadStarted){
		isLoadStarted = true;
		auto n = note.lock();
		if (n){
			connect(n.get(), &Note::notePlainTextRdy, this, [=](const QString &txt){
				disconnect(n.get(), &Note::notePlainTextRdy, this, 0);
				cachedTxt = txt;
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
	if (cachedTxt.isEmpty())
		return;
	qApp->processEvents();
	auto &kwds = keywords();
	size_t found = 0;
	for (auto &k :kwds){
		if (cachedTxt.contains(k, Qt::CaseInsensitive))
			found++;
	}
	if (kwds.size() == found){
		model->layoutAboutToBeChanged();
		isMarked = true;
		showFromHereToParent();
		model->layoutChanged();
	}
}

void NoteInTree::hideAndResetMarksAll()
{
	isMarked = false;
	shown.clear();
	for (auto &c : children){
		c->hideAndResetMarksAll();
	}
}

void NoteInTree::sortShown()
{
	std::sort(shown.begin(), shown.end(), [](const auto &a, const auto &b){
		return a->name.compare(b->name, Qt::CaseInsensitive) < 0;
	});
}

void NoteInTree::showFromHereToParent()
{
	NoteInTree *n = this;
	while (n->parent){
		auto &sl = n->parent->shown;
		auto it = find_if(sl.begin(), sl.end(), [&](auto &p){return p == n;});
		if (it == sl.end()){
			ASSERT(find_if(n->parent->children.begin(), n->parent->children.end(), [&](auto &p){return p.get() == n;}) != n->parent->children.end());
			sl.push_back(n);
			n->parent->sortShown();
		}
		n = n->parent;
	}
}

std::vector<QString> &NoteInTree::keywords()
{
	NoteInTree *n = this;
	while (n->parent){
		n = n->parent;
	}
	return n->keywords_;
}




