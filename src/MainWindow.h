#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "NotesTreeModel.h"

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
	Note            rootNote_;
};


#endif // MAINWINDOW_H
