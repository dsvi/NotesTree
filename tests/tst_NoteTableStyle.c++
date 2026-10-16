#include <QPalette>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextTable>
#include <QTextTableCellFormat>
#include <QTextTableFormat>
#include <QtTest>

#include "HtmlColorStripper.h"
#include "NoteTableStyle.h"

/**
 * The tables of a note are painted with the palette, never with colours that
 * come from the html or, worse, from the theme that happened to be active when
 * the note was last saved.
 */
class NoteTableStyleTest : public QObject
{
	Q_OBJECT

private:
	// a palette of extremes, so that a mix of the two is easy to tell apart
	static QPalette darkPalette()
	{
		QPalette pal;
		pal.setColor(QPalette::Base, QColor(0, 0, 0));
		pal.setColor(QPalette::Text, QColor(255, 255, 255));
		pal.setColor(QPalette::AlternateBase, QColor(32, 32, 32));
		return pal;
	}

	static QPalette lightPalette()
	{
		QPalette pal;
		pal.setColor(QPalette::Base, QColor(255, 255, 255));
		pal.setColor(QPalette::Text, QColor(0, 0, 0));
		pal.setColor(QPalette::AlternateBase, QColor(232, 232, 232));
		return pal;
	}

	static QTextTable *onlyTable(QTextDocument *doc)
	{
		for (auto it = doc->rootFrame()->begin(); !it.atEnd(); ++it){
			QTextFrame *frame = it.currentFrame();
			if (frame && frame->format().isTableFormat())
				return static_cast<QTextTable *>(frame);
		}
		return nullptr;
	}

	static QBrush cellBrush(QTextTable *table, int row, int col)
	{
		return table->cellAt(row, col).format().toTableCellFormat().background();
	}

	static QTextTableFormat tableFormat(QTextTable *table)
	{
		return table->format();
	}

private slots:
	/// the grid is the palette's, and it is one single line
	void gridComesFromThePalette()
	{
		QTextDocument doc;
		doc.setHtml(R"(<table border="1"><tr><td>a</td><td>b</td></tr></table>)");
		auto used = styleTables(&doc, darkPalette());
		auto table = onlyTable(&doc);
		QVERIFY(table != nullptr);

		auto tf = tableFormat(table);
		QCOMPARE(tf.cellSpacing(), qreal(0));
		QCOMPARE(tf.border(), qreal(1));
		QCOMPARE(tf.borderStyle(), QTextFrameFormat::BorderStyle_Solid);
		QCOMPARE(tf.borderCollapse(), true);
		QCOMPARE(tf.borderBrush().color(), used.grid);

		// every colour is a shade of the text background towards the text, so
		// none of them is a colour that was made up
		QVERIFY(used.grid.red() > 0 && used.grid.red() < 255);
		QVERIFY(used.grid.red() > used.header.red());
		QVERIFY(used.header.red() > used.alt.red());
		QCOMPARE(used.alt, darkPalette().color(QPalette::AlternateBase));
	}

	/// a <th> row is the header, even though the html importer throws that
	/// information away
	void headerRowIsFilled()
	{
		QTextDocument doc;
		doc.setHtml(R"(<table><tr><th>one</th><th>two</th></tr>
<tr><td>a</td><td>b</td></tr></table>)");
		auto used = styleTables(&doc, darkPalette());
		auto table = onlyTable(&doc);

		QCOMPARE(cellBrush(table, 0, 0).color(), used.header);
		QCOMPARE(cellBrush(table, 0, 1).color(), used.header);
	}

	/// the body alternates, and the row that is not filled shows the base of
	/// the editor instead of a colour of its own
	void bodyRowsAlternate()
	{
		QTextDocument doc;
		doc.setHtml(R"(<table><tr><th>one</th></tr>
<tr><td>a</td></tr><tr><td>b</td></tr><tr><td>c</td></tr></table>)");
		auto used = styleTables(&doc, darkPalette());
		auto table = onlyTable(&doc);

		// the first row of the body is the base of the editor itself, the second
		// one is the alternate, and so on
		QCOMPARE(cellBrush(table, 1, 0), QBrush(Qt::NoBrush));
		QCOMPARE(cellBrush(table, 2, 0).color(), used.alt);
		QCOMPARE(cellBrush(table, 3, 0), QBrush(Qt::NoBrush));
	}

