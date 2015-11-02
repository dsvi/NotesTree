#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "NotesTreeModel.h"
#include "NotesTreeActions.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private:
	Ui::MainWindow *ui;
	NotesTreeModel  notesTreeModel;
	std::unique_ptr<NotesTreeActions> notesTreeActions_;
};

#endif // MAINWINDOW_H
