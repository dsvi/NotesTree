#include "App.h"

App *app;

App::App(QObject *parent) : QObject(parent)
{
	connect(this, &App::showErrorDilog, this, &App::showErrorDilogSlot);
	connect(this, &App::fatal, [=](std::exception_ptr e) {
		showErrorDilogSlot(e);
		QCoreApplication::exit(1);
	});
}

void BuildErrorMsg(const std::exception& e, QString &msg){
	msg += QString::fromUtf8(e.what());
	msg += '\n';
	try {
		std::rethrow_if_nested(e);
	} catch(const std::exception& e) {
		BuildErrorMsg(e, msg);
	}
	catch(...) {}
}

void App::showErrorDilogSlot(std::exception_ptr eptr)
{
	QString msg;
	msg.reserve(2000);
	try {
		if (eptr)
			std::rethrow_exception(eptr);
	} catch(const std::exception& e) {
		BuildErrorMsg(e, msg);
	}
	QMessageBox msgBox;
	msgBox.setText(tr("Error:","dialog about happened error"));
	msgBox.setInformativeText(msg);
	msgBox.exec();
}

