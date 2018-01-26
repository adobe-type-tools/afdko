

Change log for the Adobe Font Development Kit for OpenType (AFDKO)
==================================================================

2.6.25 (released 2018-01-26)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This release fixes the following issues:

* `#237 <https://github.com/adobe-type-tools/afdko/issues/237>`_ 'cvParameters' block is allowed only Character Variant (cvXX) features
* `#241 <https://github.com/adobe-type-tools/afdko/issues/241>`_ pip uninstall afdko fails


2.6.24 (released 2018-1-3)
~~~~~~~~~~~~~~~~~~~~~~~~~~

The afdko has been restructured so that it can be installed as a Python package. It now depends on the user's Python interpreter, and no longer contains its own Python interpreter.

In order to do this, the two Adobe-owned, non-OpenSource programs were dropped: IS and checkOutlines. If these turn out to be sorely missed, an installer for them will be added to the old Adobe afdko web-site.  The current intent is to migrate the many tests in checkOutlines to the newer checkOutlinesUFO (which does work with OpenType and Type 1 fonts, but currently does only overlap detection and removal, and a few basic path checks).

Older Releases can be downloaded and installed from the `Adobe afdko home page <http://www.adobe.com/devnet/opentype/afdko.html>`__.

Work in progress: auto update of the change log, more installation checks.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

FDK. v2.5 Dec 1 2017 change number 66097.
	This lists only the major bug fixes since the last release. For a complete list see:
	https://github.com/adobe-type-tools/afdko/commits/master
	
	buildCFF2VF.
	Add version check for fontTools module: only starting with version 3.19.0 does fontTools.cffLib build correct PrivateDict BlueValues when there are master source fonts not at the ends of the axes.
	
	makeotfexe.
	Support mapping a glyph to several Unicode values. This can now be done by providing the the UV values as a comma-separated list in the third field of the GlyphOrderAndAliasDB file, in the 'uniXXXXX' format.
	
	Fixed a crashing bug that can happen when the features file contains duplicate class kern pairs. Reported by Andreas Seidel in email.
	
	Add fatal messages if a feature file 'cvParameters' block is used in anything other than a Character Variant (cvXX) feature, or if a 'featureNames' block is used in anything other than a Stylistic Set (ssXX) feature.
	
	Relaxed restrictions on name table name IDs. Only 2 and 6 are now reserved for the implementation.
	
	Fixed bug where use of the 'size' feature in conjunction with named stylistic alternates would cause the last stylistic alternate name to be replaced by the size feature menu name. Incidentally removed old patent notices.
	
	restored old check for the fatal error of two different glyphs being mapped to the same character encoding.
	
	If the last line of a GOADB file did not end in a new-line, makeotf quit, saying the line was too long.
	
	
	otf2otc.
	Can now take a single font as input, and can take an OTC font as input.
	
	sfntdiff.
	Fixed old bug: it used to crash if no file path or only one file path was
	provided. 
	Changed behavior: it now returns non-zero only if a real error occurs: it
	used to return non-zero when there was a difference between the fonts.
	
	spot
	Fixed old bug: a PrivateDict must exist, but it is legal for it to have a
	length of 0.
	
	tx and family.
	Add support for reading and writing blended hints from/to CFF2. 
		
FDK. v2.5 April 27 2017 change number 65781.
	makeInstancesUFO.
	Preserve public.postscriptNames lib key.
	Do not use postscriptFontName attribute.
	
	makeotf.
	New option -V to print MakeOTF.py script version
	
	tx.
	Added new option '-maxs', to set the maximum number of subroutines allowed. The default is now 32K, as some legacy systems cannot support more than this.
	Add improved CFF2 support: tx can now use the option -cff2 to write from a CFF2 font (in a complete OpenType font) to a file containing the output CFF2 table, with full charstring optimization and subroutinization. This last option is still work in progress: it has not been extensively tested, and does yet support blended hints.	
	Several bug fixes for overlap removal.
	Fixed bug in subroutinization that could leave a small number of unused subroutines in the font. This clean-up is optional, as it requires 3x the processing speed with the option than without, and typically reduces the font size by less than 0.5 percent.
	
	ttxn.
	Option '-nv' will now print name ID's 3, and 5, but with the actual version number replaced by the string "VERSION SUPPRESSED".

FDK. v2.5 April 3 2017 change number 65781.
	Variable fonts.
	buildMasterOTFs. new command to build OTF font files from UFO sources, when building variable fonts.
	buildCFF2VF. New command  to build a CFF2 Variable font from the master OTF fonts.

	autohint.
	Fix bug introduced by typo on Dec 1 2015. Caused BlueFuzz to always be set to 1. Rarely causes problems, but found it with font that sets BlueFuzz to zero: with BlueFuzz set to 1, some of the alignment zones were filtered out as being closer than BlueFuzz*3.
	Fixed long-standing bug with UFO fonts where if a glyph was edited after being hinted, running autohint would process and output only the old version of the glyph from the processed layer.
		
	CheckOutlinesUFO.
	Added "quiet mode" option.
	Fixed a bug where logic could try and set an off-curve point as a start point.
	Changed the logic for assigning contour order and start point. The overlap removal changes both, and  checkOutlinesUFO makes some attempt to restore the original state when possible.	These changes will result in different contour order and start points than before the change, but fixes a bug, and will usually produce the same contour order and start point in fonts that are generated as instances from a set of master designs. There will always be cases where there will be some differences. 
	
	MakeOTF.py
	Replace old logic for deriving relative paths with python function for the same.
	When converting Type1 to CID in makeotf, the logic in mergeFonts and ConvertFontToCID.py was requiring the FDArray FontDicts to have keys, like FullName, that are not in fact required, and and are often not present in the source fonts. Fixed both mergeFonts and ConvertFontToCID.py.
	By default, makeotf will add a minimal stub DSIG table in release mode. The new options "-addDSIG" and "-omitDSIG" will force makeotf to either add or omit the stub DSIG table. This function was added because the Adobe Type group is discontinuing signing font files.
	
	makeotfexe.
	fix bug in processing UVS input file for makeotf for non-CID fonts.
	Fixed bug where makeotf would reject a name ID 25 record when specified in a feature file. This name id value used to be reserved, but is now used for for overriding the postscript family named used with arbitrary instances in variable fonts.
	
	mergeFonts.
	Removed requirement for mergeFonts that each FontDict have a FullName, Weight, and Family Name. This fixes a bug in using mergeFonts with UFO sources and converting to CID-keyed output font. Developers shouldn't have to put these fields in the source fonts, since they are not required.
	
	spot.
	Fixed bug in name table dump: Microsoft platform language tags for Big5 and PRC were swapped.
	
	stemHist.
	Removed debug print line, that caused a lot of annoying output, and was left in the last update by accident.
	
	tx.
	When getting Unicode values for output, the presence of UVS cmap meant that no UV values were read from any other cmap subtable. I fixed this bug, but 'tx' still does not support reading and showing UVS values. Doing so will be a significant amount of work, so I am deferring that to the my round of FDK work.
	Added support for CFF2 variable fonts as source fonts: when using -t1 or -cff, these will be snapshotted to an instance. If no user design vector (UDV) argument is supplied, then the output will be the default  data. If a UDV argument is supplied with the option -U, then the instance is built at the specified point in design space.
	Added new option +V/-V to remove overlaps in output Type 1 fonts ( mode -t1) and CFF fonts (mode -cff). This is still experimental: please report any bugs.
	Updated subroutinizer to much faster with larger fonts. (by Ariza Michiharu)
	Added new option (+V/-V) to remove overlaps. (by Ariza Michiharu)
	
	ttx.
	Updated to version 3.9.1 of the fontTools module from master branch on github.
	
