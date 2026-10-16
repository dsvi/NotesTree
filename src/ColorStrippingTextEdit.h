#ifndef COLORSTRIPPINGTEXTEDIT_H
#define COLORSTRIPPINGTEXTEDIT_H

#include <QImage>
#include <QTextEdit>

#include "NoteTableStyle.h"

/// QTextEdit that throws away the colour information of every html it receives
/// from the clipboard or from a drag'n'drop (see stripHtmlColors()). Programs
/// bake their own theme's colours into the html they put on the clipboard, and
/// those colours are often unreadable on a dark background, so pasted content
/// is stripped and gets painted with the palette's colours instead.
///
/// Only the incoming html is touched: colours a user applies inside the note
/// and colours already stored in the note stay as they are.
///
/// Tables are the exception: whatever look the source gave them is replaced by
/// the app's own, painted with this widget's palette (see styleTables()).
class ColorStrippingTextEdit : public QTextEdit
{
	Q_OBJECT
public:
	using QTextEdit::QTextEdit;

	/// Re-styles the tables of the document with the palette of this widget and
	/// remembers the colours it used. Called when a note is loaded and after
	/// something has been pasted.
	void restyleTables();

	/// The colours restyleTables() painted with, so that the note can be saved
	/// without them (see stripTableTheme()).
	const ThemedTableStyle &tableTheme() const { return tableTheme_; }

signals:
	/// emitted instead of inserting anything, when the clipboard carries a
	/// bare image (a screenshot: no html, no text). The document can't keep
	/// image data by itself, so the note is supposed to store the image and
	/// insert a reference back.
	void imagePasted(const QImage &img);

protected:
	/// used for both pasting and dropping
	void insertFromMimeData(const QMimeData *source) override;

private:
	ThemedTableStyle tableTheme_;
};

#endif // COLORSTRIPPINGTEXTEDIT_H
