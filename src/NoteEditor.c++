#include "NoteEditor.h"

using namespace std;

void recursiveVisitAndRemove(QDomNode n, QString tag, const std::function<void(QDomNode)> &action){
	// QT Dom api addresses nodes by indexes. So first visit them, then remove
	// otherwise indexes messed up
	for (auto c = n.firstChild(); !c.isNull(); c = c.nextSibling())
		recursiveVisitAndRemove(c, tag, action);
	while(true){
		auto c = n.firstChildElement(tag);
		if (c.isNull())
			break;
		action(c);
		n.removeChild(c);
	}
}

void recursiveVisit(QDomNode n, const std::function<void(QDomElement)> &action){
	for (auto c = n.firstChildElement(); !c.isNull(); c = c.nextSiblingElement()){
		recursiveVisit(c.toElement(), action);
		action(c);
	}
}

QString removeStyles(QString in){
	QDomDocument doc;
	doc.setContent(in);
	recursiveVisitAndRemove(doc, "p", [&](QDomNode n){
		if (!n.hasChildNodes())
			return;
		auto p = n.parentNode();
		//p.insertAfter(doc.createElement("br"), n);
		for (auto ch = n.lastChild(); !ch.isNull(); ch = n.lastChild())	
			p.insertAfter(ch, n);
		p.insertAfter(doc.createElement("br"), n);
	});
	recursiveVisitAndRemove(doc, "span", [](QDomNode n){
		auto p = n.parentNode();
		for (auto ch = n.lastChild(); !ch.isNull(); ch = n.lastChild())	
			p.insertAfter(ch, n);
	});

	QString style = "style";
	recursiveVisitAndRemove(doc, style, [](QDomNode n){});
	recursiveVisit(doc, [&](QDomElement e){
		if (e.hasAttribute(style))
			e.removeAttribute(style);
	});
	// That 0 is really important. Otherwise QTextEdit bugs
	// so is removing the comments
	return doc.toString(0).remove("<!--StartFragment-->").remove("<!--EndFragment-->");
}

QString wrapToTag(QString in, QString tag){
	in.replace("<!--StartFragment-->", "<" + tag +">");
	in.replace("<!--EndFragment-->", "</" + tag +">");
	return in;
}

