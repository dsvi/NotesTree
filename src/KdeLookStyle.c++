#include "KdeLookStyle.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QDockWidget>
#include <QGridLayout>
#include <QGroupBox>
#include <QLayout>
#include <QMenu>
#include <QMainWindow>
#include <QScrollArea>
#include <QStackedWidget>
#include <QToolBox>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QHeaderView>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSettings>
#include <QSlider>
#include <QStandardPaths>
#include <QStyleOptionFocusRect>
#include <QStyleOptionViewItem>
#include <QTableView>
#include <QTabBar>
#include <QTextEdit>
#include <QToolButton>
#include <QTreeView>

namespace {

// ---------------------------------------------------------------------------
// Metrics copied from Breeze (kstyle/breezemetrics.h). Only the ones used here.
// ---------------------------------------------------------------------------
constexpr int    ArrowSize       = 10;      // ItemView_ArrowSize
constexpr int    FrameRadius     = 5;       // Frame_FrameRadius
constexpr int    FocusLighten    = 110;     // Focus_LightenColorValue
constexpr int    SelHoverLighten = 110;     // ItemView_Hover_LightenSelectedValue
constexpr qreal  PenFrame        = 1.001;   // PenWidth::Frame
constexpr qreal  PenSymbol       = 1.001;   // PenWidth::Symbol
constexpr int    ItemPadW        = 2;       // ItemView_ItemPaddingWidth
constexpr int    ItemPadH        = 2;       // ItemView_ItemPaddingHeight
constexpr int    ItemMarginL     = 2;       // ItemView_ItemMarginLeft
constexpr int    ItemMarginT     = 1;       // ItemView_ItemMarginTop
constexpr int    ItemMarginR     = 2;       // ItemView_ItemMarginRight
constexpr int    ItemMarginB     = 1;       // ItemView_ItemMarginBottom
constexpr int    FirstItemTop    = 2;       // ItemView_FirstItemTopMarginHeight
constexpr qreal  BranchShade     = 0.25;    // branch line = mix(Base, Text, .25)
constexpr qreal  ArrowShade      = 0.15;    // ItemView_ArrowShade
constexpr qreal  HoverFillAlpha  = 0.3;     // ItemView_Blend_Value

enum Side : int
{
	SideTop    = 1,
	SideBottom = 2,
	SideLeft   = 4,
	SideRight  = 8,
	AllSides   = 15,
	NoSides    = 0,
};

/// KColorUtils::mix()
QColor mix(const QColor &a, const QColor &b, qreal f)
{
	return QColor::fromRgbF(a.redF() * (1.0 - f) + b.redF() * f,
			a.greenF() * (1.0 - f) + b.greenF() * f,
			a.blueF() * (1.0 - f) + b.blueF() * f,
			a.alphaF() * (1.0 - f) + b.alphaF() * f);
}

/// breezeutils.h centerRect(). This is offset-based rather than QRect::center()
/// based; for an even-sized box the two differ by a pixel, and Breeze positions
/// its expanders with this one.
QRect centerRect(const QRect &r, int w, int h)
{
	return QRect(r.x() + (r.width() - w) / 2, r.y() + (r.height() - h) / 2, w, h);
}

/// kdeglobals holds "r,g,b[,a]" triples. QSettings cannot be used here: it
/// percent-escapes the ':' in group names such as [Colors:View] when looking a
/// key up, so every value() call comes back empty even though allKeys() lists
/// the key. Parse the small INI file by hand instead.
QColor readKdeColor(const QString &group, const QString &key)
{
	const QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
		.filePath(QStringLiteral("kdeglobals"));
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return QColor();

	QString section;
	const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
	for (const QString &raw : lines) {
		const QString line = raw.trimmed();
		if (line.isEmpty() || line.startsWith(QLatin1Char('#')) || line.startsWith(QLatin1Char(';')))
			continue;
		if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
			section = line.mid(1, line.length() - 2);
			continue;
		}
		if (section != group)
			continue;
		const int eq = line.indexOf(QLatin1Char('='));
		if (eq < 0 || line.left(eq).trimmed() != key)
			continue;
		const QStringList parts = line.mid(eq + 1).split(QLatin1Char(','));
		if (parts.size() < 3)
			return QColor();
		const int r = parts.at(0).trimmed().toInt();
		const int g = parts.at(1).trimmed().toInt();
		const int b = parts.at(2).trimmed().toInt();
		return parts.size() > 3
			? QColor(r, g, b, parts.at(3).trimmed().toInt())
			: QColor(r, g, b);
	}
	return QColor();
}

