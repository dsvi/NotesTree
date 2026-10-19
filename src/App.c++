#include "App.h"

#include "KdeLookStyle.h"
#include "ThemedSvgIconEngine.h"

using namespace std;

/// The application configures itself, so main() only has to create App.
/// The setters are reached through App's base classes: setApplicationName is
/// QCoreApplication's, setApplicationDisplayName QGuiApplication's.
App::Identity::Identity()
{
	setApplicationName("NotesTree");
	setApplicationDisplayName("Notes Tree");
}

App::App(int &argc, char **argv)
	: QApplication(argc, argv),
	  mainWnd_(nullptr)
{
	connect(this, &App::error, this, &App::errorSlot);
	connect(this, &App::reportError, this, &App::showErrorDilogSlot);
	connect(this, &App::reportErrorMsg, this, &App::errorMsgSlot);

	ioThread_.setObjectName("io thread");
	// The io thread's event loop ends together with the app's: aboutToQuit is
	// emitted before exec() returns, so the loop stops while everything that
	// lives on it (rootNote_ and its NoteInTree proxies) is still alive.
	// quit() is thread safe and idempotent, so it can be wired directly.
	connect(this, &QCoreApplication::aboutToQuit, &ioThread_, &QThread::quit);
	ioThread_.start();
}

/// Joins the io thread. This has to happen here, in the body, and not by
/// letting a parent QObject delete the thread: ~QThread on a running thread is
/// qFatal("QThread: Destroyed while thread ... is still running"), which aborts
/// the process, so the join cannot be delegated to ownership. Running it in the
/// body also puts it before the member destructors, i.e. while downloader_ and
/// config_ are still valid - Note's slots on this thread call app()->downloader().
App::~App()
{
	// On a normal exit aboutToQuit already quit the loop, so quit() is a no-op
	// here; it is what covers the paths that never reached exec(), where
	// aboutToQuit is never emitted.
	ioThread_.quit();
	ioThread_.wait();
}

static
int getIconSize(){
	auto fm = QFontMetricsF(QApplication::font());
	auto size = fm.height();
	return size * 1.2;
}

/// The themed icon for the SVG at resource path \p icon.
///
/// The colours are deliberately not worked out here: ThemedSvgIconEngine does
/// that every time the glyph is drawn, so the icons follow the desktop's colour
/// scheme while the app is running, instead of keeping the colours of whichever
/// one happened to be active when the icon was made.
QIcon App::themedSVGIcon(QString icon, float scale)
{
	QFile file(icon);
	if (!file.open(QIODevice::ReadOnly))
		return QIcon();
	return QIcon(new ThemedSvgIconEngine(file.readAll(), std::move(icon), scale));
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
