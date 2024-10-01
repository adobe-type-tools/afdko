/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
      This software is licensed as OpenSource, under the Apache License, Version 2.0.
         This license is available at: http://opensource.org/licenses/Apache-2.0. */

// NOLINT(build/header_guard)
"[-cff2 options: defaults -S, -b, +H]\n"
"+/-S    do/don't subroutinize\n"
"+/-b    do/don't preserve glyph order\n"
"+/-H    do/don't print missing stem hint warnings to stderr\n"
"-n      remove hints\n"
"-no_opt disable charstring optimizations (e.g.: x 0 rmoveto => x hmoveto)\n"
"-go     order according to GOADB file (rather than default optimizations)\n"
"-maxs N set the maximum number of subroutines (0 means 32765)\n"
"-amnd   write output even when .notdef is missing\n"
"\n"
"CFF2 mode writes a CFF2 conversion of an abstract font.\n"
"\n"
"To generate an instance (a.k.a. snapshot) of a CFF2 input font, use\n"
"the option -U to specify the user design vector.\n"
"\n"
"For example, the command:\n"
"\n"
"    tx -t1 -U 345,200 varfont.otf\n"
"\n"
"creates a Type 1 font from an OpenType-CFF2 font instantiated at\n"
"(first axis) 345 and (second axis) 200.\n"
"\n"
"Use the option -UNC to not clamp the design vector to the min/max\n"
"of font's design space.\n"
