#include "NotesTree.h"
#include "AddNewNoteDialog.h"

using namespace std;

NotesTree::NotesTree(QWidget *parent) :
	QWidget(parent)
{
	ui.setupUi(this);
}

void NotesTree::root(Note *root)
{
	ASSERT(notesTreeModel_.noteAt(QModelIndex()) == nullptr); // should be called only once
	notesTreeModel_.root(root);
	auto treeView = ui.notesView;
	notesProxy_.setSourceModel(&notesTreeModel_);
	treeView->setModel(&notesProxy_);
	notesProxy_.sort(0, Qt::AscendingOrder);
	connect(ui.notesView->selectionModel(), &QItemSelectionModel::currentChanged, [this](const QModelIndex & index){
		if (index.isValid())
			emit noteActivated(noteAt(index)->note);
		else
			emit noteActivated(std::weak_ptr<Note>());
	});
	
	// signal to signal: the model knows the rename is coming, the editor decides
	// whether it is the note it is currently showing.
	connect(&notesTreeModel_, &NotesTreeModel::aboutToRename,
	        this, &NotesTree::noteAboutToRename);

	QAction *addNew = app()->addToolButton(this, ui.toolBoxLayout, ":/ico/add");
	addNew->setShortcuts(QKeySequence::New);
	addNew->setToolTip(tr("Create a new note. As a subnote to the selected note, or at root of the notes tree"));
	connect(addNew, &QAction::triggered, this, &NotesTree::addNew);
	
	QAction	*removeSelected = app()->addToolButton(this, ui.toolBoxLayout, ":/ico/remove");
	removeSelected->setShortcuts(QKeySequence::Delete);
	removeSelected->setToolTip(tr("Delete selected note and its subnotes"));
	removeSelected->setEnabled(false);

	connect(removeSelected, &QAction::triggered, this, &NotesTree::removeSelected);
	
	float scale = devicePixelRatioF();
	QIcon attachOpenIcon = app()->themedSVGIcon(":/ico/attachment", scale);
	QIcon attachAddIcon = app()->themedSVGIcon(":/ico/attachment-add", scale);
	QAction	*attach = app()->addToolButton(this, ui.toolBoxLayout, attachAddIcon);
	attach->setShortcut(QKeySequence(+Qt::CTRL + +Qt::Key_A));
	attach->setEnabled(false);
	auto setAttachOpen = [=](){
		attach->setIcon(attachOpenIcon);
		attach->setToolTip(tr("Open attachment folder"));
	};
	auto setAttachAdd = [=](){
		attach->setIcon(attachAddIcon);
		attach->setToolTip(tr("Add attachment"));
	};
	QAction	*copySelected = app()->addToolButton(this, ui.toolBoxLayout, ":/ico/copy"); 
	copySelected->setShortcuts(QKeySequence::Copy);
	copySelected->setToolTip(tr(
		"Copy note's files to clipboard,\n"
		"so you can later paste them anywhere in the filesystem."));
	copySelected->setEnabled(false);
	connect(copySelected, &QAction::triggered, this, &NotesTree::copySelectedPathsToClipboard);

	auto sm = treeView->selectionModel();
	connect(sm, &QItemSelectionModel::selectionChanged, [=,this](){
		auto selected = treeView->selectionModel()->selection();
		if (selected.isEmpty()){
			removeSelected->setEnabled(false);
			copySelected->setEnabled(false);
		}
		else{
			removeSelected->setEnabled(true);
			copySelected->setEnabled(true);
		}
		attach->setEnabled(false);
		setAttachOpen();
		for (auto &c : attachConnections_)
			disconnect(c);
		attachConnections_.clear();
		if (selected.size() == 1){
			attach->setEnabled(true);
			auto ndx = selected.indexes()[0];
			auto note = noteAt(ndx);
			auto n = note->note.lock();
			if (!n)
				return;
			if (note->hasAttach)
				setAttachOpen();
			else
				setAttachAdd();
			attachConnections_.push_back(connect(n.get(), &Note::attachReady, this, [=](const QString &attachDirPath){
				QDesktopServices::openUrl(QUrl("file://" + attachDirPath));
				setAttachOpen();
			}));
			attachConnections_.push_back(connect(attach, &QAction::triggered, n.get(), &Note::attach));
		}
	});
	ui.toolBoxLayout->addStretch();
	{
		ui.searchType->insertItem(NoteInTree::SearchType::WholePhrase, tr("whole phrase"));
		ui.searchType->insertItem(NoteInTree::SearchType::AllTheWords, tr("all of the words"));
		constexpr const char * settingsSearchType = "NotesTree.searchType";
		auto settings = app()->cfg()->unimportant_settings();
		auto t = settings.value(settingsSearchType); 
		if (t.isValid())
			ui.searchType->setCurrentIndex(t.toInt());
		connect(qApp, &QCoreApplication::aboutToQuit, [=, this](){
			auto settings = app()->cfg()->unimportant_settings();
			settings.setValue(settingsSearchType, ui.searchType->currentIndex());
		});
	}
	{
		QAction *search = app()->addToolButton(this, ui.toolBoxLayout, ":/ico/search");
		search->setToolTip(tr("Filter notes"));
		search->setShortcut(QKeySequence(+Qt::SHIFT + +Qt::Key_F));
		//search->setShortcut(QKeySequence::Find);
		search->setCheckable(true);
		auto doSearch = [=,this](){
			searchFor(ui.searchFor->text(), NoteInTree::SearchType(ui.searchType->currentIndex()));
		};
		auto target = ui.searchPanel->sizeHint().height();
		ui.searchPanel->hide();
		connect(search, &QAction::triggered, [=,this](bool checked){
			QPropertyAnimation *animation = new QPropertyAnimation(ui.searchPanel, "maximumHeight");
			animation->setDuration(250);
			if (checked){
				ui.searchPanel->setMaximumHeight(0);
				ui.searchPanel->show();
				ui.searchFor->setFocus();
				animation->setStartValue(0);
				animation->setEndValue(target);
				doSearch();
//				connect(animation, &QAbstractAnimation::finished, [=]{
//				});
			}
			else{
				endSearch();
				animation->setEndValue(0);
				connect(animation, &QAbstractAnimation::finished, [=,this]{
					ui.searchPanel->hide();
				});
			}
			animation->start(QAbstractAnimation::DeleteWhenStopped);
		});

		connect(ui.searchFor, &QLineEdit::textChanged, doSearch);
		connect(ui.searchType, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), doSearch);
	}
}

