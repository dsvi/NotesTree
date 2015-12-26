#include "NoteEditor.h"

using namespace std;

NoteEditor::NoteEditor(QWidget *parent) :
  QWidget(parent)
{
	ui.setupUi(this);
	connect(&autosaveTimer_, &QTimer::timeout, this, &NoteEditor::save);
	connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &NoteEditor::save);
	auto settings = ui.noteEdit->settings();
	auto systemFont = QGuiApplication::font();
	settings->setFontFamily(QWebSettings::FontFamily::StandardFont, systemFont.family());
	auto screen = QGuiApplication::primaryScreen();
	// pt suppose to be 1/72 of inch, but QT seem to assume 1/96
	//int fontSize = screen->physicalDotsPerInchX() * systemFont.pointSize() / 72;
	int fontSize = screen->physicalDotsPerInchX() * systemFont.pointSize() / 96;
	settings->setFontSize(QWebSettings::FontSize::DefaultFontSize, fontSize);
	settings->setFontSize(QWebSettings::FontSize::DefaultFixedFontSize, fontSize);
	auto page = ui.noteEdit->page();
	page->setContentEditable(true);
	page->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	connect(page, &QWebPage::linkClicked, [](const QUrl &url){
		QDesktopServices::openUrl(url);
	});

	auto boldAction = page->action(QWebPage::ToggleBold);
	boldAction->setIcon(QIcon::fromTheme("format-text-bold"));
	app->addToolButton(this, ui.toolBoxLayout, boldAction);

	auto italicAction = page->action(QWebPage::ToggleItalic);
	italicAction->setIcon(QIcon::fromTheme("format-text-italic"));
	app->addToolButton(this, ui.toolBoxLayout, italicAction);

	ui.toolBoxLayout->addStretch();

	stopNoteTracking();
}

void NoteEditor::editTextFor(std::weak_ptr<Note> wn)
{
	stopNoteTracking();
	auto note = wn.lock();
	if (!note)
		return;
	Note *n = note.get();

	connectionsToNote_.push_back(connect(this, &NoteEditor::startEditingTxt, n, &Note::startEditing));
	connectionsToNote_.push_back(connect(this, &NoteEditor::stopEditingTxt,  n, &Note::stopEditing));
	connectionsToNote_.push_back(connect(n, &Note::noteTextRdy, this, &NoteEditor::noteText));

	connectionsToNote_.push_back(connect(this, &NoteEditor::saveTxt, n, &Note::save));

	ui.noteEdit->load(QUrl("qrc:/loading-note.html"));
	emit startEditingTxt();
}

void NoteEditor::noteText(const QString &txt, const QString &basePath)
{
	this->setEnabled(true);
	QString html = txt;
	if (txt.isEmpty()){
		QFile file(":/default-note.html");
		file.open(QFile::ReadOnly);
		html = QString(file.readAll());
	}
	ui.noteEdit->setHtml(html, QUrl::fromLocalFile(basePath + "/"));
	connectionsToNote_.push_back(connect(ui.noteEdit->page(), &QWebPage::contentsChanged, this, &NoteEditor::changed));
}

void NoteEditor::stopNoteTracking()
{
	save();
	emit stopEditingTxt();
	for (auto &c : connectionsToNote_)
		disconnect(c);
	ui.noteEdit->setHtml(QString());
	this->setEnabled(false);
}

void NoteEditor::changed()
{
	haveToSave_ = true;
	autosaveTimer_.start(5000);
}

void NoteEditor::save()
{
	if (!haveToSave_)
		return;
	haveToSave_ = false;
	autosaveTimer_.stop();
	auto frame = ui.noteEdit->page()->mainFrame();
	QString txt = frame->toPlainText().trimmed();
	if (txt.isEmpty())
		emit saveTxt(QString());
	else
		emit saveTxt(frame->toHtml());
	//qDebug()<< "is still modified? " << ui.noteEdit->isModified(); // yes, still is
}
