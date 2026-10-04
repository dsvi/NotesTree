#include "MainWindow.h"
#include "App.h"
#include "KdeLookStyle.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setApplicationName("NotesTree");
	a.setApplicationDisplayName("Notes Tree");

	// A fully static build cannot dlopen() the Breeze style plugin, so add back
	// the two things it draws and Fusion does not: the tree branch lines and
	// the accent frame around the focused widget. No-op when a KDE style is
	// already active, i.e. in the dynamic build.
	KdeLookStyle::installIfApplicable();
	 
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


