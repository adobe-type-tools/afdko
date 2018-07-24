
"rotatefont {[mode][mode options][rotation options][shared options][files]}*\n",
"rotatefont [other options]\n",
"\n",
"This program is based on the 'tx' program. Without the rotation options, this\n",
"program behaves exactly like the tx program. See 'tx -u' for a list of all\n",
"options, and 'tx -h' for a discussion of how to use them.\n",
"\n",
"You must use the '-t1' or '-cff' mode setting arguments before any\n",
"other rotatefont options.\n",
"\n",
"[rotation options]\n",
"\n",
"-rt degrees xOffset yOffset     Rotate the font by <degrees> clcokwise, and then\n",
"shift it by <xOffset, yOffset>.\n",
"\n",
"-matrix a b c d e f      Transform the font by the specified PostScript\n",
"transform matrix This is NOT the same thing as the font FontMatrix, which scales\n",
"the font design em-space to 1 point. See -h for more details.\n",
"\n",
"-fm       Has effect only when the -matrix option is used. This will scale the.\n",
"          font FontMatrix (which defines the em-square design size) by the 'a'.\n",
"          parameter of the specified Postscript transform. If 'a' is 2, then the.\n",
"		  the font FontMatrix will be halved, so the specifed design em-square will.\n",
"		  be doubled..\n",
"\n",
"-rtf <file path>		Read in a file that will specify new output name,\n",
"offsets and advance widths for specific glyphs. Also, when this file is\n",
"specified, only the glyphs listed in the file will be written to the output\n",
"font. Note that the '.notdef' glyph must always be specified, as it is required\n",
"for an output font. See 'rotatefont -h' for the format of this file.\n",
"Note that when the '-rtf' option is used, x and y offsets specified with the\n",
"'-rt' option are ignored.\n",
"\n",
"-row      Specifies that the original glyph widths will be passed through\n",
"          unchanged. Has no effect if the glyph width is specified in an\n",
"          entry in the file referenced by the -rtf option.\n",
"-rtw      Specifies that the tranform matrix will be applied to the original\n",
"          glyph widths. Has no effect if the glyph width is specified in\n",
"          an entry in the file referenced by the -rtf option.\n",
"-rew      Specifies that all glyph widths will be set to the em-square size. Has\n",
"          no effect if the glyph width is specified in an entry in the file\n",
"          referenced by the -rtf option.\n",
"\n",
"In the presence of the -rtf option, the glyph widths specified in the file will\n",
"be used. However, if the -rtf option is not used, or if it  is used and a glyph\n",
"width is not specified in a file entry, then rules are used to decide what to do\n",
"with the glyph width. The user may override these rules with the options -row,\n",
"-rtw, and -rew. The rules are that the font matrix specifies no rotation, 180\n",
"degree rotation, or a horizontal shear (i.e., the 3rd transform matrix value is\n",
"0), then the transform matrix will be applied to the original glyph width, with\n",
"Y = 0. If the rotation is 90 or 270, then the glyph width will be set to the\n",
"size of the em-square.\n",
"\n",
"\n",
"\n",
"[other options]\n",
"-u              print usage\n",
"-h              print help\n",
"-v              print component versions\n",
"-g <list>       comma separated glyph selector: tag, cid, or glyph name.\n",
"           May use ranges. Example: '-g Aacute,three.superior,100-123\'n",
" \n",
"-gx <list>      comma separated glyph exclusion selector: tag, cid, or glyph name.\n",
"          May use ranges. All glyphs except those\n",
"	listed are copied. The .notdef glyph will never be excluded.\n",
" \n",
"-fd <index>     only select glyphs belonging to specified font dict\n",
"-fd <index>     only select glyphs not belonging to specified font dict\n",
"				 font dict index list arg may be arange or comma-delimited list.\n",
"				 '-fd 1' or '-fd 3,4,7' or '-fd 3-5' are all valid.\n",
"\n",
"The many other options behave just as do the options described by 'tx -u'.\n",
"\n",
