
Changelog
~~~~~~~~~

2.7.0 (released 2018-05-09)
---------------------------
- [CheckOutlinesUFO] Replaced Robofab's pens with FontPens'
  (`#230 <https://github.com/adobe-type-tools/afdko/issues/230>`__)
- Removed ``extractSVGTableSVGDocs.py`` and ``importSVGDocsToSVGTable.py``.
  These are superseded by the scripts at
  https://github.com/adobe-type-tools/opentype-svg/
- Removed ``cmap-tool.pl``, ``fdarray-check.pl``, ``fix-fontbbox.pl``,
  ``glyph-list.pl``, ``hintcidfont.pl``, ``setsnap.pl`` and ``subr-check.pl``.
  These Perl scripts are available from
  https://github.com/adobe-type-tools/perl-scripts
- Removed **CID_font_support** folder and its contents.
- [tx] Fixed assert "out of range region index found in item variation store
  subtable" (`#266 <https://github.com/adobe-type-tools/afdko/pull/266>`__)
- [makeotfexe] Fixed unnecessary truncation of the Format 4 'cmap' subtable
  (`#242 <https://github.com/adobe-type-tools/afdko/issues/242>`__)
- [buildCFF2VF] Fix support for CFF2 fonts with multiple FontDicts
  (`#279 <https://github.com/adobe-type-tools/afdko/pull/279>`__)
- [ttxn] Update for latest FontTools version
  (`#288 <https://github.com/adobe-type-tools/afdko/pull/288>`__)
- New ``ttfcomponentizer`` tool that componentizes TrueType fonts using the
  component data of a related UFO font.
- Added 64-bit support for Mac OSX and Linux
  (`#271 <https://github.com/adobe-type-tools/afdko/pull/271>`__,
  `#312 <https://github.com/adobe-type-tools/afdko/pull/312>`__,
  `#344 <https://github.com/adobe-type-tools/afdko/pull/344>`__)
- [tx] Fixed -dcf mode failing to dump hinted CFF2 variable font
  (`#322 <https://github.com/adobe-type-tools/afdko/issues/322>`__)
- [tx] Fixed conversion of multi-FD CFF2 font to CID-flavored CFF font
  (`#329 <https://github.com/adobe-type-tools/afdko/issues/329>`__)
- [tx] Fixed -cff2 failing to convert CID/CFF1 to multi-FD CFF2
  (`#345 <https://github.com/adobe-type-tools/afdko/issues/345>`__)
- Wheels for all three environments (macOS, Windows, Linux) are now available
  on `PyPI <https://pypi.org/project/afdko>`_ and
  `GitHub <https://github.com/adobe-type-tools/afdko/releases>`_


2.6.25 (released 2018-01-26)
----------------------------
This release fixes the following issues:

- [CheckOutlinesUFO] Skip glyphs whose names are referenced in the UFO's lib
  but do not exist
  (`#228 <https://github.com/adobe-type-tools/afdko/issues/228>`__)
- Partial Python 3 support in ``BezTools.py``, ``ConvertFontToCID.py``,
  ``FDKUtils.py``, ``MakeOTF.py``, ``StemHist.py``, ``autohint.py``,
  ``buildMasterOTFs.py`` and ``ufoTools.py``
  (`#231 <https://github.com/adobe-type-tools/afdko/issues/231>`__, #232, #233)
- [makeotfexe] Fixed parsing of Character Variant (cvXX) feature number
  (`#237 <https://github.com/adobe-type-tools/afdko/issues/237>`__)
- [pip] Fixed ``pip uninstall afdko``
  (`#241 <https://github.com/adobe-type-tools/afdko/issues/241>`__)


2.6.22 (released 2018-01-03)
----------------------------
The **afdko** has been restructured so that it can be installed as a Python
package. It now depends on the user's Python interpreter, and no longer
contains its own Python interpreter.

In order to do this, the two Adobe-owned, non-opensource programs were
dropped: ``IS`` and ``checkOutlines``. If these turn out to be sorely missed,
an installer for them will be added to the old Adobe afdko website. The
current intent is to migrate the many tests in checkOutlines to the newer
``checkOutlinesUFO`` (which does work with OpenType and Type 1 fonts, but
currently does only overlap detection and removal, and a few basic path checks).

Older releases can be downloaded and installed from the
`Adobe's AFDKO home page <http://www.adobe.com/devnet/opentype/afdko.html>`_.


2.5.66097 (released 2017-12-01)
-------------------------------
This only lists the major bug fixes since the last release. For a complete list
see: https://github.com/adobe-type-tools/afdko/commits/master

- [buildCFF2VF] Add version check for fontTools module: only starting with
  version 3.19.0 does fontTools.cffLib build correct PrivateDict BlueValues
  when there are master source fonts not at the ends of the axes.
- [makeotfexe] Support mapping a glyph to several Unicode values. This can now
  be done by providing the the UV values as a comma-separated list in the
  third field of the GlyphOrderAndAliasDB file, in the 'uniXXXXX' format.
- [makeotfexe] Fixed a crashing bug that can happen when the features file
  contains duplicate class kern pairs. Reported by Andreas Seidel in email.
- [makeotfexe] Add fatal messages if a feature file 'cvParameters' block is
  used in anything other than a Character Variant (cvXX) feature, or if a
  'featureNames' block is used in anything other than a Stylistic Set (ssXX)
  feature.
- [makeotfexe] Relaxed restrictions on name table name IDs. Only 2 and 6 are
  now reserved for the implementation.
- [makeotfexe] Fixed bug where use of the 'size' feature in conjunction with
  named stylistic alternates would cause the last stylistic alternate name to
  be replaced by the size feature menu name. Incidentally removed old patent
  notices.
- [makeotfexe] Restored old check for the fatal error of two different glyphs
  being mapped to the same character encoding.
- [makeotfexe] If the last line of a GOADB file did not end in a new-line,
  makeotf quit, saying the line was too long.
- [otf2otc] Can now take a single font as input, and can take an OTC font as
  input.
- [sfntdiff] Fixed old bug: it used to crash if no file path or only one file
  path was provided.
- [sfntdiff] Changed behavior: it now returns non-zero only if a real error
  occurs; it used to return non-zero when there was a difference between the
  fonts.
- [spot] Fixed old bug: a PrivateDict must exist, but it is legal for it to
  have a length of 0.
- [tx *et al.*] Add support for reading and writing blended hints from/to
  CFF2.


2.5.65811 (released 2017-04-27)
-------------------------------
- [makeInstancesUFO] Preserve public.postscriptNames lib key.
- [makeInstancesUFO] Do not use postscriptFontName attribute.
- [makeotf] New option -V to print MakeOTF.py script version.
- [tx] Added new option '-maxs', to set the maximum number of subroutines
  allowed. The default is now 32K, as some legacy systems cannot support more
  than this.
- [tx] Add improved CFF2 support: tx can now use the option -cff2 to write
  from a CFF2 font (in a complete OpenType font) to a file containing the
  output CFF2 table, with full charstring optimization and subroutinization.
  This last option is still work in progress: it has not been extensively
  tested, and does yet support blended hints.
- [tx] Several bug fixes for overlap removal.
- [tx] Fixed bug in subroutinization that could leave a small number of unused
  subroutines in the font. This cleanup is optional, as it requires 3x the
  processing speed with the option than without, and typically reduces the
  font size by less than 0.5 percent.
- [ttxn] Option '-nv' will now print name IDs 3 and 5, but with the actual
  version number replaced by the string "VERSION SUPPRESSED".
- [ufoTools] FIx autohint bug with UFO fonts: if edit a glyph and re-hint,
  autohint uses old processed layer glyph.


2.5.65781 (released 2017-04-03)
-------------------------------
- [variable fonts] **buildMasterOTFs** new script to build OTF font files from
  UFO sources, when building variable fonts.
- [variable fonts] **buildCFF2VF** new command to build a CFF2 variable font
  from the master OTF fonts.
- [autohint] Fixed bug introduced by typo on Dec 1 2015. Caused BlueFuzz to
  always be set to 1. Rarely causes problems, but found it with font that sets
  BlueFuzz to zero; with BlueFuzz set to 1, some of the alignment zones were
  filtered out as being closer than BlueFuzz*3.
- [autohint] Fixed long-standing bug with UFO fonts where if a glyph was
  edited after being hinted, running autohint would process and output only the
  old version of the glyph from the processed layer.
- [CheckOutlinesUFO] Added "quiet mode" option.
- [CheckOutlinesUFO] Fixed a bug where logic could try and set an off-curve
  point as a start point.
- [CheckOutlinesUFO] Changed the logic for assigning contour order and start
  point. The overlap removal changes both, and  checkOutlinesUFO makes some
  attempt to restore the original state when possible. These changes will
  result in different contour order and start points than before the change,
  but fixes a bug, and will usually produce the same contour order and start
  point in fonts that are generated as instances from a set of master designs.
  There will always be cases where there will be some differences.
- [MakeOTF] Replace old logic for deriving relative paths with python function
  for the same.
- [MakeOTF] When converting Type1 to CID in makeotf, the logic in mergeFonts
  and ConvertFontToCID.py was requiring the FDArray FontDicts to have keys,
  like FullName, that are not in fact required, and are often not present in
  the source fonts. Fixed both mergeFonts and ConvertFontToCID.py.
- [MakeOTF] By default, makeotf will add a minimal stub DSIG table in release
  mode. The new options "-addDSIG" and "-omitDSIG" will force makeotf to either
  add or omit the stub DSIG table. This function was added because the Adobe
  Type group is discontinuing signing font files.
- [makeotfexe] Fixed bug in processing UVS input file for makeotf for non-CID
  fonts.
- [makeotfexe] Fixed bug where makeotf would reject a nameID 25 record when
  specified in a feature file. This nameID value used to be reserved, but is
  now used for overriding the postscript family named used with arbitrary
  instances in variable fonts.
- [mergeFonts] Removed requirement for mergeFonts that each FontDict have a
  FullName, Weight, and Family Name. This fixes a bug in using mergeFonts with
  UFO sources and converting to CID-keyed output font. Developers should not
  have to put these fields in the source fonts, since they are not required.
- [spot] Fixed bug in name table dump: Microsoft platform language tags for
  Big5 and PRC were swapped.
- [stemHist] Removed debug print line, that caused a lot of annoying output,
  and was left in the last update by accident.
- [tx] When getting Unicode values for output, the presence of UVS cmap meant
  that no UV values were read from any other cmap subtable. I fixed this bug,
  but 'tx' still does not support reading and showing UVS values. Doing so will
  be a significant amount of work, so I am deferring that to my next round of
  FDK work.
- [tx] Added support for CFF2 variable fonts as source fonts: when using -t1
  or -cff, these will be snapshotted to an instance. If no user design vector
  (UDV) argument is supplied, then the output will be the default data. If a
  UDV argument is supplied with the option -U, then the instance is built at
  the specified point in the design space.
- [tx] Added new option +V/-V to remove overlaps in output Type 1 fonts (mode
  -t1) and CFF fonts (mode -cff). This is still experimental.
- [tx] Made the subroutinizer a lot faster; the speed bump is quite noticeable
  with CJK fonts. (by Ariza Michiharu)
- [tx] Added new option (+V/-V) to remove overlaps. (by Ariza Michiharu)
- [ttx] Updated to version 3.9.1 of the fontTools module from master branch on
  github.


2.5.65322 (released 2016-05-27)
-------------------------------
- [CMAP files] Updated UniCNS-UTF32-H to v1.14
- [build] Made changes to allow compiling under Xcode 7.x and OSX 10.11
- [documentation] Fixed a bunch of errors in the Feature File spec. My thanks
  to Sascha Brawer, who has been reviewing it carefully. See the issues at
  `<https://github.com/adobe-type-tools/afdko/issues/created_by/brawer>`_.
- [autohint] Fixed support for history file, which can be used with non-UFO
  fonts only. This has been broken since UFO support was added.
- [autohintexe] Fixed really old bug: ascenders and descenders get dropped
  from the alignment zone report if they are a) not in an alignment zone and
  b) there is an overlapping smaller stem hint. This happened with a lot of
  descenders.
