#ifndef APP_H
#define APP_H

#include "Config.h"
#include "Downloader.h"

// for example: res = ":/default-note.html"
QString GetResourceString(const char* res);

/// this isn't meant to be deleted
class App : public QObject
{
	Q_OBJECT
public:
	explicit App(QObject *parent = 0);
	/// can easily be null
	QMainWindow *mainWindow();
	/// set only once
	void mainWindow(QMainWindow *mainWnd);

	Config *cfg();
	QThread *ioThread();
	Downloader *downloader();

	void addToolButton(QWidget *parent, QBoxLayout *l, QAction *a);
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
	QThread      ioThread_;
	Downloader   downloader_;
	QMainWindow *mainWnd_;
	qreal  hmm_ = 0; /// pixel per mm horizontal
	qreal  vmm_ = 0;
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

extern
App *app;

#endif // APP_H
