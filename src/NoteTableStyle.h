#ifndef NOTETABLESTYLE_H
#define NOTETABLESTYLE_H

#include <QColor>
#include <QPalette>
#include <QString>

class QTextDocument;

/**
 * The colours styleTables() painted the tables with.
 *
 * They are returned so that the caller can take them back out of the html
 * before the note is saved (stripTableTheme()): a note file must not end up
 * carrying the colours of whatever theme was active when it was last edited.
 */
struct ThemedTableStyle
{
	QColor grid;    ///< the 1px lines around and between the cells
	QColor header;  ///< the fill of the header row
	QColor alt;     ///< the fill of every second row of the body

	/// The colours as they appear in the html Qt writes out.
	QStringList colorNames() const;
};

/**
 * Re-styles every table of \p doc into the app's table look:
 *
 *  - one single 1px line between any two cells, in a colour that is a shade of
 *    the palette's text background, instead of whatever border the html that
 *    was pasted in carries;
 *  - the header row (the one the html marked with <th>, which the importer
 *    turns into bold centred cells) filled with a slightly stronger shade;
 *  - the rows of the body filled alternately, with the palette's AlternateBase
 *    (or with a shade of the base when the theme's own is too subtle to see);
 *  - a padding of the cell contents that does not depend on the source, and a
 *    table as wide as the text of the note.
 *
 * Nothing is hardcoded: every colour is derived from \p palette, so the same
 * note looks right under a light and under a dark theme. The colours are put
 * into the document, which is what QTextEdit paints with, and are meant to be
 * stripped again on the way out - see stripTableTheme().
 *
 * Returns the colours used.
 */
ThemedTableStyle styleTables(QTextDocument *doc, const QPalette &palette);

/**
 * The opposite of what styleTables() leaves behind in QTextEdit's html: the
 * cell backgrounds and the grid colour are removed again when they are exactly
 * one of \p used's colours. Anything else - including a colour a note already
 * had before it was opened - is left alone.
 */
QString stripTableTheme(const QString &html, const ThemedTableStyle &used);

#endif // NOTETABLESTYLE_H