	/// a table whose first row is not bold has no header: nothing is invented
	void tableWithoutAHeader()
	{
		QTextDocument doc;
		doc.setHtml(R"(<table><tr><td>a</td></tr><tr><td>b</td></tr></table>)");
		auto used = styleTables(&doc, darkPalette());
		auto table = onlyTable(&doc);

		QCOMPARE(cellBrush(table, 0, 0), QBrush(Qt::NoBrush));
		QCOMPARE(cellBrush(table, 1, 0).color(), used.alt);
	}

	/// the borders the source html asks for are dropped: they would be drawn in
	/// the colours of whatever program wrote the html
	void bordersOfTheSourceAreReplaced()
	{
		QTextDocument doc;
		doc.setHtml(R"html(<table><tr>
<td style="border:3px solid rgb(221,221,221)">a</td>
<td style="border-top:1px solid;border-bottom:2px solid">b</td>
</tr></table>)html");
		styleTables(&doc, darkPalette());
		auto table = onlyTable(&doc);

		for (int col = 0; col < 2; ++col){
			QTextTableCellFormat cf = table->cellAt(0, col).format().toTableCellFormat();
			QCOMPARE(cf.topBorder(), qreal(0));
			QCOMPARE(cf.bottomBorder(), qreal(0));
			QCOMPARE(cf.leftBorder(), qreal(0));
			QCOMPARE(cf.rightBorder(), qreal(0));
			QCOMPARE(cf.topBorderStyle(), QTextFrameFormat::BorderStyle_None);
		}
	}

	/// a cell is never more cramped than the app's padding, but a source that
	/// asks for more keeps it
	void paddingIsNotSmallerThanTheApps()
	{
		QTextDocument doc;
		doc.setHtml(R"html(<table><tr>
<td style="padding:2px">a</td>
<td style="padding:13px">b</td>
<td>c</td>
</tr></table>)html");
		styleTables(&doc, darkPalette());
		auto table = onlyTable(&doc);

		QCOMPARE(table->format().cellPadding(), qreal(8));
		QTextTableCellFormat cramped = table->cellAt(0, 0).format().toTableCellFormat();
		QCOMPARE(cramped.topPadding(), qreal(8));
		QTextTableCellFormat roomy = table->cellAt(0, 1).format().toTableCellFormat();
		QCOMPARE(roomy.topPadding(), qreal(13));
		QTextTableCellFormat plain = table->cellAt(0, 2).format().toTableCellFormat();
		QCOMPARE(plain.topPadding(), qreal(0)); // inherits the table's
	}

	/// what the editor hands to the note is the html without the palette's
	/// colours: the note must not end up painted with the theme of the machine
	/// it was last edited on
	void savedNoteDoesNotKeepTheTheme()
	{
		QTextDocument doc;
		doc.setHtml(R"(<table><tr><th>one</th><th>two</th></tr>
<tr><td>a</td><td>b</td></tr><tr><td>c</td><td>d</td></tr></table>)");
		auto used = styleTables(&doc, darkPalette());

		QString painted = doc.toHtml();
		QVERIFY(htmlHasColors(painted)); // the fills are in there ...
		QString saved = stripTableTheme(painted, used);
		QCOMPARE(htmlHasColors(saved), false); // ... and out again

		// the structure the styling asked for survives, so the note keeps
		// looking like a table in a browser as well
		QVERIFY(saved.contains("border-collapse:collapse"));
		QVERIFY(saved.contains("cellpadding=\"8\""));

		// and reading it back gives the same look under the same palette
		QTextDocument again;
		again.setHtml(saved);
		auto repainted = styleTables(&again, darkPalette());
		QCOMPARE(repainted.grid, used.grid);
		auto table = onlyTable(&again);
		QCOMPARE(cellBrush(table, 0, 0).color(), used.header);
		QCOMPARE(cellBrush(table, 1, 0), QBrush(Qt::NoBrush));
		QCOMPARE(cellBrush(table, 2, 0).color(), used.alt);
	}

