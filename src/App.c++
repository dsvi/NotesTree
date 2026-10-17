#include "App.h"

#include "KdeLookStyle.h"

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

/// Recolours a flat icon. The SVG glyphs are a single solid colour, so keeping
/// their alpha and replacing their RGB is an exact recolour, antialiasing and
/// all.
static QPixmap tintedPixmap(const QPixmap &src, const QColor &color)
{
	QPixmap out(src.size());
	out.setDevicePixelRatio(src.devicePixelRatioF());
	out.fill(Qt::transparent);
	QPainter p(&out);
	p.drawPixmap(0, 0, src);
	p.setCompositionMode(QPainter::CompositionMode_SourceIn);
	p.fillRect(out.rect(), color);
	p.end();
	return out;
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
	pixPainter.end();

	QIcon result(pix);
	// Qt's tool buttons ask for QIcon::Active while the cursor is over an
	// auto-raise button (QCommonStyle CE_ToolButtonLabel; Breeze uses the same
	// rule), so give the icon an accent-coloured variant for that state.
	// Disabled buttons ask for QIcon::Disabled. Without a variant Qt synthesises
	// one in QCommonStyle::generatedIconPixmap, which ramps the glyph towards
	// Disabled|Window and leaves it near-white (#d7d9dc on this palette). Breeze
	// instead recolours the SVG with the palette's disabled text colour, so do
	// the same; the accent never leaks into a disabled button because
	// State_Enabled is tested before State_MouseOver.
	const QColor accent = KdeLookStyle::hoverColor(QApplication::palette());
	if (accent.isValid() && accent != c)
		result.addPixmap(tintedPixmap(pix, accent), QIcon::Active, QIcon::Off);
	const QColor disabled = QApplication::palette().color(QPalette::Disabled, QPalette::Text);
	if (disabled.isValid() && disabled != c)
		result.addPixmap(tintedPixmap(pix, disabled), QIcon::Disabled, QIcon::Off);
	return result;
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
