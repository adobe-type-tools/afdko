This directory contains resources that are used to build other projects. They
are placed here because they may be useful to clients in their own right.

Filename    Description
---------   -----------
txops.h     Type 1/Type 2 charstring operator definitions.
dictops.h   CFF DICT operator definitions.

isocs0.h    CFF ISO Adobe charset aggregate initializer [GID->SID].
excs0.h     CFF Expert charset aggregate initializer [GID->SID].
exsubcs0.h  CFF Expert Subset charset aggregate initializer [GID->SID].

stdenc0.h   CFF Standard encoding aggregate initializer [SID->code].
stdenc1.h   CFF Standard encoding aggregate initializer [code->SID].
stdenc2.h	Standard encoding aggregate initializer [code->gname].
stdenc3.h	Standard encoding aggregate initializer [gname->code].

exenc0.h    CFF Expert encoding aggregate initializer [SID->code].
exenc1.h    CFF Expert encoding aggregate initializer [code->SID].

stdstr0.h   CFF standard strings aggregate initializer [string->SID].
stdstr1.h   CFF standard strings aggregate initializer [SID->string].

agl2uv.h    Adobe Glyph List (AGL) aggregate initializer [gname->UV].
zding2uv.h  Zapf Dingbats glyph list aggregate initializer [gname->UV].

macromn0.h  Mac OS Roman aggregate initializer [code->UV].

macexprt.h  Mac Expert encoding aggregate initializer [code->gname].

macarab.h   Mac OS Arabic aggregate initializer [code->UV].
macce.h     Mac OS Central European aggregate initializer [code->UV].
maccroat.h  Mac OS Croatian aggregate initializer [code->UV].
maccyril.h  Mac OS 9.0 Cyrillic aggregate initializer [code->UV].
macdevan.h  Mac OS Devanagari aggregate initializer [code->UV].
macfarsi.h  Mac OS Farsi aggregate initializer [code->UV].
macgreek.h  Mac OS Greek aggregate initializer [code->UV].
macgujar.h  Mac OS Gujarati aggregate initializer [code->UV].
macgurmk.h  Mac OS Gurmukhi aggregate initializer [code->UV].
machebrw.h  Mac OS Hebrew aggregate initializer [code->UV].
maciceln.h  Mac OS Icelandic aggregate initializer [code->UV].
macrmian.h  Mac OS Romanian aggregate initializer [code->UV].
macthai.h   Mac OS Thai aggregate initializer [code->UV].
macturk.h   Mac OS Turkish aggregate initializer [code->UV].

agl2uv.h    Adobe Glyph List (AGL) aggregate initializer [gname->UV].
uv2agl.h	Adobe Glyph List (AGL) aggregate initializer [UV->gname].
dblmapuv.h	AGL double mapped glyphs aggregate initializer [sec-UV->pri-UV].

zding2uv.h  Zapf Dingbats glyph list aggregate initializer [gname->UV].
uv2zding.h  Zapf Dingbats glyph list aggregate initializer [UV->gname].

applestd.h	Standard Apple Glyph Ordering [GID->gname].

Abbrev.     Description
-------     -----------
GID         Glyph index
SID         CFF string identifier
code        Glyph encoding
string      ASCII string
gname       Glyph name
UV          Unicode value
