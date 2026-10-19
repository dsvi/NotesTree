#ifndef THEMED_SVG_ICON_ENGINE_H
#define THEMED_SVG_ICON_ENGINE_H

#include <QByteArray>
#include <QHash>
#include <QIcon>
#include <QIconEngine>
#include <QSize>
#include <QString>

class QPainter;

/**
 * The engine behind App::themedSVGIcon(): a flat SVG glyph painted in the
 * colours of the palette that is current when it is drawn.
 *
 * The toolbar glyphs ship as single colour SVGs and the app recolours them to
 * match the desktop. Recolouring them once, when the icon is made, freezes that
 * moment's colours into a pixmap: when the colour scheme changes while the app
 * is running, Qt's own KDE theme rebuilds the application palette from
 * kdeglobals, every widget repaints with the new one, and only the icons go on
 * showing the colours of the theme that happened to be active at startup.
 *
 * Resolving the colours in pixmap() instead makes them follow the theme by
 * themselves, with nothing to remember and nothing to update: an icon handed
 * to several widgets, or built once and set on an action later, is recoloured
 * just the same.
 *
 * Rendering an SVG is not free, so the pixmaps are cached here - and QIcon
 * caches them again on top of that, keyed by QIconEngine::key(), which is why
 * key() carries the palette generation. Without it the outer cache would go on
 * serving the previous theme's pixmaps for the whole life of the icon.
 */
class ThemedSvgIconEngine : public QIconEngine
{
public:
	/// \p svg the SVG source, already loaded.
	/// \p name identifies this icon inside QIconEngine::key().
	/// \p devicePixelRatio is the ratio of the screen the icon was made for:
	/// the glyph is rendered at the physical resolution and the pixmap says
	/// so, which is what keeps it sharp on a HiDPI screen.
	ThemedSvgIconEngine(QByteArray svg, QString name, qreal devicePixelRatio);

	QString key() const override;
	QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
	void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
	QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
	QIconEngine *clone() const override;

private:
	/// one rendered pixmap per size and mode, valid for cacheGeneration_ only
	QHash<quint64, QPixmap> cache_;
	quint64                 cacheGeneration_;

	QByteArray svg_;
	QString    name_;
	qreal      devicePixelRatio_;
};


#endif // THEMED_SVG_ICON_ENGINE_H
