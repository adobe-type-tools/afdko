---
title: Support of CFF2 in AFDKO
layout: default
---

Support of CFF2 in AFDKO
---

Copyright 2021 Adobe. All Rights Reserved. This software is licensed as
OpenSource, under the Apache License, Version 2.0. This license is available
at: http://opensource.org/licenses/Apache-2.0.

Document version 0.1
Last updated 18 January 2023

## 1. Workflow

Until recently the process of building a static OTF font was roughly the following:

  1. Pass a font outline source (Type 1, UFO, or rarely CFF) to the `makeotf`
     Python script, along with a feature file and sometimes other files.
  2. `makeotf` would use `tx` to translate the glyph outline source into Type 1
     format (if necessary)
  3. `makeotf` would pass the Type 1 source and the other files to
     `makeotfexe`.
  4. `makeotfexe` would translate the Type 1 source into CFF format internally.
  5. `makeotfexe` would then "read" the CFF file into an internal format for
     reference, and would then build the other OTF tables such as GSUB, GPOS,
     hmtx, and so on.

This process, although inefficient, worked for many years because the
information stored in a CFF table was more or less a subset of what can be
stored in a Type 1 font.  This is not true of CFF2, however, and accordingly
CFF2 Variable Fonts need to be built differently.

The temporary solution to that problem was to build multiple CFF-based static
OTFs with GSUB, GPOS, GDEF, and other tables built by `makeotfexe`, and then
"merge" these fonts into a single CFF2-based variable font.  This process works
but is inflexible. Each static font is associated with a location in the
designspace.  Some of the fonts can be "sparse" (when that works), but there is
no effective way to specify GSUB or GPOS locations other than those of the
source fonts. There are simple workarounds for GSUB in the form of special
grammar in the designspace file, but no good way of altering kerning (for
example) "between" source fonts.

A better solution requires a "variable font first" workflow. Glyph paths and
basic kerning data can still be developed in the way static fonts are (often
stored in UFO format).  These can then be merged into a single "bare" font in
CFF2 format. That font can then be passed to a program analogous to
`makeotfexe`, but with a feature file (that may include other files) specifying
the kerning and substitution for the entire variable font. 

This change in VF workflow calls for related changes to the workflow for
building static OTFs.  Instead of translating the glyph outline source into
Type 1, `makeotf` can simply translate it directly into CFF using `tx`. It can
then pass that CFF along with the other files to the `makeotfexe` replacement
(currently called `addfeatures`) which will then read it and the feature file
and build the additional OTF tables. This is more efficient and also an 
opportunity to eliminate redundant code in the AFDKO source base.

## 2. CFF2 "Portability"

There is, however, an additional challenge to this new workflow. A CFF table,
like its Type 1 predecessor, is effectively a self-contained font. The
additional OTF tables added by `makeotfexe` and now `addfeatures` provide
additional functionality, but "the basics" are still stored within the CFF
table itself. This is not true of CFF2, which has been simplified to remove
data that is redundant relative to other OTF tables.  Therefore, it is not
sufficient to pass *just* a CFF2 table to `addfeatures` because much of the
information needed to build a final font would be missing.

The eventual solution to this problem will be to add an option to the `cff2`
mode of `tx` to output not just a CFF2 table but a minimal OTF that includes
the additional information needed.  Because this work is best done in the
context of other structural changes to the `tx` source base, the interim
solution is to build this CFF2-based OTF with scripts using the FontTools
library. There is one script to convert a CFF table into a "bare" static CFF2
OTF, which can be used in those rare cases (mostly for testing) when a static
CFF2 OTF is needed.  Otherwise multiple CFF sources can be merged into a 
"bare" CFF2 OTF simply by merging them with the existing `buildcff2vf` script
without supplying a feature file.

What remains to document is what additional OTF tables are needed and what 
information is drawn from each.

Note that while a CFF table can contain an encoding and a CFF2 table cannot,
fonts built with `makeotf` have typically built a CFF with a "standard" encoding
and put the actual encoding data into the `cmap` table. Accordingly, any 
encoding data built into a CFF table is currently ignored by `addfeatures`, which
builds a `cmap` table just as `makeotfexe` would build it.

### `hmtx`

The `hmtx` table stores the advance width and left side bearing of each glyph.
A CFF CharString specifies the advance width and each left side bearing can be
calculated from the glyph outlines. A CFF2 CharString table omits the advance
width.

Therefore the "bare" CFF2 OTF must include a `hmtx` table and, if the font is
variable, a `HVAR` table.  When `addfeatures` is building a CFF2-based OTF
these will simply be copied unmodified from the source OTF.

### `hhea`

The `hhea` table contains the number of entries in the `hmtx` table, and additional
fields. The values of the additional fields will be used as fallbacks for values
not specified in feature files or calculated by `addfeatures`.

### `post`

The post table stores some basic parameters used for PostScript printing and can
optionally include glyph names. Because a CFF table also includes glyph names,
the `post` tables built by `makeotfexe` did not include them.  CFF2 tables cannot
include names.

Because glyph names are typically used to specify glyphs in feature files (unless
the font uses CID) the "bare" OTF must include a `post` table with those names in
those cases.  `addfeatures` will copy that data to the output OTF by default, but
if the names are not needed it has an option to output a `post` table without them.

The other fields in the input `post` table will be used as initial values which
can then be overridden in the feature file.

### `maxp`

This table just contains the number of glyphs. It is always regenerated by 
`addfeatures`.

### `name`

CFF tables can contain FullName, FamilyName, Copyright, and PostScript Name
strings.  With CFF2 these are only stored in the `name` table.

`addfeatures` will read the `name` table from the "bare" OTF and merge
additional values into it. When a Platform/Encoding/Language/Name combination
is specified in both the "bare" table and by `addfeatures` the latter will
override the former.

### `head`

The `unitsPerEm` field is read from the "bare" `head` table.  `xMin`,`xMax` and
`yMin`,`yMax` may be taken from the table or recalculated depending on
configuration.  Other fields are ignored.

### `MVAR`

The `MVAR` table stores tag-indexed variations in a variable font.  If there is an
`MVAR` table in the "bare" OTF it will be read and its values merged with those added
by `addfeatures`. When a tag is specified by the feature files but also in the "bare"
MVAR the former overrides the latter.

### `fvar` and `avar`

These variable font tables are referenced by the code in `addfeatures` but copied
unmodified into the output OTF.

## 3. Document revisions

**v0.1 [18 January 2023]: First version**
