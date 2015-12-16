#ifndef APP_H
#define APP_H

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

	void addToolButton(QWidget *parent, QBoxLayout *l, QAction *a);
	void addToolBoxSpacer(QBoxLayout *l);

signals:
	/// informs user about an error, and quits app if \p e and none of it's nested are derived from RecoverableException
	void error(std::exception_ptr e);
	void reportError(std::exception_ptr e);
public slots:
	void showErrorDilogSlot(std::exception_ptr e);
	void errorSlot(std::exception_ptr e);
	

private:
	QMainWindow *mainWnd_;
	qreal  hmm_ = 0; /// pixel per mm horizontal
	qreal  vmm_ = 0;
};

inline
void App::mainWindow(QMainWindow *mainWnd)
{
	mainWnd_ = mainWnd;
}

inline
QMainWindow *App::mainWindow()
{
	return mainWnd_;
}

extern
App *app;

#endif // APP_H
