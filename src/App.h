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
public slots:
	void showErrorDilogSlot(std::exception_ptr e);
};

extern
App *app;

#endif // APP_H