FDK. v2.5 May 27 2016 change number 65322.
	Adobe CMAP files for CJK fonts.
	Updated UniCNS-UTF32-H to v1.14

	Build issues.
	Made changes to allow compiling under Xcode 7.x and OSX 10.11
	
	Documentation:
	Fixed a bunch of errors in the Feature File spec. My thanks to Sascha Brawer, 
	who has been reviewing this carefully. See the issues raised by him in Jan
	and Feb 2016 on https://github.com/adobe-type-tools/afdko.
	
	autohint.
	Fixed support for history file, which can be used with non-UFO fonts only.
	This has been broken since UFO support was added.
	
	autohintexe.
	Fixed really old bug:  ascenders and descenders get dropped from the
	alignment zone report if they are a) not in an alignment zone and b) there
	is an overlapping smaller stem hint. This happened with a lot of descenders.
	
	checkOutlines.
	Fixed bug in ufoTools.py that kept checkOutlines (NOT checkOutlinesUFO) from
	working with a UFO font. Fixed bug which mis-identified orientation of path
	which is very thin and in part convex. I am a bit concerned about the
	solution, as what I did was to delete some logic that was used to
	double-check the default rules for determining orientation. However, the
	default logic is the standard way to determine orientation and should always
	be correct. The back-up logic was definitely not always correct as it was
	applied only to a single point, and was correct only if the curve associated
	with the point is concave. It is in fact applied at several different points
	on a path, with the majority vote winning. Since the backup logic is used
	only when a path is very thin, I suspect that it was a sloppy solution to
	fix a specific case.  The change was tested with several large fonts, and
	found no false positives.
	
	makeInstances.
	Fixed bug which produced distorted shapes for those glyphs which were written with the
	Type 1 'seac' operator, aka Type 1 composite glyphs.
	
	makeotf.
	Fixed bug where using both kern format A and B in a single lookup caused
	random values to be assigned.
	Fixed bug where a format A kern value (a single value) would be applied
	to X positioning rather than Y positioning for the features 'vkrn'. Applied same
	logic to vpal, valt, and vhal.
	Finally integrated Georg Seifert's code for supporting hyphen in development
	glyph names. This version differs from Georg's branch in that it does not
	allow any of the special characters in final names, the left side names in
	the GlyphAliasAndOrderDB. However, allowing this is a smaller tweak than it
	used to be: just use the same arguments in cb.c:gnameFinalScan() as in
	gnameDevScan(). This update also includes Geeorg Seifert's changes for allow
	source fonts to have CID names in the form 'cidNNNN'.
	Fixed bugs in ConvertToCID.py module, that kept the -cn option from working
	with a simple source font.
	- fixed bug that the script expected in several places that the fontinfo file
	would contain at least one user defined FontDict.
	- fixed bug that the script expected that the src font would have Weight and
	AdobeCopyright fields in the font dict.
	- fixed a bug that kept the ‘-nS’ option for having any effect when the ‘-cn’ option is used.
	Remove use of 'strsep()': function is not defined in the Windows C library
	Fixed bug in removing duplicate and conflicting entries.
	Changed logic to leave the first pair defined out of a set of duplicate or conflicting entries.
	Fixed bug in processing GDEF glyph class statements: if multiple GlyphClass statements were used.
	the additional glyphs were added to a new set of 4 glyph classes, rather than merged with the 
	allowed 4 glyph classes.
	Fixed issue in GDEF definition processing. Made it an error to specify both LigCaretByPosition
	and LigCaretByIndex for a glyph.
	Corrected error message: language and system statements are allowed in named lookups within
	a feature definition, but are not allowed in stand-alone lookups.
	Corrected typo in MakeOTF.py help text about what the default source font path.
	Fixed an old bug in makeotf. If a mark-to-base or mark-to-mark lookup has
	statements that do not all reference the same mark classes, makeotf used to
	write a 'default' anchor attachment point of (0.0) for any mark class that
	was not referenced by a given statement. Fixed this bug by reporting a fatal
	error: the feature file must be re-written so that all the statements in a
	lookup must all reference the same set of mark classes.
	Suppressed warning about not using GOADB file when building a CID font. Some
	of the changes I made a few weeks ago to allow building fonts with CID's
	specified as glyphs names with the form 'cidNNNNN' allowed this warning to
	be be shown, but it is not appropriate for CID-keyed fonts.
	Fixed old bug where using option -'cn' to convert a nonCID source font to
	CID would cause a mis-match between the maxp table	number of glyphs and the
	numver of glyph actually in the output font, because the conversion used the
	source font data rather than the first pass name-keyed OTF which had been
	subject to glyph subsetting with the GOADB file.
	Fixed bug in reading UVS files for non_CID fonts.
	
	misc.
	Fix copyright statements that are incompatible with the OpenSource license.
	Thanks to Dmitry Smirnov for pointing these out. These were in some make
	files, an example Adobe CMAP file, and some of the technical documentation.
	Fixed typos in help text in PrrofPDF.py. Thank you Arno Enslin.
	
	ttxn
	Fix bug in ttxn.py that broke it when dumping some tables, when used with
	latest font tools library on github.

	tx.
	Fixed bug in rounding fractional values when flattening library elements,
	used in design of CJK fonts.
	Fixed bug in handling FontDict FontMatrix array values: not enough precision
	was used, so that 1/2048 was written as 1/2049 in some cases.
	Fixed bug in reading UFO fonts, so that glyphs with no <outline> element and
	with a <lib> element would be skipped.
	Minor code changes to allow 'tx' to compile as a 64 bit program.
	Fixed bug in dumping afm format data, introduced when tx was updated to be 64 bit.
	Fixed bug in processing seac, introduced in work on rounding fractional values.
	Fixed bug in writing AFM files: -1 value would be written as 4294967295 instead of -1.
	Add option -noOpt, rename blend operator from 'reserved' to 'blend'. This was done in
	order to support experiments with  multiple master fonts.
	When reading a UFO font: if it has no Postscript version entry, set the version to
	1.0.
	When writing a UFO font: If StemSnap[H,V] are missing, but Std[H,V]W are
	present, then use the Std[H,V]W values to supply the UFO postscript
	StemSnap[H,V] values.
	Fixed old bug in 'tx' with rounding decimal values for BlueScale is one of
	the few Postscript values with several places of decimal precision. It is
	stored as an ascii text decimal point number in T1, T2, and UFO files, but
	is stored internally as a C 'float' value in some programs. Real values in C
	cannot exactly represent all decimal values. For example, the closest that a
	C 'float' value can come to "0.375" is "0.03750000149".	When writing output
	fonts, tx was writing out the latter value in ascii text, rather than
	rounding back to 0.0375. Fixed by rounding to 8 decimal places on writing
	the value out. This bug had no practical consequences, as 0.0375 and
	0.03750000149 both convert to exactly the same float value, but was
	annoying, and could cause rounding differences in any programs that use
	higher precision fields to hold the BlueScale value.
	
FDK. v2.5 Dec 1 2015 change number 65012.
	makeotf.
	Fixed bug in MakeOTF.py that kept makeotf from building fonts with spaces in the path.
	Fixed bug in ConvertFontToCID module that kept makeotf from converting UFO fonts to CID.
	Changed support for Unicode Variation Sequence file ( option -ci) so that
	when used with name-keyed fonts, the Region-Order field is omitted, and the
	glyph name may be either a final name or developer glyph name. Added warning
	when glyph in the UVS entry is not found in font. See MakeOTF User's Guide.
	Fixed bug in makeotfexe: it now always makes a cmap table subtable MS
	platform, Unicode, format 4 for CID fonts. This is required by Windows. If
	there are no BMP unicode values, then it makes a stub subtable, mapping GID 0
	to UVS 0.
	
	tx and related programs.
	When reading a UFO source font, do not complain if the fontinfo.plistentry
	"styleName" is present but has is an empty string. This is valid, and is
	common when the style is "Regular".
	
