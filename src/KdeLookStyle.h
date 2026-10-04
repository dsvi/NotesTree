#ifndef KDE_LOOK_STYLE_H
#define KDE_LOOK_STYLE_H

#include <QColor>
#include <QPalette>
#include <QProxyStyle>

class QWidget;
class QStyleOption;
class QStyleOptionViewItem;

/**
 * Adds the parts of the Breeze look that a fully static build cannot get from
 * plugins: tree branch lines, chevron expanders, Breeze item spacing, row hover
 * highlighting and the accent-coloured focus frame.
 *
 * The static binary cannot dlopen() any Qt plugin, so neither breeze6.so nor the
 * plasma-integration platform theme is available and Qt falls back to Fusion.
 * The colour palette itself still comes from kdeglobals, because Qt's own
 * QKdeTheme is compiled into QtGui when Qt is built with D-Bus support.
 *
 * Everything drawn here is a direct port of the corresponding Breeze code
 * (kstyle/breezestyle.cpp, breezehelper.cpp, breezemetrics.h); the accent colour
 * is read from kdeglobals the same way KColorScheme would resolve it.
 *
 * installIfApplicable() is a no-op when a KDE style is already in use, so the
 * dynamic build keeps the real Breeze rendering untouched.
 */
class KdeLookStyle : public QProxyStyle
{
public:
	explicit KdeLookStyle(const QString &baseStyle = QStringLiteral("Fusion"));

	/// Installs the style unless the current style already comes from KDE.
	static void installIfApplicable();

	/// The colour Breeze uses to highlight whatever the cursor is over: Breeze's
	/// Helper::hoverColor() is KColorScheme's HoverColor decoration, i.e.
	/// [Colors:View] DecorationHover in kdeglobals, falling back to
	/// DecorationFocus and then to the palette Highlight. In the standard Breeze
	/// schemes both decoration colours are the accent colour.
	///
	/// Public because the app bakes its own toolbar icons, so it has to know the
	/// accent colour in order to give them a hovered (QIcon::Active) variant.
	static QColor hoverColor(const QPalette &palette);

	/// Gives item views (and their viewports) WA_Hover, like Breeze does, so
	/// that State_MouseOver reaches the item delegate at all.
	void polish(QWidget *widget) override;
	void unpolish(QWidget *widget) override;

	/// Breeze's item-view padding, which is what makes its rows taller.
	QSize sizeFromContents(ContentsType type, const QStyleOption *option,
			const QSize &size, const QWidget *widget) const override;

	/// Breeze's frame widths: item views and other scroll areas get 0 unless
	/// they sit in a spaced layout (breezestyle.cpp PM_DefaultFrameWidth).
	int pixelMetric(PixelMetric metric, const QStyleOption *option,
			const QWidget *widget) const override;

	/// Breeze's text/decoration placement inside item-view cells
	/// (breezestyle.cpp SE_ItemViewItemDecoration / SE_ItemViewItemText).
	/// Breeze does not paint the full-row selection backdrop that Fusion does:
	/// the selection frame comes from the item panel, inset by the item margins.
	int styleHint(StyleHint hint, const QStyleOption *option,
			const QWidget *widget, QStyleHintReturn *returnData) const override;

	QRect subElementRect(SubElement element, const QStyleOption *option,
			const QWidget *widget) const override;

	void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
			QPainter *painter, const QWidget *widget = nullptr) const override;

private:
	/// Re-reads [Colors:View] DecorationFocus/DecorationHover from kdeglobals
	/// whenever the palette changes.
	void refresh(const QPalette &palette) const;

	mutable QPalette cachedPalette_;
	mutable bool paletteCached_ = false;

	mutable QColor accent_;   ///< Breeze' DecorationFocus, the accent colour.
	mutable QColor hover_;    ///< Breeze' DecorationHover, the hover colour.

};


#endif // KDE_LOOK_STYLE_H
