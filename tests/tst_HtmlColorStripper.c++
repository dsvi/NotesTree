// Test for stripHtmlColors().
//
// The html the stripper has to handle lives in tests/fixtures/ as plain files
// (see tests/fixtures/README.md): each one is a simplified stand-in for a
// pattern found in real notes - browser pastes that bake the light theme into
// <style> blocks, web tables with rgb() inline styles, legacy colour
// attributes, html saved by this app itself, html that is not well formed xml.
//
// On top of the per-fixture expectations every *.html file in that directory
// is swept: nothing but colour may be removed, no colour may survive, and
// stripping twice must equal stripping once.
//
// build & run:
//   cmake -S tests -B tests/build && cmake --build tests/build && ./tests/build/tst_HtmlColorStripper

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QRegularExpression>

#include "HtmlColorStripper.h"

namespace {

// ------------------------------------------------------------- test helpers

/// Removes tags, style sheets and comments, so only the text stays. Used to
/// check that stripping colours doesn't eat any content.
QString plainText(QString h)
{
	static const QRegularExpression styleRe("(?is)<(script|style)[^>]*>.*?</\\1>");
	static const QRegularExpression commentRe("(?s)<!--.*?-->");
	static const QRegularExpression tagRe("(?s)<[^>]*>");
	h.remove(styleRe);
	h.remove(commentRe);
	h.remove(tagRe);
	h.replace("&nbsp;", " ");
	h.replace("&lt;", "<");
	h.replace("&gt;", ">");
	h.replace("&quot;", "\"");
	h.replace("&amp;", "&");
	return h.simplified();
}

/// True if \p css still declares a colour. Quoted parts are skipped: notes
/// contain pasted junk like font-family:'georgia','font-size:1.45em;color:rgb(0,0,0)',
/// where the colour sits inside a (mangled) font name instead of being a
/// colour declaration.
bool cssMentionsColor(QStringView css)
{
	QString unquoted;
	QChar quote;
	for (QChar ch : css){
		if (!quote.isNull()){
			if (ch == quote)
				quote = QChar();
			continue;
		}
		if (ch == u'"' || ch == u'\''){
			quote = ch;
			continue;
		}
		unquoted += ch;
	}
	static const QRegularExpression colorDecl(
		"(?i)(^|[;{\\s])(color|background|background-color|background-image|"
		"border(-[a-z]+)*-color|outline-color|box-shadow|text-shadow|fill|stroke|"
		"caret-color|accent-color|text-decoration-color|stop-color|scrollbar-color)\\s*:");
	static const QRegularExpression colorValue(
		"(?i)(#[0-9a-fA-F]{3,8}\\b|\\brgba?\\(|\\bhsla?\\(|\\b(currentcolor|transparent)\\b)");
	return colorDecl.match(unquoted).hasMatch() || colorValue.match(unquoted).hasMatch();
}

/// Everything in \p html that still carries a colour after stripping.
/// Only the places where colour can be defined are looked at, so that a note
/// whose *text* mentions "#ff843c" is not reported.
QStringList colorLeftovers(const QString &html)
{
	static const QRegularExpression tagRe("(?s)<[^>]*>");
	static const QRegularExpression styleAttrRe(
		"(?i)style\\s*=\\s*(?:\"([^\"]*)\"|'([^']*)')");
	static const QRegularExpression legacyAttrRe(
		"(?i)\\b(color|bgcolor|bordercolor|bordercolorlight|bordercolordark)\\s*=");
	static const QRegularExpression styleBlockRe("(?is)<style[^>]*>(.*?)</style>");

	QStringList found;
	auto it = tagRe.globalMatch(html);
	while (it.hasNext()){
		QStringView tag = it.next().capturedView();
		if (legacyAttrRe.match(tag.toString()).hasMatch())
			found << "colour attribute in tag: " + tag.toString();
		auto attrIt = styleAttrRe.globalMatch(tag);
		while (attrIt.hasNext()){
			auto m = attrIt.next();
			QStringView value = m.capturedView(1).isEmpty() ? m.capturedView(2) : m.capturedView(1);
			if (cssMentionsColor(value))
				found << "colour in style attribute: " + value.toString();
		}
	}
	auto blockIt = styleBlockRe.globalMatch(html);
	while (blockIt.hasNext()){
		QStringView css = blockIt.next().capturedView(1);
		if (cssMentionsColor(css))
			found << "colour in style block: " + css.toString();
	}
	return found;
}

// ---------------------------------------------------------------- fixtures
// plain html files in tests/fixtures/, set at compile time by CMake

QString fixturesDir()
{
	return QStringLiteral(NOTES_TEST_FIXTURES);
}

/// All fixture file names, sorted so failures are reproducible.
QStringList fixtureFiles()
{
	return QDir(fixturesDir()).entryList({"*.html"}, QDir::Files, QDir::Name);
}

/// Reads one fixture. The fixtures belong to the repository, so a missing or
/// empty one is a broken checkout, not a reason to skip.
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

} // namespace

