/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
      This software is licensed as OpenSource, under the Apache License, Version 2.0.
         This license is available at: http://opensource.org/licenses/Apache-2.0. */

// NOLINT(build/header_guard)
"** Warning: this mode is still under development and has a number of bugs **\n"
"\n"
"[-pdf options: default -0]\n"
"-0      show glyph palette (320 glyphs/page)\n"
"-1      -0 + glyph outlines (1 glyph/page)\n"
"\n"
"PDF mode writes PDF data that graphically represents the glyphs in an abstract\n"
"font.\n"
"\n"
"The -0 option shows a glyph palette of 24 point filled glyphs up to a maximum\n"
"of 320 glyphs per page. The -1 option shows a single 500 point outline glyph\n",
"per page that are linked to the corresponding glyph in the palette. Thus you\n"
"can mouse click on a glyph in the palette to get to the outline page of the\n"
"same glyph.\n"
