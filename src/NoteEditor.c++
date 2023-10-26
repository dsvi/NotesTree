#include "NoteEditor.h"

using namespace std;

NoteEditor::NoteEditor(QWidget *parent) :
  QWidget(parent)
{
	ui.setupUi(this);
	connect(&autosaveTimer_, &QTimer::timeout, this, &NoteEditor::save);
	auto settings = ui.noteEdit->settings();
	auto systemFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
	auto systemFixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	settings->setFontFamily(QWebSettings::FontFamily::StandardFont, systemFont.family());
	settings->setFontFamily(QWebSettings::FontFamily::FixedFont, systemFixedFont.family());
//	settings->setFontSize(QWebSettings::FontSize::DefaultFontSize,  systemFont.pointSize());
//	settings->setFontSize(QWebSettings::FontSize::DefaultFixedFontSize, systemFixedFont.pointSize());
	auto page = ui.noteEdit->page();
	page->setContentEditable(true);
	page->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	connect(page, &QWebPage::linkClicked, [](const QUrl &url){
		QDesktopServices::openUrl(url);
	});
	connect(ui.noteEdit, &QWebView::loadFinished, [this](bool ok){
		if (!ok)
			return;
		auto palette = QApplication::palette();
		auto textColor = palette.text().color().name();
		auto linkColor = palette.link().color().name();
		auto linkVisitedColor = palette.linkVisited().color().name();
		QString css = GetResourceString(":/css").arg(textColor).arg(linkColor).arg(linkVisitedColor);
		ui.noteEdit->page()->mainFrame()->evaluateJavaScript(css);
		ui.noteEdit->page()->mainFrame()->evaluateJavaScript("document.documentElement.contentEditable = true");
		if (!ui.searchPanel->isHidden())
			highlightFoundText();
	});
	{
		auto act = page->action(QWebPage::ToggleBold);
		act->setIcon(QIcon(":/ico/bold"));
		act->setShortcut(QKeySequence::Bold);
		app->addToolButton(this, ui.toolBoxLayout, act);
	}
	{
		auto act = page->action(QWebPage::ToggleItalic);
		act->setIcon(QIcon(":/ico/italic"));
		act->setShortcut(QKeySequence::Italic);
		app->addToolButton(this, ui.toolBoxLayout, act);
	}
	{
		auto act = page->action(QWebPage::ToggleUnderline);
		act->setIcon(QIcon(":/ico/underline"));
		act->setShortcut(QKeySequence::Underline);
		app->addToolButton(this, ui.toolBoxLayout, act);
	}
	{
		auto act = page->action(QWebPage::ToggleStrikethrough);
		act->setIcon(QIcon(":/ico/strikethrough"));
		act->setShortcut(QKeySequence(+Qt::CTRL + +Qt::Key_S));
		app->addToolButton(this, ui.toolBoxLayout, act);
	}
	app->addToolBoxSpacer(ui.toolBoxLayout);
	{
		auto act = page->action(QWebPage::ToggleSuperscript);
		act->setIcon(QIcon(":/ico/superscript"));
		app->addToolButton(this, ui.toolBoxLayout, act);
	}
	{
		auto act = page->action(QWebPage::ToggleSubscript);
		act->setIcon(QIcon(":/ico/subscript"));
		app->addToolButton(this, ui.toolBoxLayout, act);
	}
	{
		auto act = new QAction(this);
		act->setIcon(QIcon(":/ico/preformatted"));
		act->setToolTip(tr("Monospaced preformatted text"));
		connect(act, &QAction::triggered, [=,this]{
			QString txt = ui.noteEdit->selectedText();
			txt.replace("\n","\\n");
			txt.replace("\r","\\r");
			txt.replace("\'","\\'");
			txt.prepend("<pre>");
			txt.append("</pre>");
			auto frame = ui.noteEdit->page()->mainFrame();
			frame->evaluateJavaScript("document.execCommand('delete',false,'');");
			QString js = QString("document.execCommand('insertHTML',false,'%1');").arg(txt);
			frame->evaluateJavaScript(js);
		});
		act->setEnabled(false);
		connect(ui.noteEdit, &QWebView::selectionChanged, [=,this](){
			if (ui.noteEdit->selectedText().isEmpty())
				act->setEnabled(false);
			else
				act->setEnabled(true);
		});
		app->addToolButton(this, ui.toolBoxLayout, act);
	}
	{
		auto act = new QAction(this);
		act->setIcon(QIcon(":/ico/link"));
		act->setToolTip(tr("Make link"));
		connect(act, &QAction::triggered, [=,this]{
			QString txt = ui.noteEdit->selectedText();
			QString suggest = txt.contains(" ") ? QString() : txt;
			bool ok;
			QString target = QInputDialog::getText(this, tr("Link target"),
			                                     tr("Link to:"), QLineEdit::Normal,
			                                     suggest, &ok);
			if (!ok || target.isEmpty())
				return;
			txt.prepend(QString("<a href=\"%1\">").arg(target));
			txt.append("</a>");
			txt.replace("\n","\\n");
			txt.replace("\r","\\r");
			txt.replace("\'","\\'");
			auto frame = ui.noteEdit->page()->mainFrame();
			frame->evaluateJavaScript("document.execCommand('delete',false,'');");
			QString js = QString("document.execCommand('insertHTML',false,'%1');").arg(txt);
			frame->evaluateJavaScript(js);
		});
		act->setEnabled(false);
		connect(ui.noteEdit, &QWebView::selectionChanged, [=,this](){
			if (ui.noteEdit->selectedText().isEmpty())
				act->setEnabled(false);
			else
				act->setEnabled(true);
		});
		app->addToolButton(this, ui.toolBoxLayout, act);
	}
	{
		auto act = new QAction(this);
		act->setIcon(QIcon(":/ico/remove style"));
		act->setToolTip(tr("Remove colors or formatting"));
		QMenu *menu = new QMenu(this);
		auto remStyles = menu->addAction(tr("remove styles (colors)"));
		auto remFormat = menu->addAction(tr("to plain text"));
		connect(remStyles, &QAction::triggered, [=,this]{
			QString html = ui.noteEdit->selectedHtml();
			QString newHtml;
			QRegExp reg("<[^>]+(style\\s*=\\s*\"[^\"]*\")");
			int from = 0;
			int to;
			int pos = 0;
			while (true){
				pos = reg.indexIn(html, pos);
				if (pos < 0)
					break;
				//qDebug()<< reg.cap(0);
				//qDebug()<< reg.cap(1);
				pos += reg.cap(0).size();
				auto cap = reg.cap(1);
				to = reg.pos(1);
				newHtml += html.midRef(from, to - from);
				from = to + cap.size();
			}
			newHtml += html.midRef(from, html.size());
			newHtml.replace("\n","\\n");
			newHtml.replace("\r","\\r");
			newHtml.replace("\'","\\'");
			auto frame = ui.noteEdit->page()->mainFrame();
			frame->evaluateJavaScript("document.execCommand('delete',false,'');");
			QString js = QString("document.execCommand('insertHTML',false,'%1');").arg(newHtml);
			frame->evaluateJavaScript(js);
		});
		connect(remFormat, &QAction::triggered, [=,this]{
			QString txt = ui.noteEdit->selectedText();
			txt.replace("\n","<br>");
			txt.replace("\r","<br>");
			txt.replace("\'","\\'");
			auto frame = ui.noteEdit->page()->mainFrame();
			frame->evaluateJavaScript("document.execCommand('delete',false,'');");
			QString js = QString("document.execCommand('insertHTML',false,'%1');").arg(txt);
			frame->evaluateJavaScript(js);
		});
		act->setMenu(menu);
		act->setEnabled(false);
		connect(ui.noteEdit, &QWebView::selectionChanged, [=,this](){
			if (ui.noteEdit->selectedText().isEmpty())
				act->setEnabled(false);
			else
				act->setEnabled(true);
		});
		app->addToolButton(this, ui.toolBoxLayout, act);
	}
	ui.toolBoxLayout->addStretch();
	{
		QAction *search = new QAction(this);
		search->setIcon(QIcon(":/ico/search"));
		search->setToolTip(tr("Find text in the note"));
		//search->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F));
		search->setShortcut(QKeySequence::Find);
		search->setCheckable(true);
		auto target = ui.searchPanel->sizeHint().height();
		ui.searchPanel->hide();
		connect(search, &QAction::triggered, [=,this](bool checked){
			QPropertyAnimation *animation = new QPropertyAnimation(ui.searchPanel, "maximumHeight");
			animation->setDuration(250);
			if (checked){
				ui.searchPanel->setMaximumHeight(0);
				ui.searchPanel->show();
				ui.searchFor->setFocus();
				animation->setStartValue(0);
				animation->setEndValue(target);
				// animating blured web edit is slow, so blur only after the animation
				connect(animation, &QAbstractAnimation::finished, [=,this]{
					highlightFoundText();
				});
			}
			else{
				unHighlightFoundText();
				animation->setEndValue(0);
				connect(animation, &QAbstractAnimation::finished, [=,this]{
					ui.searchPanel->hide();
				});
			}
			animation->start(QAbstractAnimation::DeleteWhenStopped);
		});
		connect(ui.searchFor, &QLineEdit::textChanged, [=,this](){
			highlightFoundText();
		});
		app->addToolButton(this, ui.toolBoxLayout, search);
	}
	stopNoteTracking();
}

void NoteEditor::highlightFoundText()
{
	QString txt = ui.searchFor->text();
	if (txt.isEmpty()){
		unHighlightFoundText();
		return;
	}
	ui.noteEdit->findText("", QWebPage::HighlightAllOccurrences);
	bool found = ui.noteEdit->findText(txt, QWebPage::HighlightAllOccurrences);
	if (!found)
		ui.noteEdit->setGraphicsEffect(new QGraphicsBlurEffect());
	else
		ui.noteEdit->setGraphicsEffect(nullptr);
}

void NoteEditor::unHighlightFoundText()
{
	ui.noteEdit->findText("", QWebPage::HighlightAllOccurrences);
	ui.noteEdit->setGraphicsEffect(nullptr);
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
	if (txt.isEmpty())
		html = GetResourceString(":/default-note.html");
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
	if (!autosaveTimer_.isActive())
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
	QString html = frame->toHtml();
	if (txt.isEmpty() && !html.contains("src", Qt::CaseInsensitive))
		emit saveTxt(QString());
	else
		emit saveTxt(html);
	//qDebug()<< "is still modified? " << ui.noteEdit->isModified(); // yes, still is
}


