﻿#include "App.h"

using namespace std;

App *app;

App::App(QObject *parent) : QObject(parent)
{
	connect(this, &App::error, this, &App::errorSlot);
	connect(this, &App::reportError, this, &App::showErrorDilogSlot);

	auto screen = QGuiApplication::primaryScreen();
	hmm_ = screen->physicalDotsPerInchX() / 25.4;
	vmm_ = screen->physicalDotsPerInchY() / 25.4;
}

void App::addToolButton(QWidget *parent, QBoxLayout *l, QAction *a)
{
	auto b = new QToolButton(parent);
	b->setAutoRaise(true);
	b->setIconSize(QSize(6*hmm_, 6*vmm_));
	b->setDefaultAction(a);
	l->addWidget(b);
}

void App::addToolBoxSpacer(QBoxLayout *l)
{
	l->addSpacing(2 * hmm_);
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

void App::showErrorDilogSlot(std::exception_ptr eptr)
{
	QString msg;
	msg.reserve(2000);
	try {
		std::rethrow_exception(eptr);
	} catch(const std::exception& e) {
		BuildErrorMsg(e, msg);
	}
	QMessageBox msgBox;
	msgBox.setText(tr("Error:","dialog about happened error"));
	msgBox.setInformativeText(msg);
	msgBox.exec();
}

void App::errorSlot(std::exception_ptr e)
{
	showErrorDilogSlot(e);
	if (!isRecoverable(e))
		QCoreApplication::exit(1);
}


