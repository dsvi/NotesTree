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
	void getNoteTxt(std::weak_ptr<Note> n);
	void saveNoteTxt(const QString  n);
public slots:
	void showTextFor(std::weak_ptr<Note> n);
	void noteText(const QString &txt);
	void stopNoteTracking();
	void save();

private:
	Ui::NoteEditor ui;
	std::vector<QMetaObject::Connection> connectionsToNote_;
	QTimer autosaveTimer_;
	bool   haveToSave_ = false;
};

#endif // NOTEEDITOR_H
