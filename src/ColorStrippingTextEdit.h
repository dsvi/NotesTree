#ifndef COLORSTRIPPINGTEXTEDIT_H
#define COLORSTRIPPINGTEXTEDIT_H

#include <QImage>
#include <QTextEdit>

/// QTextEdit that throws away the colour information of every html it receives
/// from the clipboard or from a drag'n'drop (see stripHtmlColors()). Programs
/// bake their own theme's colours into the html they put on the clipboard, and
/// those colours are often unreadable on a dark background, so pasted content
/// is stripped and gets painted with the palette's colours instead.
///
/// Only the incoming html is touched: colours a user applies inside the note
/// and colours already stored in the note stay as they are.
class ColorStrippingTextEdit : public QTextEdit
{
	Q_OBJECT
public:
	using QTextEdit::QTextEdit;

signals:
	/// emitted instead of inserting anything, when the clipboard carries a
	/// bare image (a screenshot: no html, no text). The document can't keep
	/// image data by itself, so the note is supposed to store the image and
	/// insert a reference back.
	void imagePasted(const QImage &img);

protected:
	/// used for both pasting and dropping
	void insertFromMimeData(const QMimeData *source) override;
};

#endif // COLORSTRIPPINGTEXTEDIT_H