FDK. v2.5 Nov 22 2015 change number 64958.
	autohint and tx.
	Switched to using new text format that is plist-compatible for T1 hint data in UFO fonts.
	See header of FDK/Tools/SharedData/FDKScripts/ufoTools.py for format.
	
	autohint
	Finally fixed excessive generation of flex hints. This has been an issue for
	decades, but never got fixed because it didn't show up anywhere as a
	problem. The last version of makeotf turned on parsing warnings, and so now
	we notice.
	
	checkOutlinesUFO
	Fixed bug where abutting paths didn't get merged if there were no changes in the set of points.
	Fixed bug where a .glif file without an <outline> element was treated as
	fatal error. It is valid for the <outline> element to be missing.
	
	checkOutlines
	Changed -I option so that it also turns off checking for tiny paths. Added
	new option -5 to turn this check back on again.
	Increased max number of paths in a glyph from 64 to 128, per request from a developer.
	
	CompareFamily.py
	Fix old bug in applying ligature width tests for CID fonts, and fixed issue
	with fonts that do not have Mac name table names. The logic now reports
	missing Mac name table names only if there actually are some: if there are
	none, these messages are suppressed.

	
	fontplot/waterfallplot/hintplot/fontsetplot
	Fix bugs that prevented these from being used with CID-keyed fonts and ufo
	fonts. Since the third party library that generates the PDF files is very
	limited, I did this by simply converting the source files to a name-keyed
	Type 1 temporary font file, and then applying the tools the temporary file.
	
	makeInstancesUFO:
	Added a call to the ufonormalizer tool for each instance. Also added a call
	to the defcon library to remove all private lib keys from lib.py and each
	glyph in the default layer, excepting only "public.glyphOrder".
	
	MakeOTF User Guide.
	Fix typos reported by Gustavo Ferreira
	
	MakeOTF.py.
	Increased max number of directories to look upwards when searching for
	GOADB/FontMenuNameDB from 2 to 3.
	Added three new options.
	-omitMacNames/useMacNames	Write only Windows platform menu names in name table,
	apart from the names specified in the feature file.
	-useMacNames writes Mac as well as Windows names.

	-overrideMenuNames
	Allow feature file name table entries to override
	default values and the values from the font menu name DB
	for name IDs. Name ID's 2 and 6 cannot be overridden.
	Use this with caution, and make sure you have provided
	feature file name table entries for all platforms.

	-skco/nskco				do/do not suppress kern class optimization by using left
	side class 0 for non-zero kern values. Optimizing saves a few
	hundred to thousand bytes, but confuses some programs.
	Optimizing is the default behavior, and previously was the only option.
	Allow building an OTF from a UFO font only. The internal features.fea file
	will be used if there is no "features" file in the font's parent directory.
	If the GlyphAliasAndOrderDB file is missing, only a warning will be issued.
	If the FontMenuNameDB is missing, makeotf will attempt to build the font
	menu names from the UFO fontinfo file, using the first of the following keys
	found: "openTypeNamePreferredFamilyName", "familyName", the family name part
	of the PostScriptName,	and finally the value "NoFamilyName". For style, the
	keys are: "openTypeNamePreferredSubfamilyName", "styleName", the style name
	part of the PostScriptName, and finally the value "Regular".
	Fixed bug where MakeOTF allowed the input file path and the output file path
	to be the same.
	
	makeotfexe.
	Extended the set of characters allowed in glyph names to include + * : ~ ^ !
	Allow developer glyph names to start with numbers: final names must still
	follow the PS spec.
	Fixed crashing bug with more than 32K glyphs in a name-keyed font, reported
	by Gustavo Ferreira. Merged changes from Kahled Hosny, to remove requirement
	that 'size' feature menu names have Mac platform names.
	Added three new options: see above.
	Code maintenance in generation of the feature file parser. Rebuilt the
	'antler' parser generator to get rid of a compile-time warning for
	zzerraction, and changed featgram,g so that it would generate the current
	featgram.c, rather than having to edit the latter directly. Deleted the
	object files for the 'antler' parser generator, and updated the read-me for
	the parser generator.
	Fixed really old bug: relative include file references in feature files
	haven't worked right since the FDK moved from Mac OS 9 to OSX. They are now
	relative to the parent directory of the including feature file. If that is
	not found, then makeotf tries to apply the reference as relative to the main
	feature file.
	Changed glyph name parsing rules so that ‘friendly’ glyph names can start
	with a sequence of numbers. Final glyph names still cannot start with a
	number.

	spot.
	Fixed bug in dumping stylistic feature names.
	Fixed bug proofing vertical features: needed to use vkern values. Fix contributed by Hiroki Kanou.
	
	tx family
	Fix crash when using '-gx' option with source UFO fonts for 'tx' family of tools.
	Fix crash when a UFO glyph point has a name attribute with an empty string.
	Fix crash when a UFO font has no public.glyphOrder dict in the lib.plist file.
	Fix really old bug in reading TTF fonts, reported by Belleve Invis. TrueType
	glyphs with nested component references and x/y offsets or translation get
	shifted.
	Added new option '-fdx' to select glyphs by excluding all glyphs with the
	specified FDArray indicies. This and the '-fd' option now take lists and
	ranges of indices, as well as a single index value.
	
	ufonormalizer
	Added a command to call the ufonormalizer tool.
	
	Misc
	Updated to latest modules for booleanOperatons, defcon (ufo3 branch),
	fontMath (ufo3 branch), fontTools, mutatorMath, and robofab (ufo3 branch).
	The FDK no longer contains any private branches of third party modules.
	Rebuilt the Mac OSX, Linux and Windows Python interpreters in the AFDKO,
	bringing the Python version up to 2.7.10. The python interpreters are now
	built for 64 bite systems, and will not run on 32 bit systems.
	

FDK. v2.5 Aug 4 2015 change number 64700.
	autohint.
	Fixed bug in ufoTools.py that was harmless but annoying. Everytime that
	'autohint -all' was run, it added a new program name entry to the history
	list in the hash map for each processed glyph. You saw this only if you
	opened the hashmap file with a text editor, and perhaps eventually in
	slightly slower performance.
	
	checkOutlinesUFO. 
	Fixed bug where presence of outlines with only one or two points caused a stack dump.

	makeotf.
	Fixed bug reported by Paul van der Laan: failed to build ttf file when
	the output file name contains spaces.
	
	spot.
	Fixed new bug that caused spot to crash when dumping GPOS 'size' feature in 
	feature file format.
	
FDK. v2.5 July 17 2015 change number 64655.
	autohint.
	Fixed bug in ufoFontTools.py which placed a new hint block after a flex
	operator, when it should be before.
	Fixed new bug in hinting non-UFO fonts, introduced by switch to absolute
	coordinates in the bez file interchange format.
	Fixed bugs in using hashmap to detect previously hinted glyphs.
	Fixed bugs in handling the issue that checkOutlinesUFO.py, which uses the
	defcon library to write UFO glif files, will in some cases write glif files
	with different file names than they had in the default glyph layer.

	makeotf. Fixed bug with Unicode values in the absolute path to to the font
	home directory.
	Add support for Character Variant (cvXX) feature params.
	Fixed bug where setting Italic style forced OS/2 version to be 4.
	
	spot. Added support for cvXX feature params. 
	Fixed in crash in dumping long contextual substitution strings, such as in
	'GentiumPlus-R.ttf'.
	
	tx, IS, mergeFonts rotateFont:
	fixed bug in handling CID glyph ID greater than 32K.
	Changed to write widths and FontBBox as integer values
	Changed to write SVG, UFO, and dump coordinates with 2 places of precision
	when there is a fractional part.
	Fixed bugs in handling the '-gx' option to exclude glyphs. Fixed problem
	with CID > 32K. Fixed problem when font has 65536 glyphs: all glyphs after
	first last would be excluded.
	Fixed rounding errors in writing out decimal values to cff and t1 fonts
	Increased interpreter stack depth to allow for CUBE operators (Library
	elements) with up to 9 blend axes.
	
	misc
	Fixed windows builds: had to provide a roundf() function, and more includes for
	the _tmpFile function. Fixed a few compile errors.
	Fix bug in documentation for makeInstancesUFO
	Fix bug in BezTools.py on Windows, when having to use a temp file


FDK. v2.5 May 26 2015 change number 64261.
	autohintexe. Worked through a lot of problems with fractional coordinates.
	In the previous release, autohintexe was changed to read and write
	fractional values. However, internal value storage used a Fixed format with
	only 7 bits of precision for the value. This meant that underflow errors
	occurred with 2 decimal places, leading to incorrect coordinates. I was able
	to fix this by changing the code to use 8 bits of precision, which supports
	2 decimal places (but not more!) without rounding errors, but this required
	many changes. The current autohint output will match the output of the
	previous version for integer input values, with two exceptions. Fractional
	stem values will (rarely) differ in the second decimal place. The new
	version will also choose different hints in glyphs which have coordinate
	values outside of the range -16256 to +16256; the previous version had a bug
	in calculating weights for stems.
	
	autohint. Changed logic for writing bez files to write absolute coordinate
	values instead of relative coordinate values. Fixed bug where truncation of
	decimal values lead to cumulative errors in positions adding up to more than
	1 design unit over the length of a path.
	
	tx. Fixed bugs in handling fractional values. tx had a bug with writing
	fractional values that are very near an integer value for the modes -dump.
	-svg, and -ufo. 'tx' also always applied the logic for applying a user
	transform matrix, even though the default transform is the identity
	transform. This has the side-effect of rounding to integer values.
	
