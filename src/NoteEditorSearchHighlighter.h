#pragma once

#include <QSyntaxHighlighter>

class NoteEditorSearchHighlighter : public QSyntaxHighlighter
{
public:
	explicit NoteEditorSearchHighlighter(QObject *parent = nullptr);
	
	void highlight(QString txt);
	
	// QSyntaxHighlighter interface
protected:
	void highlightBlock(const QString &text) override;
private:
	QString textToHighlight_;
};

