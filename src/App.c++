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

static
int getIconSize(){
	auto fm = QApplication::fontMetrics();
	auto size = fm.height();
	return size * 1.2;
}

QIcon App::themedSVGIcon(QString icon, float scale)
{
	// open svg resource load contents to qbytearray
	QFile file(icon);
	file.open(QIODevice::ReadOnly);
	QByteArray baData = file.readAll();
	// load svg contents to xml document and edit contents
	QDomDocument doc;
	doc.setContent(baData);
	auto c = QApplication::palette().text().color();
	auto color = QString("#%1%2%3").arg(c.red(), 0, 16).arg(c.green(), 0, 16).arg(c.blue(), 0, 16);
	for (auto el : {"path","polygon"}){
		auto list = doc.elementsByTagName(el);
		for (int i = 0; i < list.count(); i++)	
			list.at(i).toElement().setAttribute("fill", color);
	}
	// create svg renderer with edited contents
	QSvgRenderer svgRenderer(doc.toByteArray());
	auto size = getIconSize();
	// create pixmap target (could be a QImage)
	QPixmap pix(size*scale, size*scale);
	pix.fill(Qt::transparent);
	// create painter to act over pixmap
	QPainter pixPainter(&pix);
	// use renderer to render over painter which paints on pixmap
	svgRenderer.render(&pixPainter);
	return QIcon(pix);
}

QAction *App::addToolButton(QWidget *parent, QLayout *l, QIcon icon)
{
	auto act = new QAction(parent);
	auto b = new QToolButton(parent);
	auto size = getIconSize();
	b->setIconSize(QSize(size,size));
	act->setIcon(icon);
	b->setAutoRaise(true);
	b->setPopupMode(QToolButton::InstantPopup);
	b->setDefaultAction(act);
	l->addWidget(b);
	return act;
}

QAction *App::addToolButton(QWidget *parent, QLayout *l, QString icon)
{
	return addToolButton(parent, l, themedSVGIcon(icon, parent->devicePixelRatioF()));
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
