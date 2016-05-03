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
	qRegisterMetaType<std::vector<QString>>();

	rootNote_.moveToThread(app->ioThread());
	//connect(this, &NotesTreeModel::loadFrom, rootNote_, &Note::createHierarchyFromRoot);
	ui->notesTree->root(&rootNote_);
	connect(ui->notesTree, &NotesTree::noteActivated, ui->noteEditor, &NoteEditor::editTextFor);

	connect(app->cfg(), &Config::rootChanged, &rootNote_, &Note::createHierarchyFromRoot);
	app->cfg()->emitRootChanged();

	try {
		auto pt = app->cfg()->laodUnimportantConfig();
		auto w  = pt.get_optional<int>("MainWindow.width");
		auto h  = pt.get_optional<int>("MainWindow.height");
		if (w && h)
			resize(*w, *h);
		auto state  = pt.get_optional<QByteArray>("MainWindow.splitterState");
		if (state)
			ui->splitter->restoreState(*state);
	}
	catch(...){
		qCritical() << app->errorMessage(std::current_exception(), tr("Can't load config file."));
	}
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
	ui->noteEditor->save();
	try{
		Config::ptree pt = app->cfg()->laodUnimportantConfig();
		auto sz = size();
		pt.put("MainWindow.width", sz.width());
		pt.put("MainWindow.height", sz.height());
		pt.put("MainWindow.splitterState", ui->splitter->saveState());
		app->cfg()->saveUnimportantConfig(pt);
	}
	catch(...){
		qCritical() << app->errorMessage(std::current_exception(), tr("Can't save config file."));
	}
	ev->accept();
}