NoteEditor::NoteEditor(QWidget *parent) :
  QWidget(parent)
{
	ui.setupUi(this);
	
	connect(&autosaveTimer_, &QTimer::timeout, this, &NoteEditor::save);
		
	auto edit = ui.noteEdit;
	
	auto enableOnlyWhenTextSelected = [=](QAction *act){
		act->setEnabled(false);
		connect(edit, &QTextEdit::selectionChanged, [=](){
			if (edit->textCursor().hasSelection())
				act->setEnabled(true);
			else
				act->setEnabled(false);
		});
	};
	{
		// this adds context menu "copy link"
		edit->setTextInteractionFlags(ui.noteEdit->textInteractionFlags() | Qt::LinksAccessibleByMouse);
		
		auto actOpenLink = new QAction(edit);
		actOpenLink->setText(tr("Open link"));
		connect(actOpenLink, &QAction::triggered, [=]{
			auto ref = edit->textCursor().charFormat().anchorHref();
			QDesktopServices::openUrl(ref);
		});
		
		auto actOpenAsLink = new QAction(edit);
		actOpenAsLink->setText(tr("Open as link"));
		connect(actOpenAsLink, &QAction::triggered, [=]{
			auto ref = edit->textCursor().selection().toPlainText();
			QDesktopServices::openUrl(ref);
		});
		
		connect(edit, &QTextEdit::customContextMenuRequested, this, [=](const QPoint &p){
			unique_ptr<QMenu> menu(edit->createStandardContextMenu(p));
			auto ref = edit->textCursor().charFormat().anchorHref();
			
			if (!ref.isEmpty())
				menu->insertAction(menu->actions().at(0), actOpenLink);
			else
			if (edit->textCursor().hasSelection())
				menu->insertAction(menu->actions().at(0), actOpenAsLink);
			menu->exec(edit->mapToGlobal(p));
		});
		edit->setContextMenuPolicy(Qt::CustomContextMenu);
	}
	{
		auto act = app->addToolButton(this, ui.toolBoxLayout, ":/ico/bold");
		act->setShortcut(QKeySequence::Bold);
		enableOnlyWhenTextSelected(act);
		act->setCheckable(true);
		connect(edit, &QTextEdit::selectionChanged, [=](){
			if (edit->textCursor().charFormat().fontWeight() == QFont::Bold)
				act->setChecked(true);
			else
				act->setChecked(false);
		});
		connect(act, &QAction::triggered, [=]{
			auto c = edit->textCursor();
			auto f = c.charFormat();
			if (f.fontWeight() == QFont::Bold)
				f.setFontWeight(QFont::Normal);
			else
				f.setFontWeight(QFont::Bold);
			c.setCharFormat(f);
		});
	}
	{
		auto act = app->addToolButton(this, ui.toolBoxLayout, ":/ico/italic");
		act->setShortcut(QKeySequence::Italic);
		enableOnlyWhenTextSelected(act);
		act->setCheckable(true);
		connect(edit, &QTextEdit::selectionChanged, [=](){
			if (edit->textCursor().charFormat().fontItalic())
				act->setChecked(true);
			else
				act->setChecked(false);
		});
		connect(act, &QAction::triggered, [=]{
			auto c = edit->textCursor();
			auto f = c.charFormat();
			f.setFontItalic(!f.fontItalic());
			c.setCharFormat(f);
		});
	}
	{
		auto act = app->addToolButton(this, ui.toolBoxLayout, ":/ico/superscript");
		act->setCheckable(true);
		enableOnlyWhenTextSelected(act);
		connect(edit, &QTextEdit::selectionChanged, [=](){
			if (edit->textCursor().charFormat().verticalAlignment() == QTextCharFormat::AlignSuperScript)
				act->setChecked(true);
			else
				act->setChecked(false);
		});
		connect(act, &QAction::triggered, [=]{
			auto c = edit->textCursor();
			auto f = c.charFormat();
			if (f.verticalAlignment() == QTextCharFormat::AlignSuperScript)
				f.setVerticalAlignment(QTextCharFormat::AlignNormal);
			else
				f.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
			c.setCharFormat(f);
		});
	}
	{
		auto act = app->addToolButton(this, ui.toolBoxLayout, ":/ico/subscript");
		act->setCheckable(true);
		enableOnlyWhenTextSelected(act);
		connect(edit, &QTextEdit::selectionChanged, [=](){
			if (edit->textCursor().charFormat().verticalAlignment() == QTextCharFormat::AlignSubScript)
				act->setChecked(true);
			else
				act->setChecked(false);
		});
		connect(act, &QAction::triggered, [=]{
			auto c = edit->textCursor();
			auto f = c.charFormat();
			if (f.verticalAlignment() == QTextCharFormat::AlignSubScript)
				f.setVerticalAlignment(QTextCharFormat::AlignNormal);
			else
				f.setVerticalAlignment(QTextCharFormat::AlignSubScript);
			c.setCharFormat(f);
		});
	}	
	{
		auto act = app->addToolButton(this, ui.toolBoxLayout, ":/ico/header");
		act->setToolTip(tr("Make header"));
		enableOnlyWhenTextSelected(act);
		connect(act, &QAction::triggered, [=]{
			auto cursor = edit->textCursor();
			auto selection = cursor.selection().toHtml();
			cursor.beginEditBlock();
			cursor.removeSelectedText();
			cursor.insertHtml(wrapToTag(selection, "h2"));
			cursor.endEditBlock();
		});
	}
	{
		auto act = app->addToolButton(this, ui.toolBoxLayout, ":/ico/preformatted");
		act->setToolTip(tr("Monospaced preformated text"));
		enableOnlyWhenTextSelected(act);
		connect(act, &QAction::triggered, [=]{
			auto cursor = edit->textCursor();
			auto selection = cursor.selection().toHtml();
			cursor.beginEditBlock();
			cursor.removeSelectedText();
			cursor.insertHtml(wrapToTag(selection, "pre"));
			cursor.endEditBlock();
		});
	}
	{
		auto act = app->addToolButton(this, ui.toolBoxLayout, ":/ico/link");
		act->setToolTip(tr("Make link"));
		enableOnlyWhenTextSelected(act);
		connect(act, &QAction::triggered, [=,this]{
			auto cursor = edit->textCursor();
			auto txt = cursor.selection().toPlainText();
			QString suggest = txt.contains(" ") ? QString() : txt;
			bool ok;
			QString target = QInputDialog::getText(this, tr("Link target"),
			                                     tr("Link to:"), QLineEdit::Normal,
			                                     suggest, &ok);
			if (!ok || target.isEmpty())
				return;
			txt.prepend(QString("<a href=\"%1\">").arg(target));
			txt.append("</a>");
			cursor.removeSelectedText();
			cursor.insertHtml(txt);
			cursor.endEditBlock();
		});
	}
	{
		auto act = app->addToolButton(this, ui.toolBoxLayout, ":/ico/remove style");
		act->setToolTip(tr("Remove colors or formatting"));
		QMenu *menu = new QMenu(this);
		auto remStyles = menu->addAction(tr("remove styles (colors and fonts)"));
		auto remFormat = menu->addAction(tr("to plain text"));
		connect(remStyles, &QAction::triggered, [=]{
			auto cursor = edit->textCursor();
			auto selection = cursor.selection().toHtml();
			cursor.beginEditBlock();
			cursor.removeSelectedText();
			cursor.insertHtml(removeStyles(selection));
			cursor.endEditBlock();
		});
		connect(remFormat, &QAction::triggered, [=]{
			auto cursor = edit->textCursor();
			auto selection = cursor.selection().toPlainText();
			cursor.beginEditBlock();
			cursor.removeSelectedText();
			cursor.insertHtml(selection.toHtmlEscaped().replace("\n","<br>"));
			cursor.endEditBlock();
		});
		act->setMenu(menu);
		act->setEnabled(false);
		connect(edit, &QTextEdit::selectionChanged, [=](){
			if (edit->textCursor().hasSelection())
				act->setEnabled(true);
			else
				act->setEnabled(false);
		});
	}
	ui.toolBoxLayout->addStretch();
	{
		QAction *search = app->addToolButton(this, ui.toolBoxLayout, ":/ico/search");
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
	if (!searchHighlighter_)
		searchHighlighter_ = new NoteEditorSearchHighlighter(this);
	searchHighlighter_->setDocument(ui.noteEdit->document());
	searchHighlighter_->highlight(txt);
	
	bool found = ui.noteEdit->toPlainText().contains(txt, Qt::CaseInsensitive);
	if (!found)
		ui.noteEdit->setGraphicsEffect(new QGraphicsBlurEffect());
	else
		ui.noteEdit->setGraphicsEffect(nullptr);
}

void NoteEditor::unHighlightFoundText()
{
	if (searchHighlighter_)
		searchHighlighter_->highlight(QString());

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
	connectionsToNote_.push_back(connect(this, &NoteEditor::handleRefs, n, &Note::handleRefs));
	connectionsToNote_.push_back(connect(n, &Note::updateUrl, this, &NoteEditor::updateUrl));
	
	//ui.noteEdit->textCursor().insertImage(":/ico/NotesTree");
	emit startEditingTxt();
}

void NoteEditor::noteText(const QString &txt, const QString &basePath)
{
	this->setEnabled(true);
	QString html = txt;
	auto doc = ui.noteEdit->document();
	doc->setBaseUrl("file:/" + basePath + "/");
	doc->setHtml(html);
	connectionsToNote_.push_back(connect(doc, &QTextDocument::contentsChanged, this, &NoteEditor::changed));
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
	emit handleRefs(ui.noteEdit->toHtml());
}

void NoteEditor::updateUrl(QString from, QString to)
{
	auto edit = ui.noteEdit;
	auto html = edit->toHtml();
	auto newHtml = VisitSrcUrls(html, [=](QDomElement e){
		auto url = e.attribute("src");
		if (url == from)
			e.setAttribute("src", to);
	});
	edit->setHtml(newHtml);
	changed();
}


void NoteEditor::save()
{
	if (!haveToSave_)
		return;
	haveToSave_ = false;
	autosaveTimer_.stop();
	QString txt = ui.noteEdit->toPlainText().trimmed();
	QString html = ui.noteEdit->toHtml();
	if (txt.isEmpty())
		emit saveTxt(QString());
	else
		emit saveTxt(html);
}


QString VisitSrcUrls(QString html, std::function<void(QDomElement)> &&f)
{
	QDomDocument doc;
	doc.setContent(html);
	vector<QDomElement> list;
	for (auto tag: {"img", "source"}){
		auto l = doc.elementsByTagName(tag);
		for (int i =0; i < l.count(); i++)
			list.push_back(l.at(i).toElement());		
	}
	for (auto &e : list)
		f(e);
	return doc.toString(0);
}