/// Helper::itemViewItemMargins()
QMargins itemViewMargins(const QStyleOptionViewItem *opt)
{
	QMargins margins(ItemMarginL, ItemMarginT, ItemMarginR, ItemMarginB);
	if (!opt)
		return margins;

	const QWidget *w = opt->widget;
	const auto *frame = qobject_cast<const QFrame *>(w);
	const bool hasFrame = frame && frame->frameShape() == QFrame::StyledPanel;
	const bool isFirst = opt->index.row() == 0;

	if (isFirst)
		margins.setTop(FirstItemTop);
	if (hasFrame)
		margins -= QMargins(1, isFirst ? 1 : 0, 1, 0);

	const auto *view = qobject_cast<const QAbstractItemView *>(w);
	if (view && view->selectionBehavior() != QAbstractItemView::SelectRows)
		return margins;

	using Pos = QStyleOptionViewItem::ViewItemPosition;
	const bool reverse = opt->direction == Qt::RightToLeft;
	if ((reverse && opt->viewItemPosition == Pos::End)
			|| (!reverse && opt->viewItemPosition == Pos::Beginning)
			|| opt->viewItemPosition == Pos::Middle)
		margins.setRight(0);
	if ((reverse && opt->viewItemPosition == Pos::Beginning)
			|| (!reverse && opt->viewItemPosition == Pos::End)
			|| opt->viewItemPosition == Pos::Middle)
		margins.setLeft(0);

	return margins;
}

/// Helper::renderFocusRect() - rounded fill plus optional rounded outline.
void renderFocusRect(QPainter *p, const QRectF &rect, const QBrush &brush,
		const QColor &outline, int sides)
{
	p->save();
	p->setRenderHint(QPainter::Antialiasing);
	p->setBrush(brush);

	if (!(outline.isValid() && sides)) {
		p->setPen(Qt::NoPen);
		p->drawRect(rect);
	} else {
		p->setClipRect(rect);
		QRectF copy = rect.adjusted(0.5 * PenFrame, 0.5 * PenFrame,
				-0.5 * PenFrame, -0.5 * PenFrame);
		const qreal radius = qMax(qreal(FrameRadius) - 0.5 * PenFrame, 0.0);
		if (!(sides & SideTop))    copy.adjust(0, -radius, 0, 0);
		if (!(sides & SideBottom)) copy.adjust(0, 0, 0, radius);
		if (!(sides & SideLeft))   copy.adjust(-radius, 0, 0, 0);
		if (!(sides & SideRight))  copy.adjust(0, 0, radius, 0);
		p->setPen(outline);
		p->drawRoundedRect(copy, radius, radius);
	}

	p->restore();
}

