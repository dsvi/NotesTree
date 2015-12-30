#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "App.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	qRegisterMetaType<std::exception_ptr>();
	qRegisterMetaType<std::weak_ptr<Note>>();
	qRegisterMetaType<std::vector<std::weak_ptr<Note>>>();
	qRegisterMetaType<std::shared_ptr<NoteInTree>>();

	rootNote_.moveToThread(app->ioThread());
	//connect(this, &NotesTreeModel::loadFrom, rootNote_, &Note::createHierarchyFromRoot);
	ui->notesTree->root(&rootNote_);
	connect(ui->notesTree, &NotesTree::noteActivated, ui->noteEditor, &NoteEditor::editTextFor);

	connect(app->cfg(), &Config::rootChanged, &rootNote_, &Note::createHierarchyFromRoot);
	app->cfg()->emitRootChanged();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
	ui->noteEditor->save();
	ev->accept();
}

