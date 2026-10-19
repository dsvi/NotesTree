#include "ThemedSvgIconEngine.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

/// The toolbar icons have to follow the desktop's colour scheme while the app
/// is running. They are flat SVGs the app recolours, so the question is whether
/// the recolouring happens when the icon is drawn or when it is made - and the
/// difference is only visible by drawing the same icon twice with a palette
/// change in between.
class TestThemedSvgIconEngine : public QObject
{
	Q_OBJECT

private slots:
	/// the whole point: one QIcon, two palettes, two colours
	void followsThePalette();
	/// the hovered and the disabled glyph, the other two colours an icon of
	/// this app is drawn in
	void hoveredAndDisabledVariants();
	/// QIcon clones the engine when an icon is copied, and the app hands the
	/// same icon to actions and buttons; every copy has to follow as well
	void copiesFollowToo();
	/// the size the style asks for is a device independent one
	void rendersAtTheScreenRatio();

private:
	/// the colour in the middle of a glyph drawn in this mode
	static QColor glyph(const QIcon &icon, QIcon::Mode mode);
};

/// a glyph covering the whole viewBox, so the middle of the pixmap is the
/// glyph's colour and nothing else
static const QByteArray fullSquareSvg =
	"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\""
	" viewBox=\"0 0 32 32\"><path d=\"M0 0h32v32H0z\"/></svg>";

QColor TestThemedSvgIconEngine::glyph(const QIcon &icon, QIcon::Mode mode)
{
	const QPixmap pm = icon.pixmap(QSize(16, 16), mode, QIcon::Off);
	if (pm.isNull())
		return QColor(Qt::transparent);
	return pm.toImage().convertToFormat(QImage::Format_ARGB32).pixelColor(8, 8);
}

void TestThemedSvgIconEngine::followsThePalette()
{
	QIcon icon(new ThemedSvgIconEngine(fullSquareSvg, "test", 1.0));

	QPalette light;
	light.setColor(QPalette::Text, QColor(20, 20, 20));
	QApplication::setPalette(light);
	QCOMPARE(glyph(icon, QIcon::Normal), QColor(20, 20, 20));

	// a colour scheme switch boils down to this: the palette is rebuilt, the
	// icon is not
	QPalette dark;
	dark.setColor(QPalette::Text, QColor(230, 230, 230));
	QApplication::setPalette(dark);
	QCOMPARE(glyph(icon, QIcon::Normal), QColor(230, 230, 230));

	// and back again: a cache that updates only once is just as wrong as one
	// that never updates
	QApplication::setPalette(light);
	QCOMPARE(glyph(icon, QIcon::Normal), QColor(20, 20, 20));
}

void TestThemedSvgIconEngine::hoveredAndDisabledVariants()
{
	QPalette p;
	p.setColor(QPalette::Text, QColor(20, 20, 20));
	p.setColor(QPalette::Disabled, QPalette::Text, QColor(150, 150, 150));
	QApplication::setPalette(p);

	QIcon icon(new ThemedSvgIconEngine(fullSquareSvg, "test", 1.0));

	// the cursor over an auto raise button: the accent of the colour scheme,
	// which is what Breeze uses for it
	QCOMPARE(glyph(icon, QIcon::Active), QColor("#ff0000"));
	// a disabled action: without a variant of its own Qt fades the glyph
	// towards the window colour instead
	QCOMPARE(glyph(icon, QIcon::Disabled), QColor(150, 150, 150));

	// the accent is looked up together with the palette, so a scheme that
	// changes both has to change both here as well
	p.setColor(QPalette::Text, QColor(230, 230, 230));
	QApplication::setPalette(p);
	QCOMPARE(glyph(icon, QIcon::Normal), QColor(230, 230, 230));
	QCOMPARE(glyph(icon, QIcon::Active), QColor("#ff0000"));
}

void TestThemedSvgIconEngine::copiesFollowToo()
{
	QPalette p;
	p.setColor(QPalette::Text, QColor(20, 20, 20));
	QApplication::setPalette(p);

	QIcon icon(new ThemedSvgIconEngine(fullSquareSvg, "test", 1.0));
	QIcon copy = icon;         // QIcon clones the engine here
	QIcon assigned;
	assigned = icon;

	QPalette dark;
	dark.setColor(QPalette::Text, QColor(230, 230, 230));
	QApplication::setPalette(dark);

	QCOMPARE(glyph(icon,    QIcon::Normal), QColor(230, 230, 230));
	QCOMPARE(glyph(copy,    QIcon::Normal), QColor(230, 230, 230));
	QCOMPARE(glyph(assigned, QIcon::Normal), QColor(230, 230, 230));
}

void TestThemedSvgIconEngine::rendersAtTheScreenRatio()
{
	QPalette p;
	p.setColor(QPalette::Text, QColor(0, 255, 0));
	QApplication::setPalette(p);

	QIcon icon(new ThemedSvgIconEngine(fullSquareSvg, "test", 2.0));
	const QPixmap pm = icon.pixmap(QSize(16, 16), QIcon::Normal, QIcon::Off);

	// the size the style asked for is the logical one; the surface is bigger by
	// the screen's ratio and says so, which is what keeps the glyph sharp on a
	// HiDPI screen instead of leaving the style to resample it afterwards
	QCOMPARE(pm.devicePixelRatio(), 2.0);
	QCOMPARE(pm.size(), QSize(32, 32));
	QCOMPARE(pm.toImage().convertToFormat(QImage::Format_ARGB32).pixelColor(16, 16),
			QColor(0, 255, 0));
}

int main(int argc, char *argv[])
{
	// The accent colour is read from kdeglobals, so the test needs a config
	// location of its own instead of whatever the desktop it runs on has
	// configured. QStandardPaths is asked again on every lookup, but the
	// application palette is taken at startup, so this is before App is built.
	QTemporaryDir configDir;
	if (!configDir.isValid())
		return 1;
	qputenv("XDG_CONFIG_HOME", configDir.path().toUtf8());

	QFile kdeglobals(configDir.path() + "/kdeglobals");
	if (!kdeglobals.open(QIODevice::WriteOnly | QIODevice::Text))
		return 1;
	kdeglobals.write("[Colors:View]\nDecorationHover=255,0,0\nDecorationFocus=0,0,255\n");
	kdeglobals.close();

	QApplication app(argc, argv);
	TestThemedSvgIconEngine test;
	return QTest::qExec(&test, argc, argv);
}

#include "tst_ThemedSvgIconEngine.moc"