/// Helper::renderViewItemPosition()
void renderViewItemPosition(QPainter *p, QStyleOptionViewItem::ViewItemPosition position,
		Qt::LayoutDirection direction, const QRect &rect, const QColor &color,
		const QColor &outline)
{
	if (rect.width() <= 1 || rect.height() <= 1)
		return;

	using Pos = QStyleOptionViewItem::ViewItemPosition;
	switch (position) {
	case Pos::OnlyOne:
		renderFocusRect(p, rect, color, outline, AllSides);
		break;
	case Pos::Beginning:
	case Pos::Middle:
	case Pos::End: {
		p->save();
		p->setClipRect(rect, Qt::IntersectClip);
		const int radius = FrameRadius;
		const int side = radius + 1;
		if (position == Pos::Beginning) {
			if (direction == Qt::RightToLeft)
				renderFocusRect(p, rect.adjusted(-1, 0, -side, 0), color, outline, SideLeft);
			else
				renderFocusRect(p, rect.adjusted(side, 0, 1, 0), color, outline, SideRight);
		} else if (position == Pos::Middle) {
			renderFocusRect(p, rect.adjusted(-1, 0, 1, 0), color, outline, NoSides);
		} else {
			if (direction == Qt::RightToLeft)
				renderFocusRect(p, rect.adjusted(-side, 0, 1, 0), color, outline, SideRight);
			else
				renderFocusRect(p, rect.adjusted(-1, 0, -side, 0), color, outline, SideLeft);
		}
		p->restore();
		break;
	}
	default:
		break;
	}
}