FDK. v2.5 April 8 2015 change number 64043.
	checkOutlinesUFO.
	Added  new logic to delete any glyphs from the processed layer which are
	not in the ‘glyphs’ layer.

	makeotf.
	When building CID font, some error messages were printed twice. 
	Add new option -stubCmap4. This causes makeotf to build only a stub cmap 4
	subtable, with just two segments. Needed only for special cases like
	AdobeBlank, where every byte is an issue. Windows requires a cmap format 4
	subtable, but not that it be useful.

	makeCIDFont.
	Output FontDict was sized incorrectly. A function at the end adds some FontInfo keys, but did not increment the size of the dict. Legacy logic is to make the FontInfo dict be 3 larger than the current number of keys.

	makeInstanceUFO:
	Changed FDK branch of mutatorMath so that kern values, glyph widths, and the
	BlueValues family of global hint values are all rounded to integer even when
	the –decimal option is used.
	makeInstanceUFO now deletes the default ‘glyphs’  layer of the target
	instance font before generating the instance. This solves the problem that
	when glyphs are removed from the master instances, the instance font still
	has them.
	Added a new logic to delete any glyphs from the processed layer which are
	not in the ‘glyphs’ layer.
	Removed the ‘-all’ option: even though mutatorMath rewrites all the glyphs,
	the hash values are still valid for glyphs which have not been edited. This
	means that if the developer edits only a few glyphs in the master designs,
	only those glyphs in the instances will get processed by checkOutlinesUFO
	and autohint .

	Support decimal coordinate values in fonts in UFO workflow.

	checkOutlinesUFUO (but not checkOutlines), autohint, and makeInstancesUFO
	will now all pass through decinal coordinates without rounding, if you use
	the new option "-decimal". 'tx' will dump decinal values with 3 decimal places.

	'tx' already reported fractional values, but needed to be modified to report
	only 3 decimal places when writing UFO glif files, and in PDF output mode -
	Acrobat won't read PDF files with 9 decimal places in position values.
	
	This allows a developer to use a much higher precision of point positioning
	without using a large em-square. The Adobe Type group found that using an
	em-square of other than 1000 design units still causes problems in layout
	and text selection line height in many apps, despite it being legal by the
	Type 1 and CFF specifications. 
	
	Note that code design issues in 'autohint'currently limit the decimal
	precision and accuracy to 2 decimal places. 1.01 is works, 1.001 will be
	rounded to 0.
	
	
	
FDK. v2.5 March 3 2015 change number 63782.
	tx.
	Fix bug in reading ttf's. Font version was taken from the name table, which
	can include a good deal more than just the font version. Changed to read
	fontRevision from the head table.
	
	detype1.
	Changed to wrap line only after an operator name, so that the coordinates
	for a command and the command name would stay on one line.
	
	Misc.
	otf2otc.py. Pad table data with 0's so as to align tables on a 4 boundary. Submitted by Cosimo Lupo.
	
FDK v2.5 Feb 21 2015 change number 63718.
	autohint
	Fixed a bug with processing flex hints in ufoTools.py, that caused outline distortion.
	
	compareFamily.
	Fixed bug in processing hints: it would miss fractional hints, and so
	falsely report a glyph as having no hints.
	Fixed so that it would survive a CFF font with a missing Full Name key.
	

	checkOutlinesUFO
	Coordinates are written as integers, as well as being rounded.
	Changed save function so that only the processed glyph layer is saved, and
	the default layer is not touched.
	Changed so that XML type is written as 'UTF-8' rather than 'utf-8'. This was
	actually a change in the FontTools xmlWriter.py module.
	Fixed typos in usage and help text.
	Fixed hash dictionary handling so that it will work with autohint, when
	skipping already processed glyphs.
	Fixed false report of overlap removal when only change was removing flat curve
	Fixed stack dump when new glyph is seen which is not in hash map of
	previously processed glyphs.
	Added logic to make a reasonable effort to sort the new glyph contours in
	the same order as the source glyph contours, so the final contour order will
	not depend on (x,y) position. This was needed because the pyClipper library
	(which is used to remove overlaps) otherwise sorts the contours in (x,y)
	position order, which can result in different contour order in different
	instance fonts from the same set of master fonts.
	
	makeInstancesUFO.
	Changed so that the option -i (selection of	which instances to build) actually works.
	Removed dependence on existence of instance.txt file.
	Changed to call checkOutlinesUFO rather than checkOutlines
	Removed hack of converting all file paths to absolute file paths: this was a
	work-around for a bug in robofab-ufo3k that is now fixed.
	Removed all references to old instances.txt meta data file.
	Fixed so that current dir does not have to be the parent dir of the design space file.
	
	Misc
	Merged fixes from the Github AFDKO OpenSource depot.
	Updated to latest modules for defcon, fontMath, robofab, and mutatorMath.
	Fix for Yosemite (Mac OSX 10.10) in FDK/Tools/setFDKPaths. When an FDK script 
	is run from another Python interpreter, such as the one in Robofont, the parent
	Python interpreter may set the Unix environment variables PYTHONHOME and
	PYTHONPATH. This can cause the AFDKO Python interpreter to load some modules
	from its own library, and others from the parent interpreters library. If these
	are incompatible, a crash ensues.  The fix is to unset the variables PYTHONHOME
	and PYTHONPATH before the AFDKO interpreter is called. 
	Note: AS a separate issue, under Mac OSX 10.10, Python calls to FDK commands
	will  only	work  if  the calling app is run from the command-line (e.g:
	“open /Applications/RoboFont.app“), and the argument "shell="True" is added
	to the subprocess module call to open a system command. I favor also adding
	the argument "stderr=subprocess.STDOUT", else you will not see error
	messages from the Unix shell. Example:
	"log = subprocess.check_output("makeotf -u" , stderr=subprocess.STDOUT , shell=True)".

FDK v2.5 Dec 02 2014 change number 63408.
	spot.
	Fixed error message in GSUB chain contextual 3 proof file output. spot was
	adding it as a shell comment to the proof output, cuasing conversion to PDF
	to fail.

	makeotf.
	Increase limit for glyph name length from 31 to 63 characters. This is not
	encouraged in shipping fonts, as there may be text engines that will not
	accept glyphs with more than 31 characters. This was done to allow building
	test fonts to look for such cases.
	
FDK v2.5 Sep 18 2014 change number 63209.
	makeInstancesUFO.
	Added new script to build instance fonts from UFO master design fonts. This
	uses the design space XML file exported by Superpolator 3 in order	to
	define the design space, and the location of the masters and instance fonts
	in the design space. The definition of the format of this file, and the
	library to use the design space file data, is in the OpenSource mutatorMath
	library on GitHub, and maintained by Erik van Blokland. There are several
	advantages of the Superpolator design space over the previous makeInstances
	script, which uses the Type1 Multiple Master font format to hold the master
	designs. The new version:
	- allows different master designs and locations for each glyph
	- allows master designs to be arbitrarily placed in the design space, and
	hence allows intermediate masters.
	In order to use the mutatorMath library, the FDK-supplied Python now
	contains the robofab, fontMath, and defcon libraries, as well as
	mutatorMath.

	ttx. Updated to the latest branch of the fontTools library as maintained by
	Behdad Esfahbod on GitHub. Added a patch to cffLib.py to fix a minor problem
	with choosing charset format with large glyph sets.
	
	Misc.
	Updated four Adobe-CNS1-* ordering files.

FDK v2.5 Sep 8 2014 change number 63164.
	makeotf.
	Fixed MakeOTF.py to detect "IsOS/2WidthWeightSlopeOnly" as well as the
	misspelled "IsOS/2WidthWeigthSlopeOnly", when processing the fontinfo file.

	Changed behavior when 'subtable' keyword is used in a lookup other than
	class kerning. This condition now triggers only a warning, not a fatal
	error. Requested by FontForge developers.
	
	Fixed bug which preventing making TTF fonts under Windows. This was a
	problem in quoting paths used with the 'ttx' program.
	
	Installation.
	Fixed installation issues. Removed old Windows install files from the
	Windows AFDKOPython directory. This was causing installation of a new FDK
	version under Windows to fail when the user's PATH environment variable
	contained the path to the AFDKOPython directory. Also fixed command file for
	invoking ttx.py.
	
	Misc.
	Updated files used for building ideographic fonts with Unicode IVS
	sequences: FDK/Tools/SharedData/Adobe
	Cmaps/Adobe-CNS1/Adobe-CNS1_sequences.txt and Adobe-Korea1_sequences.txt
	
FDK v2.5 May 14 2014 change number 62754.
	IS, addGlobalColor. When using the -'bc' option, fixed bug with overlow for CID value
	in dumping glyph header. Fixed bug in IS to avoid crash when logic for glyphs > 72 points is used.

	makeotfexe.
	Fixed bug that	applied '-gs' option as default behavior, subsetting the source font to the 
	list of glyphs in the GOADB.
	
