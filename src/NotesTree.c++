#include "NotesTree.h"
#include "AddNewNoteDialog.h"

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
	treeView->setModel(&notesTreeModel_);
	connect(ui.notesView->selectionModel(), &QItemSelectionModel::currentChanged, [this](const QModelIndex & index){
		if (index.isValid())
			emit noteActivated(notesTreeModel_.noteAt(index)->note);
		else
			emit noteActivated(std::weak_ptr<Note>());
	});

	QAction *addNew = new QAction(QIcon(":/ico/add"), QString(), this);
	//QAction *addNew = new QAction(QIcon::fromTheme("list-add"), QString(), this);
	addNew->setShortcuts(QKeySequence::New);
	addNew->setToolTip(tr("Create a new note. As a subnote to the selected note, or at root of the notes tree"));
	connect(addNew, &QAction::triggered, this, &NotesTree::addNew);
	app->addToolButton(this, ui.toolBoxLayout, addNew);

	QAction	*removeSelected = new QAction(QIcon(":/ico/remove"), QString(), this);
	//QAction	*removeSelected = new QAction(QIcon::fromTheme("list-remove"), QString(), this);
	removeSelected->setShortcuts(QKeySequence::Delete);
	removeSelected->setToolTip(tr("Delete selected note and its subnotes"));
	removeSelected->setEnabled(false);
	app->addToolButton(this, ui.toolBoxLayout, removeSelected);
	connect(removeSelected, &QAction::triggered, this, &NotesTree::removeSelected);

	QIcon attachOpenIcon = QIcon(":/ico/attachment");
	QIcon attachAddIcon = QIcon(":/ico/attachment-add");
	QAction	*attach = new QAction(attachOpenIcon, QString(), this);
	attach->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
	attach->setEnabled(false);
	app->addToolButton(this, ui.toolBoxLayout, attach);
	auto setAttachOpen = [=](){
		attach->setIcon(attachOpenIcon);
		attach->setToolTip(tr("Open attachment folder"));
	};
	auto setAttachAdd = [=](){
		attach->setIcon(attachAddIcon);
		attach->setToolTip(tr("Add attachment"));
	};
	auto sm = treeView->selectionModel();
	connect(sm, &QItemSelectionModel::selectionChanged, [=](){
		auto selected = treeView->selectionModel()->selection();
		if (selected.isEmpty())
			removeSelected->setEnabled(false);
		else
			removeSelected->setEnabled(true);
		attach->setEnabled(false);
		setAttachOpen();
		for (auto &c : attachConnections_)
			disconnect(c);
		attachConnections_.clear();
		if (selected.size() == 1){
			attach->setEnabled(true);
			auto ndx = selected.indexes()[0];
			auto note = notesTreeModel_.noteAt(ndx);
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
		Config::ptree pt = app->cfg()->laodUnimportantConfig();
		auto t = pt.get_optional<uint>("NotesTree.searchType");
		if (t)
			ui.searchType->setCurrentIndex(*t);
		connect(qApp, &QCoreApplication::aboutToQuit, [=](){
			Config::ptree pt = app->cfg()->laodUnimportantConfig();
			pt.put("NotesTree.searchType", ui.searchType->currentIndex());
			app->cfg()->saveUnimportantConfig(pt);
		});
	}
	{
		QAction *search = new QAction(this);
		search->setIcon(QIcon(":/ico/search"));
		search->setToolTip(tr("Filter notes"));
		search->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F));
		//search->setShortcut(QKeySequence::Find);
		search->setCheckable(true);
		auto doSearch = [=](){
			searchFor(ui.searchFor->text(), NoteInTree::SearchType(ui.searchType->currentIndex()));
		};
		auto target = ui.searchPanel->sizeHint().height();
		ui.searchPanel->hide();
		connect(search, &QAction::triggered, [=](bool checked){
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
				connect(animation, &QAbstractAnimation::finished, [=]{
					ui.searchPanel->hide();
				});
			}
			animation->start(QAbstractAnimation::DeleteWhenStopped);
		});

		connect(ui.searchFor, &QLineEdit::textChanged, doSearch);
		connect(ui.searchType, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), doSearch);
		app->addToolButton(this, ui.toolBoxLayout, search);
	}
}

void NotesTree::addNew()
{
	auto sm = ui.notesView->selectionModel();
	AddNewNoteDialog dialog(app->mainWindow());
	auto selected = sm->selectedIndexes();
	if (selected.size() != 1)
		dialog.hideWhereToAddSelection();
	dialog.adjustSize();
	if ( dialog.exec() == QDialog::Rejected )
		return;
	auto r = dialog.result();
	QModelIndex mdlNdx;
	if (selected.size() == 1 && !r->addToRoot)
		mdlNdx = selected[0];
	notesTreeModel_.addNote(mdlNdx, r->name);
}

void NotesTree::removeSelected()
{
	auto sm = ui.notesView->selectionModel();
	auto selected = sm->selectedIndexes();
	if (selected.isEmpty())
		return;
	QMessageBox dialog(app->mainWindow());
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
	notesTreeModel_.removeNotes(selected);
}

void NotesTree::searchFor(const QString &str, NoteInTree::SearchType t )
{
	notesTreeModel_.searchFor(str, t);
}

void NotesTree::endSearch()
{
	notesTreeModel_.endSearch();
}
