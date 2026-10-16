#include "NoteTableStyle.h"

#include <QFont>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextTable>
#include <QTextTableCell>
#include <QTextTableCellFormat>
#include <QTextTableFormat>

namespace {

// None of the colours below is a constant colour: each one is a mix of the
// palette's text background and its text, which is how Breeze shades its tree
// branch lines (see KdeLookStyle). A light theme therefore gets a light table
// and a dark theme a dark one, and the note itself never learns about it.
constexpr qreal GridShade   = 0.25; // the lines between the cells: the same
                                    // shade Breeze gives its branch lines
constexpr qreal HeaderShade = 0.20; // the fill of the header row
constexpr qreal AltShade    = 0.10; // the fill of every second row: the least
                                    // it has to stand out from the base
constexpr qreal CellPadding = 8;    // px between the text and the cell frame

/// KColorUtils::mix(), the same one KdeLookStyle ports.
QColor mix(const QColor &a, const QColor &b, qreal f)
{
	return QColor::fromRgbF(a.redF() * (1.0 - f) + b.redF() * f,
		a.greenF() * (1.0 - f) + b.greenF() * f,
		a.blueF() * (1.0 - f) + b.blueF() * f,
		a.alphaF() * (1.0 - f) + b.alphaF() * f);
}

/// True when every character of \p cell that is painted at all is bold.
bool isBold(const QTextTableCell &cell)
{
	bool hasText = false;
	bool bold    = true;
	const int end = cell.lastPosition();
	QTextCursor cursor(cell.firstCursorPosition());
	do {
		const QTextBlock block = cursor.block();
		for (auto frag = block.begin(); !frag.atEnd(); ++frag){
			if (frag.fragment().text().trimmed().isEmpty())
				continue;
			hasText = true;
			if (frag.fragment().charFormat().fontWeight() < QFont::Bold)
				bold = false;
		}
	} while (cursor.movePosition(QTextCursor::NextBlock) && cursor.position() <= end);
	return hasText && bold;
}

/// How many of the rows at the top of \p table read as a header.
///
/// QTextEdit's html importer turns a <th> into a bold centred <td> and does not
/// record anywhere that it was a header, so the weight of the text is all there
/// is to go by. A table built with insertTable() does set the header count, and
/// that one is taken as it is.
int headerRowCount(const QTextTable *table)
{
	int rows = table->format().headerRowCount();
	if (rows <= 0 && table->rows() > 1){
		bool bold = true;
		for (int col = 0; col < table->columns(); ++col){
			auto cell = table->cellAt(0, col);
			if (cell.row() != 0 || cell.column() != col)
				continue; // part of a span: it is looked at from its first cell
			if (!isBold(cell))
				bold = false;
		}
		if (bold)
			rows = 1;
	}
	return qBound(0, rows, table->rows() - 1);
}

/// The borders the pasted html asked for are dropped, so that the single grid
/// of the table itself is what gets drawn, in the palette's colour.
void clearCellBorders(QTextTableCellFormat &cf)
{
	static const QTextFormat::Property borders[] = {
		QTextFormat::TableCellTopBorder,      QTextFormat::TableCellBottomBorder,
		QTextFormat::TableCellLeftBorder,     QTextFormat::TableCellRightBorder,
		QTextFormat::TableCellTopBorderStyle, QTextFormat::TableCellBottomBorderStyle,
		QTextFormat::TableCellLeftBorderStyle, QTextFormat::TableCellRightBorderStyle,
		QTextFormat::TableCellTopBorderBrush, QTextFormat::TableCellBottomBorderBrush,
		QTextFormat::TableCellLeftBorderBrush, QTextFormat::TableCellRightBorderBrush,
	};
	for (auto p : borders)
		cf.clearProperty(p);
}

/// The padding of the source is kept as long as it is not smaller than the
/// app's; a table that asks for nothing in particular gets the app's.
void raiseCellPadding(QTextTableCellFormat &cf)
{
	if (cf.hasProperty(QTextFormat::TableCellTopPadding))
		cf.setTopPadding(qMax(cf.topPadding(), CellPadding));
	if (cf.hasProperty(QTextFormat::TableCellBottomPadding))
		cf.setBottomPadding(qMax(cf.bottomPadding(), CellPadding));
	if (cf.hasProperty(QTextFormat::TableCellLeftPadding))
		cf.setLeftPadding(qMax(cf.leftPadding(), CellPadding));
	if (cf.hasProperty(QTextFormat::TableCellRightPadding))
		cf.setRightPadding(qMax(cf.rightPadding(), CellPadding));
}

void styleTable(QTextTable *table, const ThemedTableStyle &colors)
{
	QTextTableFormat tf = table->format();
	tf.setCellSpacing(0);
	tf.setCellPadding(CellPadding);
	// With collapsed borders the layout draws every edge once, and an edge that
	// has no border of its own takes the table's: that is the single 1px grid.
	tf.setBorder(1);
	tf.setBorderBrush(colors.grid);
	tf.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
	tf.setBorderCollapse(true);
	// a table that is only as wide as its own content looks like it is floating
	// in the note, so the table takes the width of the text
	tf.setWidth(QTextLength(QTextLength::PercentageLength, 100));
	table->setFormat(tf);

	const int headerRows = headerRowCount(table);
	for (int row = 0; row < table->rows(); ++row)
		for (int col = 0; col < table->columns(); ++col){
			auto cell = table->cellAt(row, col);
			if (cell.row() != row || cell.column() != col)
				continue; // a span: it is styled from the cell that starts it
			QTextTableCellFormat cf = cell.format().toTableCellFormat();
			clearCellBorders(cf);
			raiseCellPadding(cf);
			if (row < headerRows)
				cf.setBackground(colors.header);
			else if ((row - headerRows) % 2)
				cf.setBackground(colors.alt);
			cell.setFormat(cf);
		}
}

/// The tables nested inside \p frame's cells. \p frame itself is not styled.
void styleNestedTables(const QTextFrame *frame, const ThemedTableStyle &colors)
{
	for (auto it = frame->begin(); !it.atEnd(); ++it){
		QTextFrame *child = it.currentFrame();
		if (!child)
			continue;
		if (child->format().isTableFormat())
			styleTable(static_cast<QTextTable *>(child), colors);
		styleNestedTables(child, colors);
	}
}

/// Removes every match of \p re whose colour, captured by \p colorGroup, is one
/// of \p colors.
QString removeColors(QString html, const QRegularExpression &re, int colorGroup,
		const QSet<QString> &colors)
{
	QString out;
	qsizetype copied = 0;
	auto matches = re.globalMatch(html);
	while (matches.hasNext()){
		auto m = matches.next();
		if (!colors.contains(m.captured(colorGroup).toLower()))
			continue;
		out += html.mid(copied, m.capturedStart() - copied);
		copied = m.capturedEnd();
	}
	out += html.mid(copied);
	return out;
}

// the cell fills: <td bgcolor="#1d1f22">
const QRegularExpression &bgColorAttrRe()
{
	static const QRegularExpression re(
		"\\s+bgcolor\\s*=\\s*(\"?)\\s*(#[0-9a-fA-F]{6})\\1",
		QRegularExpression::CaseInsensitiveOption);
	return re;
}

// the grid: border-color:#4c4c4c; and its border-*-color variants. The prefix
// keeps it inside a style attribute: a note that talks about css must not lose
// the colour it quotes.
const QRegularExpression &borderColorDeclRe()
{
	static const QRegularExpression re(
		"(?:\"|;)\\s*\\Kborder(?:-[a-z]+)*-color\\s*:\\s*(#[0-9a-fA-F]{6})\\s*;",
		QRegularExpression::CaseInsensitiveOption);
	return re;
}

} // namespace