void NotesTree::addNew()
{
	auto sm = ui.notesView->selectionModel();
	AddNewNoteDialog dialog(app()->mainWindow());
	auto selected = sm->selectedIndexes();
	if (selected.size() != 1)
		dialog.hideWhereToAddSelection();
	dialog.adjustSize();
	if ( dialog.exec() == QDialog::Rejected )
		return;
	auto r = dialog.result();
	QModelIndex mdlNdx;
	if (selected.size() == 1 && !r->addToRoot)
		mdlNdx = toSource(selected[0]);
	notesTreeModel_.addNote(mdlNdx, r->name);
}

void NotesTree::removeSelected()
{
	auto sm = ui.notesView->selectionModel();
	auto selected = sm->selectedIndexes();
	if (selected.isEmpty())
		return;
	QMessageBox dialog(app()->mainWindow());
	if (selected.size() == 1)
		dialog.setText(tr("Delete the note and all it's subnotes?"));
	else
		dialog.setText(tr(
			"Delete the %1 selected notes and all their subnotes?", "number of selected notes")
			.arg(selected.size()));
	dialog.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	dialog.setDefaultButton(QMessageBox::No);
	if ( dialog.exec() != QMessageBox::Yes )
		return;
	notesTreeModel_.removeNotes(toSource(selected));
}

void NotesTree::copySelectedPathsToClipboard()
{
	auto sm = ui.notesView->selectionModel();
	auto selected = sm->selectedIndexes();
	if (selected.isEmpty())
		return;
	vector<weak_ptr<Note>> noteWPs;
	for(auto &ndx : selected){
		auto wp = noteAt(ndx)->note;
		noteWPs.emplace_back(wp);
	}
	auto collector = new PathsCollector(this);
	connect(collector, &PathsCollector::collected, this, [collector](const vector<QString>& lst){
		collector->deleteLater();
		if (lst.empty())
			return;
		QString list;
		for (auto &u : lst){
			list.append(u);
			list.append("\n");
		}
		auto data = make_unique<QMimeData>();
		data->setData("text/uri-list", list.toUtf8());
		list.prepend("copy\n");
		list.resize(list.size()-1); // remove last \n
		data->setData("x-special/gnome-copied-files", list.toUtf8());
		QApplication::clipboard()->setMimeData(data.release());
	});
	collector->startCollectingFor(noteWPs);
}

NoteInTree *NotesTree::noteAt(const QModelIndex &viewNdx) const
{
	return notesTreeModel_.noteAt(toSource(viewNdx));
}

QModelIndex NotesTree::toSource(const QModelIndex &viewNdx) const
{
	return notesProxy_.mapToSource(viewNdx);
}

QModelIndexList NotesTree::toSource(const QModelIndexList &viewNdxs) const
{
	QModelIndexList ret;
	for (auto &i : viewNdxs)
		ret << toSource(i);
	return ret;
}

void NotesTree::searchFor(const QString &str, NoteInTree::SearchType t )
{
	notesTreeModel_.searchFor(str, t);
}

void NotesTree::endSearch()
{
	notesTreeModel_.endSearch();
}

void PathsCollector::startCollectingFor(const std::vector<std::weak_ptr<Note> > &nlst)
{
	total_ = 0;
	for (auto p : nlst){
		auto sp = p.lock();
		if (!sp)
			continue;
		connect(sp.get(), &Note::pathsReady, this, &PathsCollector::collect, Qt::UniqueConnection);
		connect(this, &PathsCollector::givePaths, sp.get(), &Note::getNoteRelatedPaths, Qt::UniqueConnection);
		total_ ++;
	}
	givePaths();
	if (total_ == 0 )
		emit collected(vector<QString>());
}

void PathsCollector::collect(const std::vector<QString> &lst)
{
	if (total_ <= 0)
		return;
	uris_.insert(uris_.end(), lst.begin(), lst.end());
	if (--total_ == 0)
		emit collected(uris_);
}
