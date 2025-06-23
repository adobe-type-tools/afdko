Adobe Font Development Kit for OpenType (AFDKO) Overview

Adobe Font Development Kit for OpenType (AFDKO) Overview
========================================================

# 1. Introduction

This release of the Adobe Font Development Kit for OpenType (AFDKO) contains a set of tools used to make OpenType fonts by adding the OpenType-specific data to a TrueType or Type1 font. It does not offer tools for designing or editing glyphs. These command line programs will allow you to build an OpenType font from an existing font, verify the contents of the font, and proof the font on screen or on paper. The AFDKO also contains useful technical documentation and some example font source material. Note! Although the FDK directory tree contains a number of Python scripts, none of them can be used by double-clicking on them; they can only be called by typing commands into a command line window.

See the [`afdko`](https://github.com/adobe-type-tools/afdko) home page for installation instructions.

Once the AFDKO is installed, all the command line programs can be run by entering the command name in a Terminal window (OSX, Windows, Linux), along with the necessary option text. All command line tools provide usage and help text with the options `-u` and `-h`.

# 2. Overview of Programs

The tools fall into several different functional groups.

## 2.1 Making/editing fonts

### `otfautohint`

This program is the Adobe auto-hinter. It is used by several AFDKO tools.
otfautohint can be used with UFO files (cubic outlines), and OpenType/CFF or Type 1 fonts.

### `checkoutlinesufo`

With the `-e` option, `checkoutlinesufo` is used to remove overlaps. When working with UFO files, the results of overlap removal are written to a new layer (`com.adobe.type.processedglyphs`), the original glyph is not touched.

### `makeotf`

This program will build an OpenType/CFF font from a font (source) file and auxiliry metadata files.
Typical input formats are
* UFO (Unified Font Object, font source format, cubic outlines)
* PFA (Type 1 font)
* TXT (decrypted plain-text version of a Type 1 font, obtained via `detype1`)
* TTF (TrueType source font file)

`makeotf` also optionally accepts further metadata files, e.g. a feature file that defines the OpenType layout rules, overrides for makeotf’s default values, a GlyphOrderAndAliasDB, etc. For details, see the makeotf user guide.

### `buildmasterotfs`

Step 1 of 2 to build a CFF Variable Font – this script builds static OTF files (with overlaps) from a .designspace file with compatible UFO masters. 

### `buildcff2vf`

Step 2 of 2 to build a CFF Variable Font – this script will use a .designspace file and the output OTFs of `buildmasterotfs` to assemble a CFF2 variable font file.

### `makeinstancesufo`

This script will build UFO instances from a .designspace file and a set of compatible UFO masters, to prepare for building static OTF fonts. By default, the instances are run through `checkoutlinesufo` (for overlap removal) and `otfautohint` (for PS hinting). Finally, the UFO instances are normalized.  
More info on designspace files and interpolation systems can be found at [Superpolator.com](https://superpolator.com/designspace.html) and [DesignSpaceDocument Specification](https://fonttools.readthedocs.io/en/latest/designspaceLib/index.html).

### `mergefonts`

This program will merge glyphs from one font into another, optionally copying a subset from the source fonts, and changing the names of the glyphs. It can also be used to subset and change the glyph names in a font. By using the same font more than once as a source with different mapping files, glyphs can be duplicated under other names. It can also convert a named-keyed font to a CID-keyed font.

### `otc2otf`

Extracts all OpenType fonts (.otf or .ttf) from an OpenType Collection font file (.ttc or .otc).

### `otf2otc`

Generates an OpenType Collection font file (.ttc) from two or more OpenType fonts (.otf or .ttf).

### `otf2ttf`

Converts OpenType-CFF fonts to TrueType.

### `rotatefont`

This tool will rotate and translate glyphs in a font, including the hints. However, hints will be discarded if the rotation is not a multiple of 90 degrees.

### `sfntedit`

This allows you to cut and paste the entire binary block of a font table from one font to another. You do this by first using it on a source font with the `-x` option to copy a table from the source font to a separate file, and then using it with the `-a` option to add that table to a target font. It can also be used to simply delete a table, and to fix the font table checksums.

### `otfstemhist`

This program is actually the same tool as `otfautohint` but with a different
"face". It provides reports which help in selecting the global hint data and
alignment zones for CFF hinting. You should look at the reports from this
tool in order to select the most common stem widths, and then use a program such
as FontLab or RoboFont to set the values in the font. This should be done before
hinting the font.

### `ttfcomponentizer`

Componentizes glyphs of a TrueType font with information from an external UFO font. The script only supports components that are not scaled, rotated or flipped.

### `tx`

This tool can be used to convert most font formats to CFF or Type 1 fonts. TrueType fonts will lose their hinting in the process. It cannot convert from PostScript to TrueType, and cannot rasterize TrueType fonts. When used to convert from a Type 1 or CFF font, it undoes subroutinization, so that each glyph contains all the drawing operators, rather than referring to subroutines for common path elements. However, it does have an option to apply subroutinization to the output font.

### `type1`/`detype1`

These two programs will respectively decrypt or encrypt a Type 1 font to and from a plain-text representation. This is useful for fixing or controlling individual data fields. It is also good for copying a specific path element to many glyphs.

## 2.2 Proofing

### `spot`

This program can report data from an OpenType font in a variety of ways. All tables can be shown as a text report of the low-level structure of the table. Most tables can be shown with a report in a more human-readable format. Some tables, including the GPOS and GSUB layout tables can be shown graphically, by outputting a PostScript file that can be converted to a PDF.

### `tx`

This tool has two modes that are useful for proofing. With the `-bc` option, it will rasterize a glyph to the command line window, using ASCII characters. This is surprisingly useful when you want a quick look at a glyph. With the `-pdf` option, it will write a PDF file which shows the glyphs in the font, either 320 per page or 1 per page.

### `ttx`

The XML format output offers a useful report for most of the data in an OpenType font.

### `charplot`

Make a PDF file showing all the glyphs in the font. Shows one glyph per page. Shows the nodes and control points.

### `digiplot`

Make a PDF file showing all the glyphs in the font. Format favored by CJK developers; shows a large filled outline, useful glyph metrics, and a larger outline with the nodes marked.

### `fontplot`

Make a PDF file showing all the glyphs in the font. This fills a page with many glyphs, and shows just the filled outlines at a small point size, with some info around each glyph. Works with OpenType fonts and Type 1 fonts.

### `fontsetplot`

Make a PDF file showing all the glyphs in in a list fonts. The glyphs are shown in glyph ID order, with a fixed spacing of one em, with the glyphs from one font in one line, so that all glyphs with the same glyph name from different fonts should be in a single column. This is used to visually confirm that that charsets are the same, and that glyph shapes are similar. It is useful for catching cases where a place-holder glyph outline was pasted in for a glyph name, but never corrected to the correct shape. The fonts are sorted first by length of character set, then by alphabetic order of the character sets, then by PostScript name.

### `hintplot`

Shows one glyph per page. Shows hints and alignment zones.

### `waterfallplot`

Shows a waterfall of point sizes for all glyphs in the font. This is used to evaluate hinting. In order to allow hinting to be seen, the font is embedded in the PDF, and glyphs are referenced by char code. Does not yet work with TrueType and CID-keyed fonts.

Note that the tools ending in “plot” are all simply small command-file scripts that call a single Python script with different options. The `-h` option for all these scripts will produce the same list of **all** the options supported by the single Python script. You can customize the PDF proofs by providing additional options. Look at the command files; these can be edited to supply additional options to the main script. This was completed shortly before the FDK was released, and has seen little use. It will improve over time. It uses the OpenSource Python package “Pickle” to create the PDF documents, and Just van Rossum's fontTools Python package for accessing the font data.

## 2.3 Validation

### `otfautohint`

The auto-hinting program will report at length about hinting issues. Some of these you can ignore, such as reports about near misses when a stem could be controlled by a hint-zone but is just a little too wide or too narrow. By adjusting either the stem widths or the hint-zones according to these reports, you can include more stems in the set that are controlled by hints, but you can also reasonably decide that is not worth the effort. However, many complaints do need fixing, such not having nodes at vertical or horizontal extremes of a curve.

### `checkoutlinesufo`

This tool can be used to report on the general quality of glyph outline data,  reporting outline overlaps, collinear points, and flat curves.

### `comparefamily`

`comparefamily` examines all the fonts in a directory and runs many quality checks. It will check consistency of data across a family of fonts, as well as in a single font. It will point out any errors in naming within a style-linked group.  
Typical options for running `comparefamily` are `-rm` (menu name report), `-rn` (metrics report), `-rp` (PANOSE number report).

### `sfntdiff`

This tool does a low-level comparison of two OpenType font files. It is most useful for quickly checking if two fonts are identical, or in which tables they differ.

### `ttxn`

This tool is used to test if two fonts are functionally the same. It sorts and modifies the output from the ttx and tx tools to build a normalized text dump that eliminates differences due to issues such as OTL table record order, glyph order and subroutinization differences. It writes one file for each table in the font, which can be `diff`ed in the next step. `ttxn` It is particularly useful in comparing older and newer versions of the same font.

### `tx`

This tool has a couple of modes that are good for testing. It will show only data about the font program within a font file, either CFF or TrueType. The `-dump` option is good for looking at a text report of the CFF font global and glyph data. The `-bc` option takes additional option that outputs a hash of the rasterized glyphs to a file. This is good for finding if hinting has changed in a font; if the hash files for a set of glyphs are the same between two fonts, then the glyphs rasterized identically at the specified point size. You can use this to judge if fonts are functionally equivalent, even if the outlines are not exactly the same. Finally, converting any font to CFF will yield error reports if the font data is not syntactically correct, and if any glyphs are not hinted.
