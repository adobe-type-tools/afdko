/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

// NOLINT(build/header_guard)
"[-cff options: defaults -E, +F, -O, -S, +T, -V, -Z, -b, +W, -d, +H]\n"
"+/-E    do/don't optimize for embedding\n"
"+/-F    do/don't optimize Family zones\n"
"+/-O    do/don't optimize for ROM\n"
"+/-S    do/don't subroutinize\n"
"+/-T    do/don't optimize font (for testing purpose; not for production)\n"
"+/-V    do/don't remove path overlaps\n"
"+/-Z    don't/do decompose SEAC and dotsection operators\n",
"+/-b    do/don't preserve glyph order\n"
"+/-W    do/don't optimize widths\n"
"+/-d    do/don't print duplicate hintsubs warnings to stderr\n"
"+/-H    do/don't print missing stem hint warnings to stderr\n"
"-n      remove hints\n"
"-std    force the output font to have StandardEncoding\n"
"-no_opt disable charstring optimizations (e.g.: x 0 rmoveto => x hmoveto)\n"
"-go     order according to GOADB file (rather than default optimizations)\n"
"-maxs N set the maximum number of subroutines (0 means 32765)\n"
"-amnd   write output even when .notdef is missing\n"
"\n"
"CFF mode writes a CFF conversion of an abstract font. The precise form of the\n"
"CFF font that is written can be controlled to a limited extent by the options\n",
"above.\n"
"\n"
"The +E (embedding) option removes purely descriptive keys from the font without\n"
"affecting its rendering. The +F (Family) option removes FamilyBlues and\n"
"FamilyOtherBlues from hint dictionaries if they are identical to the BlueValues\n"
"and OtherBlues, respectively. The +O (optimize) option removes the same keys as\n"
"the +E option as well as performing some charstring optimizations that are\n",
"suitable for fonts embedded in printer ROMs. The +T (testing) option performs\n"
"charstring optimizations that are normally desirable but prevent bitmap\n"
"comparisons from being performed on some charstrings. These optimizations may\n"
"be disabled using the -T option. Note: +/-T implies +/-F. The -Z\n"
"option converts charstrings using the seac operator into a regular path by\n"
"combining the component glyphs specified in the charstring and removes the\n",
"dotsection operator. Normally glyphs are sorted to reduce the CFF data size.\n"
"The +b option forces the original glyph order to be preserved (with the\n"
"exception of the .notdef glyph which is always assigned glyph index 0). The +d\n"
"(debug) option enables duplicate hintsubrs warnings on stderr.\n"
"\n"
"For example, the command:\n"
"\n"
"    tx -cff +d -a rdr_____.pfb\n"
"\n"
"converts the font file \"rdr_____.pfb\" to CFF, with charstring warnings enabled,\n"
"and writes the results to the file \"rdr_____.cff\".\n"
