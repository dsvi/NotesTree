#include "HtmlColorStripper.h"

#include <QRegularExpression>
#include <QSet>
#include <QStringList>

// Everything here is deliberately string based instead of QDom based: html
// coming from the clipboard is rarely well formed xml (<br>, <img src=...>),
// which QDomDocument::setContent() refuses to parse. Working on the raw string
// also keeps the parts we don't touch byte for byte identical.
namespace {

// ---------------------------------------------------------------- css colours

/// The full list of css colour keywords (plus the two keywords that stand for
/// "the colour is inherited from somewhere else", which is exactly what we
/// want to get rid of as well).
const QSet<QString> &colorNames()
{
	static const QSet<QString> names = []{
		const char *all =
			"aliceblue antiquewhite aqua aquamarine azure beige bisque black "
			"blanchedalmond blue blueviolet brown burlywood cadetblue chartreuse "
			"chocolate coral cornflowerblue cornsilk crimson cyan darkblue "
			"darkcyan darkgoldenrod darkgray darkgreen darkgrey darkkhaki "
			"darkmagenta darkolivegreen darkorange darkorchid darkred darksalmon "
			"darkseagreen darkslateblue darkslategray darkslategrey darkturquoise "
			"darkviolet deeppink deepskyblue dimgray dimgrey dodgerblue firebrick "
			"floralwhite forestgreen fuchsia gainsboro ghostwhite gold goldenrod "
			"gray green greenyellow grey honeydew hotpink indianred indigo ivory "
			"khaki lavender lavenderblush lawngreen lemonchiffon lightblue "
			"lightcoral lightcyan lightgoldenrodyellow lightgray lightgreen "
			"lightgrey lightpink lightsalmon lightseagreen lightskyblue "
			"lightslategray lightslategrey lightsteelblue lightyellow lime "
			"limegreen linen magenta maroon mediumaquamarine mediumblue "
			"mediumorchid mediumpurple mediumseagreen mediumslateblue "
			"mediumspringgreen mediumturquoise mediumvioletred midnightblue "
			"mintcream mistyrose moccasin navajowhite navy oldlace olive olivedrab "
			"orange orangered orchid palegoldenrod palegreen paleturquoise "
			"palevioletred papayawhip peachpuff peru pink plum powderblue purple "
			"rebeccapurple red rosybrown royalblue saddlebrown salmon sandybrown "
			"seagreen seashell sienna silver skyblue slateblue slategray slategrey "
			"snow springgreen steelblue tan teal thistle tomato turquoise violet "
			"wheat white whitesmoke yellow yellowgreen "
			"transparent currentcolor";
		QSet<QString> s;
		for (const auto &n : QString::fromUtf8(all).split(u' ', Qt::SkipEmptyParts))
			s.insert(n);
		return s;
	}();
	return names;
}

bool isHexColor(QStringView t)
{
	if (t.isEmpty() || t.front() != u'#')
		return false;
	auto digits = t.size() - 1;
	if (digits != 3 && digits != 4 && digits != 6 && digits != 8)
		return false;
	for (auto c : t.mid(1))
		if (!((c >= u'0' && c <= u'9') || (c >= u'a' && c <= u'f') || (c >= u'A' && c <= u'F')))
			return false;
	return true;
}

bool isColorToken(QStringView token)
{
	if (token.isEmpty())
		return false;
	if (isHexColor(token))
		return true;
	QString l = token.toString().toLower();
	if (l.startsWith("rgb(") || l.startsWith("rgba(")
	 || l.startsWith("hsl(") || l.startsWith("hsla("))
		return true;
	return colorNames().contains(l);
}

/// Browsers paste vendor prefixed properties (-webkit-box-shadow, -ms-box-shadow,
/// -moz-...), so the prefix is ignored when a property is looked up.
QString plainPropertyName(const QString &prop)
{
	for (const QString &prefix : QStringList{ "-webkit-", "-moz-", "-ms-", "-o-", "-khtml-" })
		if (prop.startsWith(prefix))
			return prop.mid(prefix.size());
	return prop;
}

/// Properties whose value is nothing but colour: the whole declaration goes.
bool isColorProperty(const QString &prop)
{
	static const QSet<QString> props {
		"color", "background", "background-color", "background-image",
		"box-shadow", "text-shadow", "fill", "stroke", "stop-color",
		"flood-color", "lighting-color",
	};
	const QString p = plainPropertyName(prop);
	// anything that ends in "-color" carries colour: border-*-color,
	// outline-color, text-decoration-color, scrollbar-color,
	// -webkit-tap-highlight-color, ...
	return props.contains(p) || p.endsWith("-color");
}

/// Properties that mix colour with other information (a border is a width, a
/// style and a colour): only the colour tokens are dropped from the value.
bool isColorBearingProperty(const QString &prop)
{
	static const QSet<QString> props {
		"border", "border-top", "border-right", "border-bottom", "border-left",
		"border-block", "border-block-start", "border-block-end",
		"border-inline", "border-inline-start", "border-inline-end",
		"outline", "text-decoration", "text-emphasis", "column-rule",
		"text-stroke",
	};
	return props.contains(plainPropertyName(prop));
}

// ------------------------------------------------------------- css scanning

/// Index of \p c in \p s outside of quotes and parentheses, -1 if there is none.
qsizetype topLevelIndexOf(QStringView s, QChar c, qsizetype from = 0)
{
	int depth = 0;
	QChar quote;
	for (qsizetype i = from; i < s.size(); ++i){
		QChar ch = s.at(i);
		if (!quote.isNull()){
			if (ch == quote)
				quote = QChar();
			continue;
		}
		if (ch == u'"' || ch == u'\''){
			quote = ch;
			continue;
		}
		if (ch == u'(')
			++depth;
		else if (ch == u')')
			depth = qMax(0, depth - 1);
		else if (!depth && ch == c)
			return i;
	}
	return -1;
}

/// Drops the colour tokens of a shorthand value: "1px solid #ccc" -> "1px solid".
/// A value without colour comes back as it was.
QString stripColorsFromValue(QStringView value)
{
	QString out;
	bool removed = false;
	qsizetype i = 0;
	while (i < value.size()){
		if (value.at(i).isSpace() || value.at(i) == u','){
			++i;
			continue;
		}
		qsizetype start = i;
		int depth = 0;
		QChar quote;
		while (i < value.size()){
			QChar ch = value.at(i);
			if (!quote.isNull()){
				if (ch == quote)
					quote = QChar();
				++i;
				continue;
			}
			if (ch == u'"' || ch == u'\''){
				quote = ch;
				++i;
				continue;
			}
			if (ch == u'('){
				++depth;
				++i;
				continue;
			}
			if (ch == u')'){
				depth = qMax(0, depth - 1);
				++i;
				continue;
			}
			if (!depth && (ch.isSpace() || ch == u','))
				break;
			++i;
		}
		QStringView token = value.mid(start, i - start);
		if (isColorToken(token)){
			removed = true;
			continue;
		}
		if (!out.isEmpty())
			out += u' ';
		out += token;
	}
	return removed ? out : value.toString();
}

qsizetype skipSpace(QStringView s, qsizetype from)
{
	while (from < s.size() && s.at(from).isSpace())
		++from;
	return from;
}

/// True if \p s holds at least one real declaration, as opposed to left over
/// separators and whitespace.
bool hasDeclarations(QStringView s)
{
	for (auto ch : s)
		if (ch.isLetterOrNumber())
			return true;
	return false;
}

/// Keeps the declarations that don't carry colour. What is kept is copied out of
/// the input verbatim, so a list that needs no change comes back byte for byte
/// identical.
QString filterDeclarations(QStringView decls)
{
	QString out;
	qsizetype i = 0;
	while (i < decls.size()){
		qsizetype sep = topLevelIndexOf(decls, u';', i);
		qsizetype end = (sep < 0 ? decls.size() : sep);
		qsizetype next = (sep < 0 ? decls.size() : sep + 1);
		auto d = decls.mid(i, end - i).trimmed();
		if (!d.isEmpty()){
			qsizetype colon = topLevelIndexOf(d, u':');
			QString prop = colon < 0 ? QString() : d.left(colon).trimmed().toString().toLower();
			if (colon >= 0 && isColorProperty(prop)){
				// the declaration, its ';' and the whitespace after it all go
				i = skipSpace(decls, next);
				continue;
			}
			if (colon >= 0 && isColorBearingProperty(prop)){
				QStringView raw = d.mid(colon + 1).trimmed();
				QString v = stripColorsFromValue(raw);
				if (v.isEmpty()){
					i = skipSpace(decls, next);
					continue;
				}
				if (v == raw.toString())
					out += decls.mid(i, end - i); // nothing was dropped: keep verbatim
				else
					out += prop + ": " + v;
				if (sep >= 0)
					out += decls.at(sep);
				i = next;
				continue;
			}
		}
		out += decls.mid(i, end - i);
		if (sep >= 0)
			out += decls.at(sep);
		i = next;
	}
	return out;
}

/// True if the content of a block contains a nested block, i.e. the block holds
/// rules (@media, @keyframes) instead of declarations (@page, @font-face).
bool containsTopLevelBrace(QStringView s)
{
	int depth = 0;
	QChar quote;
	for (qsizetype i = 0; i < s.size(); ++i){
		QChar ch = s.at(i);
		if (!quote.isNull()){
			if (ch == quote)
				quote = QChar();
			continue;
		}
		if (ch == u'"' || ch == u'\''){
			quote = ch;
			continue;
		}
		if (ch == u'(')
			++depth;
		else if (ch == u')')
			depth = qMax(0, depth - 1);
		else if (!depth && ch == u'{')
			return true;
	}
	return false;
}

/// Index of the brace closing the one at \p open, -1 if unbalanced.
qsizetype matchingBrace(QStringView s, qsizetype open)
{
	int depth = 0;
	QChar quote;
	for (qsizetype i = open; i < s.size(); ++i){
		QChar ch = s.at(i);
		if (!quote.isNull()){
			if (ch == quote)
				quote = QChar();
			continue;
		}
		if (ch == u'"' || ch == u'\''){
			quote = ch;
			continue;
		}
		if (ch == u'(')
			++depth;
		else if (ch == u')')
			depth = qMax(0, depth - 1);
		else if (ch == u'{')
			++depth;
		else if (ch == u'}'){
			--depth;
			if (depth <= 0)
				return i;
		}
	}
	return -1;
}

/// Filters a whole style sheet: colour declarations are dropped, rules and
/// at-rules left empty disappear.
QString filterCss(QStringView css)
{
	QString out;
	auto add = [&out](const QString &s){
		if (!out.isEmpty())
			out += u' ';
		out += s;
	};
	qsizetype i = 0;
	while (i < css.size()){
		if (css.at(i).isSpace()){
			++i;
			continue;
		}
		if (css.mid(i, 2) == u"/*"){
			qsizetype e = css.indexOf(u"*/", i + 2);
			if (e < 0)
				break;
			i = e + 2;
			continue;
		}
		// find the end of the prelude: the '{' of the block, or the ';' of a
		// statement, or the '}' of the enclosing block.
		qsizetype j = i;
		int depth = 0;
		QChar quote;
		bool found = false;
		for (; j < css.size(); ++j){
			QChar ch = css.at(j);
			if (!quote.isNull()){
				if (ch == quote)
					quote = QChar();
				continue;
			}
			if (ch == u'"' || ch == u'\''){
				quote = ch;
				continue;
			}
			if (ch == u'(')
				++depth;
			else if (ch == u')')
				depth = qMax(0, depth - 1);
			else if (!depth && (ch == u'{' || ch == u';' || ch == u'}')){
				found = true;
				break;
			}
		}
		QString head = css.mid(i, (found ? j : css.size()) - i).trimmed().toString();
		if (!found){
			if (!head.isEmpty()){
				QString d = filterDeclarations(head);
				if (hasDeclarations(d))
					add(d);
			}
			break;
		}
		QChar term = css.at(j);
		if (term == u';'){
			if (!head.isEmpty()){
				if (head.startsWith(u'@'))
					add(head + ";");
				else {
					QString d = filterDeclarations(head);
					if (hasDeclarations(d))
						add(d);
				}
			}
			i = j + 1;
			continue;
		}
		if (term == u'}'){
			i = j + 1;
			continue;
		}
		qsizetype close = matchingBrace(css, j);
		if (close < 0)
			break;
		QStringView body = css.mid(j + 1, close - j - 1);
		if (containsTopLevelBrace(body)){
			QString inner = filterCss(body);
			if (!inner.trimmed().isEmpty())
				add(head + " { " + inner + " }");
		}
		else {
			QString d = filterDeclarations(body);
			if (hasDeclarations(d))
				add(head + " { " + d + " }");
		}
		i = close + 1;
	}
	return out;
}

// ------------------------------------------------------------- html scanning

/// Attributes that carry a colour all by themselves.
bool isLegacyColorAttribute(const QString &name, const QString &tag)
{
	static const QSet<QString> always { "color", "bgcolor", "bordercolor" };
	// these are only colour attributes on <body>; elsewhere they are ordinary
	// attributes that must not be touched.
	static const QSet<QString> onBody { "text", "link", "vlink", "alink" };
	if (always.contains(name))
		return true;
	return tag.compare(u"body", Qt::CaseInsensitive) == 0 && onBody.contains(name);
}

/// End of the tag that starts at \p lt, honouring quoted attribute values.
qsizetype findTagEnd(const QString &html, qsizetype lt)
{
	QChar quote;
	for (qsizetype i = lt + 1; i < html.size(); ++i){
		QChar ch = html.at(i);
		if (!quote.isNull()){
			if (ch == quote)
				quote = QChar();
			continue;
		}
		if (ch == u'"' || ch == u'\'')
			quote = ch;
		else if (ch == u'>')
			return i;
	}
	return -1;
}

/// Rebuilds the tag (given without the '<' and '>') without colour attributes.
QString rewriteTag(QStringView tag)
{
	if (tag.isEmpty() || tag.front() == u'/' || tag.front() == u'!' || tag.front() == u'?')
		return tag.toString();

	qsizetype nameEnd = 0;
	while (nameEnd < tag.size() && !tag.at(nameEnd).isSpace() && tag.at(nameEnd) != u'/')
		++nameEnd;
	QString name = tag.left(nameEnd).toString();

	QStringView rest = tag.mid(nameEnd);
	bool selfClosing = false;
	auto trimmedRest = rest.trimmed();
	if (trimmedRest.endsWith(u'/')){
		selfClosing = true;
		trimmedRest = trimmedRest.left(trimmedRest.size() - 1);
	}

	static const QRegularExpression attrRe(
		R"re(([^\s"'>/=]+)(\s*=\s*)?("[^"]*"|'[^']*'|[^\s"'=<>`]*))re");

	QStringList kept;
	QStringList original;
	auto it = attrRe.globalMatch(trimmedRest);
	while (it.hasNext()){
		auto m = it.next();
		QString attrName = m.captured(1).toLower();
		original.append(m.captured(0));
		if (isLegacyColorAttribute(attrName, name))
			continue;
		if (attrName == u"style"){
			QString value = m.captured(3);
			if (value.size() >= 2 && (value.front() == u'"' || value.front() == u'\''))
				value = value.mid(1, value.size() - 2);
			QString filtered = filterDeclarations(value);
			if (!hasDeclarations(filtered))
				continue;
			kept.append("style=\"" + filtered + "\"");
			continue;
		}
		kept.append(m.captured(0));
	}

	if (kept == original)
		return tag.toString();

	QString out = name;
	for (const auto &a : kept){
		out += ' ';
		out += a;
	}
	if (selfClosing)
		out += " /";
	return out;
}

} // namespace

QString stripHtmlColors(QString html)
{
	QString out;
	out.reserve(html.size());

	qsizetype i = 0;
	while (i < html.size()){
		qsizetype lt = html.indexOf(u'<', i);
		if (lt < 0){
			out += html.mid(i);
			break;
		}
		out += html.mid(i, lt - i);

		// comments may contain html, and their content is not markup
		if (QStringView(html).mid(lt, 4) == u"<!--"){
			qsizetype end = html.indexOf(u"-->", lt + 4);
			if (end < 0){
				out += html.mid(lt);
				break;
			}
			out += html.mid(lt, end + 3 - lt);
			i = end + 3;
			continue;
		}

		qsizetype gt = findTagEnd(html, lt);
		if (gt < 0){
			out += html.mid(lt);
			break;
		}
		QStringView tag = QStringView(html).mid(lt + 1, gt - lt - 1);

		// <style> holds a style sheet, not attributes
		if (tag.startsWith(u'/')){
			out += "<" + rewriteTag(tag) + ">";
			i = gt + 1;
			continue;
		}
		qsizetype nameEnd = 0;
		while (nameEnd < tag.size() && !tag.at(nameEnd).isSpace() && tag.at(nameEnd) != u'/')
			++nameEnd;
		if (tag.left(nameEnd).compare(u"style", Qt::CaseInsensitive) == 0){
			qsizetype close = -1;
			for (qsizetype p = gt + 1; (p = html.indexOf(u'<', p)) >= 0; ++p)
				if (QStringView(html).mid(p, 7).compare(u"</style", Qt::CaseInsensitive) == 0){
					close = p;
					break;
				}
			QStringView css = QStringView(html).mid(gt + 1, (close < 0 ? html.size() : close) - gt - 1);
			QString filtered = filterCss(css);
			// nothing was dropped: keep the sheet exactly as it came in
			if (filtered.simplified() == css.toString().simplified())
				filtered = css.toString();
			if (!filtered.trimmed().isEmpty()){
				out += "<" + rewriteTag(tag) + ">";
				out += filtered;
				out += "</style>";
			}
			if (close < 0)
				i = html.size();
			else {
				qsizetype closeEnd = findTagEnd(html, close);
				i = (closeEnd < 0 ? html.size() : closeEnd + 1);
			}
			continue;
		}

		out += "<" + rewriteTag(tag) + ">";
		i = gt + 1;
	}
	return out;
}