- [checkOutlines] Fixed bug in ufoTools.py that kept checkOutlines (NOT
  checkOutlinesUFO) from working with a UFO font.
- [checkOutlines] Fixed bug which misidentified orientation of path which is
  very thin and in part convex. I am a bit concerned about the solution, as
  what I did was to delete some logic that was used to double-check the default
  rules for determining orientation. However, the default logic is the standard
  way to determine orientation and should always be correct. The backup logic
  was definitely not always correct as it was applied only to a single point,
  and was correct only if the curve associated with the point is concave. It is
  in fact applied at several different points on a path, with the majority vote
  winning. Since the backup logic is used only when a path is very thin, I
  suspect that it was a sloppy solution to fix a specific case. The change was
  tested with several large fonts, and found no false positives.
- [makeInstances] Fixed bug which produced distorted shapes for those glyphs
  which were written with the Type 1 'seac' operator, a.k.a. Type 1 composite
  glyphs.
- [makeotfexe] Fixed bug where using both kern format A and B in a single
  lookup caused random values to be assigned.
- [makeotfexe] Fixed bug where a format A kern value (a single value) would be
  applied to X positioning rather than Y positioning for the features 'vkrn'.
  Applied same logic to vpal, valt, and vhal.
- [makeotfexe] Finally integrated Georg Seifert's code for supporting hyphen in
  development glyph names. This version differs from Georg's branch in that it
  does not allow any of the special characters in final names (i.e. the left
  side names in the GlyphAliasAndOrderDB). However, allowing this is a smaller
  tweak than it used to be: just use the same arguments in
  ``cb.c:gnameFinalScan()`` as in ``gnameDevScan()``. This update also includes
  Georg's changes for allow source fonts to have CID names in the form
  'cidNNNN'.
- [ConvertToCID] Fixed bug that the script expected in several places that the
  fontinfo file would contain at least one user defined FontDict.
- [ConvertToCID] Fixed bug that the script expected that the src font would
  have Weight and AdobeCopyright fields in the font dict.
- [makeotf] Fixed a bug that kept the ‘-nS’ option for having any effect when
  the ‘-cn’ option is used.
- [makeotfexe] Remove use of 'strsep()'; function is not defined in the Windows
  C library.
- [makeotf] Fixed bug in removing duplicate and conflicting entries. Changed
  logic to leave the first pair defined out of a set of duplicate or
  conflicting entries.
- [makeotfexe] Fixed bug in processing GDEF glyph class statements: if multiple
  GlyphClass statements were used; the additional glyphs were added to a new
  set of 4 glyph classes, rather than merged with the allowed 4 glyph classes.
- [makeotfexe] Fixed issue in GDEF definition processing. Made it an error to
  specify both LigCaretByPosition and LigCaretByIndex for a glyph.
- [makeotfexe] Corrected error message: language and system statements are
  allowed in named lookups within a feature definition, but are not allowed in
  stand-alone lookups.
- [makeotf] Corrected typo in MakeOTF.py help text about what the default
  source font path.
- [makeotfexe] Fixed an old bug in makeotf. If a mark-to-base or mark-to-mark
  lookup has statements that do not all reference the same mark classes,
  makeotfexe used to write a 'default' anchor attachment point of (0.0) for any
  mark class that was not referenced by a given statement. Fixed this bug by
  reporting a fatal error: the feature file must be re-written so that all the
  statements in a lookup must all reference the same set of mark classes.
- [makeotf] Suppressed warning about not using GOADB file when building a CID
  font. Some of the changes I made a few weeks ago to allow building fonts with
  CIDs specified as glyphs names with the form 'cidNNNNN' allowed this warning
  to be be shown, but it is not appropriate for CID-keyed fonts.
- [makeotf] Fixed old bug where using option -'cn' to convert a non-CID source
  font to CID would cause a mismatch between the maxp tablenumber of glyphs
  and the number of glyph actually in the output font, because the conversion
  used the source font data rather than the first pass name-keyed OTF which had
  been subject to glyph subsetting with the GOADB file.
- [makeotf] Fixed bug in reading UVS files for non_CID fonts.
- Fixed copyright statements that are incompatible with the open source
  license. Thanks to Dmitry Smirnov for pointing these out. These were in some
  make files, an example Adobe CMAP file, and some of the technical
  documentation.
- Fixed typos in help text in ProofPDF.py. Thank you Arno Enslin.
- [ttxn] Fixed bug in ttxn.py that broke it when dumping some tables, when used
  with latest fonttools library.
- [tx] Fixed bug in rounding fractional values when flattening library
  elements, used in design of CJK fonts.
- [tx] Fixed bug in handling FontDict FontMatrix array values: not enough
  precision was used, so that 1/2048 was written as 1/2049 in some cases.
- [tx] Fixed bug in reading UFO fonts, so that glyphs with no <outline> element
  and with a <lib> element would be skipped.
- [tx] Minor code changes to allow 'tx' to compile as a 64 bit program.
- [tx] Fixed bug in dumping AFM format data, introduced when tx was updated to
  be 64 bit.
- [tx] Fixed bug in processing seac, introduced in work on rounding fractional
  values.
- [tx] Fixed bug in writing AFM files: -1 value would be written as 4294967295
  instead of -1.
- [tx] Added option -noOpt, renamed blend operator from 'reserved' to 'blend'.
  This was done in order to support experiments with multiple master fonts.
- [tx] When reading a UFO font: if it has no Postscript version entry, set the
  version to 1.0.
- [tx] When writing a UFO font: if StemSnap[H,V] are missing, but Std[H,V]W are
  present, use the Std[H,V]W values to supply the UFO's postscript
  StemSnap[H,V] values.
