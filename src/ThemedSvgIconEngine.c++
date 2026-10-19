#include "ThemedSvgIconEngine.h"

#include "KdeLookStyle.h"

#include <QApplication>
#include <QDomDocument>
#include <QPainter>
#include <QSvgRenderer>

namespace {

/// The colours an icon is drawn with, plus a counter that changes when they do.
///
/// Both come from the desktop's own configuration, and a colour scheme switch
/// rewrites both at once: QKdeTheme reads kdeglobals to rebuild the application
/// palette, and the accent is another key of the same file. So comparing the
/// palette is enough to notice the switch, and the accent - the only one of the
/// two that costs a file read - is re-read at that moment rather than on every
/// call.
struct Theme
{
	QPalette palette;
	QColor   accent;	///< Breeze' DecorationHover, the hovered glyph colour
	quint64  generation = 0;
};

const Theme &theme()
{
	static Theme t;
	// QPalette() is built from the application palette, so the first call would
	// otherwise look like a cache hit and leave accent_ unset.
	static bool  first = true;

	const QPalette &now = QApplication::palette();
	if (first || now != t.palette) {
		t.palette = now;
		t.accent  = KdeLookStyle::hoverColor(now);
		first = false;
		++t.generation;
	}
	return t;
}

/// The colour of the glyph in this icon mode. QCommonStyle::CE_ToolButtonLabel
/// asks for Active while the cursor is over an auto-raise button and for
/// Disabled while the action is off; Breeze answers the first with the accent
/// colour and the second with the palette's disabled text, and State_Enabled is
/// tested before State_MouseOver, so the accent never leaks into a disabled
/// button.
QColor glyphColor(const Theme &t, QIcon::Mode mode)
{
	switch (mode) {
	case QIcon::Disabled: {
		const QColor disabled = t.palette.color(QPalette::Disabled, QPalette::Text);
		if (disabled.isValid())
			return disabled;
		break;
	}
	case QIcon::Active:
		if (t.accent.isValid())
			return t.accent;
		break;
	default:
		break;
	}
	return t.palette.text().color();
}

/// The SVG with every glyph element recoloured. The glyphs are a single solid
/// colour and their background rect is explicitly fill="none", so touching only
/// path and polygon recolours exactly, antialiasing and all.
QByteArray tintedSvg(const QByteArray &svg, const QColor &color)
{
	QDomDocument doc;
	if (!doc.setContent(svg))
		return svg;

	const QString fill = color.name(QColor::HexRgb);
	for (auto tag : {QStringLiteral("path"), QStringLiteral("polygon")}) {
		auto list = doc.elementsByTagName(tag);
		for (int i = 0; i < list.count(); i++)
			list.at(i).toElement().setAttribute(QStringLiteral("fill"), fill);
	}
	return doc.toByteArray();
}

/// One rendered pixmap per (size, mode, state). The sizes an icon is asked for
/// are screen sizes, so 24 bits each leaves plenty of room for the two small
/// enums below them.
quint64 pixmapKey(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
	return	(quint64(quint32(size.width()))  << 40)
	     |	(quint64(quint32(size.height())) << 16)
	     |	(quint64(mode)  << 4)
	     |	(quint64(state) << 0);
}

} // namespace

ThemedSvgIconEngine::ThemedSvgIconEngine(QByteArray svg, QString name, qreal devicePixelRatio)
	: cacheGeneration_(~quint64(0)),
	  svg_(std::move(svg)),
	  name_(std::move(name)),
	  devicePixelRatio_(devicePixelRatio > 0 ? devicePixelRatio : 1.0)
{
}

QString ThemedSvgIconEngine::key() const
{
	// QIcon keeps what pixmap() returns in a cache of its own, keyed by this
	// string together with the size and the mode. The key therefore has to
	// change whenever the colours change, or the cache goes on handing out the
	// pixmaps of the theme that was active when the icon was first drawn.
	return name_ + QLatin1Char('#') + QString::number(theme().generation);
}

QSize ThemedSvgIconEngine::actualSize(const QSize &size, QIcon::Mode, QIcon::State)
{
	// an SVG has no size of its own: it is whatever size it is asked for
	return size.isValid() ? size : QSize(-1, -1);
}

QPixmap ThemedSvgIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
	if (!size.isValid() || size.isEmpty())
		return QPixmap();

	const Theme &t = theme();
	if (t.generation != cacheGeneration_) {
		cache_.clear();
		cacheGeneration_ = t.generation;
	}

	const quint64 key = pixmapKey(size, mode, state);
	auto it = cache_.constFind(key);
	if (it != cache_.constEnd())
		return it.value();

	QSvgRenderer renderer(tintedSvg(svg_, glyphColor(t, mode)));
	if (!renderer.isValid())
		return QPixmap();

	// The requested size is in device independent pixels, so the surface is
	// made bigger by the screen's ratio and says so. Rendering into the logical
	// rect rather than the whole viewport is what makes the painter honour that.
	QPixmap pm(int(size.width()  * devicePixelRatio_ + 0.5),
		   int(size.height() * devicePixelRatio_ + 0.5));
	pm.setDevicePixelRatio(devicePixelRatio_);
	pm.fill(Qt::transparent);

	QPainter painter(&pm);
	renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(size)));
	painter.end();

	cache_.insert(key, pm);
	return pm;
}

void ThemedSvgIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
	painter->drawPixmap(rect, pixmap(rect.size(), mode, state));
}

QIconEngine *ThemedSvgIconEngine::clone() const
{
	// QIcon clones the engine when the icon is copied, so every copy gets its
	// own cache; the one built so far comes along, which is the point.
	return new ThemedSvgIconEngine(*this);
}
