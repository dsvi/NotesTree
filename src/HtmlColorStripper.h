#ifndef HTMLCOLORSTRIPPER_H
#define HTMLCOLORSTRIPPER_H

#include <QString>

/// Removes every colour information from \p html, so the text is painted with
/// the colours of the widget's palette instead of the colours baked into the
/// html. That is what makes content pasted from other programs (browsers, pdf
/// viewers, other editors) readable when the app runs under a dark theme:
/// those programs bake their own light-theme colours into the html, and those
/// colours are unreadable on a dark background.
///
/// What is removed:
///  - colour declarations inside style="..." attributes: color, background,
///    background-color, background-image, border-*-color, outline-color,
///    box-shadow, text-shadow, fill, stroke, caret-color, ...
///  - the colour part of shorthands that carry more than colour, so the rest
///    of the formatting survives:
///    `border: 1px solid #dddddd` -> `border: 1px solid`
///  - the legacy html colour attributes: color, bgcolor, bordercolor and, on
///    <body>, text/link/vlink/alink. So <font color="red"> and
///    <td bgcolor="#f9f9f9"> lose their colour too.
///  - colour declarations inside <style> blocks; rules and whole blocks left
///    without declarations are removed.
///
/// Everything else - the structure, links, images, fonts, sizes, margins,
/// paddings, border widths and styles - is kept as is.
QString stripHtmlColors(QString html);

/// Whether \p html carries any colour information at all.
inline bool htmlHasColors(const QString &html)
{
	return stripHtmlColors(html) != html;
}

#endif // HTMLCOLORSTRIPPER_H
