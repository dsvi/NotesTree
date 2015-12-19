#include "NoteEditor.h"

NoteEditor::NoteEditor(QWidget *parent) :
  QWidget(parent)
{
	ui.setupUi(this);
	connect(&autosaveTimer_, &QTimer::timeout, this, &NoteEditor::save);
	connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &NoteEditor::save);
	auto settings = ui.noteEdit->settings();
	settings->setFontFamily(QWebSettings::FontFamily::StandardFont, QGuiApplication::font().family());
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

void NoteEditor::showTextFor(std::weak_ptr<Note> n)
{
	stopNoteTracking();

	auto note = n.lock();
	if (!note)
		return;

	connectionsToNote_.push_back(connect(this, &NoteEditor::getNoteTxt, note.get(), &Note::load));
	connectionsToNote_.push_back(connect(note.get(), &Note::noteTextRdy, this, &NoteEditor::noteText));

	connectionsToNote_.push_back(connect(this, &NoteEditor::saveNoteTxt, note.get(), &Note::save));

	ui.noteEdit->load(QUrl("qrc:/loading-note.html"));
	emit getNoteTxt(n);
}

void NoteEditor::noteText(const QString &txt)
{
	this->setEnabled(true);
	ui.noteEdit->setHtml(txt);
	connectionsToNote_.push_back(connect(ui.noteEdit->page(), &QWebPage::contentsChanged, [this](){
		haveToSave_ = true;
		autosaveTimer_.start(5000);
	}));
}

void NoteEditor::stopNoteTracking()
{
	save();
	for (auto &c : connectionsToNote_)
		disconnect(c);
	ui.noteEdit->setHtml(QString());
	this->setEnabled(false);
}

void NoteEditor::save()
{
	if (!haveToSave_)
		return;
	haveToSave_ = false;
	autosaveTimer_.stop();
	emit saveNoteTxt(ui.noteEdit->page()->mainFrame()->toHtml());
	//qDebug()<< "is still modified? " << ui.noteEdit->isModified(); // yes, still is
}

