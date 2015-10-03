#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "App.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
	ui->setupUi(this);
  ui->addNote->setIcon(QIcon(":/ico/add"));
  ui->removeNote->setIcon(QIcon(":/ico/remove"));
	try{
		notesTreeModel.rootPath("/home/ds/OTest");
		ui->notesView->setModel( &notesTreeModel );
		ui->notesView->sortByColumn(0, Qt::AscendingOrder);
	}
	catch(std::exception &e){
		emit app->showErrorDilog(std::current_exception());
	}
}

MainWindow::~MainWindow()
{
	delete ui;
}
