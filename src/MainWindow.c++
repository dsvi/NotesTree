#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "App.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	notesTreeModel.rootPath("/home/ds/OTest");
	ui->notesView->setModel( &notesTreeModel );
	ui->notesView->sortByColumn(0, Qt::AscendingOrder);
	notesTreeActions_ = std::make_unique<NotesTreeActions>(ui->notesView);
	ui->addNote->setDefaultAction(notesTreeActions_->getAddNew());
	ui->removeNote->setDefaultAction(notesTreeActions_->getRemoveSelected());
}

MainWindow::~MainWindow()
{
	delete ui;
}