FDK v2.5 April 30 2014 change number 62690.
	makeotf
	When building output TTF font from an input TTF font, will now suppress
	warnings that hints are missing. Added a new option "-shw" to suppress
	these warnings for other fonts that with unhinted glyphs. These warnings
	are shown only when the font is built in release mode.
	If the cmap format 4 UTF16 subtable is too large to write, then makeotfexe 
	writes a stub subtable with just the first two segments.
	The last two versions allowed using '-' in glyph names. Removed this, as it
	breaks using glyph tag ranges in feature files.
	
	misc.
	Updated copyright, and removed
	patent references. Made extensive changes to the source code tree
	and build process, to make it easier to build the OpenSource FDK.
	Unfortunately, the source code for the IS and checkOutlines programs
	cannot be OpenSourced.
	
	tx, mergeFonts, rotateFonts
	Removed "-bc" option support, as this includes patents that cannot be shared
	in OpenSource.
	All tx-related tools now report when a font exceeds the max allowed
	subroutine recursion depth.
	
	mergeFonts, rotateFont, tx
	Added common options to all when possible: all now support ufo and svg fonts,
	the '-gx' option to exclude fonts, the '-std' option for cff output, 
	and the '-b' option for cff output.
	
FDK v2.5 April 5 2014 change number 61944.
	makeotf.
	Added new option '-gs'. If the '-ga' or '-r' option is used, then '-gs'
	will omit from the font any glyphs which are not named in the GOADB file.
	
	Linux.
	Replaced the previous build (which worked only on 64-bit systems)
	with a 32 bit version, and rebuilt checkOutlines with debug messages turned off.
	
	ttx.
	Fixed FDK/Tools/win/ttx.cmd file so that the 'ttx' command works again.
	
FDK v2.5 Mar 25 2014 change number 61911.
	makeotf.
	Add support for two new 'features' file keywords, for the OS/2 table.
	Specifying 'LowerOpSize' and 'UpperOpSize' now sets the values
	'usLowerOpticalPointSize' and 'usUpperOpticalPointSize' in the OS/2
	table, and set the table version to 5.
	Fixed the "-newNameID4" option so that if the style name is
	"Regular", it is omitted for the Windows platform name ID 4, as well
	as in the Mac platform version. See change in build 61250.
	
	tx.
	When the user does not specify an output destination file path ( in
	which case tx tries to write to stdout), tx now reports a fatal
	error if the output is a UFO font, rather than crashing.
	tx no longer crashes when encountering an empty "<dict/>" XML
	element.
	
	spot.
	Added logic to dump the new fields in OS/2 table version 5,
	usLowerOpticalPointSize and usUpperOpticalPointSize. An example of
	these values can be seen in the Windows 8 system font Sitka.ttc.
	
	UFO workflow.
	Fixed autohint and checkOutlines so that the '-o" option works, by
	copying the source ufo font to the destination ufo font name, and
	then running the program on the destination ufo font.
	Fixed tools that the PostScript font name is not required.

	Added Linux build.

FDK v2.5 Feb 17 2014 change number 61250.
	tx.
	Fixed rare crashing bug in reading a font file, where a charstring
	ends exactly on a refill buffer boundary.
	Fixed rare crashing bug in subroutinzation.
	Fixed bug in 'tx' where it reported values for wrong glyph with more
	than 32K glyphs in the font.
	Fixed bug where 'tx' wouldn't dump a TrueType Collection font file
	that contained OpenType/CFF fonts.
	Fixed issue where it failed to read a UFO font if the UFO font lacked
	a fontinfo.plist file, or a psFontName entry.
	
	IS.
	Fixed IS so that it no longer scales the fontDict FontMatrix, when a
	scale factor is supplied, unless you provide an argument to request
	this.
	
	makeotf.
	The option '-newNameID4' now builds both Mac and Win name ID 4 using
	name ID 1 and 2, as specified in the OpenType spec. The style name
	is omitted from name ID 4 it is "Regular".
	Changed logic for specifying ValueFormat for PosPair value
	records. Previous logic always used the minimum ValueFormat.
	Since changing ValueFormat between one PosPair record and the
	next requires starting a new subtable, feature files that used
	more than one position adjustment in a PosPair value record
	often got more subtable breaks then necessary, especially when
	specifying a PairPos statement with an all zero Value Record
	value after a Pair Pos statement with a non-zero Value Record.
	With the new logic, if the minimum ValueFormat for the new
	ValueRecord is different than the ValueFormat used with the
	ValueRecord for the previous PairPos statement, and the previous
	ValueFormat permits expressing all the values in the current
	ValueRecord, then the previous ValueFormat is used for the new
	ValueRecord.
	
	otc2otf'and 'otf2otc.
	Added commands 'otc2otf'and 'otf2otc' to build OpenType collection
	files from a OpenType font files, and vice-versa.
	
	ttx.
	Updated the FontTools library to the latest build on the GitHub branch
	maintained by Behdad Esfahbod, as of Jan 14 2014.

	UFO workflow.
	Fixed bugs in ufoTools.py. The glyph list was being returned in
	alphabetic order, even when the public.glyphOrder key was present in
	lib.py. Failed when the glyphOrder key was missing.
	
	
FDK v2.5 Oct 21 2013 change number 60908.
	Added some support for UFO workflow.
	
	tx. 
	tx can now take UFO font as a source font file for all outputs excpet rasterization.
	It prefers GLIF file from the layer
	'glyphs.com.adobe.type.processedGlyphs'. You can select another
	preferred layer with the option '-altLayer <layer name>'. Use 'None'
	for the layer name in order to have tx ignore the preferred layer
	and read GLIF files only from the default layer.
	
	tx can now write to a UFO with the option "-ufo". Note that it is
	NOT a full UFO writer. It writes only the information from the
	Postscript font data. If the source is an OTF or TTF font, it will
	not copy any of the meta data from outside the font program table.
	Also, if the destination is an already existing UFO font, tx will
	overwrite it with the new data: it will not merge the new font data
	with the old.
	
	Fixed bugs with CID values > 32K: use to write these as negative numbers
	when dumping to text formats such as AFM.
	
	autohint
	checkOutlines.
	
	These programs can now be used with UFO fonts. When the source is a
	UFO font, the option '-o" to write to another font is not permitted.
	The changed GLIF files are written to the layer
	'glyphs.com.adobe.type.processedGlyphs'. Each script maintains a hash
	of the width and marking path operators in order to be able to tell
	if the glyph data in the default layer has changed since the script
	was last run. This allows the scripts to process only those glyphs
	which have changed since the last run. The first run of autohint can
	take two minutes for a 2000 glyph font; the second run takes less then a
	second, as it does not need to process the unchanged glyphs.
	
	stemHist
	makeotf
	Can now take ufo fonts as source fonts.


FDK v2.5 Feb 26 2013 change number 60418.
	autohint
	Fixed bug: autohint did not skip commented-out lines in fontinfo file.
	
	makeOTF
	Add support for source font files in the 'detype1' plain text format.
	Added logic for "Language" keyword in fontinfo file. If present, 
	will attempt to set the CID font makeotf option -"cs" to set he Mac script value.
	
	compareFamily.
	
	Added check in Family Test 10 that font really is monospaced or not when either
	the FontDict isFixedPitch value or the Panose value says that it is monospaced.
	
	spot.
	
	Fixed bug that kept 'palt'/'vpal' features from being applied when proofing kerning.
	
FDK v2.5 Sept 4 2012 change number 58732.
	checkOutlines.
	
	Fixed bug where checkOutline would falsely identify the arms of an X as coincident paths,
	when the arms are formed by only two paths with identical bounding boxes.
	