class TestHtmlColorStripper : public QObject
{
	Q_OBJECT

private slots:
	void fixturesAreNotVacuous();
	void styleBlocksFromBrowserPaste();
	void inlineStylesFromWebPage();
	void appSavedHtmlKeepsItsFormatting();
	void legacyColorAttributes();
	void coloredLinksLoseTheirColor();
	void borderColorGoesBorderStays();
	void mangledFontFamilyIsLeftAlone();
	void colorlessHtmlIsUntouched();
	void malformedHtmlIsUntouched();
	void idempotent();
	void noColorLeftovers();
	void textIsPreserved();
};

/// The sweep tests are only worth something if the fixtures actually contain
/// colour - and the untouched path needs at least one fixture without it.
void TestHtmlColorStripper::fixturesAreNotVacuous()
{
	QStringList names = fixtureFiles();
	QVERIFY2(names.size() >= 10,
	         qPrintable(QStringLiteral("only %1 fixtures in %2")
	                    .arg(names.size()).arg(fixturesDir())));
	int colored = 0;
	int colorless = 0;
	for (const QString &n : names)
		htmlHasColors(fixture(n)) ? ++colored : ++colorless;
	QVERIFY2(colored > 0, "no fixture carries colour: the tests would prove nothing");
	QVERIFY2(colorless > 0, "every fixture has colour: the untouched path is not covered");
}

void TestHtmlColorStripper::styleBlocksFromBrowserPaste()
{
	// the whole <style> blocks are dropped: they only said "this is the
	// light theme's text/link colour"
	QString out = stripHtmlColors(fixture("browser-paste-style-blocks.html"));
	QCOMPARE(out, QString(
		"<html contenteditable=\"true\"><head><meta charset=\"UTF-8\"></head><body>5.70"
		"<div><br></div><div>куплено 1-11-2023</div></body></html>"));
}

void TestHtmlColorStripper::inlineStylesFromWebPage()
{
	QString out = stripHtmlColors(fixture("web-table-rgb-inline.html"));

	// colours are gone
	QVERIFY2(!out.contains("rgb("), qPrintable(out));
	QVERIFY2(!out.contains("background-color"), qPrintable(out));
	QVERIFY2(!out.contains("color:"), qPrintable(out));

	// the layout and the typography are not
	QVERIFY2(out.contains("border-collapse: collapse"), qPrintable(out));
	QVERIFY2(out.contains("width: 1140px"), qPrintable(out));
	QVERIFY2(out.contains("font-family: 'Source Sans Pro', sans-serif"), qPrintable(out));
	QVERIFY2(out.contains("padding: 8px"), qPrintable(out));
	QVERIFY2(out.contains("font-weight: normal"), qPrintable(out));
	// the border keeps its width and style, only its colour is dropped
	QVERIFY2(out.contains("border: 1px solid;"), qPrintable(out));
	QVERIFY2(out.contains("border-width: 0px 1px 2px"), qPrintable(out));
	QVERIFY2(out.contains("border-bottom-style: solid"), qPrintable(out));

	// attributes that don't carry colour survive
	QVERIFY2(out.contains("id=\"material-props\""), qPrintable(out));
	QVERIFY2(out.contains("class=\"table table-bordered\""), qPrintable(out));
}

void TestHtmlColorStripper::appSavedHtmlKeepsItsFormatting()
{
	QString out = stripHtmlColors(fixture("app-saved-qt-html.html"));

	QVERIFY2(!out.contains("#ff843c"), qPrintable(out));
	QVERIFY2(!out.contains("color"), qPrintable(out));
	// the underline is not a colour, it stays
	QVERIFY2(out.contains("text-decoration: underline"), qPrintable(out));
	QVERIFY2(out.contains("font-size:xx-large"), qPrintable(out));
	QVERIFY2(out.contains("margin-top:18px"), qPrintable(out));
	QVERIFY2(out.contains("-qt-block-indent:0"), qPrintable(out));
	QVERIFY2(out.contains("<img src=\"./1.embed/0_a3e9d_880232c_orig\" />"), qPrintable(out));
}

void TestHtmlColorStripper::legacyColorAttributes()
{
	QString out = stripHtmlColors(fixture("legacy-attributes.html"));

	QVERIFY2(!out.contains("bgcolor"), qPrintable(out));
	QVERIFY2(!out.contains("color="), qPrintable(out));
	QVERIFY2(!out.contains("#DDDDDD"), qPrintable(out));
	QVERIFY2(!out.contains("#0645ad"), qPrintable(out));
	// non colour attributes of the same tags stay
	QVERIFY2(out.contains("border=\"1\""), qPrintable(out));
	QVERIFY2(out.contains("cellpadding=\"2\""), qPrintable(out));
	QVERIFY2(out.contains("align=\"center\""), qPrintable(out));
	QVERIFY2(out.contains("face=\"arial, verdana, courier\""), qPrintable(out));
	QCOMPARE(colorLeftovers(out), QStringList());
}