QStringList ThemedTableStyle::colorNames() const
{
	// spelled the way QTextEdit's serializer spells them
	return { grid.name(), header.name(), alt.name() };
}

ThemedTableStyle styleTables(QTextDocument *doc, const QPalette &palette)
{
	const QColor base = palette.color(QPalette::Base);
	const QColor text = palette.color(QPalette::Text);

	ThemedTableStyle colors;
	colors.grid   = mix(base, text, GridShade);
	colors.header = mix(base, text, HeaderShade);
	// the theme's own alternating colour is the right one for the rows, but in a
	// dark theme it is often barely a shade away from the base and the striping
	// vanishes next to the header fill: it never goes under AltShade
	colors.alt = palette.color(QPalette::AlternateBase);
	const qreal room = qAbs(text.lightnessF() - base.lightnessF());
	if (!colors.alt.isValid()
			|| qAbs(colors.alt.lightnessF() - base.lightnessF()) < AltShade * room)
		colors.alt = mix(base, text, AltShade);

	if (doc)
		for (auto it = doc->rootFrame()->begin(); !it.atEnd(); ++it){
			QTextFrame *frame = it.currentFrame();
			if (!frame || !frame->format().isTableFormat())
				continue;
			styleTable(static_cast<QTextTable *>(frame), colors);
			styleNestedTables(frame, colors);
		}
	return colors;
}

QString stripTableTheme(const QString &html, const ThemedTableStyle &used)
{
	QSet<QString> colors;
	for (const QString &name : used.colorNames())
		colors.insert(name.toLower());
	if (colors.isEmpty())
		return html;

	QString out = removeColors(html, bgColorAttrRe(), 2, colors);
	out = removeColors(out, borderColorDeclRe(), 1, colors);
	return out;
}
