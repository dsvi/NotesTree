#include "NoteEditorSearchHighlighter.h"

NoteEditorSearchHighlighter::NoteEditorSearchHighlighter(QObject *parent)
  : QSyntaxHighlighter{parent}
{
	
}

void NoteEditorSearchHighlighter::highlight(QString words)
{
	textToHighlight_ = words;
	rehighlight();
}

void NoteEditorSearchHighlighter::highlightBlock(const QString &text)
{
	if (textToHighlight_.isEmpty())
		return;
	QTextCharFormat format;
	format.setFontWeight(QFont::Bold);
	format.setBackground(Qt::yellow);
	
	auto len = textToHighlight_.length();
	for (
		qsizetype i = text.indexOf(textToHighlight_, 0, Qt::CaseInsensitive);
		i != -1;
		i = text.indexOf(textToHighlight_, i + len, Qt::CaseInsensitive) )
	{
		setFormat(i, len, format);
	}
}
