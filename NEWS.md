Changelog
=========

2.7.1 (released 2018-XX-XX)
---------------------------

- Implemented an integration testing framework
  ([#346](https://github.com/adobe-type-tools/afdko/pull/346))
- \[ttxn\] Fixed ClassRecord AttributeError
  ([#350](https://github.com/adobe-type-tools/afdko/issues/350))
- \[ttxn\] Trailing spaces are now stripped from script and language tags
- \[tx\] Get blended glyph stems when the output is not CFF2
  ([#378](https://github.com/adobe-type-tools/afdko/pull/378))
- \[spot\] Fixed crash due to buffer overrun errors from long glyph names
  ([#373](https://github.com/adobe-type-tools/afdko/issues/373))
- \[ProofPDF\] Added 'pageIncludeTitle' option
  ([#379](https://github.com/adobe-type-tools/afdko/pull/379))
- \[ProofPDF\] Removed search for 'CID charsets' folder
  ([#368](https://github.com/adobe-type-tools/afdko/pull/368))
- Removed **CID charsets** folder and its contents
  ([#264](https://github.com/adobe-type-tools/afdko/issues/264),
  [#368](https://github.com/adobe-type-tools/afdko/pull/368))
- \[ProofPDF\] Fixed broken 'lf' option (CID layout)
  ([#382](https://github.com/adobe-type-tools/afdko/issues/382))
- \[ProofPDF\] Fixed crash when font has no BlueValues
  ([#394](https://github.com/adobe-type-tools/afdko/issues/394))
- \[makeinstancesufo\] Disabled ufonormalizer's writeModTimes option;
  Fixed Windows command
  ([#413](https://github.com/adobe-type-tools/afdko/pull/413))
- \[ufoTools\] Fixed line breaks when writting UFOs files on Windows
  ([#413](https://github.com/adobe-type-tools/afdko/pull/413))
- \[makeotf\] Implemented correct exit codes
  ([#417](https://github.com/adobe-type-tools/afdko/issues/417))
- \[tx\] Fixed Windows crash
  ([#195](https://github.com/adobe-type-tools/afdko/issues/195))
- \[tx\] Fixed crash handling copyright symbol in UFO trademark string
  ([#425](https://github.com/adobe-type-tools/afdko/pull/425))
- \[makeotf\] Ignore trailing slashes in input font path
  ([#280](https://github.com/adobe-type-tools/afdko/issues/280))

2.7.0 (released 2018-05-09)
---------------------------

- \[CheckOutlinesUFO\] Replaced Robofab's pens with FontPens'
  ([#230](https://github.com/adobe-type-tools/afdko/issues/230))
- Removed `extractSVGTableSVGDocs.py` and
  `importSVGDocsToSVGTable.py`. These are superseded by the scripts at
  <https://github.com/adobe-type-tools/opentype-svg/>
- Removed `cmap-tool.pl`, `fdarray-check.pl`, `fix-fontbbox.pl`,
  `glyph-list.pl`, `hintcidfont.pl`, `setsnap.pl` and `subr-check.pl`.
  These Perl scripts are available from
  <https://github.com/adobe-type-tools/perl-scripts>
- Removed **CID\_font\_support** folder and its contents.
- \[tx\] Fixed assert "out of range region index found in item
  variation store subtable"
  ([#266](https://github.com/adobe-type-tools/afdko/pull/266))
- \[makeotfexe\] Fixed unnecessary truncation of the Format 4 'cmap'
  subtable
  ([#242](https://github.com/adobe-type-tools/afdko/issues/242))
- \[buildCFF2VF\] Fix support for CFF2 fonts with multiple FontDicts
  ([#279](https://github.com/adobe-type-tools/afdko/pull/279))
- \[ttxn\] Update for latest FontTools version
  ([#288](https://github.com/adobe-type-tools/afdko/pull/288))
- New `ttfcomponentizer` tool that componentizes TrueType fonts using
  the component data of a related UFO font.
- Added 64-bit support for Mac OSX and Linux
  ([#271](https://github.com/adobe-type-tools/afdko/pull/271),
  [#312](https://github.com/adobe-type-tools/afdko/pull/312),
  [#344](https://github.com/adobe-type-tools/afdko/pull/344))
- \[tx\] Fixed -dcf mode failing to dump hinted CFF2 variable font
  ([#322](https://github.com/adobe-type-tools/afdko/issues/322))
- \[tx\] Fixed conversion of multi-FD CFF2 font to CID-flavored CFF
  font ([#329](https://github.com/adobe-type-tools/afdko/issues/329))
- \[tx\] Fixed -cff2 failing to convert CID/CFF1 to multi-FD CFF2
  ([#345](https://github.com/adobe-type-tools/afdko/issues/345))
- Wheels for all three environments (macOS, Windows, Linux) are now
  available on [PyPI](https://pypi.org/project/afdko) and
  [GitHub](https://github.com/adobe-type-tools/afdko/releases)

2.6.25 (released 2018-01-26)
----------------------------

This release fixes the following issues:

- \[CheckOutlinesUFO\] Skip glyphs whose names are referenced in the
  UFO's lib but do not exist
  ([#228](https://github.com/adobe-type-tools/afdko/issues/228))
- Partial Python 3 support in `BezTools.py`, `ConvertFontToCID.py`,
  `FDKUtils.py`, `MakeOTF.py`, `StemHist.py`, `autohint.py`,
  `buildMasterOTFs.py` and `ufoTools.py`
  ([#231](https://github.com/adobe-type-tools/afdko/issues/231),
  #232, #233)
- \[makeotfexe\] Fixed parsing of Character Variant (cvXX) feature
  number
  ([#237](https://github.com/adobe-type-tools/afdko/issues/237))
- \[pip\] Fixed `pip uninstall afdko`
  ([#241](https://github.com/adobe-type-tools/afdko/issues/241))

2.6.22 (released 2018-01-03)
----------------------------

The **afdko** has been restructured so that it can be installed as a
Python package. It now depends on the user's Python interpreter, and no
longer contains its own Python interpreter.

In order to do this, the two Adobe-owned, non-opensource programs were
dropped: `IS` and `checkOutlines`. If these turn out to be sorely
missed, an installer for them will be added to the old Adobe afdko
website. The current intent is to migrate the many tests in
checkOutlines to the newer `checkOutlinesUFO` (which does work with
OpenType and Type 1 fonts, but currently does only overlap detection and
removal, and a few basic path checks).

Older releases can be downloaded and installed from the [Adobe's AFDKO
home page](http://www.adobe.com/devnet/opentype/afdko.html).

2.5.66097 (released 2017-12-01)
-------------------------------

This only lists the major bug fixes since the last release. For a
complete list see:
<https://github.com/adobe-type-tools/afdko/commits/master>

- \[buildCFF2VF\] Add version check for fontTools module: only
  starting with version 3.19.0 does fontTools.cffLib build correct
  PrivateDict BlueValues when there are master source fonts not at the
  ends of the axes.
- \[makeotfexe\] Support mapping a glyph to several Unicode values.
  This can now be done by providing the the UV values as a
  comma-separated list in the third field of the GlyphOrderAndAliasDB
  file, in the 'uniXXXXX' format.
- \[makeotfexe\] Fixed a crashing bug that can happen when the
  features file contains duplicate class kern pairs. Reported by
  Andreas Seidel in email.
- \[makeotfexe\] Add fatal messages if a feature file 'cvParameters'
  block is used in anything other than a Character Variant (cvXX)
  feature, or if a 'featureNames' block is used in anything other
  than a Stylistic Set (ssXX) feature.
- \[makeotfexe\] Relaxed restrictions on name table name IDs. Only 2
  and 6 are now reserved for the implementation.
- \[makeotfexe\] Fixed bug where use of the 'size' feature in
  conjunction with named stylistic alternates would cause the last
  stylistic alternate name to be replaced by the size feature menu
  name. Incidentally removed old patent notices.
- \[makeotfexe\] Restored old check for the fatal error of two
  different glyphs being mapped to the same character encoding.
- \[makeotfexe\] If the last line of a GOADB file did not end in a
  new-line, makeotf quit, saying the line was too long.
- \[otf2otc\] Can now take a single font as input, and can take an OTC
  font as input.
- \[sfntdiff\] Fixed old bug: it used to crash if no file path or only
  one file path was provided.
- \[sfntdiff\] Changed behavior: it now returns non-zero only if a
  real error occurs; it used to return non-zero when there was a
  difference between the fonts.
- \[spot\] Fixed old bug: a PrivateDict must exist, but it is legal
  for it to have a length of 0.
- \[tx *et al.*\] Add support for reading and writing blended hints
  from/to CFF2.

2.5.65811 (released 2017-04-27)
-------------------------------

- \[makeInstancesUFO\] Preserve public.postscriptNames lib key.
- \[makeInstancesUFO\] Do not use postscriptFontName attribute.
- \[makeotf\] New option -V to print MakeOTF.py script version.
- \[tx\] Added new option '-maxs', to set the maximum number of
  subroutines allowed. The default is now 32K, as some legacy systems
  cannot support more than this.
- \[tx\] Add improved CFF2 support: tx can now use the option -cff2 to
  write from a CFF2 font (in a complete OpenType font) to a file
  containing the output CFF2 table, with full charstring optimization
  and subroutinization. This last option is still work in progress: it
  has not been extensively tested, and does yet support blended hints.
- \[tx\] Several bug fixes for overlap removal.
- \[tx\] Fixed bug in subroutinization that could leave a small number
  of unused subroutines in the font. This cleanup is optional, as it
  requires 3x the processing speed with the option than without, and
  typically reduces the font size by less than 0.5 percent.
- \[ttxn\] Option '-nv' will now print name IDs 3 and 5, but with
  the actual version number replaced by the string "VERSION
  SUPPRESSED".
- \[ufoTools\] FIx autohint bug with UFO fonts: if edit a glyph and
  re-hint, autohint uses old processed layer glyph.

2.5.65781 (released 2017-04-03)
-------------------------------

- \[variable fonts\] **buildMasterOTFs** new script to build OTF font
  files from UFO sources, when building variable fonts.
- \[variable fonts\] **buildCFF2VF** new command to build a CFF2
  variable font from the master OTF fonts.
- \[autohint\] Fixed bug introduced by typo on Dec 1 2015. Caused
  BlueFuzz to always be set to 1. Rarely causes problems, but found it
  with font that sets BlueFuzz to zero; with BlueFuzz set to 1, some
  of the alignment zones were filtered out as being closer than
  BlueFuzz\*3.
- \[autohint\] Fixed long-standing bug with UFO fonts where if a glyph
  was edited after being hinted, running autohint would process and
  output only the old version of the glyph from the processed layer.
- \[CheckOutlinesUFO\] Added "quiet mode" option.
- \[CheckOutlinesUFO\] Fixed a bug where logic could try and set an
  off-curve point as a start point.
- \[CheckOutlinesUFO\] Changed the logic for assigning contour order
  and start point. The overlap removal changes both, and
  checkOutlinesUFO makes some attempt to restore the original state
  when possible. These changes will result in different contour order
  and start points than before the change, but fixes a bug, and will
  usually produce the same contour order and start point in fonts that
  are generated as instances from a set of master designs. There will
  always be cases where there will be some differences.
- \[MakeOTF\] Replace old logic for deriving relative paths with
  python function for the same.
- \[MakeOTF\] When converting Type 1 to CID in makeotf, the logic in
  mergeFonts and ConvertFontToCID.py was requiring the FDArray
  FontDicts to have keys, like FullName, that are not in fact
  required, and are often not present in the source fonts. Fixed both
  mergeFonts and ConvertFontToCID.py.
- \[MakeOTF\] By default, makeotf will add a minimal stub DSIG table
  in release mode. The new options '-addDSIG' and '-omitDSIG' will
  force makeotf to either add or omit the stub DSIG table. This
  function was added because the Adobe Type group is discontinuing
  signing font files.
- \[makeotfexe\] Fixed bug in processing UVS input file for makeotf
  for non-CID fonts.
- \[makeotfexe\] Fixed bug where makeotf would reject a nameID 25
  record when specified in a feature file. This nameID value used to
  be reserved, but is now used for overriding the postscript family
  named used with arbitrary instances in variable fonts.
- \[mergeFonts\] Removed requirement for mergeFonts that each FontDict
  have a FullName, Weight, and Family Name. This fixes a bug in using
  mergeFonts with UFO sources and converting to CID-keyed output font.
  Developers should not have to put these fields in the source fonts,
  since they are not required.
- \[spot\] Fixed bug in name table dump: Microsoft platform language
  tags for Big5 and PRC were swapped.
- \[stemHist\] Removed debug print line, that caused a lot of annoying
  output, and was left in the last update by accident.
- \[tx\] When getting Unicode values for output, the presence of UVS
  cmap meant that no UV values were read from any other cmap subtable.
  I fixed this bug, but 'tx' still does not support reading and
  showing UVS values. Doing so will be a significant amount of work,
  so I am deferring that to my next round of FDK work.
- \[tx\] Added support for CFF2 variable fonts as source fonts: when
  using -t1 or -cff, these will be snapshotted to an instance. If no
  user design vector (UDV) argument is supplied, then the output will
  be the default data. If a UDV argument is supplied with the option
  -U, then the instance is built at the specified point in the design
  space.
- \[tx\] Added new option +V/-V to remove overlaps in output Type 1
  fonts (mode -t1) and CFF fonts (mode -cff). This is still
  experimental.
- \[tx\] Made the subroutinizer a lot faster; the speed bump is quite
  noticeable with CJK fonts. (by Ariza Michiharu)
- \[tx\] Added new option (+V/-V) to remove overlaps. (by Ariza
  Michiharu)
- \[ttx\] Updated to version 3.9.1 of the fontTools module from master
  branch on GitHub.

2.5.65322 (released 2016-05-27)
-------------------------------

- \[CMAP files\] Updated UniCNS-UTF32-H to v1.14
- \[build\] Made changes to allow compiling under Xcode 7.x and OSX
  10.11
- \[documentation\] Fixed a bunch of errors in the Feature File spec.
  My thanks to Sascha Brawer, who has been reviewing it carefully. See
  the issues at
  <https://github.com/adobe-type-tools/afdko/issues/created_by/brawer>.
- \[autohint\] Fixed support for history file, which can be used with
  non-UFO fonts only. This has been broken since UFO support was
  added.
- \[autohintexe\] Fixed really old bug: ascenders and descenders get
  dropped from the alignment zone report if they are a) not in an
  alignment zone and b) there is an overlapping smaller stem hint.
  This happened with a lot of descenders.
- \[checkOutlines\] Fixed bug in ufoTools.py that kept checkOutlines
  (NOT checkOutlinesUFO) from working with a UFO font.
- \[checkOutlines\] Fixed bug which misidentified orientation of path
  which is very thin and in part convex. I am a bit concerned about
  the solution, as what I did was to delete some logic that was used
  to double-check the default rules for determining orientation.
  However, the default logic is the standard way to determine
  orientation and should always be correct. The backup logic was
  definitely not always correct as it was applied only to a single
  point, and was correct only if the curve associated with the point
  is concave. It is in fact applied at several different points on a
  path, with the majority vote winning. Since the backup logic is used
  only when a path is very thin, I suspect that it was a sloppy
  solution to fix a specific case. The change was tested with several
  large fonts, and found no false positives.
- \[makeInstances\] Fixed bug which produced distorted shapes for
  those glyphs which were written with the Type 1 'seac' operator,
  a.k.a. Type 1 composite glyphs.
- \[makeotfexe\] Fixed bug where using both kern format A and B in a
  single lookup caused random values to be assigned.
- \[makeotfexe\] Fixed bug where a format A kern value (a single
  value) would be applied to X positioning rather than Y positioning
  for the features 'vkrn'. Applied same logic to vpal, valt, and
  vhal.
- \[makeotfexe\] Finally integrated Georg Seifert's code for
  supporting hyphen in development glyph names. This version differs
  from Georg's branch in that it does not allow any of the special
  characters in final names (i.e. the left side names in the
  GlyphAliasAndOrderDB). However, allowing this is a smaller tweak
  than it used to be: just use the same arguments in
  `cb.c:gnameFinalScan()` as in `gnameDevScan()`. This update also
  includes Georg's changes for allow source fonts to have CID names
  in the form 'cidNNNN'.
- \[ConvertToCID\] Fixed bug that the script expected in several
  places that the fontinfo file would contain at least one user
  defined FontDict.
- \[ConvertToCID\] Fixed bug that the script expected that the source
  font would have Weight and Adobe Copyright fields in the font dict.
- \[makeotf\] Fixed a bug that kept the '-nS' option for having any
  effect when the '-cn' option is used.
- \[makeotfexe\] Remove use of 'strsep()'; function is not defined
  in the Windows C library.
- \[makeotf\] Fixed bug in removing duplicate and conflicting entries.
  Changed logic to leave the first pair defined out of a set of
  duplicate or conflicting entries.
- \[makeotfexe\] Fixed bug in processing GDEF glyph class statements:
  if multiple GlyphClass statements were used; the additional glyphs
  were added to a new set of 4 glyph classes, rather than merged with
  the allowed 4 glyph classes.
- \[makeotfexe\] Fixed issue in GDEF definition processing. Made it an
  error to specify both LigCaretByPosition and LigCaretByIndex for a
  glyph.
- \[makeotfexe\] Corrected error message: language and system
  statements are allowed in named lookups within a feature definition,
  but are not allowed in stand-alone lookups.
- \[makeotf\] Corrected typo in MakeOTF.py help text about what the
  default source font path.
- \[makeotfexe\] Fixed an old bug in makeotf. If a mark-to-base or
  mark-to-mark lookup has statements that do not all reference the
  same mark classes, makeotfexe used to write a 'default' anchor
  attachment point of (0.0) for any mark class that was not referenced
  by a given statement. Fixed this bug by reporting a fatal error: the
  feature file must be re-written so that all the statements in a
  lookup must all reference the same set of mark classes.
- \[makeotf\] Suppressed warning about not using GOADB file when
  building a CID font. Some of the changes I made a few weeks ago to
  allow building fonts with CIDs specified as glyphs names with the
  form 'cidNNNNN' allowed this warning to be be shown, but it is not
  appropriate for CID-keyed fonts.
- \[makeotf\] Fixed old bug where using option -'cn' to convert a
  non-CID source font to CID would cause a mismatch between the maxp
  table number of glyphs and the number of glyph actually in the output
  font, because the conversion used the source font data rather than
  the first pass name-keyed OTF which had been subject to glyph
  subsetting with the GOADB file.
- \[makeotf\] Fixed bug in reading UVS files for non\_CID fonts.
- Fixed copyright statements that are incompatible with the open
  source license. Thanks to Dmitry Smirnov for pointing these out.
  These were in some make files, an example Adobe CMAP file, and some
  of the technical documentation.
- Fixed typos in help text in ProofPDF.py. Thank you Arno Enslin.
- \[ttxn\] Fixed bug in ttxn.py that broke it when dumping some
  tables, when used with latest fontTools library.
- \[tx\] Fixed bug in rounding fractional values when flattening
  library elements, used in design of CJK fonts.
- \[tx\] Fixed bug in handling FontDict FontMatrix array values: not
  enough precision was used, so that 1/2048 was written as 1/2049 in
  some cases.
- \[tx\] Fixed bug in reading UFO fonts, so that glyphs with no
  `<outline>` element and with a `<lib>` element would be skipped.
- \[tx\] Minor code changes to allow 'tx' to compile as a 64 bit
  program.
- \[tx\] Fixed bug in dumping AFM format data, introduced when tx was
  updated to be 64 bit.
- \[tx\] Fixed bug in processing seac, introduced in work on rounding
  fractional values.
- \[tx\] Fixed bug in writing AFM files: -1 value would be written as
  4294967295 instead of -1.
- \[tx\] Added option -noOpt, renamed blend operator from 'reserved'
  to 'blend'. This was done in order to support experiments with
  multiple master fonts.
- \[tx\] When reading a UFO font: if it has no Postscript version
  entry, set the version to 1.0.
- \[tx\] When writing a UFO font: if StemSnap\[H,V\] are missing, but
  Std\[H,V\]W are present, use the Std\[H,V\]W values to supply the
  UFO's postscript StemSnap\[H,V\] values.
- \[tx\] Fixed old bug with rounding decimal values for BlueScale is
  one of the few Postscript values with several places of decimal
  precision. It is stored as an ASCII text decimal point number in T1,
  T2, and UFO files, but is stored internally as a C 'float' value
  in some programs. Real values in C cannot exactly represent all
  decimal values. For example, the closest that a C 'float' value
  can come to "0.375" is "0.03750000149".When writing output
  fonts, tx was writing out the latter value in ASCII text, rather
  than rounding back to 0.0375. Fixed by rounding to 8 decimal places
  on writing the value out. This bug had no practical consequences, as
  0.0375 and 0.03750000149 both convert to exactly the same float
  value, but was annoying, and could cause rounding differences in any
  programs that use higher precision fields to hold the BlueScale
  value.

2.5.65012 (released 2015-12-01)
-------------------------------

- \[makeotf\] Fixed bug that kept makeotfexe from building fonts with
  spaces in the path.
- \[ConvertFontToCID\] Fixed bug that kept makeotf from converting UFO
  fonts to CID.
- \[makeotf\] Changed support for Unicode Variation Sequence file
  (option '-ci') so that when used with name-keyed fonts, the
  Region-Order field is omitted, and the glyph name may be either a
  final name or developer glyph name. Added warning when glyph in the
  UVS entry is not found in font. See MakeOTF User's Guide.
- \[makeotfexe\] now always makes a cmap table subtable MS platform,
  Unicode, format 4 for CID fonts. This is required by Windows. If
  there are no BMP Unicode values, then it makes a stub subtable,
  mapping GID 0 to UVS 0.
- \[tx *et al.*\] When reading a UFO source font, do not complain if
  the fontinfo.plist entry `styleName` is present but has an empty
  string. This is valid, and is common when the style is **Regular**.

2.5.64958 (released 2015-11-22)
-------------------------------

- \[autohint/tx\] Switched to using new text format that is
  plist-compatible for T1 hint data in UFO fonts. See header of
  ufoTools.py for format.
- \[autohint\] Finally fixed excessive generation of flex hints. This
  has been an issue for decades, but never got fixed because it did
  not show up anywhere as a problem. The last version of makeotf
  turned on parsing warnings, and so now we notice.
- \[checkOutlinesUFO\] Fixed bug where abutting paths did not get
  merged if there were no changes in the set of points.
- \[checkOutlinesUFO\] Fixed bug where a `.glif` file without an
  `<outline>` element was treated as fatal error. It is valid for the
  `<outline>` element to be missing.
- \[checkOutlines\] Changed -I option so that it also turns off
  checking for tiny paths. Added new option -5 to turn this check back
  on again.
- \[checkOutlinesUFO\] Increased max number of paths in a glyph from
  64 to 128, per request from a developer.
- \[CompareFamily\] Fixed old bug in applying ligature width tests for
  CID fonts, and fixed issue with fonts that do not have Mac name
  table names. The logic now reports missing Mac name table names only
  if there actually are some: if there are none, these messages are
  suppressed.
- \[fontplot/waterfallplot/hintplot/fontsetplot\] Fixed bugs that
  prevented these from being used with CID-keyed fonts and UFO fonts.
  Since the third party library that generates the PDF files is very
  limited, I did this by simply converting the source files to a
  name-keyed Type 1 temporary font file, and then applying the tools
  the temporary file.
- \[makeInstancesUFO\] Added a call to the ufonormalizer tool for each
  instance. Also added a call to the defcon library to remove all
  private lib keys from lib.py and each glyph in the default layer,
  excepting only "public.glyphOrder".
- Fixed typos in MakeOTF User Guide reported by Gustavo Ferreira
- \[MakeOTF\] Increased max number of directories to look upwards when
  searching for GOADB and FontMenuNameDB files from 2 to 3.
- \[MakeOTF/makeotfexe\] Added three new options:

  - `omitMacNames` and `useMacNames` write only Windows platform
    menu names in name table, apart from the names specified in
    the feature file. `useMacNames` writes Mac as well as
    Windows names.
  - `overrideMenuNames` allows feature file name table entries
    to override default values and the values from the
    FontMenuNameDB for name IDs. NameIDs 2 and 6 cannot be
    overridden. Use this with caution, and make sure you have
    provided feature file name table entries for all platforms.
  - `skco`/`nskco` do/do not suppress kern class optimization by
    using left side class 0 for non-zero kern values. Optimizing
    saves a few hundred to thousand bytes, but confuses some
    programs. Optimizing is the default behavior, and previously
    was the only option.

- \[MakeOTF\] Allow building an OTF from a UFO font only. The internal
  `features.fea` file will be used if there is no `features` file in
  the font's parent directory. If the GlyphAliasAndOrderDB file is
  missing, only a warning will be issued. If the FontMenuNameDB is
  missing, makeotf will attempt to build the font menu names from the
  UFO fontinfo file, using the first of the following keys found:
  `openTypeNamePreferredFamilyName`, `familyName`, the family name
  part of the `postScriptName`, and finally the value
  **NoFamilyName**. For style, the keys are:
  `openTypeNamePreferredSubfamilyName`, `styleName`, the style name
  part of the `postScriptName`, and finally the value **Regular**.
- \[MakeOTF\] Fixed bug where it allowed the input and output file
  paths to be the same.
- \[makeotfexe\] Extended the set of characters allowed in glyph names
  to include `+ * : ~ ^ !`.
- \[makeotfexe\] Allow developer glyph names to start with numbers;
  final names must still follow the PS spec.
- \[makeotfexe\] Fixed crashing bug with more than 32K glyphs in a
  name-keyed font, reported by Gustavo Ferreira.
- \[makeotfexe\] Merged changes from Khaled Hosny, to remove
  requirement that 'size' feature menu names have Mac platform
  names.
- \[makeotfexe\] Code maintenance in generation of the feature file
  parser. Rebuilt the 'antler' parser generator to get rid of a
  compile-time warning for zzerraction, and changed featgram.g so that
  it would generate the current featgram.c, rather than having to edit
  the latter directly. Deleted the object files for the 'antler'
  parser generator, and updated the read-me for the parser generator.
- \[makeotfexe\] Fixed really old bug: relative include file
  references in feature files have not worked right since the FDK
  moved from Mac OS 9 to OSX. They are now relative to the parent
  directory of the including feature file. If that is not found, then
  makeotf tries to apply the reference as relative to the main feature
  file.
- \[spot\] Fixed bug in dumping stylistic feature names.
- \[spot\] Fixed bug proofing vertical features: needed to use vkern
  values. Fix contributed by Hiroki Kanou.
- \[tx *et all.*\] Fix crash when using '-gx' option with source UFO
  fonts.
- \[tx *et all.*\] Fix crash when a UFO glyph point has a name
  attribute with an empty string.
- \[tx *et all.*\] Fix crash when a UFO font has no public.glyphOrder
  dict in the lib.plist file.
- \[tx *et all.*\] Fix really old bug in reading TTF fonts, reported
  by Belleve Invis. TrueType glyphs with nested component references
  and x/y offsets or translation get shifted.
- \[tx *et all.*\] Added new option '-fdx' to select glyphs by
  excluding all glyphs with the specified FDArray indices. This and
  the '-fd' option now take lists and ranges of indices, as well as
  a single index value.
- Added a command to call the ufonormalizer tool.
- Updated to latest version of booleanOperatons, defcon (ufo3 branch),
  fontMath (ufo3 branch), fontTools, mutatorMath, and robofab (ufo3
  branch). The AFDKO no longer contains any private branches of third
  party modules.
- Rebuilt the Mac OSX, Linux and Windows Python interpreters in the
  AFDKO, bringing the Python version up to 2.7.10. The Python
  interpreters are now built for 64-bit systems, and will not run on
  32-bit systems.

2.5.64700 (released 2015-08-04)
-------------------------------

- \[ufoTools\] Fixed bug that was harmless but annoying. Every time
  that `autohint -all` was run, it added a new program name entry to
  the history list in the hashmap for each processed glyph. You saw
  this only if you opened the hashmap file with a text editor, and
  perhaps eventually in slightly slower performance.
- \[checkOutlinesUFO\] Fixed bug where presence of outlines with only
  one or two points caused a stack dump.
- \[makeotf\] Fixed bug reported by Paul van der Laan: failed to build
  TTF file when the output file name contains spaces.
- \[spot\] Fixed new bug that caused spot to crash when dumping GPOS
  'size' feature in feature file format.

2.5.64655 (released 2015-07-17)
-------------------------------

- \[ufoTools\] Fixed bug which placed a new hint block after a flex
  operator, when it should be before.
- \[autohint\] Fixed new bug in hinting non-UFO fonts, introduced by
  the switch to absolute coordinates in the bez file interchange
  format.
- \[ufoTools\] Fixed bugs in using hashmap to detect previously hinted
  glyphs.
- \[ufoTools\] Fixed bugs in handling the issue that
  checkOutlinesUFO.py (which uses the defcon library to write UFO glif
  files) will in some cases write glif files with different file names
  than they had in the default glyph layer.
- \[makeotf\] Fixed bug with Unicode values in the absolute path to to
  the font home directory.
- \[makeotf\] Add support for Character Variant (cvXX) feature params.
- \[makeotf\] Fixed bug where setting Italic style forced OS/2 version
  to be 4.
- \[spot\] Added support for cvXX feature params.
- \[spot\] Fixed in crash in dumping long contextual substitution
  strings, such as in 'GentiumPlus-R.TTF'.
- \[tx\] Fixed bug in handling CID glyph ID greater than 32K.
- \[tx\] Changed to write widths and FontBBox as integer values.
- \[tx\] Changed to write SVG, UFO, and dump coordinates with 2 places
  of precision when there is a fractional part.
- \[tx\] Fixed bugs in handling the '-gx' option to exclude glyphs.
  Fixed problem with CID \> 32K. Fixed problem when font has 65536
  glyphs: all glyphs after first last would be excluded.
- \[tx\] Fixed rounding errors in writing out decimal values to cff
  and t1 fonts.
- \[tx\] Increased interpreter stack depth to allow for CUBE operators
  (Library elements) with up to 9 blend axes.
- Fixed Windows builds: had to provide a roundf() function, and more
  includes for the \_tmpFile function. Fixed a few compile errors.
- Fixed bug in documentation for makeInstancesUFO.
- Fixed bug in BezTools.py on Windows, when having to use a temp file.

2.5.64261 (released 2015-05-26)
-------------------------------

- \[autohintexe\] Worked through a lot of problems with fractional
  coordinates. In the previous release, autohintexe was changed to
  read and write fractional values. However, internal value storage
  used a Fixed format with only 7 bits of precision for the value.
  This meant that underflow errors occurred with 2 decimal places,
  leading to incorrect coordinates. I was able to fix this by changing
  the code to use 8 bits of precision, which supports 2 decimal places
  (but not more!) without rounding errors, but this required many
  changes. The current autohint output will match the output of the
  previous version for integer input values, with two exceptions.
  Fractional stem values will (rarely) differ in the second decimal
  place. The new version will also choose different hints in glyphs
  which have coordinate values outside of the range -16256 to +16256;
  the previous version had a bug in calculating weights for stems.
- \[autohint\] Changed logic for writing bez files to write absolute
  coordinate values instead of relative coordinate values. Fixed bug
  where truncation of decimal values lead to cumulative errors in
  positions adding up to more than 1 design unit over the length of a
  path.
- \[tx\] Fixed bugs in handling fractional values: `tx` had a bug with
  writing fractional values that are very near an integer value for
  the modes -dump, -svg, and -ufo. `tx` also always applied the logic
  for applying a user transform matrix, even though the default
  transform is the identity transform. This has the side-effect of
  rounding to integer values.

2.5.64043 (released 2015-04-08)
-------------------------------

- \[checkOutlinesUFO\] Added new logic to delete any glyphs from the
  processed layer which are not in the 'glyphs' layer.
- \[makeotf\] When building CID font, some error messages were printed
  twice.
- \[makeotf\] Added new option `stubCmap4`. This causes makeotf to
  build only a stub cmap 4 subtable, with just two segments. Needed
  only for special cases like AdobeBlank, where every byte is an
  issue. Windows requires a cmap format 4 subtable, but not that it be
  useful.
- \[makeCIDFont\] Output FontDict was sized incorrectly. A function at
  the end adds some FontInfo keys, but did not increment the size of
  the dict. Legacy logic is to make the FontInfo dict be 3 larger than
  the current number of keys.
- \[makeInstanceUFO\] Changed AFDKO's branch of mutatorMath so that
  kern values, glyph widths, and the BlueValues family of global hint
  values are all rounded to integer even when the `decimal` option is
  used.
- \[makeInstanceUFO\] Now deletes the default 'glyphs' layer of the
  target instance font before generating the instance. This solves the
  problem that when glyphs are removed from the master instances, the
  instance font still has them.
- \[makeInstanceUFO\] Added a new logic to delete any glyphs from the
  processed layer which are not in the 'glyphs' layer.
- \[makeInstanceUFO\] Removed the `all` option: even though
  mutatorMath rewrites all the glyphs, the hash values are still valid
  for glyphs which have not been edited. This means that if the
  developer edits only a few glyphs in the master designs, only those
  glyphs in the instances will get processed by checkOutlinesUFO and
  autohint.
- Support fractional coordinate values in UFO workflow:

  - checkOutlinesUFO (but not checkOutlines), autohint, and
    makeInstancesUFO will now all pass through decimal
    coordinates without rounding, if you use the new option
    "decimal". tx will dump decimal values with 3 decimal
    places.
  - tx already reported fractional values, but needed to be
    modified to report only 3 decimal places when writing UFO
    glif files, and in PDF output mode: Acrobat will not read
    PDF files with 9 decimal places in position values.
  - This allows a developer to use a much higher precision of
    point positioning without using a larger em-square. The
    Adobe Type group found that using an em-square of other than
    1000 design units still causes problems in layout and text
    selection line height in many apps, despite it being legal
    by the Type 1 and CFF specifications.
  - Note that code design issues in autohint currently limit the
    decimal precision and accuracy to 2 decimal places: 1.01
    works but 1.001 will be rounded to 0.

2.5.63782 (released 2015-03-03)
-------------------------------

- \[tx\] Fix bug in reading TTFs. Font version was taken from the name
  table, which can include a good deal more than just the font
  version. Changed to read fontRevision from the head table.
- \[detype1\] Changed to wrap line only after an operator name, so
  that the coordinates for a command and the command name would stay
  on one line.
- \[otf2otc\] Pad table data with zeros so as to align tables on a 4
  boundary. Submitted by Cosimo Lupo.

2.5.63718 (released 2015-02-21)
-------------------------------

- \[ufoTools\] Fixed a bug with processing flex hints that caused
  outline distortion.
- \[compareFamily\] Fixed bug in processing hints: it would miss
  fractional hints, and so falsely report a glyph as having no hints.
- \[compareFamily\] Support processing CFF font without a FullName
  key.
- \[checkOutlinesUFO\] Coordinates are written as integers, as well as
  being rounded.
- \[checkOutlinesUFO\] Changed save function so that only the
  processed glyph layer is saved, and the default layer is not
  touched.
- \[checkOutlinesUFO\] Changed so that XML type is written as
  'UTF-8' rather than 'utf-8'. This was actually a change in the
  FontTools xmlWriter.py module.
- \[checkOutlinesUFO\] Fixed typos in usage and help text.
- \[checkOutlinesUFO\] Fixed hash dictionary handling so that it will
  work with autohint, when skipping already processed glyphs.
- \[checkOutlinesUFO\] Fixed false report of overlap removal when only
  change was removing flat curve
- \[checkOutlinesUFO\] Fixed stack dump when new glyph is seen which
  is not in hash map of previously processed glyphs.
- \[checkOutlinesUFO\] Added logic to make a reasonable effort to sort
  the new glyph contours in the same order as the source glyph
  contours, so the final contour order will not depend on (x,y)
  position. This was needed because the pyClipper library (which is
  used to remove overlaps) otherwise sorts the contours in (x,y)
  position order, which can result in different contour order in
  different instance fonts from the same set of master fonts.
- \[makeInstancesUFO\] Changed so that the option -i (selection of
  which instances to build) actually works.
- \[makeInstancesUFO\] Removed dependency on the presence of
  instance.txt file.
- \[makeInstancesUFO\] Changed to call checkOutlinesUFO rather than
  checkOutlines
- \[makeInstancesUFO\] Removed hack of converting all file paths to
  absolute file paths: this was a work-around for a bug in
  robofab-ufo3k that is now fixed.
- \[makeInstancesUFO\] Removed all references to old instances.txt
  meta data file.
- \[makeInstancesUFO\] Fixed so that current dir does not have to be
  the parent dir of the design space file.
- Merged fixes from the GitHub AFDKO open source repo.
- Updated to latest version defcon, fontMath, robofab, and
  mutatorMath.
- Fix for Yosemite (Mac OSX 10.10) in FDK/Tools/setFDKPaths. When an
  AFDKO script is ran from another Python interpreter, such as the one
  in RoboFont, the parent Python interpreter may set the Unix
  environment variables PYTHONHOME and PYTHONPATH. This can cause the
  AFDKO Python interpreter to load some modules from its own library,
  and others from the parent interpreters library. If these are
  incompatible, a crash ensues. The fix is to unset the variables
  PYTHONHOME and PYTHONPATH before the AFDKO interpreter is called.
  Note: As a separate issue, under Mac OSX 10.10, Python calls to FDK
  commands will only work if the calling app is run from the
  command-line (e.g: `open /Applications/RoboFont.app`), and the
  argument `shell="True"` is added to the subprocess module call to
  open a system command. I favor also adding the argument
  `stderr=subprocess.STDOUT`, else you will not see error messages
  from the Unix shell. Example:
  `log = subprocess.check_output( "makeotf -u", stderr=subprocess.STDOUT, shell=True)`.

2.5.63408 (released 2014-12-02)
-------------------------------

- \[spot\] Fixed error message in GSUB chain contextual 3 proof file
  output; was adding it as a shell comment to the proof output,
  causing conversion to PDF to fail.
- \[makeotf\] Increased the limit for glyph name length from 31 to 63
  characters. This is not encouraged in shipping fonts, as there may
  be text engines that will not accept glyphs with more than 31
  characters. This was done to allow building test fonts to look for
  such cases.

2.5.63209 (released 2014-09-18)
-------------------------------

- \[makeInstancesUFO\] Added new script to build instance fonts from
  UFO master design fonts. This uses the design space XML file
  exported by Superpolator 3 in order to define the design space, and
  the location of the masters and instance fonts in the design space.
  The definition of the format of this file, and the library to use
  the design space file data, is in the open source mutatorMath
  library on GitHub, and maintained by Erik van Blokland. There are
  several advantages of the Superpolator design space over the
  previous **makeInstances** script, which uses the Type 1 Multiple
  Master font format to hold the master designs. The new version a)
  allows different master designs and locations for each glyph, and b)
  allows master designs to be arbitrarily placed in the design space,
  and hence allows intermediate masters. In order to use the
  mutatorMath library, the AFDKO-supplied Python now contains the
  robofab, fontMath, and defcon libraries, as well as mutatorMath.
- \[ttx\] Updated to the latest branch of the fontTools library as
  maintained by Behdad Esfahbod on GitHub. Added a patch to cffLib.py
  to fix a minor problem with choosing charset format with large glyph
  sets.
- Updated four Adobe-CNS1-\* ordering files.

2.5.63164 (released 2014-09-08)
-------------------------------

- \[makeotf\] Now detects `IsOS/2WidthWeightSlopeOnly` as well as the
  misspelled `IsOS/2WidthWeigthSlopeOnly`, when processing the
  fontinfo file.
- \[makeotfexe\] Changed behavior when 'subtable' keyword is used in
  a lookup other than class kerning. This condition now triggers only
  a warning, not a fatal error. Change requested by FontForge
  developers.
- \[makeotf\] Fixed bug which prevented making TTF fonts under
  Windows. This was a problem in quoting paths used with the 'ttx'
  program.
- Fixed installation issues: removed old Windows install files from
  the Windows AFDKOPython directory. This was causing installation of
  a new AFDKO version under Windows to fail when the user's PATH
  environment variable contained the path to the AFDKOPython
  directory. Also fixed command file for invoking ttx.py.
- Updated files used for building ideographic fonts with Unicode IVS
  sequences:
  `FDK/Tools/SharedData/Adobe Cmaps/Adobe-CNS1/Adobe-CNS1_sequences.txt`
  and `Adobe-Korea1_sequences.txt`.

2.5.62754 (released 2014-05-14)
-------------------------------

- \[IS/addGlobalColor\] When using the '-bc' option, fixed bug with
  overflow for CID value in dumping glyph header. Fixed bug in IS to
  avoid crash when logic for glyphs \> 72 points is used.
- \[makeotfexe\] Fixed bug that applied '-gs' option as default
  behavior, subsetting the source font to the list of glyphs in the
  GOADB.

2.5.62690 (released 2014-04-30)
-------------------------------

- \[makeotf\] When building output TTF font from an input TTF font,
  will now suppress warnings that hints are missing. Added a new
  option '-shw' to suppress these warnings for other fonts that with
  unhinted glyphs. These warnings are shown only when the font is
  built in release mode.
- \[makeotfexe\] If the cmap format 4 UTF16 subtable is too large to
  write, then makeotfexe writes a stub subtable with just the first
  two segments. The last two versions allowed using '-' in glyph
  names. Removed this, as it breaks using glyph tag ranges in feature
  files.
- Updated copyright, and removed patent references. Made extensive
  changes to the source code tree and build process, to make it easier
  to build the open source AFDKO. Unfortunately, the source code for
  the **IS** and **checkOutlines** programs cannot be open sourced.
- \[tx/mergeFonts/rotateFonts\] Removed '-bc' option support, as
  this includes patents that cannot be shared in open source.
- \[tx\] All tx-related tools now report when a font exceeds the max
  allowed subroutine recursion depth.
- \[tx/mergeFonts/rotateFonts\] Added common options to all when
  possible: all now support UFO and SVG fonts, the '-gx' option to
  exclude fonts, the '-std' option for cff output, and the '-b'
  option for cff output.

2.5.61944 (released 2014-04-05)
-------------------------------

- \[makeotf\] Added new option '-gs'. If the '-ga' or '-r'
  option is used, then '-gs' will omit from the font any glyphs
  which are not named in the GOADB file.
- \[Linux\] Replaced the previous build (which worked only on 64-bit
  systems) with a 32 bit version, and rebuilt checkOutlines with debug
  messages turned off.
- \[ttx\] Fixed FDK/Tools/win/ttx.cmd file so that the 'ttx' command
  works again.

2.5.61911 (released 2014-03-25)
-------------------------------

- \[makeotf\] Add support for two new 'features' file keywords, for
  the OS/2 table. Specifying 'LowerOpSize' and 'UpperOpSize' now
  sets the values 'usLowerOpticalPointSize' and
  'usUpperOpticalPointSize' in the OS/2 table, and set the table
  version to 5.
- \[makeotf\] Fixed the '-newNameID4' option so that if the style
  name is "Regular", it is omitted for the Windows platform name ID
  4, as well as in the Mac platform version. See change in
  build 61250.
- \[tx\] When the user does not specify an output destination file
  path (in which case tx tries to write to stdout), tx now reports a
  fatal error if the output is a UFO font, rather than crashing.
- \[tx\] Fixed crash when encountering an empty `<dict/>` XML
  element.
- \[spot\] Added logic to dump the new fields in OS/2 table version 5,
  **usLowerOpticalPointSize** and **usUpperOpticalPointSize**. An
  example of these values can be seen in the Windows 8 system font
  Sitka.TTC.
- \[ufo workflow\] Fixed autohint and checkOutlines so that the '-o'
  option works, by copying the source UFO font to the destination UFO
  font name, and then running the program on the destination UFO font.
- \[ufo workflow\] Fixed tools that the PostScript font name is not
  required.
- Added Linux build.

2.5.61250 (released 2014-02-17)
-------------------------------

- \[tx\] Fixed rare crashing bug in reading a font file, where a
  charstring ends exactly on a refill buffer boundary.
- \[tx\] Fixed rare crashing bug in subroutinization.
- \[tx\] Fixed bug where it reported values for wrong glyph with more
  than 32K glyphs in the font.
- \[tx\] Fixed bug where the tool would not dump a TrueType Collection
  font file that contained OpenType/CFF fonts.
- \[tx\] Fixed issue where it failed to read a UFO font if the UFO
  font lacked a fontinfo.plist file, or a psFontName entry.
- \[IS\] Fixed IS so that it no longer scales the fontDict FontMatrix,
  when a scale factor is supplied, unless you provide an argument to
  request this.
- \[makeotf\] The option '-newNameID4' now builds both Mac and Win
  name ID 4 using name ID 1 and 2, as specified in the OpenType spec.
  The style name is omitted from name ID 4 it is "Regular".
- \[makeotf\] Changed logic for specifying ValueFormat for PosPair
  value records. Previous logic always used the minimum ValueFormat.
  Since changing ValueFormat between one PosPair record and the next
  requires starting a new subtable, feature files that used more than
  one position adjustment in a PosPair value record often got more
  subtable breaks then necessary, especially when specifying a PairPos
  statement with an all zero Value Record value after a PairPos
  statement with a non-zero Value Record. With the new logic, if the
  minimum ValueFormat for the new ValueRecord is different than the
  ValueFormat used with the ValueRecord for the previous PairPos
  statement, and the previous ValueFormat permits expressing all the
  values in the current ValueRecord, then the previous ValueFormat is
  used for the new ValueRecord.
- Added commands **otc2otf** and **otf2otc** to build OpenType
  collection files from a OpenType font files, and vice-versa.
- \[ttx\] Updated the FontTools library to the latest build on the
  GitHub branch maintained by Behdad Esfahbod, as of Jan 14 2014.
- \[ufo workflow\] Fixed bugs in ufoTools.py. The glyph list was being
  returned in alphabetic order, even when the public.glyphOrder key
  was present in lib.py. Failed when the glyphOrder key was missing.

2.5.60908 (released 2013-10-21)
-------------------------------

- \[tx\] Can now take UFO font as a source font file for all outputs
  except rasterization. It prefers GLIF file from the layer
  `glyphs.com.adobe.type.processedGlyphs`. You can select another
  preferred layer with the option '-altLayer `<layer name>`'. Use
  'None' for the layer name in order to have tx ignore the preferred
  layer and read GLIF files only from the default layer.
- \[tx\] Can now write to a UFO with the option '-ufo'. Note that it
  is NOT a full UFO writer. It writes only the information from the
  Postscript font data. If the source is an OTF or TTF font, it will
  not copy any of the meta data from outside the font program table.
  Also, if the destination is an already existing UFO font, tx will
  overwrite it with the new data: it will not merge the new font data
  with the old.
- \[tx\] Fixed bugs with CID values \> 32K: used to write these as
  negative numbers when dumping to text formats such as AFM.
- \[autohint/checkOutlines\] These programs can now be used with UFO
  fonts. When the source is a UFO font, the option '-o' to write to
  another font is not permitted. The changed GLIF files are written to
  the layer 'glyphs.com.adobe.type.processedGlyphs'. Each script
  maintains a hash of the width and marking path operators in order to
  be able to tell if the glyph data in the default layer has changed
  since the script was last run. This allows the scripts to process
  only those glyphs which have changed since the last run. The first
  run of autohint can take two minutes for a 2000 glyph font; the
  second run takes less then a second, as it does not need to process
  the unchanged glyphs.
- \[stemHist/makeotf\] Can now take UFO fonts as source fonts.

2.5.60418 (released 2013-02-26)
-------------------------------

- \[autohint\] Now skips comment lines in fontinfo file.
- \[makeotf\] Added support for source font files in the 'detype1'
  plain text format. Added logic for "Language" keyword in fontinfo
  file; if present, will attempt to set the CID font makeotf option
  -"cs" to set he Mac script value.
- \[compareFamily\] Added check in Family Test 10 that font really is
  monospaced or not when either the FontDict isFixedPitch value or the
  Panose value says that it is monospaced.
- \[spot\] Fixed bug that kept 'palt'/'vpal' features from being
  applied when proofing kerning.

2.5.59149 (released 2012-10-31)
-------------------------------

- \[makeotf\] When building OpenType/TTF files, changed logic to copy
  the OS/2 table usWinAscent/Descent values over the head table
  yMax/yMin values, if different. This was because:

  - both pairs are supposed to represent the real font bounding box
    top and bottom,and should be equal;
  - the TTF fonts we use as sources for makeotf are built by FontLab;
  - FontLab defines the font bounding box for TrueType fonts by using
    off-curve points as well as on-curve points. If a path does not have
    on-curve points at the top and bottom extremes, the font bounding
    box will end up too large. The OS/2 table usWinAscent/Descent
    values, however, are set by makeotf using the converted T1 paths,
    and are more accurate. Note that I do not try to fix the head table
    xMin and xMax. These are much less important, as the head table yMin
    and yMax values are used for line layout by many apps on the Mac,
    and I know of no application for the xMin and yMin values.

- \[makeotf\] Changed default Unicode H CMAP file for Adobe-Japan1 CID
  fonts to use the UniJIS2004-UTF32-H file.
- Added the CID font vertical layout files used with KozMinPr6N and
  KozGoPr6N: AJ16-J16.VertLayout.KozGo and AJ16-J16.VertLayout.KozMin.
- Updated several Unicode CMAP files, used only with CID fonts.
- Added new Perl script, glyph-list.pl, used in building CID fonts.
  This replaces the three scripts extract-cids.pl, extract-gids.pl,
  and extract-names.pl, which have been removed from the AFDKO.

2.5.58807 (released 2012-09-13)
-------------------------------

- \[makeotf\] Discovered that when building TTF fonts, the GDEF table
  was not being copied to the final TTF font file. Fixed.

2.5.58732 (released 2012-09-04)
-------------------------------

- \[autohint\] Added new feature to support sets of glyphs with
  different baselines. You can now specify several different sets of
  global alignment zones and stem widths, and apply them to particular
  sets of glyphs within a font when hinting. See option '-hfd' for
  documentation.
- \[autohint\] Allow AC to handle fonts with no BlueValues, aka
  alignment zones.
- \[autohint\] Respect BlueFuzz value in font.
- \[autohint\] Fixed the options to suppress hint substitution and to
  allow changes.
- \[autohint\] When hinting a font with no alignment zones or invalid
  alignment zones (and with the '-nb' option), set the arbitrary
  alignment zone outside the FontBBox, rather than the em-square.
- \[checkOutlines\] Fixed bug where the arms of an X would be falsely
  identified as coincident paths, when they are formed by only two
  paths with identical bounding boxes.
- \[checkOutlines\] Fixed bug where very thin elements would get
  identified as a tiny sub path, and get deleted.
- \[checkOutlines\] Fixed bug in determining path orientation. Logic
  was just following the on-path points, and could get confused by
  narrow concave inner paths, like parentheses with an inner contour
  following the outer contour, as in the Cheltenham Std HandTooled
  faces.
- \[checkOutlines\] Fixed bugs in determining path orientation.
  Previous logic did not handle multiple inner paths, or multiple
  contained outer paths. Logic was also dependent on correctly sorting
  paths by max Y of path bounding box. Replaced approximations with
  real bezier math to determine path bounding box accurately.
- \[checkOutlines\] Changed test for suspiciously large bounding box
  for an outline. Previous test checked for glyph bounding box outside
  of fixed limits that were based on a 1000 em square. The new test
  looks only for paths that are entirely outside a rectangle based on
  the font's em square, and only reports them: it does not ever
  delete them. Added new option '-b' to set the size of the design
  square used for the test.
- \[checkOutlines\] Fixed bug where it would leave a temp file on disk
  when processing a Type 1 font.
- \[checkOutlines\] Removed test for coincident control points. This
  has not been an issue for decades. It is frequently found in fonts
  because designers may choose to not use one of the two control
  points on a curve. The unused control point then has the same
  coordinates as its nearest end-point, and would to cause
  checkOutlines to complain.
- \[compareFamily\] Single Test 6. Report error if there is a patent
  number in the copyright. Adobe discovered that a company can be sued
  if it ships any product with an expired patent number.
- \[compareFamily\] Single Test 22 (check RSB and LSB of ligature vs.
  the left and right ligature components) did not parse contextual
  ligature substitution rules correctly. Now fixed.
- \[compareFamily\] Family Test 18. Survive OTF fonts with no blue
  values.
- \[compareFamily\] Family Test 2 (Check that the Compatible Family
  group has same nameIDs in all languages): Added the WPF nameIDs 21
  and 22 to the exception list, which may not exist in all faces of a
  family.
- \[fontsetplot\] Fixed so it works with CID fonts. Also fixed so that
  widow line control works right. Added new low level option for
  controlling point size of group header.
- \[fontsetplot\] Fixed syntax of assert statements. Produced error
  messages on first use of the \*plot commands.
- \[kernCheck\] Fix so that it survives fonts with contextual kerning.
  It does not, however, process the kern pairs in contextual kerning.
- \[makeotf\] Fixed bug in mark to ligature. You can now use an
  `<anchor NULL>` element without having to follow it by a dummy mark
  class reference.
- \[makeotf\] Fixed bug which limited source CID fonts to a maximum of
  254 FDArray elements, rather than the limit of 255 FDArray elements
  that is imposed by the CFF spec.
- \[makeotf\] Fixed bugs in automatic GDEF generation. When now GDEF
  is defined, all conflicting class assignments in the GlyphClass are
  filtered out. If a glyph is assigned to a make class, that
  assignment overrides any other class assignment. Otherwise, the
  first assignment encountered will override a later assignment. For
  example, since the BASE class is assigned first, assignment to the
  BASE class will override later assignments to LIGATURE or COMPONENT
  classes.
- \[makeotf\] Fix bug in validating GDEF mark attachment rules. This
  now validates the rules, rather than random memory. Had now effect
  on the output font, but did sometimes produce spurious error
  messages.
- \[makeotf\] Fix crashing bug when trying to report that a glyph
  being added to a mark class is already in the mark class.
- \[makeotf\] If the OS/2 code page bit 29 (Macintosh encoding) is
  set, then also set bit 0 (Latin (1252). Under Windows XP and Windows
  7, if only the Mac bit is set, then the font is treated as having no
  encoding, and you cannot apply the font to even basic Latin text.
- \[makeotf\] By default, set Windows name ID 4 (Full Name) same as
  Mac nameID 4, instead of setting it to the PostScript name. This is
  in order to match the current definition of the name ID 4 in the
  latest OpenType spec. A new option to makeotf ('-useOldNameID4'),
  and a new key in the fontinfo file ("UseOldNameID4"), will cause
  makeotf to still write the PS name to Windows name ID 4.
- \[makeotf\] Add support for WPF names, name ID 21 and 22.
- \[makeotf\] Fixed attachment order of marks to bug in generating
  Mark to Ligature (GPOS lookup type 4). The component glyphs could be
  reversed.
- \[makeotf\] Fixed bug in auto-generating GDEF table when Mark to
  Mark (GPOS lookup Type 4) feature statements are used. The target
  mark glyphs were registered as both GDEF GlyphClass Base and Mark
  glyphs, and the former took precedence. makeotfexe now emits a
  warning when a glyph gets assigned to more than one class when
  auto-generating a GDEF table GlyphClass, and glyphs named in mark to
  mark lookups are assigned only to the Mark GDEF glyph class.
- \[makeotf\] Fixed bugs in generating TTF fonts from TTF input. It
  now merges data from the head and hhea tables, and does a better job
  of dealing with the 'post' table. The previous logic made
  incorrect glyph names when the glyphs with names from the Mac Std
  Encoding were not all contiguous and at the start of the font.
- \[makeotf\] Added new option '-cn' for non-CID source fonts, to
  allow reading multiple global font alignment zones and stem widths
  from the fontinfo file, and using this to build a CID-keyed CFF
  table with an identity CMAP. This is experimental only; such fonts
  may not work in many apps.
- \[makeotf\] Fixed bug where the coverage table for an element in the
  match string for a chaining contextual statement could have
  duplicate glyphs. This happens when a glyph is specified more than
  once in the class definition for the element. The result is that the
  format 2 coverage table has successive ranges that overlap: the end
  of one range is the same glyph ID as the start of the next range;
  harmless, but triggers complaints in font validators.
- \[makeotf\] Updated to latest Adobe CMAP files for ideographic
  fonts. Changed name of CMAP directories in the AFDKO, and logic for
  finding the files.
- \[makeotf\] When providing a GDEF feature file definition, class
  assignments now may be empty:

        table GDEF {
            GlyphClassDef ,,,;
        } GDEF;

  is a valid statement. You just need to provide all three commas and
  the final colon to define the four classes. The following statement
  builds a GDEF GlyphClass with an empty Components class.

        table GDEF {
            GlyphClassDef [B], [L], [M], ;
        } GDEF;

- \[makeotf\] The glyph alias file now defines order in which glyphs
  are added to the end of the target font, as well as defining the
  subset and renaming.
- \[makeotf\] The `-cid <cidfontinfo>` option for converting a
  font to CID can now be used without a glyph alias file, if the
  source font glyphs have names in the form "cidXXXX", as is output
  when mergeFonts is used to convert from CID to name-keyed. If the
  `-cid <cidfontinfo>` option is used, and there is no glyph alias
  file, then any glyphs in the font without a name in the form
  "cidXXXX" will be ignored.
- \[spot\] Added error message for duplicate glyph IDs in coverage
  tables with format 2, a problem caused by a bug in makeotf with some
  Adobe fonts that use chaining contextual substitution. Note: error
  message is written only if level 7 GSUB/GPOS dump is requested.
- \[spot\] Minor formatting changes to the GSUB/GPOS level 7 dump, to
  make it easier to edit this into a real feature file.
- \[spot\] When writing out feature file syntax for GPOS 'ignore
  pos' rules, the rule name is now written as 'ignore pos', not
  just 'ignore'.
- \[spot\] Can now output glyph names up to 128 chars (Note: these are
  not legal PostScript glyph names, and should be encountered only in
  development fonts.)
- \[spot\] Has new option '-ngid' which suppresses output of the
  trailing glyph ID `@<gid>` for TTF fonts.
- \[spot\] No longer dumps the DefaultLangSys entry when there is
  none.
- \[spot\] Changed dump logic for contextual and chain contextual
  lookups so that spot will not dump the lookups referenced by the
  substitution or position rules in the contextual lookups. The
  previous logic led to some lookups getting dumped many times, and
  also to infinite loops in cases where a contextual lookup referenced
  other contextual lookups.
- \[spot\] Added support for Apple kern subtable format 3. Fixed old
  bug causing crash when dumping font with Apple kern table from
  Windows OS.
- \[spot\] Fixed error when dumping Apple kern table subtable format
  0, when kern table is at end of font file.
- \[spot\] Fixed crashing bug seen in DejaVuSansMono.TTF: spot did not
  expect an anchor offset to be zero in a Base Array base Record.
- \[spot\] Removed comma from lookupflag dump, to match feature file
  spec.
- \[spot\] Added logic to support name table format 1, but it probably
  does not work, since I have been unable to find a font to test with
  this format.
- \[spot\] Fixed spelling error for "Canadian" in OS/2 code page
  fields.
- \[spot\] Changed dump of cmap subtable 14: hex values are
  uppercased, and base+UVS values are written in the order \[base,
  uvs\].
- \[stemHist\] Always set the alignment zones outside the font BBox,
  so as to avoid having the source font alignment zones affect
  collection of stem widths.
- \[stemHist\] Fix bug where the glyph names reported in the stem and
  alignment reports were off by 1 GID if the list of glyphs included
  the '.notdef' glyph.
- \[tx\] Added support for option '-n' to remove hints for writing
  Type 1 and CFF output fonts.
- \[tx\] Added new option "+b" to the cff output mode, to force
  glyph order in the output font to be the same as in the input font.
- \[tx\] Fixed bug in flattening 'seac' operator. If the glyph
  components were not in the first 256 glyphs, then the wrong glyph
  would be selected.
- \[tx\] Added new library to read in svg fonts as a source. tx can
  now read all the SVG formats that it can write. Handles only the
  path operators: M, m, L, L, C, c, Z, z, and the font and glyph
  attributes: 'font-family', 'unicode', 'horiz-adv-x',
  'glyph-name', 'missing-glyph'.
- \[tx\] Fixed bug in converting TTF to OpenType/CFF. It flipped the
  sign of the ItalicAngle in the 'post' table, which in turn flipped
  the sign of the OS/2 table fields ySubscriptXOffset and
  ySuperscriptXOffset. This bug showed up in TTF fonts built by
  makeotf, as makeotf uses 'tx' to build a temporary Type 1 font
  from the source TTF.
- \[tx\] Fixed bug where '-usefd' option was not respected, when
  converting from CID to name-keyed fonts.
- Updated the internal Python interpreter to version 2.7.
- Updated Adobe Cmaps/Adobe-Japan1 files:

  - Adobe-Japan1\_sequences.txt
  - UniJIS-UTF32-H
  - UniJIS2004-UTF32-H
  - UniJISX0213-UTF32-H
  - UniJISX02132004-UTF32-H

- Added several scripts related to CID font production:

  - cmap-tool.pl
  - extract-cids.pl
  - extract-gids.pl
  - extract-names.pl
  - fdarray-check.pl
  - fix-fontbbox.pl
  - hintcidfont.pl
  - subr-check.pl

2.5.25466 (released 2012-03-04)
-------------------------------

- \[charplot\] This was non-functional since build 21898. Now fixed.
- \[checkOutlines\] Changed so that the test for nearly vertical or
  horizontal lines is invoked only if the user specifies the options
  '-i' or '-4', instead of always. It turns out that this test,
  when fixed automatically, causes more problems than it cures in CJK
  fonts.
- \[compareFamily\] Changed so that the default is to check stem
  widths and positions for bogus hints. Used 'tx' rather than Python
  code for parsing charstring in order to speed up hint check.
- \[compareFamily\] Updated script tags and language tags according to
  OpenType specification version 1.6.
- \[documentation\] In feature file syntax reference, fixed some
  errors and bumped the document version to 1.10.
- \[documentation\] Fixed typo in example in section 4.d: lookFlag
  values are separated by spaces, not commas.
- \[documentation\] Fixed typo in example in section 8.c on stylistic
  names: quotes around name string need to be matching double quotes.
  Reported by Karsten Luecke.
- \[documentation\] Changed agfln.txt copyright notice to BSD license.
- \[makeInstances\] Fixed bug where a space character in any of the
  path arguments caused it to fail.
- \[makeInstances\] Fixed bug that can make the FontBBox come out
  wrong when using ExtraGlyphs.
- \[makeInstances\] Fixed rounding bug that could (rarely) cause
  makeInstances to think that a composite glyph is being scaled (which
  is not supported by this script) when it is not.
- \[makeotf\] Fixed bug in generating TTF fonts from TTF input.
  Previous version simply did not work.
- \[spot\] Added support for "Small" fonts, an Adobe internal
  Postscript variant used for CJK fonts.
- \[spot\] Added support for large kern tables, such as in the Vista
  font Cambria, where the size of the kern subtable exceeds the value
  that can be held in the subtable "length" field. In this case, the
  "length" filed must be ignored.
- \[spot\] Fixed proof option to show GPOS records in GID order by
  default, and in lookup order only with the '-f' option. It had
  always been proofing the GPOS rules in lookup order since 2003.
- \[spot\] Fixed double memory deallocation when dumping TTC files;
  this could cause a crash.
- \[spot\] When decompiling GSUB table to feature file format (-t
  GSUB=7) and reporting skipped lookups identify lookups which are
  referenced by a chaining contextual rule.
- \[sfntedit\] Changed final "done" message to be sent to stdout
  instead of stderr. Reported by Adam Twardoch.
- \[stemHist\] Fixed typo in help text, reported by Lee Digidea:
  '-all' option was not working.
- \[tx\] Added new option '-std' to force StdEncoding in output CFF
  fonts.

2.5.21898 (released 2009-05-01)
-------------------------------

- \[autohint/checkOutlines\] Fixed rare case when an _rrcurveto_ is
  preceded by such a long list of _rlineto_ that the stack limit is
  passed.
- \[autohint/checkOutlines\] Fixed to restore font.pfa output file to
  StandardEncoding Encoding vector. Since requirements of CFF
  StandardEncoding differs from Type 1 StandardEncoding, a
  StandardEncodingEncoding vector in a Type 1 font was sometimes
  getting converted to a custom Encoding vector when being
  round-tripped through the CFF format which autohint does internally.
- \[checkOutlines\] Fixed random crash on Windows due to buffer
  overrun.
- \[checkOutlines\] Changed default logging mode to not report glyph
  names when there is no error report for the glyph.
- \[CompareFamily\] Added "ring" to the list of accent names used to
  find (accented glyph, base glyph) pairs for Single Face Test 23.
  Reported by David Agg.
- Renamed showfont to fontplot2 to avoid conflict with the Mac OSX
  showfont tool.
- Fixed problem with showing vertical origin and advance: was not
  using VORG and vmtx table correctly.
- \[FontLab scripts\] Added logic to Instance Generator to support
  eliminating "working" glyphs from instances, to substitute
  alternate glyph designs for specific instances, and to update more
  Font Dict fields in the instance fonts. Added help.
- Added command line equivalent, "makeInstances' which does the same
  thing, but which uses the IS tool for making the snapshot. See the
  'IS' entry.
- \[IS\] Added new tool for "intelligent scaling". This uses the
  hinting in an MM font to keep glyph paths aligned when snapshotting
  from MM fonts. The improvement is most visible in glyphs with
  several elements that need to maintain alignment, such as percent
  and perthousand. It is also useful for the same reason when scaling
  fonts from a large em-square size to a smaller size. To be
  effective, the source MM font must be hinted and must have global
  alignment zones defined. The new font must be re-hinted. For
  instances from MM fonts especially, it is a good idea to redo the
  alignment zones, as the blending of the MM base designs usually does
  not produce the best alignment zones or stem widths for the instance
  fonts. makeInstances and "Instance Generator" scripts allow you to
  preserve these modifications when redoing the MM instance snapshot.
- \[makeotf\] Fixed generation of version 1.2 GDEF table to match the
  final OpenType spec version 1.6. This version is generated only when
  the new lookup flag 'UseMarkFilteringSet" is used.
- \[makeotf\] Fixed generation of names for stylistic alternates
  features. There was a bug such that in some circumstances, the
  feature table entry for the stylistic alternate feature would point
  to the wrong lookup table.
- \[makeotf\] Fixed generation of the reverse substitution lookup
  type. This was unintentionally disabled just before the previous
  release.
- \[makeotf\] Fixed bugs in memory management of glyph objects. If the
  font built, it was correct, but this bug could cause the font to
  fail to build.
- \[spot\] Fixed to dump GDEF table version 1.2 according to the final
  OpenType spec version 1.6.
- \[spot\] Fixed feature-format dump of the lookupflags
  MarkAttachmentType and UseMarkFilteringSet to give a class name as
  an argument, rather than a class index.
- \[spot\] Extended the GDEF table dump to provide a more readable
  form.
- \[spot\] Added dump formats for htmx and vtmx to show the advance
  and side bearing metrics for all glyphs.

2.5.21340 (released 2009-01-22)
-------------------------------

- \[AGLFN\] (Adobe Glyph List for New Fonts) Created new version 1.7.
- \[AGLFN\] Reverted to the AGLFN v1.4 name and Unicode assignments
  for Delta, Omega, and mu. The v1.6 versions were better from a
  designer's point of view, but we cannot use name-to-Unicode value
  mappings that conflict with the historic usage in the Adobe Glyph
  List 2.0. See
  <http://www.adobe.com/devnet/opentype/archives/glyph.html>.
- \[AGLFN\] Dropped all the 'afii' names from the list: 'uni'
  names are actually more descriptive, and map to the right Unicode
  values under Mac OSX.
- \[AGLFN\] Dropped all the 'commaccent' names from the list:
  'uni' names map to the right Unicode values under Mac OSX before
  10.4.x.
- \[autohint\] Converted AC.py script to call a command-line program
  rather than a Python extension module, same way makeotf works, to
  avoid continuing Python version problems.
- \[autohint\] Fixed to actually emit vstem3 and hstem3 hint operators
  (counter control hints, which work to keep the space between three
  stems open and equal, as in an 'm') - this has been broken since
  the first AFDKO. It will now also look in the same directory as the
  source font for a file named "fontinfo", and will attempt to add
  stem3 hints to the glyph which are listed by name in the name list
  for the keys "HCounterChars" or "VCounterChars".
- \[autohint\] Fixed old bug where it would only pay attention to the
  bottom four of the top zone specified in the FontDict BlueValues
  list. This results in more edge hints in tall glyphs.
- \[autohint\] Fixed special case when adding flex operator which
  could result in an endless loop
- \[autohint\] Added 'logOnly' option, to allow collecting report
  without changing the font.
- \[autohint\] Added option to specify which glyphs to exclude from
  autohinting.
- \[autohint\] Suppressed generation and use of `<font-name>.plist`
  file, unless it is specifically requested.
- \[autohint\] Fixed bug where an extremely complex glyph would
  overflow a buffer of the list of hints.
- \[checkOutlines\] Improved overlap detection and path orientation:
  it will now work with outlines formed by overlapping many stroke
  elements, as is sometimes done in developing CJK fonts.
- \[checkOutlines\] added new test for nearly vertical or horizontal
  lines. Fixed bug in this new code, reported by Erik van Blokland.
- \[CompareFamily\] For the warning that the Full Family name in the
  CFF table differs from that in the name table, changed it to a
  "Warning" rather than "Error", and explained that there is no
  functional consequence.
- \[CompareFamily\] Removed check that Mac names ID 16 and 17 do not
  exist, as makeotf now does make them. See notes in MakeOTF User
  Guide about this.
- \[CompareFamily\] Fixed so it works with TTF fonts again.
- \[makeotf\] Removed code that added a default Adobe copyright to the
  name table if no copyright is specified, and removed code to add a
  default trademark.
- \[makeotf\] Added support for the lookupflag UseMarkFilteringSet.
  This is defined in the proposed changes for OpenType spec 1.6, and
  is subject to change in definition.
- \[makeotf\] Dropped restriction that vmtx/VORG/vhea tables will only
  be written for CID-keyed fonts. The presence in the feature file of
  either a 'vrt2' feature of vmtx table overrides will now cause
  these tables to be written for both CID-keyed and name-keyed fonts.
- \[makeotf\] Added warning when a feature is referenced in the aalt
  feature definition, but either does not exist or does not contribute
  any rules to the aalt feature. The aalt feature can take only single
  and alternate substitution rules.
- \[makeotf\] Added support for the following lookup types:

  - GSUB type 2 Multiple Substitution
  - GSUB type 8 Reverse Chaining Single Substitution
  - GPOS type 3 Cursive Adjustment
  - GPOS type 4 Mark-to-Base Attachment
  - GPOS type 5 Mark-to-Ligature Attachment
  - GPOS type 6 Mark-to-Mark Attachment

- \[makeotf\] Added support for explicit definition of the GDEF table,
  and automatic creation of the GDEF when any of the lookup flag
  settings for ignoring a glyph class is used, or any mark classes are
  defined.
- \[makeotf\] Support using TTF fonts as input, to build an
  OpenType/TTF font, with the limitation that glyph order and glyph
  names cannot be changed. This is rather ugly under the hood, but it
  works. The MakeOTF.py Python script uses the tx tool to convert the
  TTF font to CFF data without changing glyph order or names. It then
  builds an OpenType/CFF font. It then uses the sfntedit tool to copy
  the TTF glyph data to the OpenType font, and to delete the CFF
  table.
- \[makeotf\] Added support for building Unicode Variation Selectors
  for CID-keyed fonts, using the new cmap subtable type 14.
- \[makeotf\] Fixed bug with inheritance of default rules by scripts
  and languages in feature file feature definitions. Explicitly
  defined languages were only getting default rules defined after the
  last script statement, and when a script is named, languages of the
  script which are not named got no rules at all.
- \[makeotf\] Fixed bug where you could not run makeotf when the
  current directory is not the same is the source font's home
  directory.
- \[makeotf\] Set OS/2.lastChar field to U+FFFF when using mappings
  beyond the BMP.
- \[makeotf\] Create the Mac platform name table font menu names by
  the same rules as used for the Windows menu names. Add new keywords
  to the FontMenuNameDB file syntax. If you use the old keywords, you
  get the old format; if you use the new syntax, you get nameIDs 1, 2
  and 16 and 17 just like for the Windows platform.
- \[makeotf\] Fixed bug in name table font menu names: if you entered
  a non-English Preferred name ("f=") and not a compatible family
  name ("c="), you would end up with a nameID 16 but no nameID 17.
- \[makeotf\] Fixed bogus 'deprecated "except" syntax' message
  under Windows.
- \[makeotf\] Fixed bug where contextual pos statements without
  backtrack or lookahead context were writing as a non-contextual
  rule. Reported by Karsten Luecke.
- \[makeotf\] Added new option to make stub GSUB table when no GSUB
  rules are present.
- \[makeotf\] Added warning if the aalt feature definition references
  any feature tags that either do not exist in the font, or do not
  contribute any rules that the aalt feature can use.
- \[sfntdiff\] Fixed so that only error messages are written to
  stderr; all others now written to stdout.
- \[sfntdiff\] Fixed bug in dump of 'name' table: when processing
  directories rather than individual files, the name table text was
  never updated after the first file for the second directory.
- \[spot\] Fixed option '-F' to show the contextual rule sub-lookup
  indices, and to flag those which have already been used by another
  lookup.
- \[spot\] If a left side class 0 is empty, do not report it.
- \[spot\] For GSUB/GPOS=7 FEA dump, give each class a unique name in
  the entire font by appending the lookup ID to the class names. It
  was just `[LEFTCLASS]()<class index>_<subtable index>`, but
  these names are repeated in every lookup. It is now
  `LEFTCLASS_c<class index>_s<subtable index>_l<lookup
  index>`.
- \[spot\] When a positioning value record has more than one value,
  print the full 4 item value record. Previously, it would just print
  non-zero values. This was confusing when dumping Adobe Arabic, as
  you would see two identical values at the end of some pos rules. In
  fact, each of these pos rule does have two adjustment values, one
  for x and one for y advance adjustment, that happen to be the same
  numeric value.
- \[spot\] Fixed to write backtrack context glyphs in the right order.
- \[tx\] Added option to NOT clamp design coordinates to within the
  design space when snapshotting MM fonts.
- \[tx\] Added option to subroutinize the font when writing to CFF.
  This option is derived from the same code used by makeotfexe, but
  takes only about 10% the memory and runs much faster. This should
  allow subroutinizing large CJK fonts that makeotfexe could not
  handle. This is new code, so please test results carefully, i.e. if
  you use it, always check that the flattened glyphs outlines from the
  output font are identical to the flattened glyph outlines from the
  input font.
- \[ttxn\] Added options to suppress hinting in the font program, and
  version and build numbers.

2.0.27 (released 2007-11-10)
----------------------------

- \[compareFamily\] Fixed Single Test 3 (reported by Mark Simonson and
  others); the test should compare the Mac platform name ID 4 (name ID
  1 + space + name ID 2) with the target value, but was instead using
  the value of the name ID 18 (Compatible Full Name).
- \[compareFamily\] Fixed Family Test 2 to print a report that helps
  determine which {platform, script, language, name ID} is present in
  some fonts but not others.
- \[IS\] Fixed a bug where applying hint-based scaling can cause an
  off-by-1 error when the the _closepath_ position is supposed to
  coincide with the original _moveto_, leading to an effective final
  1-unit _lineto_, that may overlap the initial path. In the old MM
  design world, we worked around this issue by designing the MMs so
  that there was always a one unit difference between a final _curveto_
  point and the original _moveto_. FontLab doesn't support doing that
  automatically, so when making an instance, **IS** will instead simply
  look for cases where the _moveto_ and _closepath_ positions differ by
  one unit, and move the _moveto_ position to coincide with the
  _closepath_ position.
- \[makeotf\] Fixed bug where specifying thousands of singleton kern
  pairs could silently overflow offsets, and makeotf would build a bad
  font without any warning. (reported by Adam Twardoch)
- \[makeotf\] Relative file paths can now be used even if the current
  directory is not the source font directory. Project files can now be
  saved to directories other than the source font directory. Note that
  paths stored in project file are relative to the project file's
  directory. (reported by Andreas Seidel)
- \[makeotf/spot\] Added support for Unicode Variation Sequences (UVSes).
  See MakeOTF User's Guide and
  [Unicode Technical Standard #37](http://unicode.org/reports/tr37/)
- \[spot\] Fixed bug where contents of 'size' GPOS feature would be
  printed for all dump levels.
- \[spot\] Fixed failure to process 'post' table format 4.0. which
  occurs in some Apple TT fonts, such as Osaka.dfont
- Updated Adobe-Japan-1 CMAP files for building CID fonts.

2.0.26 (released 2007-05-05)
----------------------------

- Added `featurefile.plist` for BBedit. Install this in the location
  shown at the top of the file; it enables code coloring for FEA syntax.
  The file is in FDK/Tools/SharedData
- Added `MSFontValidatorIssues.html` to FDK/Technical Documentation. It
  lists the error messages from the MS FontValidator tool that can be
  ignored for OpenType/CFF fonts.
- \[FontLab macros\] Added InstanceGenerator. Another script for making
  instances from an MM VFB font. It's simpler than MakeInstances macro.
- \[FontLab macros\] Removed debug statement in Set Start Points which
  blocked processing more than 50 glyphs. (reported by George Ryan)
- \[FontLab macros\] Added explanation of CheckOutlines errors to help
  dialog.
- \[checkOutlines\] Added option '-he' to print explanation of error
  messages.
- \[compareFamily\] Added error if the font's CFF table contains a
  Type 2 `seac` operator. The CFF spec does not support this operator.
  Some very old tools allow this to happen.
- \[makeotf\] Fixed a bug in decomposing glyphs that are defined as
  composite glyphs in a Type 1 source font. This bug caused the base
  component to be shifted when then left side bearing of the composite
  glyph differs from that of the base glyph. This could be consequential,
  as FontLab has an option to not decompose composite glyphs when
  generating a Type 1 font.
- \[makeotf\] Fixed a bug in recognizing the "Korea1" order when trying
  to pick a default Mac cmap script ID from a CID-keyed font's ROS
  (Registry-Order-Supplement) field.
- \[tx\] Fixed a bug in decomposing pretty much all composite glyphs
  from Type 1 and CFF source fonts. It happened only when a Type 1 or
  CFF font was being subset, i.e. converted into a font with fewer
  glyphs. tx now has the option '+Z' to force this to occur.

2.0.25 (released 2007-03-30)
----------------------------

- \[autohint\] Added a new option to allow extending the list of glyph
  names for which autohint will try to make counter hints.
- \[autohint\] Fixed bug where Type 2 operator stack limit could be
  exceeded when optimizing Type 2 charstrings during conversion from
  bez format.
- \[autohint\] Fixed bug in setting OtherBlues alignment zone values.
- \[FontLab macros\] The Autohint macro behaves quite differently when
  adding 'flex' hints is turned off; it makes more hint substitutions,
  since these are not allowed within the segment of the outline that
  contributes the 'flex' stem. Turned it on, so that hint results will
  be the same as the command-line tool. This does not affect the outline
  data.
- \[checkOutlines\] Fixed bug that prevented the reporting of two
  successive points with the same coordinates. The code to convert from
  the source outline data to bez format was suppressing zero-length line
  segments, so the checkOutlines module never experienced the problem.
- \[compareFamily\] Added new options '-st n1,n2..' and '-ft n1,n2..' to
  allow executing only specific tests.
- \[compareFamily\] Fixed test "Warn if a style-linked family group does
  not have FamilyBlues". When reporting the error that FamilyBlues differ
  in a style-linked family group (where at least one font does have real
  FamilyBlues), use BlueValues as implied FamilyBlues when the latter
  attribute is missing from a font. Same for FamilyOtherBlues.
- \[compareFamily\] Warn about zones outside of font's BBox only if the
  entire zone is outside of the BBox, not just one edge, and warn only
  for BlueValue zones, not FamilyBlueValue zones.
- \[compareFamily\] Fixed fsType check. Complain if fsType is not 8 only
  for Adobe fonts, determined by checking if the name table trademark
  string is empty or contains "Adobe".
- \[compareFamily\] Fixed Single Face Test 3 to compare the CFF Full Name
  with the name table Preferred Full Name (ID 18) rather than the Full
  Name (ID 4).
- \[compareFamily\] Fixed bug where it failed with CID fonts, because it
  referenced the "Private" dict attribute of the font's topDict, which
  does not exist in CID fonts.
- \[compareFamily\] Fixed 'size' test to support the format that indicates
  only intended design size, where no range is supplied.
- \[compareFamily\] Fixed ligature width check to also check that left
  and right side bearings match those of the left and right components,
  and to use the 'liga' feature to identify ligatures and their components,
  instead of heuristics based on glyph names.
- \[makeotf\] Disallowed negative values in the feature file for the OS/2
  table winAscent and winDescent fields.
- \[makeotf\] Fixed a bug where a lookup excluded with the `exclude_dflt`
  keyword was nevertheless included if the script/language was specified
  with a languagesystem statement.
- \[makeotf\] Fixed issue on Windows where a user would see a debug
  assert dialog when the OS/2 vendorID was not specified in the feature
  file, and the Copyright string contained an 8-bit ASCII character, like
  the 'copyright' character.
- \[makeotf\] Fixed issue on Windows where name ID 17 would be garbage if
  no FontMenuNameDB was supplied, and the PostScript name did not contain
  a hyphen.
- \[makeotf\] Added warning for Mac OSX pre 10.5 compatibility: total size
  of glyphs names plus 2 bytes padding per glyph must be less than 32K, or
  OSX will crash.
- \[makeotf\] Fixed crash that occurred if the feature file did not have
  a languagesystem statement.
- \[makeotf\] Fixed bug in subroutinizer which allowed a subroutine stack
  depth of up to 10, but the Type 1 and Type 2 specs allow only 9. This
  caused most rasterizers to declare the font invalid.
- \[makeotf\] Removed '-cv' option; CJK vertical CMaps have not been
  supported since FDK 1.6.
- \[spot\] Added support for low-level and feature file style
  text dumps of GPOS Attachment formats 3, 4, 5 and 6.
- \[spot\] Added dump of lookup flag value to the feature-file style
  report.
- \[spot\] Added MarkAndAttachmentClassDef record to GDEF table report.
- \[spot\] Added support for GSUB lookup type 2 (Multiple) when within
  contextual substitutions.
- \[spot\] Fixed bug in GSUB lookup 5, causing crash in dumping trado.ttf.
- \[spot\] Fixed bug in level 7 (feature-file syntax) dump of GPOS table;
  was omitting the value record for extension lookup types.
- \[spot\] Fixed crash on Windows when proofing contextual substitution
  statements.
- \[spot\] Made Windows version behave like Mac when proofing: PostScript
  file data is always sent to standard output, and must be re-directed to
  a file.
- \[spot\] Improved documentation of proofing output and '-P' option.
- \[spot\] Fixed DSIG table reporting of TTC fonts with the version 2 TTC
  header, even if the header reports it is version 1, like meiryo.ttc.
- \[spot\] Enabled proofing TTC fonts that don't have glyph names in the
  post table.
- \[spot\] Fixed origin offset of bounding box for TTF fonts.
- \[spot\] Fixed crash in proofing TTF fonts when the last glyph is
  non-marking, like trado.ttf in LongHorn.

2.0.24 (released 2006-11-06) — Public release of FDK 2.0
----------------------------

- \[autohint/checkOutlines/ProofPDF\] Fixed glyph name handling to avoid
  stack dump when glyph is not in font. Added support for CID values that
  are not zero-padded to 5 hex digits.
- \[autohint\] Fixed bug where edge hints would be generated rather than
  regular hints across the baseline, when there were fewer than 8 pairs of
  BlueValues.
- \[checkOutlines\] Fixed bug where it would not report an overlap if
  there were an even number of overlapping contours.
- \[compareFamily\] Fixed Italic angle Single Test 12 to look at the middle
  third of the test glyph stems, rather than the top and bottom of the
  glyph bounding box, when guessing the font's italic angle.
- \[compareFamily\] Fixed Single Test 15 to allow a difference of 1 unit in
  the font BBox, to allow for rounding differences.
- \[compareFamily\] Fixed Single Test 26 to identify uXXXX names as valid
  Unicode names; had bug in the regular expression that required 5 digits.
- \[compareFamily\] Fixed Single Test 22 to treat glyphs in the combining
  mark Unicode range u3000-u036F range as accent glyphs; require that they
  have the same width as the base glyph.
- \[compareFamily\] Changed report from Error to Warning for check that
  only the first four Panose values are non-zero.
- \[compareFamily\] Fixed bug that caused a stack dump in Single Test 16
  and 22.
- \[compareFamily\] Added tests for Mac OSX pre 10.4 compatibility: no
  charstring is < 32K, total size of glyphs names plus padding is less
  than 32K.
- \[compareFamily\] Added test that known shipping font does not have OS/2
  table version 4, and that new fonts do.
- \[compareFamily\] Fixed Single Test 11: allow BASE offset to differ from
  calculated offset by up to 10 design units before it complains.
- \[compareFamily/makeotf\] Fixed failure when tools path contains a space.
- \[kernCheck\] New tool; looks for kern GPOS rules that conflict, and also
  looks for glyph pairs that overlap.
- \[kernCheck\] Added option to allow running only the check of GPOS
  subtables for kerning rules that mask each other.
- \[makeotf\] Fixed '-adds' option.
- \[makeotf\] Added new option '-fi' to specify path to fontinfo file.
- \[makeotf\] Added new option '-rev' to increment the fontRevision field.
- \[makeotf\] If the (cid)fontinfo file contains the keyword/value for
  FSType, will check that the value is the same as the OS/2 fsType field
  in the final OTF font. This has to do with historic Adobe CJK font
  development practices.
- \[makeotf\] Added support for setting some of the Plane 1+ bits in the
  OS/2 ulUnicodeRange fields.
- \[mergeFonts\] Will now apply to the output font the values for the
  fields Weight and XUID, from the cidfontinfo file.
- \[spot\] Added support for showing some of the Plane 1+ bits in the OS/2
  ulUnicodeRange fields.
- \[stemHist\] When requiring that reports not already exist, don't delete
  older stem reports when asking for new alignment zone reports, and
  vice-versa.
- \[setsnap.pl\] New tool to pick standard stem hint values. This Perl
  script takes in the report from stemHist, and recommends a set of values
  to use for the Type 1 font standard stem arrays. This is not as good as
  choosing the most relevant values yourself, but better than not providing
  any values.
- In _Overview.html_, added warning about 'languagesystem Dflt dflt' and FDK
  1.6 feature files.
- In _MakeOTFUserGuide.pdf_, expanded discussion of fontinfo file, updated
  documentation of OS/2 v4 table bits with Adobe's practices for the next
  library release.
- In _OT Feature File Syntax.html_, fixed incorrect sign for winAscent
  keyword, extended discussion of DFLT script tag and useExtension keyword,
  and fixed minor typos.
- Added two new tech notes on using rotateFont and mergeFonts.

2.0.22 (released 2006-09-12)
----------------------------

- \[compareFamily\] Single Test 3 now also checks that Mac name ID 4 starts
  with Preferred Family name, and is the same as the CFF table Full Name.
- \[compareFamily\] Added test for existence and validation of BASE table
  in Single Test 11.
- \[compareFamily\] Fixed bug where failed when reporting font BBox error.
- \[compareFamily\] Added test that some specific glyph names were not
  changed from previous version of font, in Single Test 26.
- \[compareFamily\] Added "Single Face Test 27: Check
  strikeout/subscript/superscript positions". Checks values against default
  calculations based on the em-box size.
- \[compareFamily\] Added "Single Face Test 28: Check font OS/2 codepages
  for a common set of code page bits". Checks OS/2 ulCodePageRange and
  ulUnicodeRange blocks against the default makeotf heuristics.
- \[compareFamily\] Added in Single Test 12 a rough calculation of the
  italic angle. Warns if this is more than 3 degrees different than the
  post table Italic angle value.
- \[compareFamily\] Added in Family Test 15 a check that all fonts in a
  preferred family have the same hhea table underline size and position
  values.
- \[compareFamily\] Added "Family Test 16: Check that for all faces in a
  preferred family group, the width of any glyph is not more than 3 times
  the width of the same glyph in any other face".
- \[compareFamily\] Fixed Family Test 3 to be more efficient.
- \[makeotf/makeotfexe\] Added a new option '-maxs `<integer>`' to limit
  the number of subroutines generated by subroutinization. Used only when
  building test fonts to explore possible errors in processing the
  subroutines.
- \[makeotf/makeotfexe\] Allow working names to be longer than 31
  characters; warn but don't quit, if final names are longer than 31
  characters.

2.0.21 (released 2006-08-31)
----------------------------

- \[makeotf\] Fixed bug where 'size' feature was built incorrectly when it
  was the only GPOS feature in the font.
- \[spot\] Improved error messages for 'size' feature problems.
- \[compareFamily\] Added dependency on environment variables:
  **CF_DEFAULT_URL** should be set to the foundry's URL; it's compared
  with name ID 11.
  **CF_DEFAULT_FOUNDRY_CODE** should be set to the foundry's 4-letter
  vendor code; it's compared with OS/2 table achVendID field.
- \[compareFamily\] Check that CFF PostScript name is the same as Mac and
  Windows name table name ID 6.
- \[compareFamily\] Check that named IDs 9 and 11 (designer name and
  foundry URL) exist.
- \[compareFamily\] Extended Single Test 4 to verify that version string
  is in correct format '(Version|OTF) n.nnn'.
- \[compareFamily\] Improved Panose test to check that values are not all
  0, and that the CFF font dict 'isFixedPitch' field matches the Panose
  monospace value.
- \[compareFamily\] Added check to confirm the presence of Unicode cmap
  sub table.
- \[compareFamily\] Added check to confirm that latn/dflt and DFLT/dflt
  script/languages are present, if there are any GPOS or GSUB rules. Also
  checks that script and language tags are in registered lists, and that
  all faces have the same set of language and script tags, and feature
  list under each language and script pair.
- \[compareFamily\] Added check to confirm that all faces in the family
  have the same OS/2 table fsType embedding permission values.
- \[compareFamily\] Added check to confirm that if font has Bold style
  bit, the CFF forceBold flag is on. Also vice-versa, if the font weight
  is less than 700.
- \[compareFamily\] Added check to confirm that the font does not have a
  UniqueID or XUID, if it's not CID-keyed.
- \[compareFamily\] Added glyph name checks: OS/2 default char is .notdef,
  and that there are NULL and CR glyphs in TrueType fonts, and that names
  conform to the current Adobe Glyph Dictionary. Note that latest practice
  is to use 'uni' names for final names for all the 'afii' glyphs.
- \[compareFamily\] Fixed family BlueValues tests to compare within
  compatible family name groups.
- \[compareFamily\] Changed Family Test 2 to check that all name IDs
  except 16, 17, 18, are all present with the same language/script values
  within all faces of a preferred family.
- \[compareFamily\] Changed Single Test 3, which didn't do at all what it
  described.
- \[FontLab macros\] Fixed bug introduced when changing modules shared
  with command-line scripts in build 19.

2.0.20 (released 2006-08-14)
----------------------------

- \[ProofPDF\] Fixed bug in waterfallplot mode, where Acrobat would
  report the output PDF file as being damaged.
- \[makeotf\] Fixed bug that prevented building CID fonts in release
  mode, introduced in build 19.

2.0.19 (released 2006-08-04)
----------------------------

- \[compareFamily\] Added Family Test 13 to report error if two fonts in
  the same preferred family have the same OS/2 weight, width and Italic
  settings, and the OS/2 version is greater than 3. Also reports an error
  if the fsSelection field bit 8 "WEIGHT_WIDTH_SLOPE_ONLY" is set
  differently across faces of the same preferred family name group.
- \[compareFamily\] Fixed Family Test 12 to not fail when the font has a
  script/language with no DefaultLangSys entry.
- \[makeotf\] If a font file with the requested output file name already
  exists, will delete it before running makeotfexe, so can tell if it
  failed.
- \[makeotf\] Will now set the new 'fsSlection' bits if the following
  key/value pairs are in the 'fontinfo' file:

        PreferOS/2TypoMetrics 1
        IsOS/2WidthWeigthSlopeOnly 1
        IsOS/2OBLIQUE 1

- \[digiplot\] Added new option to specify the font baseline, so the
  baseline can be set correctly when proofing a font file without a BASE
  table.
- \[digiplot\] Allowed using a CID layout file to supply meta info when
  proofing name-keyed fonts.
- \[ProofPDF\] Added two functions: **waterfallplot** and **fontsetplot**.
  waterfallplot does not yet work with TrueType or CID-keyed fonts.

2.0.17 (released 2006-05-15)
----------------------------

- Fixed multiple tools to allow installing the FDK on Windows on a path
  containing spaces.
- \[autohint\] Added option to suppress hint substitution.
- \[autohint\] Fixed help and message to refer to 'autohint' tool name,
  rather than to the AC script file name.
- \[autohint\] Fixed bug in processing hint masks: bytes with no bits set
  were being omitted.
- \[autohint\] Added option to allow hinting fonts without StdHW or StdVW
  entries in the font Private font-dictionary.
- \[checkOutlines\] Fixed writing out the changes when fixing outlines.
- \[checkOutlines\] Fixed bug that mangled outlines when three alternating
  perpendicular lines or vh/hv/vv/hh curveto's followed each other.
- \[checkOutlines\] Will now write report to a log file as well as to
  screen. Added option to set log file path, and added numeric suffix to
  name so as to avoid overwriting existing log files.
- \[compareFamily\] Fixed issue that happened when looking through the
  directory for font files, when encountering a file for which the user
  did not have read permission.
- \[compareFamily\] Added Single Test 24: check that 'size' feature
  Design Size is within the design range specified for the font.
- \[ProofPDF\] Added **showfont** command to show how to customize a
  command file to produce a different page layout.
- \[ProofPDF\] Fixed so fonts with em-square other then 1000 will work.
- \[fontplot/charplot/digiplot/hintplot/showfont\] Added support for
  Type 1 font files as well as OTF and TTF files.
- \[makeotf\] Fixed MakeOTF to survive being given a font path with spaces.
- \[makeotf\] Fixed '-S' and '-r' options.
- \[makeotf\] Added new option '-osv `<number>`' to allow setting the OS/2
  table version number.
- \[makeotf\] Added new option '-osbOn `<number>`' to set arbitrary
  bitfields in OS/2 table 'fsSelection' to on. May be repeated more than
  once to set more than one bit.
- \[makeotf\] Added new option '-osbOff `<number>`' to set arbitrary
  bitfields in OS/2 table 'fsSelection' to off. May be repeated more than
  once to unset more than one bit.
- \[makeotf\] If neither '-b' nor '-i' options are used, check for a file
  'fontinfo' in the same directory as the source font file, and set the
  style bits if these key/values are found:

        IsBoldStyle true
        IsItalicStyle true

- \[FontLab macros\] Built the autohint and checkOutline libraries (PyAC
  and focusdll) linked with Python2.3 so they work with FontLab 5.
- \[mergeFonts\] Added option to copy only font metrics from first source
  font.
- \[mergeFonts\] Allow empty lines and "#" comment lines in glyph alias
  and in cidfontinfo files.
- \[rotateFont\] Fixed bug where it did not allow negative numbers.
- \[rotateFont\] Allow empty lines and "#" comment lines in rotation info
  file
- \[sfntedit\] Fixed so that it will not leave the temp file behind on a
  fatal error, nor quit because one already exists.
- \[spot\] Fixed order of backtrack glyphs in dump of chaining contextual
  `sub` and `pos` statements. Now assumes that these are built in the
  correct order.
- Added two new tools, **type1** and **detype1**, that compile/decompile
  a Type 1 font from/to a plain text representation.

2.0.5 (released 2006-02-14)
---------------------------

- \[compareFamily\] Added warning if sum of OS/2 table sTypoLineGap,
  sTypoAscender, and sTypoDescender is not equal to the sum of usWinAscent
  and usWinDescent.
- \[compareFamily\] Updated test for allowable weights in style-linked
  faces to reflect the current behavior of Windows XP.
- \[compareFamily\] Added check for OpenType/CFF: Windows name table ID 4
  (Full Name) is the same as the PostScript name.
- \[compareFamily\] Added report on sets of features in different
  languagesystems, and an error message if they are not the same in all
  faces of the font.
- \[compareFamily\] Fixed incorrect message about real error when menu
  names are not correctly built.
- \[compareFamily\] Fixed test for improbable FontBBOX to use em-square
  rather than assume 1000 em.
- \[compareFamily\] Added warning if widths of ligatures are not larger
  than the width of the first glyph.
- \[compareFamily\] Added warning if accented glyphs have a width different
  than their base glyph.
- \[compareFamily\] Added error message if two faces in the same family
  have the same OS/2 width and weight class and italic style setting, and
  are not optical size variants. Optical size check is crude: the Adobe
  standard optical size names (Caption, Capt, Disp, Ds, Subh, Six) are
  removed from the PS font names and then compared; if the PS names are
  the same, then they are assumed to be optical size variants.
- \[compareFamily\] Added check that no hint is outside the FontBBox, for
  CJK fonts only.
- \[spot/otfproof\] Added "Korean" to list of tags for OS/2 codepage range.
- \[spot/otfproof\] Fixed dump of 'size' feature to support correct and old
  versions
- \[spot/otfproof\] Added dump/proof of contextual chaining positioning
  format 3.
- \[spot/otfproof\] Added warnings that only low-level dump of other
  contextual lookups is supported.
- \[makeotf\] Program is now a stand-alone C executable.
- \[makeotf\] Removed option to write contextual positioning rules in the
  _old_ incorrect format.
- \[makeotf\] MakeOTF no longer assigns Unicode Private Use Area values
  to glyphs for which it cannot identify. To use PUAs, explicitly assign
  them in the GlyphOrderAndAlias file.
- \[makeotf\] Fixed bug in name table name ID "Version": if version decimal
  value is x.000, then the value in the Version name ID string was x.001.
- \[makeotf\] Fixed bug in handling of DFLT language script: it's now
  possible to use this tag.
- \[makeotf\] Fixed feature file parsing bug where 'dflt' lookups in one
  feature were applied to the following feature if the following feature
  started with a language statement other than 'dflt'.
- \[makeotf\] Fixed serious bug where wrong width is calculated for glyphs
  where the CFF font Type 2 charstring for the glyph starts with a width
  value. This is then followed by the value pairs for the coordinates for
  the vertical hint, and then these are followed by a hint mask or control
  mask operator. The bug was that when MakeOTF reads in the charstring in
  order to derive the hmtx width, it discards the data before the control
  mask operator, leading the parser to use the CFF default width for the
  glyph.
- \[makeotf\] vhea.caretSlopeRise and vhea.caretSlopeRun is now set to 0
  and 1 respectively, rather than the opposite.
- \[makeotf\] The OS/2 table 'fsType' field is now set to the feature file
  override. If not supplied, then the value of the environment variable
  FDK_FSTYPE. If not set then 4 (Preview & Print embedding).
- \[makeotf\] Added support for contextual chaining positioning of base
  glyphs; mark and anchors not yet supported.
- \[makeotf\] Fixed bug in 'size' feature: the feature param offset is now
  set to the offset from the start of the feature table, not from from the
  start of the FeatureList table.
- \[makeotf\] Allowed 'size' feature point sizes to be specified as
  decimal points, as well as in integer decipoints.
- \[makeotf\] OS/2 table version is now set to 3.
- \[makeotf\] Added OS/2 overrides for winAscent and winDescent.
- \[makeotf\] Added hhea overrides for Ascender/Descender/LineGap.
- \[makeotf\] Set OS/2 Unicode coverage bits only if the font has a
  reasonable number of glyphs in the Unicode block; it was setting the
  bits if the font had even one glyph in the block.
- \[makeotf\] The "Macintosh" codepage bit in the OS/2 codePageRange fields
  is now set by default.
- \[FEA spec\] Fixed incorrect range example in section _2.g.ii. Named
  glyph classes_
- \[FEA spec\] Changed rule to allow lookup definitions outside of feature
  definitions in FDK 2.0.
- \[FEA spec\] Fixed incorrect uses of 'DFLT' rather than 'dflt' for a
  language tag.

1.6.8139 (released 2005-08-30)
------------------------------

- \[OTFProof\] Fixed error in dumping GSUB table: GSUB lookup 5,
  context lookup assumed that there were both a lookahead and a backtrack
  sequence.
- Updated SING META table tags to latest set.

1.6.7974 (released 2004-08-30)
------------------------------

- \[makeotf\] Fixed rule in building CJK font. When looking for Adobe
  CMap files, will no longer use a hardcoded max supplement value when
  building paths to try.

1.6.7393 (released 2004-01-14)
------------------------------

- \[compareFamily\] Fix stack dump when family has no BlueValues
  (reported by House Industries).
- \[compareFamily\] Fix stack dump when CFF-CID font has glyph with no
  `subr` calls.
- \[OTFProof\] Corrected error in last release, where spaces between
  ligature match string names were omitted.
- \[FontLab macros\] Added scripts for testing if the joins in a
  connecting script font design are good.
- \[OTFProof\] Fixed crash on proofing or dumping feature file syntax for
  GSUB lookup 5, context lookup. Also fixed rule-generating logic:
  results were previously wrong for proof and for feature-file syntax
  formats. Text dump has always been correct.
- \[OTFProof\] Fixed crash when dumping cmap subtables that reference
  virtual GIDs not in the font.
- \[OTFProof\] Fixed crash on dumping GSUB lookup type 6 chaining context
  subtable format 2. This has never worked before.
- \[OTFProof\] Added demo for SING glyphlet tables, SING and META.
- \[FontLab macros\] Added scripts for reading and writing an external
  composites definition text file.
- \[FontLab macros\] Added scripts for working with MM fonts.

1.6.6864 (released 2003-10-08)
------------------------------

- \[OTFProof\] Fixed crash after dumping contents of ttc fonts (bug
  introduced in version 6792).
- \[OTFProof\] Fixed cmap subtable 4 and 2 dumps. Cmap subtable 2 could
  show encoding values for single byte codes which were really the
  first byte of two byte character codes. In Format 4, idDelta values
  were not being added when the glyphindex was derived from the glyph
  index array. These show issues show up in some TTF CJKV fonts.

1.6.6792 (released 2003-09-24)
------------------------------

- \[OTFProof\] Fixed crash when proofing fonts with _many_ glyphs.
- \[OTFProof\] Restored "skipping lookup because already seen in
  script/language/feature" messages to proof file, which was lost in
  version 6604.
- \[OTFProof\] Added ability to proof resource fork sfnt fonts from Mac
  OSX command line. It's still necessary to use the SplitForks tool to
  make a data-fork only resource file, but spot/otfproof can now navigate
  in the resulting AppleDouble formatted resource file.
- \[OTFProof\] Added support for a text dump of the GDEF table.
- \[OTFProof\] Changed title in 'size' feature dump from _Common
  Characters_ to _name table name ID for common Subfamily name for size
  group_.
- \[AGL\] Fixed some minor errors in the Adobe Glyph List For New Fonts.

1.6.6629
--------

- \[OTFProof\] Fixed bug in dumping KERN table from Mac sfnt-wrapped
  resource fork Type 1 MM font.
- \[OTFProof\] Changed the AFM-format dump of kern pairs to list all
  kern pairs for each language/script combination in separate blocks, and
  to eliminate all class kern pairs that are masked by a singleton kern
  pair. The temp buffer file path is now taken from the system C library
  function tmpnam(), and is not necessarily in the current directory.

1.6.6568
--------

- \[OTFProof\] Fixed command-line tool to write proof files in same
  location as font, and with font-name prefix, when not auto-spooled for
  printing.
- \[OTFProof\] Fixed bug in UI version where proofing GSUB features and
  then GPOS features would cause the GPOS feature proof files to be empty.
- \[makeotf\] Fixed heuristics for picking OS/2 weight/width so that a
  font name containing _ultracondensed_ would trigger only setting the
  width, and not the weight as well.
- Updated Mac OS project files to CodeWarrior 8.

1.6.6564
--------

- \[OTFProof\] When dumping data from TTF fonts, now add `@<gid>` to all
  glyph names. This is because the rules for deriving names can lead to
  two glyphs being given the same name.
- \[OTFProof\] Fixed bug in proofing GPOS class kern pairs: was generating
  bogus kern pairs and duplicate kern pairs when the coverage format was
  type 2. Affects proof file only, not AFM or feature-format dump.
- Fixed memory overwrite bug encountered by Goichi and cleaned up various
  memory leaks in the process.
- \[compareFamily\] Added report on whether a face contains known std
  charset. Stub implementation - still need list of std charsets.
- \[AFM2Feat\] Developed tool to convert an AFM file to a file with
  kern feature `pos` rules.

1.6.6148
--------

- Rebuilt all libraries for v1.6 release 3/10/2003.

1.6.6048
--------

- Updated FinishInstall.py to reflect Python 2.2 requirements.
- Picked up last MakeOTF.pdf editing changes.
- Fixed bug in GOADB.
- Updated CID font data in example fonts.
- Updated FDK release notes and installation instructions.
- Updated to use the GlyphOrderAndAliasDB file developed while
  converting the Adobe Type Library. Maps all the old glyph names to AGL
  compatible names.

1.6.6020
--------

- \[OTFProof\] Fixed crash in handling of VORG with no entries. (Vantive
  574752)
- \[MakeOTF\] Updated documentation: added a description of how all three
  columns of the _GlyphOrderAndAliasDB_ file are used; added a new section
  on the key-value pairs of the font project file; updated the description
  of the FontMenuNameDB file entries; added minor clarifications throughout.
- Updated _digital_signature_guide.htm_ to match current Verisign website.
- \[Example fonts\] Changed the incorrect language keyword TUR to TRK.
- \[Example fonts\] Removed the many key/value pairs in the fontinfo files
  that are not used by MakeOTF.
- \[OTFProof/spot\] Fixed 3-column handling of GOAADB. (Vantive 569681)

1.6.5959
--------

- \[MakeOTF\] Suppressed the "repeat hint substitution discarded" message
  from the source file parsing library. These are so common that they
  obscure more useful messages.
- \[MakeOTF\] Set as default the option to build chaining contextual
  substitution rules with the incorrect format used by InDesign 2.0 and
  earlier.
- \[MakeOTF\] If the option above is set, then MakeOTF will write a name
  ID (1,0,0,5 - "Version") which will contain the text string which
  triggers special case code in future Adobe apps so that it will process
  the chaining contextual substitution rules as they were intended. If
  this option is NOT set, the name ID 5 will be written so as to not
  trigger this special case code. The special case treats specially any
  font where the name table name ID (1,0,0,5) exists and either matches,

        "OTF[^;]+;PS[^;]+;Core 1\.0\.[23][0-9].*"

  (example: "OTF 1.006;PS 1.004;Core 1.0.35")
  or contains,

        "Core[^;]*;makeotf\.lib"

  (example: "Core 1.0.38;makeotf.lib1.5.4898")
  or just,

        "Core;makeotf.lib"

- \[MakeOTF\] Turn off by default the option to force the .notdef glyph
  in the output OTF font be an crossed rectangle with an advance width
  of 500.
- \[MakeOTF\] Added rule to force the OS/2 WeightClass to always be at
  least 250. Shows error message if set or calculated WeightClass was less
  than this.
- \[MakeOTF\] Added test that FSType is set the same in the feature file
  as in source CID font files.
- \[OTFProof\] Page layout for CJKV font vertical layout: now writes the
  vertical columns right to left.
- \[OTFProof\] When writing vertical features, now shows the advance width
  sign as negative.
- \[OTFProof\] When making PostScript proof file, now writes DSC tags with
  correct header and page info.
- Added _Unicode and Glyph Name_ documentation to the FDK _Technical
  Documentation_ directory, to allow access to this info under the FDK
  license.

1.6.4908
--------

- \[MakeOTF/FEA syntax\] Added new vmtx table overrides, to allow setting
  vertical metrics for pre-rotated proportional glyphs that are
  specifically designed and are not simply rotated forms of proportional
  glyphs.
- \[MakeOTF/FEA syntax\] Added new OS/2 overrides to set the Unicode and
  Windows codepage range fields: UnicodeRange CodePageRange.
- \[MakeOTF/FEA syntax\] Updated language keywords to be consistent with
  OpenType spec, i.e using `dflt` instead of `DFLT`. Expanded section
  explaining use of language and script default keywords. Old keywords
  still work, but cause a warning to be emitted.
- \[FEA syntax\] Expanded explanation of kern class pairs and subtable
  breaks.
- \[MakeOTF\] Updated the search rules for CID font CMap files to support
  Adobe-Japan2-0, and to look first for UTF-32 CMAP files.

1.5.4987
--------

- Release to Adobe website Sept 2002.

1.5.4908
--------

- \[MakeOTF\] Changed the name table version string to match OT spec 1.4.
- \[CompareFamily\] Made it _really_ work with Sept 10th 2002 release of
  Just van Rossum's FontTools library.

1.5.4492
--------

- \[MakeOTF\] (hotlib 1.0.35) Fixed the error in processing GSUB
  contextual chaining substitution format 3. This was originally done
  according to the OpenType spec 1.4, which is in error by the
  established implementation of VOLT and Uniscribe. Added option '-fc' to
  cause the library to use the incorrect implementation, according to OT
  spec v1.4. By default, MakeOTF builds the correct contextual format
  per spec v1.5.
- \[MakeOTF\] (hotlib 1.0.35) Fixed Unicode cmap bug in assigning the OS/2
  table field usLastCharIndex. This is supposed to be the highest Unicode
  value in the BMP-16 cmap tables. The problem was in the logic by which
  alternates of supplemental plane glyph names were being assigned an EUS
  code, but not added to the BMP-16 Unicode cmap tables, e.g. u1D269.alt.
  When one of these alternates was assigned an EUS value, the
  usLastCharIndex was getting bumped even though the glyph was not being
  added to the BMP-16 cmap tables. Fixed by not incrementing
  usLastCharIndex in this case.
- \[MakeOTF\] Fixed bug in applying client-supplied Unicode override
  values. These were omitted if the glyph names in the font were different
  than the final glyph names, as can happen when the client uses the
  getFinalGlyphName call back to supply a glyph production name which is
  different than the final glyph name.
- \[OTFProof\] Fixed crash when proofing liga feature in CID font. Also
  fixed crash when proofing charstring with only one operand, e.g
  h/r/vmoveto.
- \[CompareFamily\] Updated to use the latest version of Just van Rossum's
  FontTools library, added support for TrueType fonts. Now requires Python
  2.2.
- \[CompareFamily\] Added family test 11: verify that for base font in
  style-linked group, Mac and Windows menu names are the same, and that
  for other fonts in the style linked group, the Mac and Windows menu
  names  differ.

1.5.4099
--------

- External release of FDK 1.5 on Adobe website.

1.5.3849
--------

- \[CompareFamily\] Fixed tabular glyph and isFixedPitch test so that they
  are now useful - used to generate far too many false errors.
- \[MakeOTF\] Fixed bug in setting Panose values from a feature file
  override. If any value in the Panose value string is 0, all subsequent
  values were also set to 0.
- \[MakeOTF\] Fixed bug where glyphs that got renamed were not subjected
  to the ordering specified by the GlyphOrderAndAliasDB file.
- Added FDK.py file to integrate all tools into a common UI.
- \[OTFCompare\] Added use of CFFChecker library for CFF table.
- \[CFFChecker\] Added resource fork handling on OSX.
- \[CompareFamily\] Added family test 10: if any face in family has a real
  panose value, report derived panose values as an error.
- \[CompareFamily\] Fixed bug  in comparing copyright notices in family
  test 7: will now really report error only if differs in other than years.
- \[CFFChecker\] Added support for multiple input files.
- \[CFFChecker\] Added support for resource fork fonts under MacOS 9.
- Added CFFChecker interface to makeotf.
- \[OTFCompare\] Added OSX prompt-based support.
- Fix R-O-S mapping for CMAP files.
- Fixed getunicodeCmap() to not hard-wire Adobe-Japan1-3 when processing
  J fonts.
- \[CFFChecker\] MacOS 9 version created.
- Added CFFChecker.
- \[CompareFamily\] Fixed to not die on font menu names with non-std ASCII.
- \[OTFProof\] Fixed vertical metrics proofing.
- \[MakeOTF\] Added warning when truncating OS/2 TypoAscender to force its
  sum with TypoDescender to be equal to the em-box.
- \[MakeOTF\] Allow fractional synthetic weight values. These are rounded
  to an integer.
- \[MakeOTF\] Changed XUID adding algorithm to NOT add the revision number
  to the XUID array.
- \[MakeOTF\] In release mode, add current year to copyright, suppress (c)
  string, and fix spaces around the phrase 'All Rights Reserved'.
- \[MakeOTF\] Fixed to permit building a font in release mode with no
  unique ID at all.
- \[MakeOTF\] Fixed bad cmap entry offset calculation.
- \[MakeOTF\] Fixed for bad cmap table entry.

1.5.1023
--------

- \[MakeOTF\] Changed algorithm for adjusting advance width/lsb/rsb of
  non-slanted synthetic glyphs when adding to italic fonts.
- \[MakeOTF\] Fixed failure of re-ordering when NOT forcing use of marking
  notdef.
- \[MakeOTF\] Fixed interaction between 'Sigma' and synthetic 'summation',
  'Pi' and 'product'.
- \[spot\] Added the option to select which feature to dump in GPOS or
  GSUB=7 dumps.
- \[OTFProof\] Added support of TT instructions in compound glyphs.
- \[CompareFamily\] Fixed incorrect unwrapping T2 charstring subroutines.
  All previous reports on whether glyphs were hinted should be doubted.
- \[MakeOTF\] Tweaked horizontal spacing of upright glyphs in oblique fonts.
- \[MakeOTF\] Added support for "italicangle", "width" and "weight" keywords
  in FontMenuNameDB.
- \[SCM/makeotf/typecomp\] Fixed Euro-adding bug.
- \[OTFProof\] Removed header note "1000 units/em" from proofs.
- \[OTFProof\] Added support for cmap version 12.
- \[OTFProof\] Removed zero padding of CID values from text reports.
- \[OTFProof\] Reduced number of warnings about missing characters.
- \[OTFProof\] Removed warning when GPOS and GSUB table may be too big,
  as no tools make this error anymore, and it is triggered
  inappropriately when font uses the extension lookup.
- \[OTFProof\] Fixed different spacing problem reported. (Vantive 420313)
- \[OTFProof\] Fixed so that vertical proofs write from right to left.
- \[MakeOTF\] Fixed problem with unspecified CMap files.

1.5.600
-------

- \[CompareFamily\] Fixed so that it will not error out when one of the
  Blues arrays is not present.
- \[OTFProof\] Fixed so that glyph names for CID fonts print properly.
- \[OTFProof\] Fixed problems with compile under SunOS.
- \[MakeOTF\] Added MakeOTFScript.py as an example file to edited, in
  order to allow scripting of makeOTF on the Mac (or any other platform).
  Minor changes to MakeOTF.py to fix this.
- \[MakeOTF\] Added an option to allow removing deprecated Type 1 operands
  from output font (e.g. `seac` and `dotsection`).
- \[MakeOTF\] Added an option to allow adding synthesized glyphs to fonts,
  leveraging a built-in sans and serif multiple master substitution font.
  The source font must contain a 'zero' and a capital 'O'.
  The glyphs that can be synthesized are:

        Euro
        Delta
        Omega
        approxequal
        asciicircum
        asciitilde
        at
        backslash
        bar
        brokenbar
        currency
        dagger
        daggerdbl
        degree
        divide
        equal
        estimated
        fraction
        greater
        greaterequal
        infinity
        integral
        less
        lessequal
        litre
        logicalnot
        lozenge
        minus
        multiply
        notequal
        numbersign
        onehalf
        onequarter
        paragraph
        partialdiff
        perthousand
        pi
        plus
        plusminus
        product
        quotedbl
        quotesingle
        radical
        section
        summation
        threequarters
        zero

1.4.583
-------

- Began tracking files by Perforce changelist label, from the Perforce
  source code management system.
- Updated compilers to Mac/CodeWarrior 6 Pro, Windows Visual C++ 6.0.
- Re-organized build directories to have mac/win/sun4 subdirectories.
- Re-organized shared include files to all be under /Programs/api, with
  non-conflicting names.
- \[Example fonts\] Updated MinionPro-Capt: now has correct frac and size
  features.
- \[Example fonts\] Added KozMinPro to samples.
- \[MakeOTF\] Fixed bug where fontinfo keyword IsStyleBold was ignored for
  CID fonts.
- \[MakeOTF\] Fixed Mac build project to load debug and release libraries
  by different names.
- \[MakeOTF\] Added feature file support for the "languagesystem" statement.
  Note that this entailed removing support for script, language, and named
  lookup statements in the size feature, and removing support for script and
  language statements in the aalt feature. See feature file spec for details.
- \[MakeOTF\] More descriptive wording in offset overflow error messages.
  Feature file error handling improved: multiple error messages are emitted
  before failing if possible, instead of just one; final glyph name as well
  as glyph alias (if applicable) reported if not found in font.
- \[MakeOTF\] Changed the 14 Corporate Use subarea Unicode values for Zapf
  Dingbats to the proposed UVs in anticipation of their being incorporated
  into the Unicode standard.
- \[MakeOTF\] Added FontWorks ('FWKS') to vendor ID list.
- \[MakeOTF\] Increased the maximum number of named lookups allowed to 8192.
- \[MakeOTF\] Now makes kern and vert features from kern data passed in by
  clients and from V CMap (respectively) only when the HOT_CONVERSION bit is
  set. (Previously, these features were made from the sources mentioned
  above if they weren't already defined in a feature file.)
- \[MakeOTF\] Fixed an obscure bug in OS/2.ulUnicodeRange computation: if
  the largest UV in the font were not in any Unicode range recognized by
  hotlib then it was counted as being in the next recognized Unicode range
  after the UV. (No known fonts are affected by this.)
- \[MakeOTF\] Forced the OS/2 codepage range bits for Chinese to either
  Simplified or Traditional, based on the Mac cmap script, if it is defined
  as either Simplified or Traditional, and will fall back to the heuristics
  if the script is undefined. If the mac.script is something other than a
  Chinese script, then the OS/2 codepage range bits for Chinese will not
  be set.
- \[OTFCompare\] The Python sys.path variable must now contain the path
  to the directory containing the OTFProof library (usually
  _FDK/Tools/Programs/otfproof/exe_). This replaces the hardcoded path
  reference in the OTFCompare.py script. On all platforms, this is done
  by adding the file "otfproof.pth", containing the path, to the Python
  installation.
- \[OTFCompare\] Fixed a bug that was causing tables smaller than 16 bytes
  to be reported as different
- \[OTFProof\] Added new proofing mode to CFF_ to print one glyph per page.
- \[OTFProof\] Added new proofing option to suppress file-specific header
  info to facilitate diff-ing of multiple proofs.
- \[OTFProof\] Added alphabetical sorting of AFM-style dump.
- \[OTFProof\] Fixed bug causing GPOS/GSUB features with digits in their
  names to not appear in the proofing list.
- \[OTFProof\] Added support for glyphsize option in CFF_ dumps.
- \[OTFProof\] Fixed conflicting include file names; must now specify
  include paths in project file.
- \[OTFProof\] Reduced some of the recursion in the subroutinization code
  to reduce stack space requirements.
- \[OTFProof\] Fixed support for included feature files in parent folder
  on the Mac.

1.3.2 (2000-10-24)
------------------

- \[OTFProof\] Fixed bug where would report error opening Mac TTF suitcase
  font, because data fork was of size 0.
- \[OTFProof\] Fixed bug where feature tags containing numbers were filtered
  out of the feature list for proofing.
- \[OTFProof\] Fixed bug where baseline was shown incorrectly for CJK fonts
  with baselines other than 120.
- \[OTFProof\] Fixed bug where y-placement changes were not shown correctly
  in vertical writing mode proofs.

1.3.1 (2000-08-15)
------------------

- \[MakeOTF\] Fixed problem with heuristics for OS/2 codepage range
  settings, for Chinese Simplified vs Traditional.
- \[MakeOTF\] Added macro to define MakeOTF version number.
- \[MakeOTF\] updated makeotflib help/usage messages: shown when args are
  incorrectly formatted.

- \[makeotf\] (makeotf/exe/makeotfutils.py)

  - added fontinfo list entry for "Language".
  - added 'parameter' variable entry for same.
  - increased num values to from 34 to 35.
  - changed initialization of 'parameter' so can more easily figure out
    which index matches which fontinfo field.

- \[makeotf\] (makeotf/exe/makeotf.py)

  - updated version numbers to 1.3.1.
  - added '-cs' and '-cl' options to help.
  - added processing of Language field, to set script and language IDs
    with '-cs' and '-cl' options.

- \[makeotf\] (makeotf/source/main.c)

  - added macro to define MakeOTF version number, used in help message,
    and in client name string for name id 5 "Version".
  - added mac_script and mac_language fields to global static 'convert'
    structure.
  - added processing of '-cs' and '-cl' arguments to parse_args().
  - added mac_script and mac_language arguments to call to cbconvert().
  - updated print_usage to match that of makeotf.py.
  - updated the ReadFontInfo() to process new Language field.

- \[makeotf\] (makeotf/source/cb.c)

  - moved initialization (as type unknown) of mac.encoding, mac.script
    and mac.language from cbconvert to cbnew().
  - added setting of mac.script and mac.language to cbconvert(), from
    arguments.
  - added mac_script and mac_language arguments to call to cbconvert().

- \[makeotf\] (source/includes/cb.h)

  - added mac_script and mac_language arguments to call to cbconvert().

- \[hotconvlib\] (coretype/source/map.c)

  - changed logic for setting OS/2 codepage range to set code page to
    Simplified or Traditional Chinese based on mac.script setting;
    fallback on heuristics only if mac.script is not set.