- [tx] Fixed old bug with rounding decimal values for BlueScale is one of the
  few Postscript values with several places of decimal precision. It is stored
  as an ASCII text decimal point number in T1, T2, and UFO files, but is stored
  internally as a C 'float' value in some programs. Real values in C cannot
  exactly represent all decimal values. For example, the closest that a C
  'float' value can come to "0.375" is "0.03750000149".When writing output
  fonts, tx was writing out the latter value in ASCII text, rather than
  rounding back to 0.0375. Fixed by rounding to 8 decimal places on writing
  the value out. This bug had no practical consequences, as 0.0375 and
  0.03750000149 both convert to exactly the same float value, but was annoying,
  and could cause rounding differences in any programs that use higher
  precision fields to hold the BlueScale value.


2.5.65012 (released 2015-12-01)
-------------------------------
- [makeotf] Fixed bug that kept makeotfexe from building fonts with spaces in
  the path.
- [ConvertFontToCID] Fixed bug that kept makeotf from converting UFO fonts to
  CID.
- [makeotf] Changed support for Unicode Variation Sequence file (option -ci)
  so that when used with name-keyed fonts, the Region-Order field is omitted,
  and the glyph name may be either a final name or developer glyph name. Added
  warning when glyph in the UVS entry is not found in font. See MakeOTF User's
  Guide.
- [makeotfexe] now always makes a cmap table subtable MS platform, Unicode,
  format 4 for CID fonts. This is required by Windows. If there are no BMP
  Unicode values, then it makes a stub subtable, mapping GID 0 to UVS 0.
- [tx *et al.*] When reading a UFO source font, do not complain if the
  fontinfo.plist entry ``styleName`` is present but has an empty string. This
  is valid, and is common when the style is **Regular**.


2.5.64958 (released 2015-11-22)
-------------------------------
- [autohint/tx] Switched to using new text format that is plist-compatible for
  T1 hint data in UFO fonts. See header of ufoTools.py for format.
- [autohint] Finally fixed excessive generation of flex hints. This has been an
  issue for decades, but never got fixed because it did not show up anywhere as
  a problem. The last version of makeotf turned on parsing warnings, and so now
  we notice.
- [checkOutlinesUFO] Fixed bug where abutting paths did not get merged if there
  were no changes in the set of points.
- [checkOutlinesUFO] Fixed bug where a .glif file without an <outline> element
  was treated as fatal error. It is valid for the <outline> element to be
  missing.
- [checkOutlines] Changed -I option so that it also turns off checking for tiny
  paths. Added new option -5 to turn this check back on again.
- [checkOutlinesUFO] Increased max number of paths in a glyph from 64 to 128,
  per request from a developer.
- [CompareFamily] Fixed old bug in applying ligature width tests for CID fonts,
  and fixed issue with fonts that do not have Mac name table names. The logic
  now reports missing Mac name table names only if there actually are some: if
  there are none, these messages are suppressed.
- [fontplot/waterfallplot/hintplot/fontsetplot] Fixed bugs that prevented these
  from being used with CID-keyed fonts and UFO fonts. Since the third party
  library that generates the PDF files is very limited, I did this by simply
  converting the source files to a name-keyed Type 1 temporary font file, and
  then applying the tools the temporary file.
- [makeInstancesUFO] Added a call to the ufonormalizer tool for each instance.
  Also added a call to the defcon library to remove all private lib keys from
  lib.py and each glyph in the default layer, excepting only
  "public.glyphOrder".
- Fixed typos in MakeOTF User Guide reported by Gustavo Ferreira
- [MakeOTF] Increased max number of directories to look upwards when searching
  for GOADB and FontMenuNameDB files from 2 to 3.
- [MakeOTF/makeotfexe] Added three new options:
	* ``omitMacNames`` and ``useMacNames`` write only Windows platform menu
	  names in name table, apart from the names specified in the feature file.
	  ``useMacNames`` writes Mac as well as Windows names.
	* ``overrideMenuNames`` allows feature file name table entries to override
	  default values and the values from the FontMenuNameDB for name IDs.
	  NameIDs 2 and 6 cannot be overridden. Use this with caution, and make
	  sure you have provided feature file name table entries for all platforms.
	* ``skco``/``nskco`` do/do not suppress kern class optimization by using
	  left side class 0 for non-zero kern values. Optimizing saves a few
	  hundred to thousand bytes, but confuses some programs. Optimizing is the
	  default behavior, and previously was the only option.
- [MakeOTF] Allow building an OTF from a UFO font only. The internal
  ``features.fea`` file will be used if there is no ``features`` file in the
  font's parent directory.
  If the GlyphAliasAndOrderDB file is missing, only a warning will be issued.
  If the FontMenuNameDB is missing, makeotf will attempt to build the font
  menu names from the UFO fontinfo file, using the first of the following keys
  found: ``openTypeNamePreferredFamilyName``, ``familyName``, the family name
  part of the ``postScriptName``, and finally the value **NoFamilyName**. For
  style, the keys are: ``openTypeNamePreferredSubfamilyName``, ``styleName``,
  the style name part of the ``postScriptName``, and finally the value
  **Regular**.
- [MakeOTF] Fixed bug where it allowed the input and output file paths to be
  the same.
- [makeotfexe] Extended the set of characters allowed in glyph names to include
  ``+ * : ~ ^ !``.
- [makeotfexe] Allow developer glyph names to start with numbers; final names
  must still follow the PS spec.
- [makeotfexe] Fixed crashing bug with more than 32K glyphs in a name-keyed
  font, reported by Gustavo Ferreira.
- [makeotfexe] Merged changes from Khaled Hosny, to remove requirement that
  'size' feature menu names have Mac platform names.
- [makeotfexe] Code maintenance in generation of the feature file parser.
  Rebuilt the 'antler' parser generator to get rid of a compile-time warning
  for zzerraction, and changed featgram.g so that it would generate the current
  featgram.c, rather than having to edit the latter directly. Deleted the
  object files for the 'antler' parser generator, and updated the read-me for
  the parser generator.
- [makeotfexe] Fixed really old bug: relative include file references in
  feature files have not worked right since the FDK moved from Mac OS 9 to OSX.
  They are now relative to the parent directory of the including feature file.
  If that is not found, then makeotf tries to apply the reference as relative
  to the main feature file.
- [spot] Fixed bug in dumping stylistic feature names.
- [spot] Fixed bug proofing vertical features: needed to use vkern values. Fix
  contributed by Hiroki Kanou.
- [tx *et all.*] Fix crash when using '-gx' option with source UFO fonts.
- [tx *et all.*] Fix crash when a UFO glyph point has a name attribute with an
  empty string.
- [tx *et all.*] Fix crash when a UFO font has no public.glyphOrder dict in the
  lib.plist file.
- [tx *et all.*] Fix really old bug in reading TTF fonts, reported by Belleve
  Invis. TrueType glyphs with nested component references and x/y offsets or
  translation get shifted.
- [tx *et all.*] Added new option '-fdx' to select glyphs by excluding all
  glyphs with the specified FDArray indicies. This and the '-fd' option now
  take lists and ranges of indices, as well as a single index value.
- Added a command to call the ufonormalizer tool.
- Updated to latest version of booleanOperatons, defcon (ufo3 branch), fontMath
  (ufo3 branch), fontTools, mutatorMath, and robofab (ufo3 branch). The AFDKO
  no longer contains any private branches of third party modules.
- Rebuilt the Mac OSX, Linux and Windows Python interpreters in the AFDKO,
  bringing the Python version up to 2.7.10. The Python interpreters are now
  built for 64-bit systems, and will not run on 32-bit systems.


2.5.64700 (released 2015-08-04)
-------------------------------
- [ufoTools] Fixed bug that was harmless but annoying. Every time that
  ``autohint -all`` was run, it added a new program name entry to the history
  list in the hashmap for each processed glyph. You saw this only if you opened
  the hashmap file with a text editor, and perhaps eventually in slightly
  slower performance.
- [checkOutlinesUFO] Fixed bug where presence of outlines with only one or two
  points caused a stack dump.
- [makeotf] Fixed bug reported by Paul van der Laan: failed to build TTF file
  when the output file name contains spaces.
- [spot] Fixed new bug that caused spot to crash when dumping GPOS 'size'
  feature in feature file format.


2.5.64655 (released 2015-07-17)
-------------------------------
- [ufoTools] Fixed bug which placed a new hint block after a flex operator,
  when it should be before.
- [autohint] Fixed new bug in hinting non-UFO fonts, introduced by the switch
  to absolute coordinates in the bez file interchange format.
- [ufoTools] Fixed bugs in using hashmap to detect previously hinted glyphs.
- [ufoTools] Fixed bugs in handling the issue that checkOutlinesUFO.py (which
  uses the defcon library to write UFO glif files) will in some cases write
  glif files with different file names than they had in the default glyph layer.
- [makeotf] Fixed bug with Unicode values in the absolute path to to the font
  home directory.
- [makeotf] Add support for Character Variant (cvXX) feature params.
- [makeotf] Fixed bug where setting Italic style forced OS/2 version to be 4.
- [spot] Added support for cvXX feature params.
- [spot] Fixed in crash in dumping long contextual substitution strings, such
  as in 'GentiumPlus-R.TTF'.
