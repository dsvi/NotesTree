#include "NotesTreeModel.h"
#include "MainWindow.h"
#include "App.h"

int main(int argc, char *argv[])
{
  //setenv("QT_DEVICE_PIXEL_RATIO","auto",0);
	QApplication a(argc, argv);
	//std::make_unique<App>
	app = new App(&a);
	try{
		MainWindow w;
		w.show();
		app->mainWindow(&w);

		return a.exec();
	}
	catch(std::exception &e){
		emit app->showErrorDilog(std::current_exception());
	}
}


