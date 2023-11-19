#ifndef NOTEEDITOR_H
#define NOTEEDITOR_H

#include "NoteEditorSearchHighlighter.h"
#include "ui_NoteEditor.h"
#include "Note.h"

class NoteEditor : public QWidget
{
	Q_OBJECT

public:
	explicit NoteEditor(QWidget *parent = 0);
signals:
	void startEditingTxt();
	void saveTxt(const QString n);
	void stopEditingTxt();
	void handleRefs(QString hmtl);
public slots:
	void editTextFor(std::weak_ptr<Note> n);
	void noteText(const QString &txt, const QString &basePath);
	void stopNoteTracking();
	void save();

	void changed();

	void updateUrl(QString from, QString to);
private:
	Ui::NoteEditor ui;
	std::vector<QMetaObject::Connection> connectionsToNote_;
	NoteEditorSearchHighlighter *searchHighlighter_ = nullptr;
	QTimer autosaveTimer_;
	bool   haveToSave_ = false;

	void highlightFoundText();
	void unHighlightFoundText();
};

/// visits `img` and `source` urls.
/// returns html possibly edited by \p f 
QString VisitSrcUrls(QString html, std::function<void(QDomElement)> &&f);

#endif // NOTEEDITOR_H