FDK v2.5 Oct 31 2012 change number 59149.
	makeotf.

	When building OpenType/TTF files, changed logic to copy the OS/2 table usWinAscent/Descent
	values over the head table yMax/yMin values, if different. Ths was because:
	- both pairs are supposed to represent the real font bounding box top and bottom,and should be equal.
	- the TTF fonts we use as sources for maketof are built by FontLab
	- FontLab defines the font bounding for TrueType fonts
	box by using off-curve points as well as on-curve points.
	If a path does not have on-curve points at the top and bottom extremes,
	the font bounding box will end up too large. The  OS/2 table usWinAscent/Descent values,
	however, are set by makeotf useing the converted T1 paths, and are more accurate. Note that
	I do not try to fix the head table xMin and xMax. These are much less important, as the
	head table yMin and yMax values are used for line layout by many apps on the
	Mac, and I know of no applicaton for the xMin and yMin values.
	-changed default Unicode H CMAP file for Adobe-Japan1 CID fonts to use the UniJIS2004-UTF32-H file.
	
	misc.
	
	Added the CID font vertical layout files used with KozMinPr6N and KozGoPr6N:
	AJ16-J16.VertLayout.KozGo and AJ16-J16.VertLayout.KozMin
	Updated several Unicode CMAP files, used only with CID fonts.
	
	Added new Perl script, glyph-list.pl, used in building CID fonts. This replaces the 
	three scripts extract-cids.pl, extract-gids.pl, and extract-names.pl, which
	have been removed from the FDK.
	
	
FDK v2.5 Sept 13 2012 change number 58807.
	makeotf.
	
	Discovered that when building TTF fonts, the GDEF table wasn't being copied
	to the final TTF font file. Fixed.
		
