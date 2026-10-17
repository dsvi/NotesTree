#include "ColorStrippingTextEdit.h"

#include "HtmlColorStripper.h"

#include <QTextCursor>
#include <QMimeData>

void ColorStrippingTextEdit::insertFromMimeData(const QMimeData *source)
{
	if (!source || !source->hasHtml()){
		// a bare image on the clipboard (a screenshot) can't live in the
		// document on its own: hand it over to the note, which stores it in
		// the embed dir and inserts a reference back.
		if (source && source->hasImage()){
			auto img = qvariant_cast<QImage>(source->imageData());
			if (!img.isNull()){
				emit imagePasted(img);
				return;
			}
		}
		QTextEdit::insertFromMimeData(source);
		return;
	}

	QString stripped = stripHtmlColors(source->html());
	// QMimeData is not copyable, so the formats are copied one by one and only
	// the html is replaced: plain text, urls and whatever custom format the
	// source app put along keep working.
	QMimeData clean;
	if (stripped != source->html()){
		for (const QString &format : source->formats())
			clean.setData(format, source->data(format));
		clean.setHtml(stripped);
		source = &clean;
	}

	QTextEdit::insertFromMimeData(source);

	// the tables are restyled as part of the same edit: one Ctrl+Z has to take a
	// pasted table out again, not only paint it back to the look the source gave
	// it. The restyling can only happen once the paste is over - the table is not
	// in the document while the paste's edit block is still open - so the block
	// is joined instead of wrapped around it.
	QTextCursor cursor = textCursor();
	cursor.joinPreviousEditBlock();
	restyleTables();
	cursor.endEditBlock();
}

void ColorStrippingTextEdit::restyleTables()
{
	// Every table or cell format change makes QTextEdit relayout the whole
	// document, and a document with nested tables is expensive to lay out: html
	// copied out of a web page is usually a pile of 1x1 wrapper tables, and one
	// relayout of such a document costs a few hundred ms. Unbatched, restyling
	// means one relayout per cell - opening a Dell product page took 21s, the
	// same note restyled inside a single edit block takes 0.3s. Nested blocks are
	// fine, so this stays one command when a paste is already being edited.
	QTextCursor cursor(document());
	cursor.beginEditBlock();
	tableTheme_ = styleTables(document(), palette());
	cursor.endEditBlock();
}