- [tx] Fixed bug in handling CID glyph ID greater than 32K.
- [tx] Changed to write widths and FontBBox as integer values.
- [tx] Changed to write SVG, UFO, and dump coordinates with 2 places of
  precision when there is a fractional part.
- [tx] Fixed bugs in handling the '-gx' option to exclude glyphs. Fixed problem
  with CID > 32K. Fixed problem when font has 65536 glyphs: all glyphs after
  first last would be excluded.
- [tx] Fixed rounding errors in writing out decimal values to cff and t1 fonts.
- [tx] Increased interpreter stack depth to allow for CUBE operators (Library
  elements) with up to 9 blend axes.
- Fixed Windows builds: had to provide a roundf() function, and more includes
  for the _tmpFile function. Fixed a few compile errors.
- Fixed bug in documentation for makeInstancesUFO.
- Fixed bug in BezTools.py on Windows, when having to use a temp file.


2.5.64261 (released 2015-05-26)
-------------------------------
- [autohintexe] Worked through a lot of problems with fractional coordinates.
  In the previous release, autohintexe was changed to read and write fractional
  values. However, internal value storage used a Fixed format with only 7 bits
  of precision for the value. This meant that underflow errors occurred with 2
  decimal places, leading to incorrect coordinates. I was able to fix this by
  changing the code to use 8 bits of precision, which supports 2 decimal places
  (but not more!) without rounding errors, but this required many changes. The
  current autohint output will match the output of the previous version for
  integer input values, with two exceptions. Fractional stem values will
  (rarely) differ in the second decimal place. The new version will also choose
  different hints in glyphs which have coordinate values outside of the range
  -16256 to +16256; the previous version had a bug in calculating weights for
  stems.
- [autohint] Changed logic for writing bez files to write absolute coordinate
  values instead of relative coordinate values. Fixed bug where truncation of
  decimal values lead to cumulative errors in positions adding up to more than
  1 design unit over the length of a path.
- [tx] Fixed bugs in handling fractional values: ``tx`` had a bug with writing
  fractional values that are very near an integer value for the modes -dump,
  -svg, and -ufo. ``tx`` also always applied the logic for applying a user
  transform matrix, even though the default transform is the identity
  transform. This has the side-effect of rounding to integer values.


2.5.64043 (released 2015-04-08)
-------------------------------
- [checkOutlinesUFO] Added  new logic to delete any glyphs from the processed
  layer which are not in the ‘glyphs’ layer.
- [makeotf] When building CID font, some error messages were printed twice.
- [makeotf] Added new option ``stubCmap4``. This causes makeotf to build only
  a stub cmap 4 subtable, with just two segments. Needed only for special cases
  like AdobeBlank, where every byte is an issue. Windows requires a cmap format
  4 subtable, but not that it be useful.
- [makeCIDFont] Output FontDict was sized incorrectly. A function at the end
  adds some FontInfo keys, but did not increment the size of the dict. Legacy
  logic is to make the FontInfo dict be 3 larger than the current number of
  keys.
- [makeInstanceUFO] Changed AFDKO's branch of mutatorMath so that kern values,
  glyph widths, and the BlueValues family of global hint values are all rounded
  to integer even when the ``decimal`` option is used.
- [makeInstanceUFO] Now deletes the default ‘glyphs’ layer of the target
  instance font before generating the instance. This solves the problem that
  when glyphs are removed from the master instances, the instance font still
  has them.
- [makeInstanceUFO] Added a new logic to delete any glyphs from the processed
  layer which are not in the ‘glyphs’ layer.
- [makeInstanceUFO] Removed the ``all`` option: even though mutatorMath
  rewrites all the glyphs, the hash values are still valid for glyphs which
  have not been edited. This means that if the developer edits only a few
  glyphs in the master designs, only those glyphs in the instances will get
  processed by checkOutlinesUFO and autohint.
- Support fractional coordinate values in UFO workflow:
	* checkOutlinesUFO (but not checkOutlines), autohint, and makeInstancesUFO
	  will now all pass through decimal coordinates without rounding, if you
	  use the new option "decimal". tx will dump decimal values with 3 decimal
	  places.
	* tx already reported fractional values, but needed to be modified to
	  report only 3 decimal places when writing UFO glif files, and in PDF
	  output mode: Acrobat will not read PDF files with 9 decimal places in
	  position values.
	* This allows a developer to use a much higher precision of point
	  positioning without using a larger em-square. The Adobe Type group found
	  that using an em-square of other than 1000 design units still causes
	  problems in layout and text selection line height in many apps, despite
	  it being legal by the Type 1 and CFF specifications.
	* Note that code design issues in autohint currently limit the decimal
	  precision and accuracy to 2 decimal places: 1.01 works but 1.001 will be
	  rounded to 0.


2.5.63782 (released 2015-03-03)
-------------------------------
- [tx] Fix bug in reading TTFs. Font version was taken from the name table,
  which can include a good deal more than just the font version. Changed to
  read fontRevision from the head table.
- [detype1] Changed to wrap line only after an operator name, so that the
  coordinates for a command and the command name would stay on one line.
- [otf2otc] Pad table data with zeros so as to align tables on a 4 boundary.
  Submitted by Cosimo Lupo.


2.5.63718 (released 2015-02-21)
-------------------------------
- [ufoTools] Fixed a bug with processing flex hints that caused outline
  distortion.
- [compareFamily] Fixed bug in processing hints: it would miss fractional
  hints, and so falsely report a glyph as having no hints.
- [compareFamily] Support processing CFF font without a FullName key.
- [checkOutlinesUFO] Coordinates are written as integers, as well as being
  rounded.
- [checkOutlinesUFO] Changed save function so that only the processed glyph
  layer is saved, and the default layer is not touched.
- [checkOutlinesUFO] Changed so that XML type is written as 'UTF-8' rather
  than 'utf-8'. This was actually a change in the FontTools xmlWriter.py module.
- [checkOutlinesUFO] Fixed typos in usage and help text.
- [checkOutlinesUFO] Fixed hash dictionary handling so that it will work with
  autohint, when skipping already processed glyphs.
- [checkOutlinesUFO] Fixed false report of overlap removal when only change was
  removing flat curve
- [checkOutlinesUFO] Fixed stack dump when new glyph is seen which is not in
  hash map of previously processed glyphs.
- [checkOutlinesUFO] Added logic to make a reasonable effort to sort the new
  glyph contours in the same order as the source glyph contours, so the final
  contour order will not depend on (x,y) position. This was needed because the
  pyClipper library (which is used to remove overlaps) otherwise sorts the
  contours in (x,y) position order, which can result in different contour order
  in different instance fonts from the same set of master fonts.
- [makeInstancesUFO] Changed so that the option -i (selection of which
  instances to build) actually works.
- [makeInstancesUFO] Removed dependency on the presence of instance.txt file.
- [makeInstancesUFO] Changed to call checkOutlinesUFO rather than checkOutlines
- [makeInstancesUFO] Removed hack of converting all file paths to absolute file
  paths: this was a work-around for a bug in robofab-ufo3k that is now fixed.
- [makeInstancesUFO] Removed all references to old instances.txt meta data file.
- [makeInstancesUFO] Fixed so that current dir does not have to be the parent
  dir of the design space file.
- Merged fixes from the Github AFDKO open source repo.
- Updated to latest version defcon, fontMath, robofab, and mutatorMath.
- Fix for Yosemite (Mac OSX 10.10) in FDK/Tools/setFDKPaths. When an AFDKO
  script is ran from another Python interpreter, such as the one in RoboFont,
  the parent Python interpreter may set the Unix environment variables
  PYTHONHOME and PYTHONPATH. This can cause the AFDKO Python interpreter to
  load some modules from its own library, and others from the parent
  interpreters library. If these are incompatible, a crash ensues. The fix is
  to unset the variables PYTHONHOME and PYTHONPATH before the AFDKO interpreter
  is called.
  Note: As a separate issue, under Mac OSX 10.10, Python calls to FDK commands
  will only work if the calling app is run from the command-line (e.g: ``open
  /Applications/RoboFont.app``), and the argument ``shell="True"`` is added
  to the subprocess module call to open a system command. I favor also adding
  the argument ``stderr=subprocess.STDOUT``, else you will not see error
  messages from the Unix shell. Example: ``log = subprocess.check_output(
  "makeotf -u", stderr=subprocess.STDOUT, shell=True)``.


2.5.63408 (released 2014-12-02)
-------------------------------
- [spot] Fixed error message in GSUB chain contextual 3 proof file output; was
  adding it as a shell comment to the proof output, causing conversion to PDF
  to fail.
- [makeotf] Increased the limit for glyph name length from 31 to 63 characters.
  This is not encouraged in shipping fonts, as there may be text engines that
  will not accept glyphs with more than 31 characters. This was done to allow
  building test fonts to look for such cases.