/// Helper::renderArrow() - Breeze draws expanders as a stroked chevron, not a
/// filled triangle, and the chevron spans the whole ArrowSize box.
void renderArrow(QPainter *p, const QRect &rect, const QColor &color, Qt::ArrowType type)
{
	const qreal size = qMin(qMin(qreal(rect.width()), qreal(rect.height())), qreal(ArrowSize));
	const qreal penOffset = PenSymbol / 2.0;
	const qreal center = size / 2.0;
	const qreal maxExtent = size * 0.75;
	const qreal minExtent = size / 4.0;

	qreal sizeOffset = 0;
	switch (int(size) % 4) {
	case 2:  sizeOffset = 0.5;  break;
	case 1:  sizeOffset = -0.25; break;
	case 3:  sizeOffset = 0.25; break;
	default: break;
	}

	QPolygonF points;
	switch (type) {
	case Qt::UpArrow:
		points << QPointF(penOffset, maxExtent + sizeOffset)
			<< QPointF(center, minExtent + penOffset + sizeOffset)
			<< QPointF(size - penOffset, maxExtent + sizeOffset);
		break;
	case Qt::DownArrow:
		points << QPointF(penOffset, minExtent + penOffset + sizeOffset)
			<< QPointF(center, maxExtent + sizeOffset)
			<< QPointF(size - penOffset, minExtent + penOffset + sizeOffset);
		break;
	case Qt::LeftArrow:
		points << QPointF(maxExtent - penOffset - sizeOffset, penOffset)
			<< QPointF(minExtent - sizeOffset, center)
			<< QPointF(maxExtent - penOffset - sizeOffset, size - penOffset);
		break;
	case Qt::RightArrow:
		points << QPointF(minExtent + penOffset + sizeOffset, penOffset)
			<< QPointF(maxExtent + sizeOffset, center)
			<< QPointF(minExtent + penOffset + sizeOffset, size - penOffset);
		break;
	default:
		return;
	}

	const QRect arrowRect = centerRect(rect, int(size), int(size));
	points.translate(arrowRect.x() + (arrowRect.width() - size) / 2.0,
			arrowRect.y() + (arrowRect.height() - size) / 2.0);

	p->save();
	p->setRenderHint(QPainter::Antialiasing);
	p->setBrush(Qt::NoBrush);
	p->setPen(QPen(color, PenSymbol, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
	p->drawPolyline(points);
	p->restore();
}

} // namespace

KdeLookStyle::KdeLookStyle(const QString &baseStyle)
	: QProxyStyle(baseStyle)
{
}

void KdeLookStyle::installIfApplicable()
{
	QStyle *current = QApplication::style();
	if (!current)
		return;

	// A real KDE style is already in use: Breeze (or Oxygen) sits on top of
	// KStyle. Adding our own drawing on top of that would only double-draw.
	for (const QMetaObject *m = current->metaObject(); m; m = m->superClass()) {
		const QString name = QString::fromLatin1(m->className());
		if (name.contains(QLatin1String("Breeze")) || name.contains(QLatin1String("Oxygen"))
				|| name.contains(QLatin1String("KStyle")))
			return;
	}

	QApplication::setStyle(new KdeLookStyle);
}

void KdeLookStyle::refresh(const QPalette &palette) const
{
	// QPalette() is initialised from the application palette, so comparing the
	// cached palette alone would treat the very first call as a cache hit and
	// leave accent_/hover_ unset. Track whether we have computed once.
	if (paletteCached_ && cachedPalette_ == palette)
		return;

	cachedPalette_ = palette;
	accent_ = readKdeColor(QStringLiteral("Colors:View"), QStringLiteral("DecorationFocus"));
	hover_  = readKdeColor(QStringLiteral("Colors:View"), QStringLiteral("DecorationHover"));
	// Breeze falls back to the palette when the colour scheme carries no
	// decoration colour; an invalid QColor would paint the expander black.
	if (!accent_.isValid())
		accent_ = palette.color(QPalette::Highlight);
	if (!hover_.isValid())
		hover_ = accent_;
	paletteCached_ = true;
}

// Same resolution as refresh() does for hover_, exposed for callers outside
// the style (see the header). Breeze's Helper::hoverColor() is the
// KColorScheme HoverColor decoration, i.e. DecorationHover in kdeglobals.
QColor KdeLookStyle::hoverColor(const QPalette &palette)
{
	QColor color = readKdeColor(QStringLiteral("Colors:View"), QStringLiteral("DecorationHover"));
	if (!color.isValid())
		color = readKdeColor(QStringLiteral("Colors:View"), QStringLiteral("DecorationFocus"));
	if (!color.isValid())
		color = palette.color(QPalette::Highlight);
	return color;
}

// ---------------------------------------------------------------------------
// Hover
//
// QAbstractItemView only tracks the hovered index when it receives
// HoverEnter/HoverMove/HoverLeave events, and those are only generated for a
// widget that has WA_Hover. Fusion never sets it, so State_MouseOver never
// reaches the delegate and nothing highlights on hover. Breeze sets it in
// polish(); this mirrors that (kstyle/breezestyle.cpp Style::polish).
// ---------------------------------------------------------------------------
void KdeLookStyle::polish(QWidget *widget)
{
	QProxyStyle::polish(widget);
	if (!widget)
		return;

	if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
		widget->setAttribute(Qt::WA_Hover, true);
		if (QWidget *vp = view->viewport())
			vp->setAttribute(Qt::WA_Hover, true);
		return;
	}

	// Breeze::polishScrollArea(): sunken, focusable scroll areas react to hover.
	if (auto *area = qobject_cast<QAbstractScrollArea *>(widget)) {
		if (auto *frame = qobject_cast<QFrame *>(widget)) {
			if (frame->frameShape() != QFrame::NoFrame
					&& frame->frameShadow() == QFrame::Sunken
					&& (widget->focusPolicy() & Qt::StrongFocus) == Qt::StrongFocus)
				widget->setAttribute(Qt::WA_Hover, true);
		}
		return;
	}

	if (qobject_cast<QLineEdit *>(widget) || qobject_cast<QTextEdit *>(widget)
			|| qobject_cast<QPlainTextEdit *>(widget) || qobject_cast<QComboBox *>(widget)
			|| qobject_cast<QAbstractSpinBox *>(widget) || qobject_cast<QSlider *>(widget)
			|| qobject_cast<QScrollBar *>(widget) || qobject_cast<QTabBar *>(widget)
			|| qobject_cast<QToolButton *>(widget) || qobject_cast<QPushButton *>(widget)
			|| qobject_cast<QRadioButton *>(widget) || qobject_cast<QCheckBox *>(widget))
		widget->setAttribute(Qt::WA_Hover, true);
}