void TestHtmlColorStripper::coloredLinksLoseTheirColor()
{
	// after stripping, a link is the only thing Qt still paints in a non
	// palette colour (the palette's Link role) - that is the goal, not a
	// leftover, but the explicit colour on the anchor itself must be gone
	QString in = fixture("colored-links.html");
	QString out = stripHtmlColors(in);

	QVERIFY2(!out.contains("color"), qPrintable(out));
	QVERIFY2(out.contains("href=\"https://example.com\""), qPrintable(out));
	QVERIFY2(out.contains("href=\"https://example.org/old\""), qPrintable(out));
	QCOMPARE(plainText(out), plainText(in));
}

void TestHtmlColorStripper::borderColorGoesBorderStays()
{
	// the most common colour declaration in real notes: the border must stay
	// visible on a dark theme, only its colour goes
	QString out = stripHtmlColors(fixture("border-shorthand.html"));
	QVERIFY2(!out.contains("rgb("), qPrintable(out));
	QVERIFY2(!out.contains("#dddddd"), qPrintable(out));
	QVERIFY2(!out.contains("#ddd"), qPrintable(out));
	QVERIFY2(!out.contains("color:"), qPrintable(out));
	QVERIFY2(out.contains("border: 1px solid;"), qPrintable(out));
	QVERIFY2(out.contains("padding: 8px"), qPrintable(out));

	// the same in isolation
	QCOMPARE(stripHtmlColors("<p style=\"border: 1px solid #dddddd\">x</p>"),
	         QString("<p style=\"border: 1px solid\">x</p>"));
	QCOMPARE(stripHtmlColors("<p style=\"border-bottom: 2px solid black; color: white\">x</p>"),
	         QString("<p style=\"border-bottom: 2px solid;\">x</p>"));
	// a border without a colour is left alone
	QCOMPARE(stripHtmlColors("<p style=\"border: 1px solid\">x</p>"),
	         QString("<p style=\"border: 1px solid\">x</p>"));
	// a declaration that is nothing but colour disappears completely, and with
	// it the style attribute if it was the only one
	QCOMPARE(stripHtmlColors("<span style=\"color:#ff0000\">x</span>"),
	         QString("<span>x</span>"));
	QCOMPARE(stripHtmlColors("<span style=\"COLOR: RGB(1, 2, 3)\">x</span>"),
	         QString("<span>x</span>"));
}

void TestHtmlColorStripper::mangledFontFamilyIsLeftAlone()
{
	// html this app saved itself: the page it was pasted from had a broken
	// font-family whose quoted value looks like colour declarations. It is a
	// font name, so it stays; the real colour behind it goes.
	QString out = stripHtmlColors(fixture("mangled-font-family.html"));
	QCOMPARE(out, QString(
		"<span style=\" font-family:'georgia','font-size:1.45em;color:rgb(0, 0, 0)'; font-size:16px;\">x</span>"));
}

void TestHtmlColorStripper::colorlessHtmlIsUntouched()
{
	// the fixture without colours, including its style sheet, comes back
	// byte for byte identical
	QString in = fixture("colorless.html");
	QCOMPARE(stripHtmlColors(in), in);
}

void TestHtmlColorStripper::malformedHtmlIsUntouched()
{
	// html that is not well formed xml, which a dom based implementation
	// would choke on, passes through untouched
	QString in = fixture("malformed-html.html");
	QCOMPARE(stripHtmlColors(in), in);
}

void TestHtmlColorStripper::idempotent()
{
	for (const QString &name : fixtureFiles()){
		QString once = stripHtmlColors(fixture(name));
		QString twice = stripHtmlColors(once);
		QVERIFY2(once == twice, qPrintable(name + ": " + twice));
	}
}

void TestHtmlColorStripper::noColorLeftovers()
{
	for (const QString &name : fixtureFiles()){
		QString out = stripHtmlColors(fixture(name));
		QStringList leftovers = colorLeftovers(out);
		QVERIFY2(leftovers.isEmpty(),
		         qPrintable(name + " still has:\n  " + leftovers.join("\n  ") + "\nas: " + out));
	}
}

void TestHtmlColorStripper::textIsPreserved()
{
	for (const QString &name : fixtureFiles()){
		QString in = fixture(name);
		QString out = stripHtmlColors(in);
		QCOMPARE(plainText(out), plainText(in));
	}
}

QTEST_MAIN(TestHtmlColorStripper)
#include "tst_HtmlColorStripper.moc"