2.5.63209 (released 2014-09-18)
-------------------------------
- [makeInstancesUFO] Added new script to build instance fonts from UFO master
  design fonts. This uses the design space XML file exported by Superpolator 3
  in order to define the design space, and the location of the masters and
  instance fonts in the design space. The definition of the format of this
  file, and the library to use the design space file data, is in the open
  source mutatorMath library on GitHub, and maintained by Erik van Blokland.
  There are several advantages of the Superpolator design space over the
  previous **makeInstances** script, which uses the Type1 Multiple Master font
  format to hold the master designs. The new version a) allows different master
  designs and locations for each glyph, and b) allows master designs to be
  arbitrarily placed in the design space, and hence allows
  intermediate masters. In order to use the mutatorMath library, the
  AFDKO-supplied Python now contains the robofab, fontMath, and defcon
  libraries, as well as mutatorMath.
- [ttx] Updated to the latest branch of the fontTools library as maintained by
  Behdad Esfahbod on GitHub. Added a patch to cffLib.py to fix a minor problem
  with choosing charset format with large glyph sets.
- Updated four Adobe-CNS1-* ordering files.


2.5.63164 (released 2014-09-08)
-------------------------------
- [makeotf] Now detects ``IsOS/2WidthWeightSlopeOnly`` as well as the
  misspelled ``IsOS/2WidthWeigthSlopeOnly``, when processing the fontinfo file.
- [makeotfexe] Changed behavior when 'subtable' keyword is used in a lookup
  other than class kerning. This condition now triggers only a warning, not a
  fatal error. Change requested by FontForge developers.
- [makeotf] Fixed bug which prevented making TTF fonts under Windows. This was
  a problem in quoting paths used with the 'ttx' program.
- Fixed installation issues: removed old Windows install files from the
  Windows AFDKOPython directory. This was causing installation of a new AFDKO
  version under Windows to fail when the user's PATH environment variable
  contained the path to the AFDKOPython directory. Also fixed command file for
  invoking ttx.py.
- Updated files used for building ideographic fonts with Unicode IVS sequences:
  ``FDK/Tools/SharedData/Adobe Cmaps/Adobe-CNS1/Adobe-CNS1_sequences.txt`` and
  ``Adobe-Korea1_sequences.txt``.


2.5.62754 (released 2014-05-14)
-------------------------------
- [IS/addGlobalColor] When using the -'bc' option, fixed bug with overflow for
  CID value in dumping glyph header. Fixed bug in IS to avoid crash when logic
  for glyphs > 72 points is used.
- [makeotfexe] Fixed bug that applied '-gs' option as default behavior,
  subsetting the source font to the list of glyphs in the GOADB.


2.5.62690 (released 2014-04-30)
-------------------------------
- [makeotf] When building output TTF font from an input TTF font, will now
  suppress warnings that hints are missing. Added a new option "-shw" to
  suppress these warnings for other fonts that with unhinted glyphs. These
  warnings are shown only when the font is built in release mode.
- [makeotfexe] If the cmap format 4 UTF16 subtable is too large to write, then
  makeotfexe writes a stub subtable with just the first two segments. The last
  two versions allowed using '-' in glyph names. Removed this, as it breaks
  using glyph tag ranges in feature files.
- Updated copyright, and removed patent references. Made extensive changes to
  the source code tree and build process, to make it easier to build the open
  source AFDKO. Unfortunately, the source code for the **IS** and
  **checkOutlines** programs cannot be open sourced.
- [tx/mergeFonts/rotateFonts] Removed "-bc" option support, as this includes
  patents that cannot be shared in open source.
- [tx] All tx-related tools now report when a font exceeds the max allowed
  subroutine recursion depth.
- [tx/mergeFonts/rotateFonts] Added common options to all when possible: all
  now support UFO and SVG fonts, the '-gx' option to exclude fonts, the '-std'
  option for cff output, and the '-b' option for cff output.


2.5.61944 (released 2014-04-05)
-------------------------------
- [makeotf] Added new option '-gs'. If the '-ga' or '-r' option is used, then
  '-gs' will omit from the font any glyphs which are not named in the GOADB
  file.
- [Linux] Replaced the previous build (which worked only on 64-bit systems)
  with a 32 bit version, and rebuilt checkOutlines with debug messages turned
  off.
- [ttx] Fixed FDK/Tools/win/ttx.cmd file so that the 'ttx' command works again.


2.5.61911 (released 2014-03-25)
-------------------------------
- [makeotf] Add support for two new 'features' file keywords, for the OS/2
  table. Specifying 'LowerOpSize' and 'UpperOpSize' now sets the values
  'usLowerOpticalPointSize' and 'usUpperOpticalPointSize' in the OS/2 table,
  and set the table version to 5.
- [makeotf] Fixed the "-newNameID4" option so that if the style name is
  "Regular", it is omitted for the Windows platform name ID 4, as well as in
  the Mac platform version. See change in build 61250.
- [tx] When the user does not specify an output destination file path (in which
  case tx tries to write to stdout), tx now reports a fatal error if the output
  is a UFO font, rather than crashing.
- [tx] Fixed crash when encountering an empty "<dict/>" XML element.
- [spot] Added logic to dump the new fields in OS/2 table version 5,
  **usLowerOpticalPointSize** and **usUpperOpticalPointSize**. An example of
  these values can be seen in the Windows 8 system font Sitka.TTC.
- [ufo workflow] Fixed autohint and checkOutlines so that the '-o" option
  works, by copying the source UFO font to the destination UFO font name, and
  then running the program on the destination UFO font.
- [ufo workflow] Fixed tools that the PostScript font name is not required.
- Added Linux build.


2.5.61250 (released 2014-02-17)
-------------------------------
- [tx] Fixed rare crashing bug in reading a font file, where a charstring
  ends exactly on a refill buffer boundary.
- [tx] Fixed rare crashing bug in subroutinization.
- [tx] Fixed bug where it reported values for wrong glyph with more than 32K
  glyphs in the font.
- [tx] Fixed bug where the tool would not dump a TrueType Collection font file
  that contained OpenType/CFF fonts.
- [tx] Fixed issue where it failed to read a UFO font if the UFO font lacked
  a fontinfo.plist file, or a psFontName entry.
- [IS] Fixed IS so that it no longer scales the fontDict FontMatrix, when a
  scale factor is supplied, unless you provide an argument to request this.
- [makeotf] The option '-newNameID4' now builds both Mac and Win name ID 4
  using name ID 1 and 2, as specified in the OpenType spec. The style name is
  omitted from name ID 4 it is "Regular".
- [makeotf] Changed logic for specifying ValueFormat for PosPair value records.
  Previous logic always used the minimum ValueFormat. Since changing
  ValueFormat between one PosPair record and the next requires starting a new
  subtable, feature files that used more than one position adjustment in a
  PosPair value record often got more subtable breaks then necessary,
  especially when specifying a PairPos statement with an all zero Value Record
  value after a PairPos statement with a non-zero Value Record. With the new
  logic, if the minimum ValueFormat for the new ValueRecord is different than
  the ValueFormat used with the ValueRecord for the previous PairPos statement,
  and the previous ValueFormat permits expressing all the values in the current
  ValueRecord, then the previous ValueFormat is used for the new ValueRecord.
- Added commands **otc2otf** and **otf2otc** to build OpenType collection files
  from a OpenType font files, and vice-versa.
- [ttx] Updated the FontTools library to the latest build on the GitHub branch
  maintained by Behdad Esfahbod, as of Jan 14 2014.
- [ufo workflow] Fixed bugs in ufoTools.py. The glyph list was being returned
  in alphabetic order, even when the public.glyphOrder key was present in
  lib.py. Failed when the glyphOrder key was missing.


2.5.60908 (released 2013-10-21)
-------------------------------
- [tx] Can now take UFO font as a source font file for all outputs except
  rasterization. It prefers GLIF file from the layer
  ``glyphs.com.adobe.type.processedGlyphs``. You can select another
  preferred layer with the option '-altLayer <layer name>'. Use 'None' for the
  layer name in order to have tx ignore the preferred layer and read GLIF
  files only from the default layer.
- [tx] Can now write to a UFO with the option "-ufo". Note that it is NOT a
  full UFO writer. It writes only the information from the Postscript font
  data. If the source is an OTF or TTF font, it will not copy any of the meta
  data from outside the font program table. Also, if the destination is an
  already existing UFO font, tx will overwrite it with the new data: it will
  not merge the new font data with the old.
- [tx] Fixed bugs with CID values > 32K: used to write these as negative
  numbers when dumping to text formats such as AFM.
- [autohint/checkOutlines] These programs can now be used with UFO fonts. When
  the source is a UFO font, the option '-o' to write to another font is not
  permitted. The changed GLIF files are written to the layer
  'glyphs.com.adobe.type.processedGlyphs'. Each script maintains a hash of the
  width and marking path operators in order to be able to tell if the glyph
  data in the default layer has changed since the script was last run. This
  allows the scripts to process only those glyphs which have changed since the
  last run. The first run of autohint can take two minutes for a 2000 glyph
  font; the second run takes less then a second, as it does not need to process
  the unchanged glyphs.
- [stemHist/makeotf] Can now take UFO fonts as source fonts.