FDK v2.5 Sept 4 2012 change number 58732.
	autohint.

	Added new feature to support sets of glyphs with different
	baselines. You can now specify several different sets of global
	alignment zones and stem widths, and apply them to particular sets
	of glyphs within a font when hinting. See option "-hfd" for
	documentation.

	Fix bug: allow AC to handle fonts with no BlueValues, aka alignment zones.

	Fix bug: respect BlueFuzz value in font.

	Fix bug: the options to suppress hint substitution and to allow changes now work.
	
	When hinting a font with no alignment zones or invalid alignment zones (and
	with the '-nb' option), set the arbitrary alignment zone outside the FontBBox,
	rather than the em-square.
	
	checkOutlines.
	
	Fixed bug where very thin elements would get identified as a tiny
	sub path, and get deleted.
	
	Fixed bug in determining path orientation. Logic was just following
	the on-path points, and could get confused by narrow concave inner
	paths, like parentheses with an inner contour following the outer
	contour, as in the Cheltenham Std HandTooled faces.

	Fixed bugs in determining path orientation. Previous logic did not
	handle multiple inner paths, or multiple contained outer paths.
	Logic was also dependent on correctly sorting paths by max Y of path
	bounding box. Replaced approximations with real bezier math
	to determine path bounding box accurately.
	
	Changed test for suspiciously large bounding box for an outline.
	Previous test checked for glyph bounding box outside of fixed limits
	that were based on a 1000 em square. The new test looks only for
	paths that are entirely outside a rectangle based on the font's em
	square, and only reports them: it does not ever delete them. Added
	new option '-b' to set the size of the design square used for the
	test.
	
	Fixed bug where it would leave a temp file on disk when processing a
	Type1 font.
	
	Removed test for coincident control points. This has not been an
	issue for decades. It is frequently found in fonts because designers
	may choose to not use one of the two control points on a curve. The
	unused control point then has the same coordinates as its nearest
	end-point, and would to cause checkOutlines to complain.

	compareFamily.
	
	Single Test 6. Report error if there is a patent number in the copyright.
	Adobe discovered that a company can be sued for lots of money if it ships
	any product with an expired patent number.
	
	Single Test 22 (check RSB and LSB of ligature vs the left and right
	ligature components) did not parse contextual ligature substitution
	rules correctly. Now fixed.
	
	Family Test 18. Survive OTF fonts with no blue values.
	
	Family Test 2 ( Check that the Compatible Family group has same name ID's in all languages.)
	Added the WPF name ID's 21 and 22 to the exception list, which may not exist in all faces of a family.
	
	fontsetplot.
	Fixed so it works with CID fonts. Also fixed so that widow line
	control works right. Added new low level option for controlling
	point size of group header.
	
	Fixed syntax of assert statements. Produced error messages on first use of
	the \*plot commands.
	
	kernCheck.
	
	Fix so that it survives fonts with contextual kerning. It does not, however,
	process the kern pairs in contextual kerning.
	
	makeotf.
	
	Fixed bug in mark to ligature. You can now use an <anchor NULL> element
	without having to follow it by a dummy mark class reference.
	
	Fixed bug which limited source CID fonts to a maximum of 254 FDArray elements,
	rather than the limit of 255 FDArray elements that is imposed by the CFF spec.
	
	Fixed bugs in automatic GDEF generation. When now GDEF is defined,
	all conflicting class assignments in the GlyphClass are filtered
	out. If a glyph is assigned to a make class, that assignment
	overrides any other class assignment. Otherwise, the first
	assignment encountered will override a later assignment. For
	example, since the BASE class is assigned first, assignment to the
	BASE class will override later assignments to LIGATURE or COMPONENT
	classes.
	
	Fix bug in validating GDEF mark attachment rules. This now validates
	the rules, rather than random memory. Had now effect on the output font,
	but did sometimes produce spurious error messages.
	
	Fix crashing bug when trying to report that a glyph being added to a mark
	class is already in the mark class.
	
	If the OS/2 code page bit 29 ( Macintosh encoding) is set, then also
	set bit 0 (Latin (1252). Under Windows XP and Windows 7, if only the
	Mac bit is set, then the font is treated as having no encoding, and
	you can't apply the font to even basic latin text.
		
	By default, set Windows name ID 4 (Full Name) same as Mac named ID
	4, instead of setting it to the PostScript name. This is in order to
	match the current definition of the name ID 4 in the latest OpenType
	spec. A new option to makeotf ("-useOldNameID4" ), and a new key in
	the fontinfo file ("UseOldNameID4"), will cause makeotf to still
	write the PS name to Windows name ID 4.
	
	Add support for WPF names, name ID 21 and 22.
	
	Fixed attachment order of marks to bug in generating Mark to
	Ligature ( GPOS lookup type 4). The component glyphs could be
	reversed.
	
	Fixed bug in auto-generating GDEF table when Mark to Mark ( GPOS
	lookup Type 4) feature statements are used. The target mark glyphs
	were registered as both GDEF GlyphClass Base and Mark glyphs, and
	the former took precedence. makeotfexe now emits a warning when a
	glyph gets assigned to more than one class when auto-generating a
	GDEF table GlyphClass, and glyphs named in mark to mark lookups are
	assigned only to the Mark GDEF glyph class,
	
	Fixed bugs in generating ttf fonts from ttf input. It now merges
	data from the head and hhea tables, and does a better job of dealing
	with the post table. The previous logic made incorrect glyph names
	when the glyphs with names from the Mac Std Encoding weren't all
	contiguous and at the start of the font.

	Added new option "-cn" for non-CID source fonts, to allow reading
	multiple global font alignment zones and stem widths from the
	fontinfo file, and using this to build a CID-keyed CFF table with an
	identity CMAP. This is experimental only; such fonts may not work in
	many apps.
	
	Fixed bug where the coverage table for an element in the match
	string for a chaining contextual statement could have duplicate
	glyphs. This happens when a glyph is specified more than once in the
	class definition for the element. The result is that the format 2
	coverage table has successive ranges that overlap: the end of one
	range is the same glyph ID as the start of the next range. Harmless,
	but triggers complaints in font validators.
	
	Updated to latest Adobe CMAP files for ideographic fonts. Changed name
	of CMAP directories in the FDK, and logic for finding the files.
		
	When providing a GDEF feature file definition, class assignments now may be empty:

.. code:: sh

        table GDEF {
            GlyphClassDef ,,,;
        } GDEF;

.
	is a valid statement. You just need to provide all three commas and the final
	colon to define the four classes.
	The following statement builds a GDEF GlyphClass with an empty Components class.

.. code:: sh

        table GDEF {
            GlyphClassDef [B], [L], [M], ;
        } GDEF;

.
	The glyph alias file now defines order in which glyphs are added to the
	end of the target font, as well as defining the subset and renaming.
	
	The "-cid <cidfontinfo>" option for converting a font to CID can now
	be used without a glyph alias file, if the source font glyphs have
	names in the form "cidXXXX", as is output when mergeFonts is used to
	convert from CID to name-keyed. If the "-cid <cidfontinfo>" option
	is used, and there is no  glyph alias file, then any glyphs in the
	font without a name in the form "cidXXXX" will be ignored.
	
	spot. 
	
	Added error message for duplicate glyph ID's in coverage tables with format 2,
	a problem caused by a bug in makeotf with some Adobe fonts that use chaining
	contextual substitution. Note: error message is written only if level 7 GSUB/GPOS
	dump is requested.
	
	Minor formatting changes to the GSUB/GPOS level 7 dump, to make it easier to 
	edit this into a real feature file. 
	
	When writing out feature file syntax for GPOS 'ignore pos' rules, the rule name
	is now written as 'ignore pos', not just 'ignore'.
	
	can now output glyph names up to 128 chars (note: these are not legal
	PostScript glyph names, and should be encountered only in development fonts.)
	
	Has new option "-ngid" which suppresses output of the trailing glyph ID "@<gid>"
	for TTF fonts.
	
	No longer dumps the DefaultLangSys entry when there is none.
	
	Changed dump logic for contextual and chain contextual lookups so
	that spot will not	dump the lookups referenced by the substitution
	or position rules in the contextual lookups. The previous logic led
	to some lookups getting dumped many times, and also to infinite
	loops in  cases where a contextual lookup referenced other
	contextual lookups.
	
	Added support for Apple kern subtable format 3. Fixed old bug
	causing crash when dumping font with Apple kern table from Windows
	OS.
	
	Fixed error when dumping Apple kern table subtable format 0, when
	kern table is at end of font file.
	
	Fixed crashing bug seen in DejaVuSansMono.ttf: spot didn't expect an anchor offset
	to be zero in a Base Array base Record.
	
	Removed comma from lookupflag dump, to match feature file spec.
	
	Added logic to support name table format 1, but it probably doesn't
	work, since I have been unable to find a font to test with this
	format.
	
	Fixed spelling error for "Canadian" in OS/2 code page fields.
	
	Changed dump of cmap subtable 14: hex values are uppercased, and base + UVS
	values are written in the order [ base, uvs].
	
	stemHist.
	
	Always set the alignment zones outside the font BBox, so as to avoid having the source
	font alignment zones affect collection of stem widths.
	
	Fix bug where the glyph names reported in the stem and alignment reports were off by 
	1 GID if the list of glyphs included the '.notdef' glyph.
	
	tx.
	
	Added support for option "-n" to remove hints for writing Type1 and CFF output fonts.

	Added new option "+b" to the cff output mode, to force glyph order in the output font
	to be the same as in the input font.

	Fixed bug in flattening 'seac' operator. If the glyph components were not in the first 256 glyphs, 
	then the wrong glyph would be selected.
	
	Added new library to read in svg fonts as a source. tx can now read
	all the svg formats that it can write. Handles only the path
	operators:
	M, m, L, L, C, c, Z, z,
	and the font and glyph attributes:
	'font-family', 'unicode', 'horiz-adv-x', 'glyph-name', 'missing-glyph'.

	Fixed bug in converting TTF to OpenType/CFF. It flipped the sign of
	the ItalicAngle in the 'post' table, which in turn flipped the sign
	of the OS/2 table fields ySubscriptXOffset and ySuperscriptXOffset.
	This bug showed up in TTF fonts built by makeotf, as makeotf uses
	'tx' to build a temporary Type 1 font from the source TTF.
		
	Fixed bug where '-usefd' option wasn't respected, when converting from CID to name-keyed fonts.
	
	
	Miscellaneous.
	
	Updated the internal Python interpreter to version 2.7.
		Adobe Cmaps/Adobe-Japan1:
		Updated files
		Adobe-Japan1_sequences.txt
		UniJIS-UTF32-H
		UniJIS2004-UTF32-H
		UniJISX0213-UTF32-H
		UniJISX02132004-UTF32-H
		
		FDKScripts:
		Added several scripts relarted to CID font production.
		cmap-tool.pl
		extract-cids.pl
		extract-gids.pl
		extract-names.pl
		fdarray-check.pl
		fix-fontbbox.pl
		hintcidfont.pl
		subr-check.pl
	
FDK v2.5 March 4 2010 change number 25466.
	charplot.
	This was non-functional in the build 21898. Now fixed.
	
	checkOutlines.
	Changed so that the test for nearly vertical or horizontal lines is invoked only if
	the user specifies the options "-i" or "-4", instead of always. It turns out that this
	test, when fixed automatically, causes more problems than it cures in CJK fonts.
	
	compareFamily.
	Changed so that the default is to check stem widths and positions for bogus hints.
	Used 'tx' rather than Python code for parsing charstring in order to speed up hint check.
	Updated script tags and language tags according to OpenType specification version 1.6.

	
	Documentation. In feature file syntax reference, fixed some errors and bumped the document version to 1.10.
	Fixed  typo in example in section 4.d: lookFlag values are separated by spaces, not commas.
	Fixed  typo in example in section 8.c on stylistic names; examples: quotes around name string need to be matching double quotes.
	Reported by Karsten Luecke.
	Changed agfln.txt copyright notice to BSD license.
	
	makeInstances.
	Fixed bug where a space character in any of the path arguments caused it to fail.
	Fixed bug that can make the FontBBox come out wrong when using Extra glyphs.
	Fixed rounding bug that could (rarely) cause makeInstances to think that a
	composite glyph is being scaled ( which is not supported by this script) when it isn't.
	
	makeotf.
	Fixed bug in generating ttf fonts from ttf input. Previous version simply didn't work.

	spot.
	Added support for "Small" fonts, an Adobe internal Postscript variant used for
	CJK fonts.
	Added support for large kern tables, such as in	the Vista font
	Cambria,  where the size of the kern subtable exceeds the value that
	can be held in the subtable "length" field. In this case, the
	"length" filed must be ignored.
	Fixed proof option to show GPOS records in GID order by default, and in
	lookup order only with the -f" option. It had always been proofing the
	GPOS rules in lookup order since 2003.
	Fixed double memory deallocation when dumping ttc files; this could cause a crash.
	When deccompiling GSUB table to feature file format (-t GSUB=7) and reporting skipped lookups.
	identify lookups which are referenced by a chaining contextual rule.
	
	sfntedit.
	Changed final "done" message to be sent to stdout instead of stderr. Reported by Adam Twardoch.
	
	stemHist.
	Fixed typo in help text, reported by Lee Digidea
	"-all" option wasn't working - now fixed.
	
	tx.
	Added new option '-std' to force StdEncoding in output CFF fonts.

FDK v2.5 May 1 2009 change number 21898.
	autohint
	- Fixed rare case when an rrcurveto is preceded by such a long list of
	rlineto's that the stack limit is passed.
	- Fixed to restore font.pfa output file to StandardEncoding Encoding
	vector. Since requirements of CFF StandardEncoding differs from
	Type1 StandardEncoding, a StandardEncoding	Encoding vector in a
	Type 1 font was sometimes getting converted to a custom Encoding
	vector when being round-tripped through the CFF format which
	autohint does internally.
	
	checkOutlines.
	- Fixed random crash on Windows due to buffer overrun.
	- Fixed rare case when an rrcurveto is preceded by such a long list of
	rlineto's that the stack limit is passed.
	- changed default logging mode to not report glyph names when there is no
	error report for the glyph.
	- Fixed to restore font.pfa output file to StandardEncoding Encoding
	vector.	Since requirements of CFF StandardEncoding differs from
	Type1 StandardEncoding, a StandardEncoding	Encoding vector in a
	Type 1 font was sometimes getting converted to a custom Encoding
	vector when being round-tripped through the CFF format which
	autohint does internally.
	
	CompareFamily. 
	- added "ring" to the list of accent names used to find (accented glyph,
	base glyph) pairs for "Single Face Test 23: Warn if any accented glyphs have
	a width different than the base glyph." Reported by David Agg.
	
	showfont/fontplot2
	- Renamed showfont to fontplot2 to avoid conflict with the Mac OSX showfont tool.
	- Fixed problem with showing vertical origin and advance: was not using VORG
	and vmtx table correctly.
	
	Instance Generator/FontLab scripts. Generating instance fonts from MM fonts.
	- Added logic to support eliminating "working" glyphs from instances, to
	substitute alternate glyph designs for specific instances, and to update
	more Font Dict fields in the instance fonts. Added help.
	- add command line equivalent, "makeInstances' which does the same thing, but
	which uses the IS tool for making the snapshot. See the 'IS' entry.
	
	IS.
	- Added new tool for "intelligent scaling". This uses the hinting in an MM
	font to keep glyph paths aligned when snapshotting from MM fonts. The
	improvement is most visible in glyphs with several elements that need to
	maintain alignment, such as percent and perthousand. It is also useful for
	the same reason when scaling fonts from a large em-square size to a smaller
	size. To be effective, the source MM font must be hinted and must have global
	alignment zones defined. The new font must be re-hinted. For instances from
	MM fonts especially, it is a good idea to re-do the alignment zones, as the
	blending of the MM base designs usually does not produce the best alignment
	zones or stem widths for the instance fonts. makeInstances and "Instance
	Generator" scripts allow you to preserve these modifications when re-doing
	the MM instance snapshot.

	makeotf
	- Fixed generation of version 1.2 GDEF table to match the final OpenType
	spec version 1.6. This version is generated only when the new lookup flag
	'UseMarkFilteringSet" is used.
	- Fixed generation of names for stylistic alternates features. There
	was a bug such that in some circumstances, the feature table entry
	for the stylistic alternate feature would point to the wrong lookup
	table.
	- Fixed generation of the reverse substitution lookup type. This was
	unintentionally disabled just before the previous release.
	- Fixed bugs in memory management of glyph objects. If the font built,
	it was correct, but this bug could cause the font to fail to build.
	
	spot.
	- Fixed to dump GDEF table version 1.2 according to the final OpenType spec
	version 1.6.
	- Fixed feature-format dump of the lookupflags MarkAttachmentType
	and UseMarkFilteringSet to give a class name as an argument, rather
	than a class index.
	- Extended the GDEF table dump to provide a more readable form.
	- added dump formats for htmx and vtmx to show the advance and side
	bearing metrics for all glyphs.
	
FDK v2.5 Jan 22 2009 change number 21340.
	AGLFN. Adobe Glyph List for New Fonts. Created new version 1.7.
	- Reverted to the AGLFN v1.4 name and Unicode
	assignments for Delta, Omega, and mu. The v1.6 versions were better from a
	designer's point of view, but we can't use name-to-Unicode value mappings
	that conflict with the historic usage in the Adobe Glyph List 2.0. Also
	removed afii and commaaccent names. See
	http://www.adobe.com/devnet/opentype/archives/glyph.html.
	-Dropped all the AFII names from the list: "uni" names are actually more
	descriptive, and map to the right Unicode values under Mac OSX.
	-Dropped all the commaccent names from the list: "uni" names map to the
	right Unicode values under Mac OSX before 10.4.x.
	
	autohint.
	-converted AC.py script to call a command-line program rather than
	a Python extension module, same way makeotf works, to avoid 
	continuing Python version problems.
	- fixed so autohint will actually emit vstem3 and hstem3 hint operators
	(counter control hints, which work to keep the space between three stems
	open and equal, as in an 'm') - this has been broken since the first FDK. It
	will now also look in the same directory as the source font for a file named
	"fontinfo", and will attempt to add stem3 hints to the glyph which are
	listed by name in the name list for the keys "HCounterChars" or
	"VCounterChars".
	- fixed old bug where autohint would only pay attention to the bottom four
	of the top zone specified in the Font Dict BlueValues list. This results in
	more edge hints in tall glyphs.
	- fixed special case when adding flex operator which could result in an endless loop 
	-added 'logOnly' option, to allow collecting report without
	changing the font.
	- added option to specify which glyphs to exclude from autohinting
	- suppressed generation and use of <font-name>.plist file, unless it is 
	specifically requested.
	- Fixed bug where an extremely complex glyph would overflow a buffer of the list of hints.

	checkOutlines
	- improve overlap detection and path orientation. checkOutlines will
	now work with outlines formed by overlapping many stroke elements,
	as is sometimes done in developing CJK fonts.
	- added new test for nearly vertical or horizontal lines. Fixed bug
	in this new code, reported by Erik van Blokland.
	
	CompareFamily.
	- For the warning that the Full Family name in the CFF table differs from
	that in the name table, changed it to a "Warning" rather than "Error", and
	explained that there is no functional consequence.
	- Removed check that Mac names ID 16 and 17 do not exist, as makeotf now
	does make them. See notes in MakeOTF User Guide about this.
	- Fixed so it works with ttf fonts again.

	makeotf.
	- removed code that added a default Adobe copyright to the name table if
	n copyright is specified, and removed code to add a default trademark.
	- added support for the lookupflag UseMarkFilteringSet. This is
	defined in the proposed changes for OpenType spec 1.6, and is
	subject to change in definition.
	- Dropped restriction that vmtx/VORG/vhea tables will only be written
	for CID-keyed fonts. The presence in the feature file of either a 'vrt2' feature
	of of vmtx table overrides will now cause these tables to be written for both 
	CID-keyed and name-keyed fonts.
	- Added warning when a feature is referenced in the aalt feature definition,
	but either does not exist or does not contribute any rules to the aalt
	feature. The aalt feature can take only single and alternate substitution
	rules.
	- Added support for the following lookup types:
	GSUB type 2 Multiple Substitution
	GSUB type 8 Reverse Chaining Single Substitution
	GPOS type 3 Cursive Adjustment
	GPOS type 4 Mark-to-Base Attachment
	GPOS type 5 Mark-to-Ligature Attachment
	GPOS type 6 Mark-to-Mark Attachment
	- Added support for explicit definition of the GDEF table, and
	automatic creation of the GDEF when any of the lookup flag settings
	for ignoring a glyph class is used, or any mark classes are defined.
	- Support using TTF fonts as input, to build an OpenType/TTF font,
	with the limitation that glyph order and glyph names cannot be
	changed. This is rather ugly under the hood, but it works. The
	MakeOTF.py Python script uses the tx tool to convert the TTF font to
	CFF data without changing glyph order or names. It then builds an
	OpenType/CFF font. It then uses the sfntedit tool to copy the TTF
	glyph data to the OpenType font, and to delete the CFF table.
	- Added support for building in Unicode Variation Selectors for CID-keyed fonts,
	using the new cmap subtable type 14.
	- fixed bug with inheritance of default rules by scripts and languages
	in feature file feature definitions. Explicitly defined languages were
	only getting default rules defined after the last script  statement, and
	when a script is named, languages of the script which are not named got no
	rules at all.
	- fixed bug where you couldn't run makeotf when the current directory is not
	the same is the source font's home directory.
	- set OS/2.lastChar field to U+FFFF when using mappings beyond the BMP.
	- Create the Mac platform name table font menu names by the same rules
	as used for the Windows menu names. Add new keywords to the FontMenuNameDB file
	syntax. If you use the old keywords, you get the old format; if you use the new syntax, you get 
	name ID's 1,2 and 16 and 17 just like for the Windows platform.
	- Fixed bug in name table font menu names; if you entered a non-English
	Preferred name ("f=") and not a compatible family name ("c="), you would end
	up with a name ID 16 but no name ID 17.
	- fixed bogus " deprecated "except" syntax" message under Windows
	- fixed makeotf bug where contextual pos statements without backtrack or
	lookahead context is writing as a non-contextual rule. Karsten Luecke
	10/15/2007
	- add new option to make stub GSUB table when no GSUB rules are preset.
	- added warning if the aalt feature definition references any feature tags
	that either do not exist in the font, or do not contribute any rules that
	the aalt feature can use.
	
	
	sfntdiff.
	- fixed so that only error messages are written to stderr; all others now written
	to stdout
	- fixed bug in dump of name tale; when processing directories rather than individual files,
	the name name table text was never updated after the first file for the second directory.
	
	spot.
	- fixed option "F to show the contextual rule sub-lookup indices, and to flag those which have already been used by another lookup.
	- if a left side class 0 is empty, dont report it.
	- For GSUB/GPOS=7 feature-file-format dump, give each class a unique name in the entire font by appending the lookup ID to the class names.
	It was just LEFTCLASS_<class index>_<subtable index>, but these names are repeated in every lookup.
	It is now:
LEFTCLASS_c<class index>_s<subtable index>_l<lookup index>,
	- When a positioning value record has more than one value, print the full 4 item value record.	Previously, it would just print non-zero values. This was confusing when dumping Adobe Arabic, as you would see tow identical values at the end of some pos rules. In fact, each of these pos rule does have two adjustment values, one for x and one for y advance adjustment, that happen to be the same numeric value.
	- fixed to write backtrack context glyphs in the right order.
	
	tx.
	- Added option to NOT clamp design coordinates to within the design space when snapshotting
	MM fonts.
	- Add option to subroutinize the font when writing to CFF. This option is
	derived from the same code used by makeotfexe, but takes only about 10% the
	memory and runs much faster. This should allow subroutinizing large CJK
	fonts that makeotfexe couldn't handle. This is new code, so please test results
	carefully, i.e. if you use it, always check that the flattened glyphs
	outlines from the output font are identical to the flattened glyph outlines
	from the input font.
	
	ttxn
	- Added options to suppress hinting in the font program, and version and build numbers.
	


