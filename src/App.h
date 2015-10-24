#ifndef APP_H
#define APP_H

class App : public QObject
{
	Q_OBJECT
public:
	explicit App(QObject *parent = 0);

signals:
	// thread safe way
	void showErrorDilog(std::exception_ptr e);
	// same as above with exit
	void fatal(std::exception_ptr e);
	// informs user about an error, and quits app if \p e and none of it's nested are derived from RecoverableException
	void error(std::exception_ptr e);
public slots:
	void showErrorDilogSlot(std::exception_ptr e);
};

extern
App *app;

#endif // APP_H
