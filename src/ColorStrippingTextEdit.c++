#include "ColorStrippingTextEdit.h"

#include "HtmlColorStripper.h"

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
	if (stripped == source->html()){
		QTextEdit::insertFromMimeData(source);
		return;
	}

	// QMimeData is not copyable, so the formats are copied one by one and only
	// the html is replaced: plain text, urls and whatever custom format the
	// source app put along keep working.
	QMimeData clean;
	for (const QString &format : source->formats())
		clean.setData(format, source->data(format));
	clean.setHtml(stripped);

	QTextEdit::insertFromMimeData(&clean);
}
