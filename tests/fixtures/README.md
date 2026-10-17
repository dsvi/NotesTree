Synthetic html fixtures for the colour-stripping tests.

Every file is a small hand-written stand-in for a real-world pattern that the
stripper has to handle.
Each fixture is read by the tests as-is, so keep the files byte-stable:
one line of html, no trailing newline.

browser-paste-style-blocks.html
    what a browser puts on the clipboard when text is copied from a light
    themed page: the theme's text and link colours live in small <style>
    blocks that must be dropped entirely.

web-table-rgb-inline.html
    a table copied from a website: rgb() inline styles, background colours,
    and the border shorthand whose colour must go while width and style stay.

border-shorthand.html
    the single most common colour declaration in real notes
    (border: 1px solid rgb(221, 221, 221), thousands of times): the border
    must remain visible on a dark theme, only its colour is dropped.

app-saved-qt-html.html
    html this application saved itself: -qt-block-indent styles, fonts and
    sizes must survive, only the one coloured span loses its colour.

legacy-attributes.html
    old generated html: bgcolor, <font color>, and the body tag's
    text/link/vlink attributes.

keyframes-media-queries.html
    a page's style sheet with @keyframes and @media: colour declarations
    nested in at-rules must go without breaking the parser.

mangled-font-family.html
    a real note had a broken font-family whose quoted value
    ('font-size:1.45em;color:rgb(0, 0, 0)') looks like colour declarations
    but is a font name: it must be left alone.

colored-links.html
    links coloured with inline styles: after stripping, the link is the only
    thing Qt still paints in a non-palette colour (the palette's Link role),
    which is the desired outcome.

colorless.html
    html without any colour: must come back byte for byte identical.

malformed-html.html
    html that is not well formed xml (unclosed tags, bare attributes): a
    dom based stripper would refuse to parse it, this one must pass it
    through untouched.

paste-with-theme.html
    the clipboard payload used by the paste-hook test: theme <style> blocks
    plus an inline coloured span.

nested-web-tables.html
    not hand written and not small: the tag skeleton of a real note that was
    pasted out of a Dell product page (22 tables nested up to 8 deep, text and
    images removed). It is here because that shape is what makes opening a note
    expensive - a document that is already on screen relayouts itself after
    every table and cell format change - so the restyling test needs the real
    nesting, not a toy table.
