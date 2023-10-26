#include "App.h"

using namespace std;

App *app;

App::App(QObject *parent) : QObject(parent)
{
	connect(this, &App::error, this, &App::errorSlot);
	connect(this, &App::reportError, this, &App::showErrorDilogSlot);
	connect(this, &App::reportErrorMsg, this, &App::errorMsgSlot);
	
	ioThread_.setObjectName("io thread");
	connect(qApp, &QCoreApplication::aboutToQuit, [&](){
		ioThread_.quit();
		ioThread_.wait();
	});
	ioThread_.start();

	downloader_.moveToThread(&ioThread_);
}

void App::addToolButton(QWidget *parent, QBoxLayout *l, QAction *a)
{
	auto b = new QToolButton(parent);
	b->setAutoRaise(true);
	b->setDefaultAction(a);
	if (a->menu()){
		b->setPopupMode(QToolButton::InstantPopup);
	}
	l->addWidget(b);
}

void App::addToolBoxSpacer(QBoxLayout *l)
{
	l->addSpacing(2);
}

void BuildErrorMsg(const std::exception& e, QString &msg){
	msg += QString::fromUtf8(e.what());
	msg += '\n';
	try {
		std::rethrow_if_nested(e);
	} catch(const std::exception& e) {
		BuildErrorMsg(e, msg);
	}
	catch(...) {
		msg += QApplication::translate("Sadly, I don't even know what the error is.","message about happened error");
	}
}

QString App::errorMessage(std::exception_ptr eptr, const QString &action)
{
	QString msg;
	msg.reserve(2000);
	msg += action;
	msg += '\n';
	try {
		std::rethrow_exception(eptr);
	} catch(const std::exception& e) {
		BuildErrorMsg(e, msg);
	}
	return msg;
}

void App::showErrorDilogSlot(std::exception_ptr eptr, const QString &action)
{
	errorMsgSlot(errorMessage(eptr, action));
}

void App::errorSlot(std::exception_ptr e)
{
	showErrorDilogSlot(e);
	if (!isRecoverable(e))
		QCoreApplication::exit(1);
}

void App::errorMsgSlot(const QString &e)
{
	QMessageBox msgBox;
	msgBox.setText(tr("Error:","dialog about happened error"));
	msgBox.setInformativeText(e);
	msgBox.exec();
}


QString GetResourceString(const char *res)
{
	QFile file(res);
	file.open(QFile::ReadOnly);
	return QString(file.readAll());
}