	/// only the colours the styling put in are taken out: a colour the note
	/// itself carries is none of our business
	void coloursOfTheNoteItselfStay()
	{
		ThemedTableStyle used;
		used.grid   = QColor(0x38, 0x38, 0x38);
		used.header = QColor(0x41, 0x41, 0x41);
		used.alt    = QColor(0x20, 0x20, 0x20);

		QString html = R"(<table style="border-color:#383838; border-style:solid">
<tr><td bgcolor="#414141">ours</td><td bgcolor="#ff843c">theirs</td></tr>
<tr><td style="border-bottom:1px solid; border-bottom-color:#ff843c">kept</td></tr>
</table>)";
		QString saved = stripTableTheme(html, used);

		QVERIFY2(!saved.contains("#383838"), qPrintable(saved));
		QVERIFY2(!saved.contains("#414141"), qPrintable(saved));
		QVERIFY2(saved.contains("#ff843c"), qPrintable(saved));
		// the rest of the style attribute is left in one piece
		QVERIFY2(saved.contains("border-style:solid"), qPrintable(saved));
		QVERIFY2(saved.contains("border-bottom:1px solid"), qPrintable(saved));
	}

	/// the very same code under a light theme paints a light table
	void lightThemeIsALightTable()
	{
		QTextDocument doc;
		doc.setHtml(R"(<table><tr><th>one</th></tr><tr><td>a</td></tr></table>)");
		auto used = styleTables(&doc, lightPalette());

		QVERIFY(used.grid.red() < 255 && used.grid.red() > 0);
		QVERIFY(used.grid.red() < used.header.red());
		QVERIFY(used.header.red() < used.alt.red());
		QVERIFY(used.alt.red() < 255);
	}

	/// a theme whose AlternateBase is a colour of its own is not argued with:
	/// the floor only kicks in when the striping would not be visible
	void strongAlternateBaseIsUsedAsIs()
	{
		QPalette pal = darkPalette();
		pal.setColor(QPalette::AlternateBase, QColor(120, 120, 120));

		QTextDocument doc;
		doc.setHtml(R"(<table><tr><td>a</td></tr><tr><td>b</td></tr></table>)");
		auto used = styleTables(&doc, pal);
		auto table = onlyTable(&doc);

		QCOMPARE(used.alt, QColor(120, 120, 120));
		QCOMPARE(cellBrush(table, 1, 0).color(), used.alt);
	}

	/// a theme without an AlternateBase still gets alternating rows
	void alternatingWithoutAlternateBase()
	{
		QPalette pal = darkPalette();
		pal.setColor(QPalette::AlternateBase, pal.color(QPalette::Base));

		QTextDocument doc;
		doc.setHtml(R"(<table><tr><td>a</td></tr><tr><td>b</td></tr></table>)");
		auto used = styleTables(&doc, pal);
		auto table = onlyTable(&doc);

		QVERIFY(used.alt != pal.color(QPalette::Base));
		QCOMPARE(cellBrush(table, 1, 0).color(), used.alt);
	}

	/// a table inside a table is styled too
	void nestedTablesAreStyled()
	{
		QTextDocument doc;
		doc.setHtml(R"(<table><tr><td>outer
<table><tr><th>in</th></tr><tr><td>ner</td></tr></table>
</td></tr></table>)");
		auto used = styleTables(&doc, darkPalette());

		int tables = 0;
		for (auto it = doc.rootFrame()->begin(); !it.atEnd(); ++it){
			QTextFrame *frame = it.currentFrame();
			if (!frame || !frame->format().isTableFormat())
				continue;
			++tables;
			auto *inner = static_cast<QTextTable *>(frame);
			for (auto sub = inner->begin(); !sub.atEnd(); ++sub){
				QTextFrame *child = sub.currentFrame();
				if (!child || !child->format().isTableFormat())
					continue;
				++tables;
				auto *t = static_cast<QTextTable *>(child);
				QCOMPARE(t->format().borderBrush().color(), used.grid);
				QCOMPARE(cellBrush(t, 0, 0).color(), used.header);
			}
		}
		QCOMPARE(tables, 2);
	}
};

QTEST_MAIN(NoteTableStyleTest)
#include "tst_NoteTableStyle.moc"