void KdeLookStyle::unpolish(QWidget *widget)
{
	QProxyStyle::unpolish(widget);
	if (!widget)
		return;

	if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
		widget->setAttribute(Qt::WA_Hover, false);
		if (QWidget *vp = view->viewport())
			vp->setAttribute(Qt::WA_Hover, false);
		return;
	}

	if (qobject_cast<QAbstractScrollArea *>(widget) || qobject_cast<QLineEdit *>(widget)
			|| qobject_cast<QTextEdit *>(widget) || qobject_cast<QPlainTextEdit *>(widget)
			|| qobject_cast<QComboBox *>(widget) || qobject_cast<QAbstractSpinBox *>(widget)
			|| qobject_cast<QSlider *>(widget) || qobject_cast<QScrollBar *>(widget)
			|| qobject_cast<QTabBar *>(widget) || qobject_cast<QToolButton *>(widget)
			|| qobject_cast<QPushButton *>(widget) || qobject_cast<QRadioButton *>(widget)
			|| qobject_cast<QCheckBox *>(widget))
		widget->setAttribute(Qt::WA_Hover, false);
}

// ---------------------------------------------------------------------------
// Item spacing
//
// Breeze grows every item-view cell by its own margins plus ItemPadding, which
// is what makes KDE rows noticeably taller than Fusion's (23 px vs 17 px for
// the default font). kstyle/breezestyle.cpp Style::sizeFromContents.
// ---------------------------------------------------------------------------
QSize KdeLookStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
		const QSize &size, const QWidget *widget) const
{
	QSize result = QProxyStyle::sizeFromContents(type, option, size, widget);

	if (type == CT_ItemViewItem && widget && !qobject_cast<const QTableView *>(widget)) {
		const auto *opt = qstyleoption_cast<const QStyleOptionViewItem *>(option);
		if (opt) {
			const QMargins m = itemViewMargins(opt);
			result += QSize(m.left() + m.right() + 2 * ItemPadW,
					m.top() + m.bottom() + 2 * ItemPadH);
		}
	}

	return result;
}

// Port of Breeze's PM_DefaultFrameWidth (breezestyle.cpp:655-740).
// Scroll areas (item views, edits, …) are frameless unless they sit inside a
// layout that visually separates them; buttons and QFrames get 0.
int KdeLookStyle::pixelMetric(PixelMetric metric, const QStyleOption *option,
			const QWidget *widget) const
{
	if (metric != PM_DefaultFrameWidth)
		return QProxyStyle::pixelMetric(metric, option, widget);
	if (!widget)
		return QProxyStyle::pixelMetric(metric, option, widget);

	constexpr int FrameWidth = 2;	// Metrics::Frame_FrameWidth
	constexpr int EditWidth = 6;	// Metrics::{LineEdit,ComboBox,SpinBox,GroupBox}_FrameWidth

	if (qobject_cast<const QMenu *>(widget))
		return 0;
	if (qobject_cast<const QLineEdit *>(widget))
		return EditWidth;
	if (qobject_cast<const QAbstractButton *>(widget)
			&& !qobject_cast<const QGroupBox *>(widget)
			&& !qobject_cast<const QDockWidget *>(widget))
		return 0;

	if (qobject_cast<const QAbstractScrollArea *>(widget)) {
		if (qobject_cast<const QAbstractSpinBox *>(widget))
			return EditWidth;
		if (qobject_cast<const QComboBox *>(widget))
			return EditWidth;
		if (qobject_cast<const QGroupBox *>(widget))
			return EditWidth;
		if (qobject_cast<const QDockWidget *>(widget))
			return 0;
		if (qobject_cast<const QToolBox *>(widget))
			return 0;
		if (qobject_cast<const QScrollArea *>(widget))
			return 0;

		// walk up to the first ancestor that owns a layout
		const QLayout *layout = nullptr;
		for (const QWidget *w = widget->parentWidget(); w && !layout; w = w->parentWidget())
			layout = w->layout();
		if (!layout)
			return 0;

		const QWidget *parent = widget->parentWidget();
		if (qobject_cast<const QDockWidget *>(parent)
				|| qobject_cast<const QMainWindow *>(parent)
				|| qobject_cast<const QStackedWidget *>(parent))
			return 0;

		if (const QGridLayout *grid = qobject_cast<const QGridLayout *>(layout)) {
			if (grid->itemAtPosition(0, 1) || grid->itemAtPosition(1, 0))
				return FrameWidth;
		}
		if (layout->spacing() > 0 && layout->count() > 1)
			return FrameWidth;
		return 0;
	}

	if (qobject_cast<const QFrame *>(widget))
		return 0;

	return QProxyStyle::pixelMetric(metric, option, widget);
}

