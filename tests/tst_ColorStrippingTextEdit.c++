// Test for the paste hook: html that arrives through the clipboard or a drop
// must end up in the document without any colour, while everything else about
// the paste keeps working.
//
// The pasted html is read from tests/fixtures/ (see the README there), the
// same files the stripper test runs on.

#include <QtTest>

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QMimeData>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextTable>
#include <QTextTableCellFormat>
#include <QTextTableFormat>

#include "ColorStrippingTextEdit.h"
#include "HtmlColorStripper.h"

namespace {

QString fixturesDir()
{
	return QStringLiteral(NOTES_TEST_FIXTURES);
}

QString fixture(const QString &fileName)
{
	QString path = QDir(fixturesDir()).filePath(fileName);
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly)){
		qWarning("cannot open fixture %s", qPrintable(path));
		return QString();
	}
	return QString::fromUtf8(f.readAll());
}

/// Counts the characters that are painted with an explicit colour instead of
/// with the palette. Links are skipped: Qt paints them with the palette's Link
/// colour, which is exactly what stripping the html is supposed to achieve.
int countColoredFragments(const QTextDocument *doc)
{
	int n = 0;
	for (QTextBlock b = doc->begin(); b.isValid(); b = b.next())
		for (auto pos = b.begin(); pos != b.end(); ++pos){
			const QTextCharFormat f = pos.fragment().charFormat();
			if (!f.anchorHref().isEmpty())
				continue;
			if (f.foreground().style() != Qt::NoBrush
			 || f.background().style() != Qt::NoBrush)
				++n;
		}
	return n;
}

/// Gives access to the protected insertFromMimeData(), which is what both
/// Ctrl+V and a drag'n'drop go through.
class Probe : public ColorStrippingTextEdit
{
public:
	void insert(const QMimeData *md){ insertFromMimeData(md); }
};

/// The last table of the document, or null when it has none.
QTextTable *lastTable(QTextDocument *doc)
{
	QTextTable *table = nullptr;
	for (auto it = doc->rootFrame()->begin(); !it.atEnd(); ++it){
		QTextFrame *frame = it.currentFrame();
		if (frame && frame->format().isTableFormat())
			table = static_cast<QTextTable *>(frame);
	}
	return table;
}

/// The html the app is given by a program that bakes its light theme into it.
QString pastedHtml()
{
	return fixture("paste-with-theme.html");
}

} // namespace

class TestColorStrippingTextEdit : public QObject
{
	Q_OBJECT

private slots:
	void pasteDropsColors();
	void pastedFragmentsHaveNoExplicitColors();
	void plainTextPasteStillWorks();
	void clipboardPasteIsStripped();
	void colorsAlreadyInTheNoteStay();
	void coloredFixtureHasNoColorsAfterPaste();
	void pastedTableTakesThePalettesLook();
	void savedHtmlLosesTheTableColours();
	void restyledPasteUndoesAsOneStep();
};

void TestColorStrippingTextEdit::pasteDropsColors()
{
	Probe edit;
	QMimeData md;
	md.setHtml(pastedHtml());
	md.setText("5.70куплено 1-11-2023");

	edit.insert(&md);

	QString html = edit.toHtml();
	QVERIFY2(!htmlHasColors(html), qPrintable(html));
	QCOMPARE(edit.toPlainText(), QString("5.70\nкуплено 1-11-2023"));
	// the bold is not a colour, it survives
	QVERIFY2(html.contains("font-weight"), qPrintable(html));
}

// A string search can only say that no colour is declared; this one asks the
// document itself whether any character has an explicit colour, i.e. whether
// the text really is painted with the palette.
void TestColorStrippingTextEdit::pastedFragmentsHaveNoExplicitColors()
{
	Probe stripped;
	QMimeData md;
	md.setHtml(pastedHtml());
	stripped.insert(&md);

	// the same html pasted without the hook, to be sure the check isn't vacuous
	QTextEdit plain;
	plain.setHtml(pastedHtml());
	int coloredBefore = countColoredFragments(plain.document());
	int coloredAfter = countColoredFragments(stripped.document());
	int fragments = 0;
	for (QTextBlock b = stripped.document()->begin(); b.isValid(); b = b.next())
		fragments += b.length();

	QVERIFY2(coloredBefore > 0, "the test html doesn't even carry a colour: the check is vacuous");
	QCOMPARE(coloredAfter, 0);
	QVERIFY(fragments > 0);
}

void TestColorStrippingTextEdit::plainTextPasteStillWorks()
{
	Probe edit;
	QMimeData md;
	md.setText("just text");
	edit.insert(&md);
	QCOMPARE(edit.toPlainText(), QString("just text"));

	// a url is inserted as a link
	Probe edit2;
	QMimeData md2;
	md2.setUrls({ QUrl("https://example.com") });
	edit2.insert(&md2);
	QVERIFY2(edit2.toHtml().contains("https://example.com"), qPrintable(edit2.toHtml()));
}

