#ifndef APP_H
#define APP_H

#include <QApplication>

#include "Config.h"
#include "Downloader.h"


/// The application object itself, so it is created exactly once, in main().
///
/// The base is QApplication rather than QGuiApplication: QGuiApplication is
/// QCoreApplication + the windowing layer only, and it never runs
/// QApplicationPrivate::init(). NotesTree is a widgets app (QMainWindow,
/// QMessageBox, QToolButton, QApplication::palette()/font()), and QtWidgets
/// qFatal()s as soon as a widget is built while qApp is not a QApplication.
/// QApplication *is* a QGuiApplication, so everything QGuiApplication offers
/// is still there.
class App : public QApplication
{
	Q_OBJECT
public:
	App(int &argc, char **argv);
	~App() override;
	/// can easily be null
	QMainWindow *mainWindow();
	/// set only once
	void mainWindow(QMainWindow *mainWnd);

	Config *cfg();
	QThread *ioThread();
	Downloader *downloader();

	QIcon themedSVGIcon(QString icon, float scale);	
	QAction *addToolButton(
		QWidget *parent,
		QLayout *l,
		QIcon    icon);	
	QAction *addToolButton(
		QWidget *parent,
		QLayout *l,
		QString icon);
	void addToolBoxSpacer(QBoxLayout *l);

	QString errorMessage(std::exception_ptr e, const QString &action = QString());

signals:
	/// informs user about an error, and quits app if \p e and none of it's nested are derived from RecoverableException
	void error(std::exception_ptr e);
	/// informs user about an error
	/// \param action description of what app was doing when the error occured
	void reportError(std::exception_ptr e, const QString &action = QString());
	void reportErrorMsg(const QString &e);
public slots:
	void showErrorDilogSlot(std::exception_ptr e, const QString &action = QString());
	void errorSlot(std::exception_ptr e);
	void errorMsgSlot(const QString &e);

private:
	/// Declared first on purpose: members are initialized in declaration
	/// order, so this runs right after the QApplication base and before
	/// config_, which resolves its data path through QStandardPaths and
	/// therefore needs the application identity to be known already.
	struct Identity { Identity(); };
	Identity	 identity_;
	QThread      ioThread_;
	Downloader   downloader_;
	QMainWindow *mainWnd_;
	Config       config_;
};

inline
void App::mainWindow(QMainWindow *mainWnd)
{
	mainWnd_ = mainWnd;
}
inline
QThread *App::ioThread()
{
	return &ioThread_;
}
inline
Downloader *App::downloader()
{
	return &downloader_;
}
inline
QMainWindow *App::mainWindow()
{
	return mainWnd_;
}
inline
Config *App::cfg()
{
	return &config_;
}

/// The application object, downcast from Qt's own singleton.
/// qApp is a macro that each Qt module redefines: qapplication.h has
///	#define qApp (static_cast<QApplication *>(QCoreApplication::instance()))
/// so qApp already finds the one application object, but only types it as far
/// as QApplication, which cannot see App's own API. This adds that last step.
/// Like qApp it is null before App is constructed and after it is destroyed,
/// so there is no separate pointer to keep in sync or leave dangling.
inline
App *app()
{
	return static_cast<App *>(qApp);
}

#endif // APP_H
