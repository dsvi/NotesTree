#include "NoteEditor.h"

using namespace std;

NoteEditor::NoteEditor(QWidget *parent) :
  QWidget(parent)
{
	ui.setupUi(this);
	connect(&autosaveTimer_, &QTimer::timeout, this, &NoteEditor::save);
	auto settings = ui.noteEdit->settings();
	auto systemFont = QGuiApplication::font();
	settings->setFontFamily(QWebSettings::FontFamily::StandardFont, systemFont.family());
	auto screen = QGuiApplication::primaryScreen();
	// pt is supposed to be 1/72 of inch, but QT seem to assume 1/96
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
//	connect(ui.noteEdit, &QWebView::loadFinished, [this](bool ok){
//		if (!ok)
//			return;
//		auto ret = ui.noteEdit->page()->mainFrame()->evaluateJavaScript("document.execCommand('enableObjectResizing',false,true);");
//		qDebug() << ret;
//	});
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
		act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
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
	ui.noteEdit->installEventFilter(this);
	{
		header_ = new QAction(this);
		header_->setIcon(QIcon(":/ico/header"));
		header_->setToolTip(tr("Make header from current parafraph"));
		QMenu *menu = new QMenu(this);
		headers_.push_back(menu->addAction(tr("Header lvl 1")));
		headers_.push_back(menu->addAction(tr("Header lvl 2")));
		headers_.push_back(menu->addAction(tr("Header lvl 3")));
		headers_.push_back(menu->addAction(tr("Header lvl 4")));
		headers_.push_back(menu->addAction(tr("Header lvl 5")));
		headers_.push_back(menu->addAction(tr("Header lvl 6")));
		for (auto a : headers_){
			a->setCheckable(true);
			a->setToolTip(tr("Set header level"));
		}
		header_->setMenu(menu);
		header_->setCheckable(true);
		for (size_t i = 0; i < headers_.size(); i++){
			auto a = headers_[i];
			connect(a, &QAction::triggered, [=](bool checked){
				if (currentEl_.isNull())
					return;
				auto page = ui.noteEdit->page();
				if (page == nullptr)
					return;
				QString tag;
				if (checked)
					tag = "H" + QString::number(i+1);
				else
					tag = "DIV";
				header_->setChecked(checked);
				QString js = QString("document.execCommand('formatBlock',false,'%1');").arg(tag);
				ui.noteEdit->page()->mainFrame()->evaluateJavaScript(js);
			});
		}
		app->addToolButton(this, ui.toolBoxLayout, header_);
	}
	{
		auto act = new QAction(this);
		act->setIcon(QIcon(":/ico/remove style"));
		act->setToolTip(tr("Remove styles"));
		connect(act, &QAction::triggered, [=]{
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
		act->setEnabled(false);
		connect(ui.noteEdit, &QWebView::selectionChanged, [=](){
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
		auto target = ui.searchPanel->height();
		ui.searchPanel->hide();
		connect(search, &QAction::triggered, [=](bool checked){
			QPropertyAnimation *animation = new QPropertyAnimation(ui.searchPanel, "maximumHeight");
			animation->setDuration(250);
			if (checked){
				ui.searchPanel->setMaximumHeight(0);
				ui.searchPanel->show();
				ui.searchFor->setFocus();
				highlightFoundText();
				animation->setStartValue(0);
				animation->setEndValue(target);
			}
			else{
				unHighlightFoundText();
				animation->setEndValue(0);
				connect(animation, &QAbstractAnimation::finished, [=]{
					ui.searchPanel->hide();
				});
			}
			animation->start();
		});
		connect(ui.searchFor, &QLineEdit::textChanged, [=](){
			highlightFoundText();
		});
		app->addToolButton(this, ui.toolBoxLayout, search);
	}
	stopNoteTracking();
}

bool NoteEditor::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() != QEvent::MouseButtonPress)
		return false;
	QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
	QWebView *view = static_cast<QWebView*>(watched);
	QPoint pos = view->mapFromGlobal(mouseEvent->globalPos());
	QWebFrame *frame = view->page()->frameAt(mouseEvent->pos());
	if (!frame)
		return false;
	QWebHitTestResult hitTestResult = frame->hitTestContent(pos);
	currentEl_ = hitTestResult.element();
	currentSelectionChanged();
	return false;
}

void NoteEditor::currentSelectionChanged()
{
	uncheckHeaders();
	QString tag = currentEl_.tagName();
	if ( tag.size() == 2 && tag.startsWith("H") ){
		tag.remove(0,1);
		bool ok;
		auto hn = tag.toUInt(&ok);
		if (!ok)
			return;
		hn--;
		for (size_t i = 0; i < headers_.size(); i++){
			if ( hn == i ){
				header_->setChecked(true);
				headers_[i]->setChecked(true);
			}
		}
	}
}

void NoteEditor::uncheckHeaders()
{
	header_->setChecked(false);
	for (auto a : headers_)
		a->setChecked(false);
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
	if (!ui.searchPanel->isHidden())
		highlightFoundText();
	connectionsToNote_.push_back(connect(ui.noteEdit->page(), &QWebPage::contentsChanged, this, &NoteEditor::changed));
}

void NoteEditor::stopNoteTracking()
{
	save();
	emit stopEditingTxt();
	for (auto &c : connectionsToNote_)
		disconnect(c);
	ui.noteEdit->setHtml(QString());
	uncheckHeaders();
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


