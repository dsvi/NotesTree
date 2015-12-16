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

	ioThread_.setObjectName("io thread");
	rootNote_.moveToThread(&ioThread_);
	ioThread_.start();
	connect(qApp, &QCoreApplication::aboutToQuit, [&](){
		ioThread_.quit();
		ioThread_.wait();
	});
	//connect(this, &NotesTreeModel::loadFrom, rootNote_, &Note::createHierarchyFromRoot);
	ui->notesTree->root(&rootNote_);
	connect(ui->notesTree, &NotesTree::noteActivated, ui->noteEditor, &NoteEditor::showTextFor);

	QMetaObject::invokeMethod(&rootNote_, "createHierarchyFromRoot", Qt::QueuedConnection, Q_ARG(QString, "/home/ds/OTest"));
}

MainWindow::~MainWindow()
{
	delete ui;
}

