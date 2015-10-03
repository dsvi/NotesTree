#include "NotesTreeModel.h"
#include "MainWindow.h"
#include "App.h"

int main(int argc, char *argv[])
{
  //setenv("QT_DEVICE_PIXEL_RATIO","auto",0);
	QApplication a(argc, argv);
	app = new App(&a);
	MainWindow w;
	w.show();

	return a.exec();
}