2.5.60418 (released 2013-02-26)
-------------------------------
- [autohint] Now skips comment lines in fontinfo file.
- [makeotf] Added support for source font files in the 'detype1' plain text
  format. Added logic for "Language" keyword in fontinfo file; if present,
  will attempt to set the CID font makeotf option -"cs" to set he Mac script
  value.
- [compareFamily] Added check in Family Test 10 that font really is monospaced
  or not when either the FontDict isFixedPitch value or the Panose value says
  that it is monospaced.
- [spot] Fixed bug that kept 'palt'/'vpal' features from being applied when
  proofing kerning.


2.5.59149 (released 2012-10-31)
-------------------------------
- [makeotf] When building OpenType/TTF files, changed logic to copy the OS/2
  table usWinAscent/Descent values over the head table yMax/yMin values, if
  different. This was because:
  * both pairs are supposed to represent the real font bounding box top and
  bottom,and should be equal;
  * the TTF fonts we use as sources for maketof are built by FontLab;
  * FontLab defines the font bounding box for TrueType fonts by using off-curve
  points as well as on-curve points.
  If a path does not have on-curve points at the top and bottom extremes, the
  font bounding box will end up too large. The OS/2 table usWinAscent/Descent
  values, however, are set by makeotf using the converted T1 paths, and are
  more accurate. Note that I do not try to fix the head table xMin and xMax.
  These are much less important, as the head table yMin and yMax values are
  used for line layout by many apps on the Mac, and I know of no application
  for the xMin and yMin values.
- [makeotf] Changed default Unicode H CMAP file for Adobe-Japan1 CID fonts to
  use the UniJIS2004-UTF32-H file.
- Added the CID font vertical layout files used with KozMinPr6N and KozGoPr6N:
  AJ16-J16.VertLayout.KozGo and AJ16-J16.VertLayout.KozMin.
- Updated several Unicode CMAP files, used only with CID fonts.
- Added new Perl script, glyph-list.pl, used in building CID fonts. This
  replaces the three scripts extract-cids.pl, extract-gids.pl, and
  extract-names.pl, which have been removed from the AFDKO.


2.5.58807 (released 2012-09-13)
-------------------------------
- [makeotf] Discovered that when building TTF fonts, the GDEF table was not
  being copied to the final TTF font file. Fixed.


2.5.58732 (released 2012-09-04)
-------------------------------
- [autohint] Added new feature to support sets of glyphs with different
  baselines. You can now specify several different sets of global alignment
  zones and stem widths, and apply them to particular sets of glyphs within a
  font when hinting. See option "-hfd" for documentation.
- [autohint] Allow AC to handle fonts with no BlueValues, aka alignment zones.
- [autohint] Respect BlueFuzz value in font.
- [autohint] Fixed the options to suppress hint substitution and to allow
  changes.
- [autohint] When hinting a font with no alignment zones or invalid alignment
  zones (and with the '-nb' option), set the arbitrary alignment zone outside
  the FontBBox, rather than the em-square.
- [checkOutlines] Fixed bug where the arms of an X would be falsely identified
  as coincident paths, when they are formed by only two paths with identical
  bounding boxes.
- [checkOutlines] Fixed bug where very thin elements would get identified as a
  tiny sub path, and get deleted.
- [checkOutlines] Fixed bug in determining path orientation. Logic was just
  following the on-path points, and could get confused by narrow concave inner
  paths, like parentheses with an inner contour following the outer contour, as
  in the Cheltenham Std HandTooled faces.
- [checkOutlines] Fixed bugs in determining path orientation. Previous logic
  did not handle multiple inner paths, or multiple contained outer paths. Logic
  was also dependent on correctly sorting paths by max Y of path bounding box.
  Replaced approximations with real bezier math to determine path bounding box
  accurately.
- [checkOutlines] Changed test for suspiciously large bounding box for an
  outline. Previous test checked for glyph bounding box outside of fixed limits
  that were based on a 1000 em square. The new test looks only for paths that
  are entirely outside a rectangle based on the font's em square, and only
  reports them: it does not ever delete them. Added new option '-b' to set the
  size of the design square used for the test.
- [checkOutlines] Fixed bug where it would leave a temp file on disk when
  processing a Type1 font.
- [checkOutlines] Removed test for coincident control points. This has not been
  an issue for decades. It is frequently found in fonts because designers may
  choose to not use one of the two control points on a curve. The unused
  control point then has the same coordinates as its nearest end-point, and
  would to cause checkOutlines to complain.
- [compareFamily] Single Test 6. Report error if there is a patent number in
  the copyright. Adobe discovered that a company can be sued if it ships any
  product with an expired patent number.
- [compareFamily] Single Test 22 (check RSB and LSB of ligature vs. the left
  and right ligature components) did not parse contextual ligature substitution
  rules correctly. Now fixed.
- [compareFamily] Family Test 18. Survive OTF fonts with no blue values.
- [compareFamily] Family Test 2 (Check that the Compatible Family group has
  same nameIDs in all languages): Added the WPF nameIDs 21 and 22 to the
  exception list, which may not exist in all faces of a family.
- [fontsetplot] Fixed so it works with CID fonts. Also fixed so that widow line
  control works right. Added new low level option for controlling point size of
  group header.
- [fontsetplot] Fixed syntax of assert statements. Produced error messages on
  first use of the \*plot commands.
- [kernCheck] Fix so that it survives fonts with contextual kerning. It does
  not, however, process the kern pairs in contextual kerning.
- [makeotf] Fixed bug in mark to ligature. You can now use an <anchor NULL>
  element without having to follow it by a dummy mark class reference.
- [makeotf] Fixed bug which limited source CID fonts to a maximum of 254
  FDArray elements, rather than the limit of 255 FDArray elements that is
  imposed by the CFF spec.
- [makeotf] Fixed bugs in automatic GDEF generation. When now GDEF is defined,
  all conflicting class assignments in the GlyphClass are filtered out. If a
  glyph is assigned to a make class, that assignment overrides any other class
  assignment. Otherwise, the first assignment encountered will override a later
  assignment. For example, since the BASE class is assigned first, assignment
  to the BASE class will override later assignments to LIGATURE or COMPONENT
  classes.
- [makeotf] Fix bug in validating GDEF mark attachment rules. This now
  validates the rules, rather than random memory. Had now effect on the output
  font, but did sometimes produce spurious error messages.
- [makeotf] Fix crashing bug when trying to report that a glyph being added to
  a mark class is already in the mark class.