// Port of Breeze's SE_ItemViewItemDecoration / SE_ItemViewItemText
// (breezestyle.cpp:1025-1112): shift the cell's text and icon by the item
// margins + padding so content lines up with the hover/selection frame.
// Breeze leaves SH_ItemView_ShowDecorationSelected at the QCommonStyle default
// (false); Fusion turns it on, which would paint a full-row Highlight backdrop
// over the branch column. Keep Breeze's behaviour.
int KdeLookStyle::styleHint(StyleHint hint, const QStyleOption *option,
			const QWidget *widget, QStyleHintReturn *returnData) const
{
	if (hint == SH_ItemView_ShowDecorationSelected)
		return false;
	return QProxyStyle::styleHint(hint, option, widget, returnData);
}

QRect KdeLookStyle::subElementRect(SubElement element, const QStyleOption *option,
			const QWidget *widget) const
{
	if (element != SE_ItemViewItemDecoration && element != SE_ItemViewItemText)
		return QProxyStyle::subElementRect(element, option, widget);

	const auto *viewItem = qstyleoption_cast<const QStyleOptionViewItem *>(option);
	if (!viewItem)
		return QProxyStyle::subElementRect(element, option, widget);
	if (element == SE_ItemViewItemText && viewItem->text.isEmpty())
		return QProxyStyle::subElementRect(element, option, widget);

	const QMargins margins = itemViewMargins(viewItem);
	QRect rect = QProxyStyle::subElementRect(element, option, widget);

	if (element == SE_ItemViewItemDecoration) {
		if (viewItem->text.isEmpty()) {
			const int marginAdjust = (viewItem->direction == Qt::LeftToRight)
				? margins.right() : margins.left();
			rect.moveRight(rect.right() + marginAdjust);
		} else {
			const int marginAdjust = (viewItem->direction == Qt::LeftToRight)
				? margins.left() : margins.right();
			const int adjustment = rect.left() + margins.left() + ItemPadW - marginAdjust;
			if (viewItem->rect.width() > adjustment)
				rect.moveLeft(adjustment);
		}
		rect.moveTop(rect.top() + margins.top() - margins.bottom());
		return rect;
	}

	// SE_ItemViewItemText
	const int leftInset = margins.left() + ItemPadW;
	const int rightInset = margins.right() + ItemPadW;
	const int textMargin = pixelMetric(PM_FocusFrameHMargin, option, widget) + 1;
	const int available = rect.width() - 2 * textMargin;
	const int required = viewItem->fontMetrics.horizontalAdvance(viewItem->text);
	const int inset = qBound(0, available - required, leftInset + rightInset);
	const int total = leftInset + rightInset;
	const int right = total > 0 ? inset * rightInset / total : 0;
	rect.setRight(rect.right() - right);
	rect.setLeft(rect.left() + (inset - right));
	rect.moveTop(rect.top() + margins.top() - margins.bottom());
	return rect;
}

void KdeLookStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
		QPainter *painter, const QWidget *widget) const
{
	switch (element) {

	// -----------------------------------------------------------------------
	// Branch lines + chevron. Port of Style::drawIndicatorBranchPrimitive().
	// -----------------------------------------------------------------------
	case PE_IndicatorBranch: {
		refresh(option->palette);

		const QPalette::ColorGroup group = option->palette.currentColorGroup();
		const QColor lineColor = mix(option->palette.color(QPalette::Base),
				option->palette.color(QPalette::Text), BranchShade);
		const QColor arrowColor = (option->state & QStyle::State_MouseOver)
			? hover_
			: mix(option->palette.color(group, QPalette::Text),
				option->palette.color(group, QPalette::Base), ArrowShade);

		const QRect rect = option->rect;
		const QPoint center = rect.center();

		// Breeze keeps expanderAdjust at 0 unless this cell actually holds an
		// expander, so the vertical line stays unbroken across rows that have
		// no children. Leaving it at half the arrow size everywhere chops the
		// line up and makes the whole tree look sparse and dotted.
		int expanderAdjust = 0;
		if (option->state & State_Children)
			expanderAdjust = qMin(qMin(rect.width(), rect.height()), ArrowSize) / 2 + 1;

		painter->save();
		painter->setPen(QPen(lineColor, 1));

		if (option->state & (State_Item | State_Children | State_Sibling))
			painter->drawLine(QPoint(center.x(), rect.top()),
				QPoint(center.x(), center.y() - expanderAdjust - 1));

		if (option->state & State_Sibling)
			painter->drawLine(QPoint(center.x(), center.y() + expanderAdjust),
				QPoint(center.x(), rect.bottom()));

		// One connector only, on the side the children are drawn (Breeze skips
		// the other half entirely).
		if (option->state & State_Item) {
			if (option->direction == Qt::RightToLeft)
				painter->drawLine(QPoint(center.x() - expanderAdjust, center.y()),
					QPoint(rect.left(), center.y()));
			else
				painter->drawLine(QPoint(center.x() + expanderAdjust, center.y()),
					QPoint(rect.right(), center.y()));
		}

		if (option->state & State_Children) {
			const Qt::ArrowType type = (option->state & State_Open)
				? Qt::DownArrow : Qt::RightArrow;
			renderArrow(painter, centerRect(rect, ArrowSize, ArrowSize), arrowColor, type);
		}

		painter->restore();
		return;
	}

	// -----------------------------------------------------------------------
	// Row background: selection plus the hover tint Fusion does not have.
	// Port of Style::drawPanelItemViewItemPrimitive().
	// -----------------------------------------------------------------------
	case PE_PanelItemViewItem: {
		const auto *opt = qstyleoption_cast<const QStyleOptionViewItem *>(option);
		if (!opt) {
			QProxyStyle::drawPrimitive(element, option, painter, widget);
			return;
		}

		QPalette::ColorGroup group = QPalette::Normal;
		if (opt->state & State_Selected) {
			if (widget && !widget->isEnabled())
				group = QPalette::Disabled;
			else if (!(opt->state & State_Active))
				group = QPalette::Inactive;
		} else {
			group = (widget && !widget->isEnabled()) ? QPalette::Disabled : QPalette::Normal;
			if (group == QPalette::Normal && !(opt->state & State_Active))
				group = QPalette::Inactive;
		}

		QPalette palette = opt->palette;
		palette.setCurrentColorGroup(group);

		const auto *view = qobject_cast<const QAbstractItemView *>(widget);
		const bool mouseOver = (opt->state & State_MouseOver)
			&& (!view || view->selectionMode() != QAbstractItemView::NoSelection);
		const bool selected = opt->state & State_Selected;

		const bool alternate = !selected && !mouseOver
			&& (opt->features & QStyleOptionViewItem::Alternate);

		QRect rect = opt->rect;
		if (view && !qobject_cast<const QTableView *>(view))
			rect = rect.marginsRemoved(itemViewMargins(opt));

		if (alternate)
			painter->fillRect(opt->rect, palette.brush(group, QPalette::AlternateBase));

		if (opt->backgroundBrush.style() != Qt::NoBrush) {
			painter->save();
			painter->setBrushOrigin(opt->rect.topLeft());
			painter->fillRect(opt->rect, opt->backgroundBrush);
			painter->restore();
		}

		if (!selected && !mouseOver)
			return;

		QColor color = palette.color(group, QPalette::Highlight);
		if (mouseOver) {
			if (!selected)
				color.setAlphaF(HoverFillAlpha);
			else
				color = color.lighter(SelHoverLighten);
		}

		QColor outline = color;
		outline.setAlphaF(selected ? 1.0 : 0.8);

		renderViewItemPosition(painter, opt->viewItemPosition, opt->direction,
				rect, color, outline);
		return;
	}

	// -----------------------------------------------------------------------
	// Accent frame around the focused input widget (tree view, note editor).
	// Breeze animates this; we switch it instantly, which is the same colour
	// the animation settles on.
	// -----------------------------------------------------------------------
	case PE_Frame: {
		if ((option->state & (State_HasFocus | State_Sunken))
				== (State_HasFocus | State_Sunken)) {
			refresh(option->palette);
			if (accent_.isValid()) {
				painter->save();
				painter->setRenderHint(QPainter::Antialiasing);
				painter->setBrush(Qt::NoBrush);
				painter->setPen(QPen(accent_, PenFrame));
				painter->drawRoundedRect(QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5),
						FrameRadius, FrameRadius);
				painter->restore();
				return;
			}
		}
		break;
	}

	// -----------------------------------------------------------------------
	// Auto-raise tool buttons. QCommonStyle asks for PE_PanelButtonTool as soon
	// as State_MouseOver arrives on a flat button (qt_qcommonstyle.cpp:3438
	// keeps State_Raised only while hovered), and Fusion answers that with
	// PE_PanelButtonCommand, i.e. the gradient panel and its border
	// (qt_qfusionstyle.cpp:504). The accent icon recolour is the only hover
	// feedback wanted here, so draw nothing for the plain hover case. Checked
	// (State_On) and pressed (State_Sunken) buttons keep the panel: that is how
	// a toggled format such as bold reads as active.
	// Note the option is a sliced plain QStyleOption (qt_qcommonstyle.cpp:3452
	// does "QStyleOption tool = *toolbutton;"), so qstyleoption_cast to
	// QStyleOptionToolButton is always null here; only the state flags survive.
	// -----------------------------------------------------------------------
	case PE_PanelButtonTool:
		if ((option->state & State_AutoRaise)
				&& !(option->state & (State_On | State_Sunken)))
			return;
		break;

	// -----------------------------------------------------------------------
	// Accent rounded rect around the current item. Port of
	// Style::drawFrameFocusRectPrimitive(); Fusion draws a dotted rectangle.
	// QTreeView passes a plain QStyleOptionFocusRect and no widget, so the view
	// has to be recovered from styleObject.
	// -----------------------------------------------------------------------
	case PE_FrameFocusRect: {
		if (!(option->state & State_HasFocus))
			break;

		refresh(option->palette);
		if (!accent_.isValid())
			break;

		// Breeze reads QPalette::Accent, which QKdeTheme never fills in, so the
		// accent comes from kdeglobals instead.
		QColor outlineColor = accent_;
		if (const auto *focusOpt = qstyleoption_cast<const QStyleOptionFocusRect *>(option)) {
			if (focusOpt->backgroundColor.isValid())
				outlineColor = focusOpt->backgroundColor;
		}
		outlineColor = outlineColor.lighter(FocusLighten);

		const auto *view = qobject_cast<const QAbstractItemView *>(option->styleObject);
		if (view && view->selectionModel()) {
			// With a selection the accent is already carried by the selection.
			if (view->selectionModel()->hasSelection())
				return;

			QStyleOptionViewItem probe;
			probe.viewItemPosition = QStyleOptionViewItem::OnlyOne;
			const QRect rect = option->rect.marginsRemoved(itemViewMargins(&probe));
			if (rect.width() < 10)
				return;

			renderFocusRect(painter, rect, Qt::NoBrush, outlineColor, AllSides);
			return;
		}

		// Everything else keeps Fusion's focus rect.
		break;
	}

	default:
		break;
	}

	QProxyStyle::drawPrimitive(element, option, painter, widget);
}
