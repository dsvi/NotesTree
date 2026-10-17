#include "MainWindow.h"
#include "App.h"
#include "KdeLookStyle.h"

int main(int argc, char *argv[])
{
	// The one and only application object: Qt keeps a pointer to it in its own
	// singleton, reachable as qApp / app(). App also configures itself (name,
	// display name), so main() only has to create it.
	// to create it.
	App a(argc, argv);

	// A fully static build cannot dlopen() the Breeze style plugin, so add back
	// the two things it draws and Fusion does not: the tree branch lines and
	// the accent frame around the focused widget. No-op when a KDE style is
	// already active, i.e. in the dynamic build.
	KdeLookStyle::installIfApplicable();
	try{
		MainWindow w;
		w.show();
		app()->mainWindow(&w);

		return a.exec();
	}
	catch(std::exception &e){
		emit app()->showErrorDilogSlot(std::current_exception());
	}
	// exec() was never reached, so aboutToQuit never ran; ~App stops the io
	// thread on its way down. The failure is worth an unsuccessful exit code.
	return 1;
}


