#ifndef NOTEEDITOR_H
#define NOTEEDITOR_H

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
public slots:
	void editTextFor(std::weak_ptr<Note> n);
	void noteText(const QString &txt, const QString &basePath);
	void stopNoteTracking();
	void save();

	void changed();
private:
	Ui::NoteEditor ui;
	std::vector<QMetaObject::Connection> connectionsToNote_;
	QTimer autosaveTimer_;
	bool   haveToSave_ = false;

};

#endif // NOTEEDITOR_H
