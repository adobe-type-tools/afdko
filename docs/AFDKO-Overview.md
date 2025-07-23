Adobe Font Development Kit for OpenType (AFDKO) Overview

Adobe Font Development Kit for OpenType (AFDKO) Overview
========================================================

# 1. Introduction

The Adobe Font Development Kit for OpenType (AFDKO) contains a set of tools for making make OpenType fonts by adding the OpenType-specific data to a TrueType or Type1 font. These command line programs will allow you to build an OpenType font from an existing font- or UFO file, to verify the contents of the font, and to proof the font on screen or on paper. The AFDKO also contains useful technical documentation and some example font source material.

See the [`afdko`](https://github.com/adobe-type-tools/afdko) home page for installation instructions.

Once the AFDKO is installed, all the command line programs can be run by entering the command name in a Terminal window, along with the necessary arguments. All command line tools provide usage (`-u`) and help (`-h`) information.

# 2. Overview of Programs

The AFDKO tools fall into several different functional groups.

## 2.1 Making/editing fonts

### `otfautohint`

`otfautohint` uses global hinting information (stems and alignment zones) to apply hints to a font or font source file. `otfautohint` can be used with UFO files (cubic outlines), and OpenType/CFF or Type 1 fonts.

### `checkoutlinesufo`

With the `-e` option, `checkoutlinesufo` is used to remove overlaps. When working with UFO files, the results of overlap removal are written to a new layer (`com.adobe.type.processedglyphs`), the original glyph is not touched.

### `makeotf`

This program will build an OpenType/CFF font from a font (source) file and auxiliary metadata files.
Typical input formats are
* UFO (font source format, cubic outlines)
* PFA (Type 1 font)
* TXT (decrypted plain-text version of a Type 1 font, obtained via `detype1`)
* TTF (TrueType source font file)

`makeotf` also optionally accepts further metadata files, e.g. a feature file that defines the OpenType layout rules, overrides for makeotf’s default values, a GlyphOrderAndAliasDB, etc. For details, see the makeotf user guide.

### `buildmasterotfs`

Step 1 of 2 to build a CFF Variable Font – this script builds static OTF files (with overlaps) from a .designspace file with compatible UFO masters. 

### `buildcff2vf`

Step 2 of 2 to build a CFF Variable Font – this script will use a .designspace file and the output OTFs of `buildmasterotfs` to assemble a CFF2 variable font file.

### `makeinstancesufo`

`makeinstancesufo` is a tool to build UFO instances from a .designspace file and a set of compatible UFO masters, to prepare for building static OTF fonts. By default, the instances are run through `checkoutlinesufo` (for overlap removal) and `otfautohint` (for PS hinting). Finally, the UFO instances are normalized.  
More info on designspace files and interpolation systems can be found at [Superpolator.com](https://superpolator.com/designspace.html) and [DesignSpaceDocument Specification](https://fonttools.readthedocs.io/en/latest/designspaceLib/index.html).

### `mergefonts`

This program will merge glyphs from one font into another, optionally copying a subset from the source fonts, and changing the names of the glyphs. It can also be used to subset and change the glyph names in a font. By using the same font more than once as a source with different mapping files, glyphs can be duplicated under other names. `mergefonts` can also convert a named-keyed font to a CID-keyed font.

### `otc2otf`

Extracts all OpenType fonts (.otf or .ttf) from an OpenType Collection font file (.ttc or .otc).

### `otf2otc`

Generates an OpenType Collection font file (.ttc) from two or more OpenType fonts (.otf or .ttf).

### `otf2ttf`

Converts OpenType-CFF fonts to TrueType.

### `rotatefont`

This tool will rotate and translate glyphs in a font, including the hints (rotated Latin glyphs are common in CJK fonts). Hints will be discarded if the rotation is not a multiple of 90 degrees.

### `sfntedit`

`sfntedit` allows cutting and pasting the entire binary block of a font table from one font to another. From a source font, tables can be extracted to a separate file with the `-x` option, and then pasted to a target font with the `-a` option. `sfntedit` can also be used to simply delete a table, and to fix the font table checksums.

### `otfstemhist`

`otfstemhist` is a tool to help preparing global hinting values. In default mode, `otfstemhist` outputs reports which list stems in order of occurrence. In zone mode (`-z`), `otfstemhist` reports on the top- and bottom extremes of glyphs.  
The resulting report files can be used to pick alignment zones common stem widths, which should be transferred to the UFO source files before running `otfautohint`.

### `ttfcomponentizer`

Componentizes glyphs of a TrueType font, using component information from an external UFO file. The process only supports components that are not scaled, rotated or flipped.

### `tx`

`tx` can be used to convert most font formats to CFF, Type 1, or UFO format. TrueType fonts will lose their hinting in the process. `tx` cannot convert from PostScript to TrueType, and cannot rasterize TrueType fonts. When used to convert from a Type 1 or CFF font, it undoes subroutinization, so that each glyph contains all the drawing operators, rather than referring to subroutines for common path elements. However, it does have an option to apply subroutinization to the output font.

### `type1`/`detype1`

* `detype1`: decrypt a Type 1 font file (PFA/PFB) to a human-readable plain-text file.
* `type1`: encrypt the above text file back to Type 1.

This workflow is useful for fixing or checking individual data fields (for example the alignment zones and stems, which are found in the encyrypted portion of a Type 1 font). It is also useful for copying a specific path element to many glyphs.

## 2.2 Proofing

### `spot`

This program can report data from an OpenType font in a variety of ways. All tables can be shown as a text report of the low-level structure of the table. Most tables can be shown with a report in a more human-readable format. Some tables, including the GPOS and GSUB layout tables can be shown graphically, by outputting a PostScript file that can be converted to a PDF.

### `tx`

`tx` has a `-pdf` mode which is useful for proofing. This mode will write a PDF file which shows the all the glyphs in the input file. By default, a font overview at 320 glyphs per page is written. With option `-1`, this overview is extended by a page for every glyph in the input font, detailing point coordinates, alignment zones, metrics, etc.
Since the PDF file is just dumped to stdout, the output needs to be redirected:  
`tx -pdf -1 font.ufo > output.pdf`

### `*plot`

All the `*plot` tools are similar in functionality, and all accept OpenType, Type 1, and UFO input files.  
The `*plot` tools are simply small command-file scripts that call a single Python script with different options. The `-h` option for all these scripts will produce the same list of **all** the options supported by the single Python script. You can customize the PDF proofs by providing additional arguments.

##### `charplot`

`charplot` creates a PDF file showing all the glyphs in the font, one glyph per page, with nodes and control points. The output is very similar to `tx -pdf -1`, with the addition of a filled glyph preview, but without the alignment zones. 

##### `digiplot`

`digiplot` creates a PDF proof very similar to `charplot`, showing all the glyphs in the font, two glyphs per page. Useful for CJK projects, characters are shown in relationship to an em-box.

##### `fontplot`

`fontplot` creates a PDF file showing all the glyphs in the font. Similar but inferior to `tx -pdf`. The resulting pages show just the filled outlines at a small point size, with some info around each glyph. 

##### `fontsetplot`

`fontsetplot` creates a spreadsheet-like PDF file, allowing the comparison of glyphs (rows) in a number of fonts (columns). Glyphs are shown in GID order, with a fixed spacing of one em, so that all glyphs with the same glyph name should align in a single column. The input fonts are sorted first by length of character set, then by alphabetic order of the character sets, then by PostScript name.

##### `hintplot`

`hintplot` creates a PDF proof showing all glyphs in the font with applied hints and applicable alignment zones. Four glyphs per page.

##### `waterfallplot`

Creates a waterfall of point sizes for all glyphs in the font, which is used to evaluate hinting. In order to allow for the hinting to be seen, the font is embedded in the PDF, and glyphs are referenced by char code. Please note that not all PDF viewers interpret glyph hinting, so use a known-good PDF viewer (Adobe Acrobat).

## 2.3 Validation

### `otfautohint`

In `-v` (verbose) mode, `otfautohint` will report “near misses” of a stem or alignment zone. While this output can be informative and reveal drafting errors (such as a glyph missing an alignment zone), these reports should not influence conscious design decisions.

### `checkoutlinesufo`

This tool reports on the general quality of glyph outline data, by flagging outline overlaps, collinear points, and flat curves.

### `comparefamily`

`comparefamily` examines all the fonts in a directory and runs many quality checks. It will check consistency of data across a family of fonts, as well as in a single font. It will point out any errors in naming within a style-linked group.  
Typical options for running `comparefamily` are `-rm` (menu name report), `-rn` (metrics report), `-rp` (PANOSE number report).

### `sfntdiff`

This tool does a low-level comparison of two OpenType font files. It is most useful for quickly checking if two fonts are identical, or in which tables they differ.

### `ttxn`

This tool is used to test if two fonts are functionally the same. It sorts and modifies the output from the `ttx` and `tx` tools to build a normalized text dump that eliminates differences due to issues such as OTL table record order, glyph order, and subroutinization differences. It writes one file for each table in the font, which can be `diff`ed in a further step. `ttxn` is particularly useful in comparing older and newer versions of the same font.

### `tx`

`tx` has a number of modes that are useful for testing. It will show only data about the font program within a font file, either CFF or TrueType. The `-dump` option is good for looking at a text report of the CFF font’s global and glyph data. Finally, converting any font to CFF will yield error reports if the font data is not syntactically correct, and if any glyphs are not hinted.
