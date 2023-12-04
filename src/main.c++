#include "MainWindow.h"
#include "App.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setApplicationName("NotesTree");
	a.setApplicationDisplayName("Notes Tree");
	 
	app = new App(&a);
	try{
		MainWindow w;
		w.show();
		app->mainWindow(&w);

		return a.exec();
	}
	catch(std::exception &e){
		emit app->showErrorDilogSlot(std::current_exception());
	}
}