void TestColorStrippingTextEdit::clipboardPasteIsStripped()
{
	QClipboard *cb = QApplication::clipboard();
	if (!cb)
		QSKIP("no clipboard available");

	cb->clear();
	cb->setMimeData([&]{ auto m = new QMimeData(); m->setHtml(pastedHtml()); return m; }());

	ColorStrippingTextEdit edit;
	edit.paste();
	cb->clear();

	QString html = edit.toHtml();
	QVERIFY2(!htmlHasColors(html), qPrintable(html));
	QCOMPARE(edit.toPlainText(), QString("5.70\nкуплено 1-11-2023"));
}

void TestColorStrippingTextEdit::colorsAlreadyInTheNoteStay()
{
	// the hook only touches what comes in from outside: a note that was saved
	// with colours is loaded unchanged
	ColorStrippingTextEdit edit;
	const QString note = "<p><span style=\" color:#ff843c;\">orange</span></p>";
	edit.setHtml(note);
	QCOMPARE(edit.toPlainText(), QString("orange"));
	QVERIFY2(edit.toHtml().contains("#ff843c"), qPrintable(edit.toHtml()));
}

void TestColorStrippingTextEdit::coloredFixtureHasNoColorsAfterPaste()
{
	// every fixture that actually paints something in an explicit colour is
	// pasted the way the app pastes it and must end up palette-painted
	int checked = 0;
	for (const QString &name : QDir(fixturesDir()).entryList({"*.html"}, QDir::Files, QDir::Name)){
		QString html = fixture(name);
		if (!htmlHasColors(html))
			continue;
		// only fixtures whose colour reaches the text itself are interesting:
		// one that colours nothing but its links is of no use here
		QTextDocument before;
		before.setHtml(html);
		if (countColoredFragments(&before) == 0)
			continue;
		++checked;

		Probe edit;
		QMimeData md;
		md.setHtml(html);
		edit.insert(&md);
		QVERIFY2(countColoredFragments(edit.document()) == 0,
		         qPrintable(name + " still paints with an explicit colour"));
		// deliberately not checking htmlHasColors(edit.toHtml()) here: Qt's own
		// serializer bakes border colours (border-top-color:#000000) back into
		// the html from its frame format, and those come from Qt, not from the
		// pasted content. What matters is that no *character* is painted with an
		// explicit colour, which the check above asks the document itself.
	}
	QVERIFY2(checked > 0, "no fixture carries colour that reaches the text: the check is vacuous");
}

// A table that arrives from the clipboard is painted with the palette right
// away: the app's table look does not wait for the note to be saved and opened
// again.
void TestColorStrippingTextEdit::pastedTableTakesThePalettesLook()
{
	Probe edit;
	edit.setHtml("<p>hello</p>");
	edit.selectAll();
	QMimeData md;
	md.setHtml(R"html(<table><tr>
<td style="border:3px solid rgb(221,221,221); background-color:rgb(248,249,250)">a</td>
</tr></table>)html");
	edit.insert(&md);

	QTextTable *table = lastTable(edit.document());
	QVERIFY2(table != nullptr, "the pasted html did not become a table");

	QTextTableFormat tf = table->format();
	QCOMPARE(tf.borderBrush().color(), edit.tableTheme().grid);
	QCOMPARE(tf.borderCollapse(), true);
	QTextTableCellFormat cf = table->cellAt(0, 0).format().toTableCellFormat();
	QCOMPARE(cf.leftBorder(), qreal(0)); // the 3px of the source are gone
	QCOMPARE(cf.background(), QBrush(Qt::NoBrush)); // and its fill as well
}

// The colours the tables were painted with are the palette's, not the note's,
// so they must not end up in the file.
void TestColorStrippingTextEdit::savedHtmlLosesTheTableColours()
{
	Probe edit;
	edit.setHtml(R"(<table><tr><th>one</th></tr><tr><td>a</td></tr></table>)");
	edit.restyleTables();

	QString saved = stripTableTheme(edit.toHtml(), edit.tableTheme());
	QCOMPARE(htmlHasColors(saved), false);
}

// The restyling belongs to the paste instead of being an edit of its own: one
// Ctrl+Z takes a pasted table out completely.
void TestColorStrippingTextEdit::restyledPasteUndoesAsOneStep()
{
	Probe edit;
	edit.setHtml("<p>hello</p>");
	edit.selectAll();
	QMimeData md;
	md.setHtml(R"(<table><tr><td>a</td></tr></table>)");
	edit.insert(&md);
	QVERIFY2(lastTable(edit.document()) != nullptr, "the pasted html did not become a table");

	edit.document()->undo();
	QVERIFY2(lastTable(edit.document()) == nullptr, "one undo left the pasted table behind");
}

QTEST_MAIN(TestColorStrippingTextEdit)
#include "tst_ColorStrippingTextEdit.moc"