- [makeotf] If the OS/2 code page bit 29 (Macintosh encoding) is set, then also
  set bit 0 (Latin (1252). Under Windows XP and Windows 7, if only the Mac bit
  is set, then the font is treated as having no encoding, and you cannot apply
  the font to even basic Latin text.
- [makeotf] By default, set Windows name ID 4 (Full Name) same as Mac nameID 4,
  instead of setting it to the PostScript name. This is in order to match the
  current definition of the name ID 4 in the latest OpenType spec. A new option
  to makeotf ("-useOldNameID4"), and a new key in the fontinfo file
  ("UseOldNameID4"), will cause makeotf to still write the PS name to Windows
  name ID 4.
- [makeotf] Add support for WPF names, name ID 21 and 22.
- [makeotf] Fixed attachment order of marks to bug in generating Mark to
  Ligature (GPOS lookup type 4). The component glyphs could be reversed.
- [makeotf] Fixed bug in auto-generating GDEF table when Mark to Mark (GPOS
  lookup Type 4) feature statements are used. The target mark glyphs were
  registered as both GDEF GlyphClass Base and Mark glyphs, and the former took
  precedence. makeotfexe now emits a warning when a glyph gets assigned to more
  than one class when auto-generating a GDEF table GlyphClass, and glyphs named
  in mark to mark lookups are assigned only to the Mark GDEF glyph class.
- [makeotf] Fixed bugs in generating TTF fonts from TTF input. It now merges
  data from the head and hhea tables, and does a better job of dealing with the
  'post' table. The previous logic made incorrect glyph names when the glyphs
  with names from the Mac Std Encoding were not all contiguous and at the start
  of the font.
- [makeotf] Added new option "-cn" for non-CID source fonts, to allow reading
  multiple global font alignment zones and stem widths from the fontinfo file,
  and using this to build a CID-keyed CFF table with an identity CMAP. This is
  experimental only; such fonts may not work in many apps.
- [makeotf] Fixed bug where the coverage table for an element in the match
  string for a chaining contextual statement could have duplicate glyphs. This
  happens when a glyph is specified more than once in the class definition for
  the element. The result is that the format 2 coverage table has successive
  ranges that overlap: the end of one range is the same glyph ID as the start
  of the next range; harmless, but triggers complaints in font validators.
- [makeotf] Updated to latest Adobe CMAP files for ideographic fonts. Changed
  name of CMAP directories in the AFDKO, and logic for finding the files.
- [makeotf] When providing a GDEF feature file definition, class assignments
  now may be empty:

  .. code:: sh

    table GDEF {
        GlyphClassDef ,,,;
    } GDEF;

  is a valid statement. You just need to provide all three commas and the final
  colon to define the four classes. The following statement builds a GDEF
  GlyphClass with an empty Components class.

  .. code:: sh

    table GDEF {
        GlyphClassDef [B], [L], [M], ;
    } GDEF;

- [makeotf] The glyph alias file now defines order in which glyphs are added to
  the end of the target font, as well as defining the subset and renaming.
- [makeotf] The "-cid <cidfontinfo>" option for converting a font to CID can
  now be used without a glyph alias file, if the source font glyphs have names
  in the form "cidXXXX", as is output when mergeFonts is used to convert from
  CID to name-keyed. If the "-cid <cidfontinfo>" option is used, and there is
  no glyph alias file, then any glyphs in the font without a name in the form
  "cidXXXX" will be ignored.
- [spot] Added error message for duplicate glyph IDs in coverage tables with
  format 2, a problem caused by a bug in makeotf with some Adobe fonts that use
  chaining contextual substitution. Note: error message is written only if
  level 7 GSUB/GPOS dump is requested.
- [spot] Minor formatting changes to the GSUB/GPOS level 7 dump, to make it
  easier to edit this into a real feature file.
- [spot] When writing out feature file syntax for GPOS 'ignore pos' rules, the
  rule name is now written as 'ignore pos', not just 'ignore'.
- [spot] Can now output glyph names up to 128 chars (Note: these are not legal
  PostScript glyph names, and should be encountered only in development fonts.)
- [spot] Has new option "-ngid" which suppresses output of the trailing glyph
  ID "@<gid>" for TTF fonts.
- [spot] No longer dumps the DefaultLangSys entry when there is none.
- [spot] Changed dump logic for contextual and chain contextual lookups so that
  spot will notdump the lookups referenced by the substitution or position
  rules in the contextual lookups. The previous logic led to some lookups
  getting dumped many times, and also to infinite loops in  cases where a
  contextual lookup referenced other contextual lookups.
- [spot] Added support for Apple kern subtable format 3. Fixed old bug causing
  crash when dumping font with Apple kern table from Windows OS.
- [spot] Fixed error when dumping Apple kern table subtable format 0, when kern
  table is at end of font file.
- [spot] Fixed crashing bug seen in DejaVuSansMono.TTF: spot did not expect an
  anchor offset to be zero in a Base Array base Record.
- [spot] Removed comma from lookupflag dump, to match feature file spec.
- [spot] Added logic to support name table format 1, but it probably does not
  work, since I have been unable to find a font to test with this format.
- [spot] Fixed spelling error for "Canadian" in OS/2 code page fields.
- [spot] Changed dump of cmap subtable 14: hex values are uppercased, and
  base+UVS values are written in the order [base, uvs].
- [stemHist] Always set the alignment zones outside the font BBox, so as to
  avoid having the source font alignment zones affect collection of stem
  widths.
- [stemHist] Fix bug where the glyph names reported in the stem and alignment
  reports were off by 1 GID if the list of glyphs included the '.notdef' glyph.
- [tx] Added support for option "-n" to remove hints for writing Type1 and CFF
  output fonts.
- [tx] Added new option "+b" to the cff output mode, to force glyph order in
  the output font to be the same as in the input font.
- [tx] Fixed bug in flattening 'seac' operator. If the glyph components were
  not in the first 256 glyphs, then the wrong glyph would be selected.
- [tx] Added new library to read in svg fonts as a source. tx can now read all
  the SVG formats that it can write. Handles only the path operators: M, m, L,
  L, C, c, Z, z, and the font and glyph attributes: 'font-family', 'unicode',
  'horiz-adv-x', 'glyph-name', 'missing-glyph'.
- [tx] Fixed bug in converting TTF to OpenType/CFF. It flipped the sign of the
  ItalicAngle in the 'post' table, which in turn flipped the sign of the OS/2
  table fields ySubscriptXOffset and ySuperscriptXOffset. This bug showed up in
  TTF fonts built by makeotf, as makeotf uses 'tx' to build a temporary Type 1
  font from the source TTF.
- [tx] Fixed bug where '-usefd' option was not respected, when converting from
  CID to name-keyed fonts.
- Updated the internal Python interpreter to version 2.7.
- Updated Adobe Cmaps/Adobe-Japan1 files:
	* Adobe-Japan1_sequences.txt
	* UniJIS-UTF32-H
	* UniJIS2004-UTF32-H
	* UniJISX0213-UTF32-H
	* UniJISX02132004-UTF32-H
- Added several scripts related to CID font production:
	* cmap-tool.pl
	* extract-cids.pl
	* extract-gids.pl
	* extract-names.pl
	* fdarray-check.pl
	* fix-fontbbox.pl
	* hintcidfont.pl
	* subr-check.pl


2.5.25466 (released 2012-03-04)
-------------------------------
- [charplot] This was non-functional since build 21898. Now fixed.
- [checkOutlines] Changed so that the test for nearly vertical or horizontal
  lines is invoked only if the user specifies the options "-i" or "-4",
  instead of always. It turns out that this test, when fixed automatically,
  causes more problems than it cures in CJK fonts.
- [compareFamily] Changed so that the default is to check stem widths and
  positions for bogus hints. Used 'tx' rather than Python code for parsing
  charstring in order to speed up hint check.
- [compareFamily] Updated script tags and language tags according to OpenType
  specification version 1.6.
- [documentation] In feature file syntax reference, fixed some errors and
  bumped the document version to 1.10.
- [documentation] Fixed typo in example in section 4.d: lookFlag values are
  separated by spaces, not commas.
- [documentation] Fixed typo in example in section 8.c on stylistic names:
  quotes around name string need to be matching double quotes. Reported by
  Karsten Luecke.
- [documentation] Changed agfln.txt copyright notice to BSD license.
- [makeInstances] Fixed bug where a space character in any of the path
  arguments caused it to fail.
- [makeInstances] Fixed bug that can make the FontBBox come out wrong when
  using ExtraGlyphs.
- [makeInstances] Fixed rounding bug that could (rarely) cause makeInstances
  to think that a composite glyph is being scaled (which is not supported by
  this script) when it is not.
- [makeotf] Fixed bug in generating TTF fonts from TTF input. Previous version
  simply did not work.
- [spot] Added support for "Small" fonts, an Adobe internal Postscript variant
  used for CJK fonts.
- [spot] Added support for large kern tables, such as in the Vista font
  Cambria,  where the size of the kern subtable exceeds the value that can be
  held in the subtable "length" field. In this case, the "length" filed must
  be ignored.
- [spot] Fixed proof option to show GPOS records in GID order by default, and
  in lookup order only with the "-f" option. It had always been proofing the
  GPOS rules in lookup order since 2003.
- [spot] Fixed double memory deallocation when dumping TTC files; this could
  cause a crash.
- [spot] When decompiling GSUB table to feature file format (-t GSUB=7) and
  reporting skipped lookups identify lookups which are referenced by a chaining
  contextual rule.
- [sfntedit] Changed final "done" message to be sent to stdout instead of
  stderr. Reported by Adam Twardoch.
- [stemHist] Fixed typo in help text, reported by Lee Digidea: "-all" option
  was not working.
- [tx] Added new option '-std' to force StdEncoding in output CFF fonts.


2.5.21898 (released 2009-05-01)
-------------------------------
- [autohint/checkOutlines] Fixed rare case when an rrcurveto is preceded by
  such a long list of rlineto that the stack limit is passed.
- [autohint/checkOutlines] Fixed to restore font.pfa output file to
  StandardEncoding Encoding vector. Since requirements of CFF StandardEncoding
  differs from Type1 StandardEncoding, a StandardEncodingEncoding vector in a
  Type 1 font was sometimes getting converted to a custom Encoding vector when
  being round-tripped through the CFF format which autohint does internally.
- [checkOutlines] Fixed random crash on Windows due to buffer overrun.
- [checkOutlines] Changed default logging mode to not report glyph names when
  there is no error report for the glyph.
- [CompareFamily] Added "ring" to the list of accent names used to find
  (accented glyph, base glyph) pairs for Single Face Test 23. Reported by David
  Agg.
- Renamed showfont to fontplot2 to avoid conflict with the Mac OSX showfont
  tool.
- Fixed problem with showing vertical origin and advance: was not using VORG
  and vmtx table correctly.
- [FontLab scripts] Added logic to Instance Generator to support eliminating
  "working" glyphs from instances, to substitute alternate glyph designs for
  specific instances, and to update more Font Dict fields in the instance
  fonts. Added help.
- Added command line equivalent, "makeInstances' which does the same thing, but
  which uses the IS tool for making the snapshot. See the 'IS' entry.
- [IS] Added new tool for "intelligent scaling". This uses the hinting in an MM
  font to keep glyph paths aligned when snapshotting from MM fonts. The
  improvement is most visible in glyphs with several elements that need to
  maintain alignment, such as percent and perthousand. It is also useful for
  the same reason when scaling fonts from a large em-square size to a smaller
  size. To be effective, the source MM font must be hinted and must have global
  alignment zones defined. The new font must be re-hinted. For instances from
  MM fonts especially, it is a good idea to redo the alignment zones, as the
  blending of the MM base designs usually does not produce the best alignment
  zones or stem widths for the instance fonts. makeInstances and "Instance
  Generator" scripts allow you to preserve these modifications when redoing the
  MM instance snapshot.
- [makeotf] Fixed generation of version 1.2 GDEF table to match the final
  OpenType spec version 1.6. This version is generated only when the new lookup
  flag 'UseMarkFilteringSet" is used.
- [makeotf] Fixed generation of names for stylistic alternates features. There
  was a bug such that in some circumstances, the feature table entry for the
  stylistic alternate feature would point to the wrong lookup table.
- [makeotf] Fixed generation of the reverse substitution lookup type. This was
  unintentionally disabled just before the previous release.
- [makeotf] Fixed bugs in memory management of glyph objects. If the font
  built, it was correct, but this bug could cause the font to fail to build.
- [spot] Fixed to dump GDEF table version 1.2 according to the final OpenType
  spec version 1.6.
- [spot] Fixed feature-format dump of the lookupflags MarkAttachmentType and
  UseMarkFilteringSet to give a class name as an argument, rather than a class
  index.
- [spot] Extended the GDEF table dump to provide a more readable form.
- [spot] Added dump formats for htmx and vtmx to show the advance and side
  bearing metrics for all glyphs.


2.5.21340 (released 2009-01-22)
-------------------------------
- [AGLFN] (Adobe Glyph List for New Fonts) Created new version 1.7.
- [AGLFN] Reverted to the AGLFN v1.4 name and Unicode assignments for Delta,
  Omega, and mu. The v1.6 versions were better from a designer's point of view,
  but we cannot use name-to-Unicode value mappings that conflict with the
  historic usage in the Adobe Glyph List 2.0. See
  http://www.adobe.com/devnet/opentype/archives/glyph.html.
- [AGLFN] Dropped all the 'afii' names from the list: "uni" names are actually
  more descriptive, and map to the right Unicode values under Mac OSX.
- [AGLFN] Dropped all the 'commaccent' names from the list: "uni" names map to
  the right Unicode values under Mac OSX before 10.4.x.
- [autohint] Converted AC.py script to call a command-line program rather than
  a Python extension module, same way makeotf works, to avoid continuing Python
  version problems.
- [autohint] Fixed to actually emit vstem3 and hstem3 hint operators (counter
  control hints, which work to keep the space between three stems open and
  equal, as in an 'm') - this has been broken since the first AFDKO. It will
  now also look in the same directory as the source font for a file named
  "fontinfo", and will attempt to add stem3 hints to the glyph which are listed
  by name in the name list for the keys "HCounterChars" or "VCounterChars".
- [autohint] Fixed old bug where it would only pay attention to the bottom four
  of the top zone specified in the FontDict BlueValues list. This results in
  more edge hints in tall glyphs.
- [autohint] Fixed special case when adding flex operator which could result in
  an endless loop
- [autohint] Added 'logOnly' option, to allow collecting report without
  changing the font.
- [autohint] Added option to specify which glyphs to exclude from autohinting.
- [autohint] Suppressed generation and use of <font-name>.plist file, unless it
  is specifically requested.
- [autohint] Fixed bug where an extremely complex glyph would overflow a buffer
  of the list of hints.
- [checkOutlines] Improved overlap detection and path orientation: it will now
  work with outlines formed by overlapping many stroke elements, as is
  sometimes done in developing CJK fonts.
- [checkOutlines] added new test for nearly vertical or horizontal lines. Fixed
  bug in this new code, reported by Erik van Blokland.
- [CompareFamily] For the warning that the Full Family name in the CFF table
  differs from that in the name table, changed it to a "Warning" rather than
  "Error", and explained that there is no functional consequence.
- [CompareFamily] Removed check that Mac names ID 16 and 17 do not exist, as
  makeotf now does make them. See notes in MakeOTF User Guide about this.
- [CompareFamily] Fixed so it works with TTF fonts again.
- [makeotf] Removed code that added a default Adobe copyright to the name table
  if no copyright is specified, and removed code to add a default trademark.
- [makeotf] Added support for the lookupflag UseMarkFilteringSet. This is
  defined in the proposed changes for OpenType spec 1.6, and is subject to
  change in definition.
- [makeotf] Dropped restriction that vmtx/VORG/vhea tables will only be written
  for CID-keyed fonts. The presence in the feature file of either a 'vrt2'
  feature of vmtx table overrides will now cause these tables to be written for
  both CID-keyed and name-keyed fonts.
- [makeotf] Added warning when a feature is referenced in the aalt feature
  definition, but either does not exist or does not contribute any rules to the
  aalt feature. The aalt feature can take only single and alternate
  substitution rules.
- [makeotf] Added support for the following lookup types:
	* GSUB type 2 Multiple Substitution
	* GSUB type 8 Reverse Chaining Single Substitution
	* GPOS type 3 Cursive Adjustment
	* GPOS type 4 Mark-to-Base Attachment
	* GPOS type 5 Mark-to-Ligature Attachment
	* GPOS type 6 Mark-to-Mark Attachment
- [makeotf] Added support for explicit definition of the GDEF table, and
  automatic creation of the GDEF when any of the lookup flag settings for
  ignoring a glyph class is used, or any mark classes are defined.
- [makeotf] Support using TTF fonts as input, to build an OpenType/TTF font,
  with the limitation that glyph order and glyph names cannot be changed. This
  is rather ugly under the hood, but it works. The MakeOTF.py Python script
  uses the tx tool to convert the TTF font to CFF data without changing glyph
  order or names. It then builds an OpenType/CFF font. It then uses the
  sfntedit tool to copy the TTF glyph data to the OpenType font, and to delete
  the CFF table.
- [makeotf] Added support for building Unicode Variation Selectors for
  CID-keyed fonts, using the new cmap subtable type 14.
- [makeotf] Fixed bug with inheritance of default rules by scripts and
  languages in feature file feature definitions. Explicitly defined languages
  were only getting default rules defined after the last script  statement, and
  when a script is named, languages of the script which are not named got no
  rules at all.
- [makeotf] Fixed bug where you could not run makeotf when the current
  directory is not the same is the source font's home directory.
- [makeotf] Set OS/2.lastChar field to U+FFFF when using mappings beyond the
  BMP.
- [makeotf] Create the Mac platform name table font menu names by the same
  rules as used for the Windows menu names. Add new keywords to the
  FontMenuNameDB file syntax. If you use the old keywords, you get the old
  format; if you use the new syntax, you get nameIDs 1, 2 and 16 and 17 just
  like for the Windows platform.
- [makeotf] Fixed bug in name table font menu names: if you entered a
  non-English Preferred name ("f=") and not a compatible family name ("c="),
  you would end up with a nameID 16 but no nameID 17.
- [makeotf] Fixed bogus 'deprecated "except" syntax' message under Windows.
- [makeotf] Fixed bug where contextual pos statements without backtrack or
  lookahead context were writing as a non-contextual rule. Reported by Karsten
  Luecke.
- [makeotf] Added new option to make stub GSUB table when no GSUB rules are
  present.
- [makeotf] Added warning if the aalt feature definition references any feature
  tags that either do not exist in the font, or do not contribute any rules
  that the aalt feature can use.
- [sfntdiff] Fixed so that only error messages are written to stderr; all
  others now written to stdout.
- [sfntdiff] Fixed bug in dump of 'name' table: when processing directories
  rather than individual files, the name table text was never updated after the
  first file for the second directory.
- [spot] Fixed option "F" to show the contextual rule sub-lookup indices, and
  to flag those which have already been used by another lookup.
- [spot] If a left side class 0 is empty, do not report it.
- [spot] For GSUB/GPOS=7 FEA dump, give each class a unique name in the entire
  font by appending the lookup ID to the class names. It was just
  "LEFTCLASS_<class index>_<subtable index>", but these names are repeated in
  every lookup. It is now
  "LEFTCLASS_c<class index>_s<subtable index>_l<lookup index>".
- [spot] When a positioning value record has more than one value, print the
  full 4 item value record. Previously, it would just print non-zero values.
  This was confusing when dumping Adobe Arabic, as you would see two identical
  values at the end of some pos rules. In fact, each of these pos rule does
  have two adjustment values, one for x and one for y advance adjustment, that
  happen to be the same numeric value.
- [spot] Fixed to write backtrack context glyphs in the right order.
- [tx] Added option to NOT clamp design coordinates to within the design space
  when snapshotting MM fonts.
- [tx] Added option to subroutinize the font when writing to CFF. This option
  is derived from the same code used by makeotfexe, but takes only about 10%
  the memory and runs much faster. This should allow subroutinizing large CJK
  fonts that makeotfexe could not handle. This is new code, so please test
  results carefully, i.e. if you use it, always check that the flattened glyphs
  outlines from the output font are identical to the flattened glyph outlines
  from the input font.
- [ttxn] Added options to suppress hinting in the font program, and version and
  build numbers.

