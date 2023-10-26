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
		auto settings = app->cfg()->unimportant_settings();
		const auto geometry = settings.value("MainWindow.geometry", QByteArray()).toByteArray();
		if (geometry.isEmpty())
			setGeometry(100, 100, 800, 800);
		else
			restoreGeometry(geometry);
		auto state  = settings.value("MainWindow.splitterState", QByteArray());
		if (state.isValid())
			ui->splitter->restoreState(state.toByteArray());
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
		auto settings = app->cfg()->unimportant_settings();
		settings.setValue("MainWindow.geometry", saveGeometry());
		settings.setValue("MainWindow.splitterState", ui->splitter->saveState());
		settings.sync();
	}
	catch(...){
		qCritical() << app->errorMessage(std::current_exception(), tr("Can't save config file."));
	}
	ev->accept();
}

