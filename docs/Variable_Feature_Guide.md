# Guide to variable features

## Introduction

As of version 1.27, the [OpenType Feature File
Specification](./OpenTypeFeatureFileSpecification.md) includes new syntax for
direct encoding of variable values in a variable font. Prior to this revision
the creation of a variable font using AFDKO (and most other font development
toolkits) has started with the generation of a number of relatively complete
"static" fonts, each with its own feature file hierarchy. Those static fonts
are then be merged to form a variable font.

This construction method (which is still supported) has the advantage of
continuity with pre-variable-font methods of producing font families from a
smaller set of interpolable masters. However, it also has some drawbacks.  One
is that it limits the designer to locations in designspace that correspond to
one of the static input fonts. This can be somewhat mitigated by the use of
"sparse" masters, but these can be cumbersome to create and the rules for how
sparse masters are merged can get complicated. Each additional master will also
typically add a number of font-level values that may not be wanted.

Another drawback is the need to maintain multiple feature file hierarchies 
with carefully synchronized data, so that the GSUB, GPOS, and GDEF tables
in the static input fonts can be merged.

The additions to the feature file syntax discussed in this style guide provide
an alternative, "variable-first" development process. The first step of this
process also involves merging static "fonts" into a variable "font", but these
fonts are "minimal" in that they just contain glyph-level data and a few
general parameters like ascent, descent, and em-height.  (The need for such
intermediate fonts may be optimized out of future versions of AFDKO.) Features
and other parameters are then specified in a single, font-wide feature file
hierarchy. The content of that hierarchy is then added to the minimal font to
produce a complete OpenType variable font.

The actual process of creating a variable font in this new way is documented
elsewhere, and will anyway likely change somewhat in the first few years after
these syntax changes have been introduced. The detailed descriptions of each
addition are documented, and will continue to be documented, in the
specification linked above. This document provides a basic introduction to the
features, along with advice about how to choose between expressions when
alternatives are available.

**Note:** As of this draft the additional support for variable values (kerning,
anchors, etc) is tentatively complete and ready for general community review.
Other planned additions to support "Feature Variations" (such as substitutions
that are limited to certain axis ranges) have not yet been specified or
implemented. This document will be updated as the specification,
implementation, reviewing, and finalizing continues.)

## The basics of a single variable value

Generally speaking, a variable value consists of a set of designspace locations
paired with values.

In a merging workflow, the location is associated with a source master in a
designspace file and the value is some value in the master, such as the *x*
coordinate of a point on a glyph, a kerning value, or the TypoAscender value in
the OS/2 table.

The added syntax allows a variable value to be declared all at once, often in a
context where a single number would be written when building a static font. The
second part of a pair—the value—will typically just be a number (and therefore
does not need much explanation). Locations are more complicated, and the best
way of specifying a location may depend on a font designer's process.

### Designspace locations and unit suffixes

A designspace location is itself a set of pairs, each consisting of an axis
specifier—almost always a three- or four-letter "tag"—and a position on that
axis.  There are three scales for specifying an axis position, each with its
own "unit letter":

 * Design units (`d`): These are units chosen by the designer and typically
   specified in a designspace file. Each axis will have a minimum, maximum,
   and default value in these units.
 * User units (`u`): These are the units presented to a user when adjusting the
   axis. A designspace file maps between design units and user units. If there
   is no mapping for an axis, the user units are treated as the same as the
   design units.  Some standardized axes, such as `wght`, have standardized
   axis values, others do not. As with design units there will always be a
   minimum, maximum, and default for each axis.
 * Normalized units (`n`): Normalized units are in a range from a minimum of -1
   to a maximum of 1 inclusive, with a default of 0. They have a precision of
   F2dot14 (14 bits for the fractional part of the number). Most OpenType
   tables that store axis positions use normalized values (the exception being
   the `fvar` table, which stores user values).

Which scale is best to use depends on how a font is being designed, and more
particularly on what a designer takes the axis location they are specifying to
correspond to. That said, the designers we have talked to say that design units
are typically the most intuitive and stable scale for specifying axis
locations.

A full designspace location is a comma-separated list of axis tag, axis value
pairs separated by an equals sign (`=`). Each axis value must have a unit.  For
example, in a variable font with a `wght` axis and an `opsz` axis, this would
fully specify a location:

```fea
wght=1000d, opsz=0n
```

(Note that `0n` always represents the default location of an axis, just as
`-1n` always represents the minimum and `1n` always represents the maximum.)

There is one extra "trick" with axis position specifiers: When the unit letter
is followed by a plus sign (`+`) like this:

```fea
wght=600d+, opsz=0n+
```

the axis position specified will be the minimum F2dot14 increment above the
position specified with the number and unit. This makes it easier to specify
locations that are "right next to each other". Following the unit letter with a
minus sign/hyphen (`-`) similarly yields the minimum F2dot14 decrement position.

### Named locations

The `locationDef` keyword can be used to give a name to a location. For example,
this statement gives the name `@DisplayBlack` to a weight of 1000 and an optical
size of 60 (both in design units):

```fea
locationDef wght=1000d, opsz=60d @DisplayBlack;
```

Note that all named locations must start with an at-sign (`@`) in both the
definition and wherever it is used.

We strongly encourage the use of named locations in feature files, for these
reasons:

 * For a typical variable font only a small number—say less than 30—of
   locations are used in specifying variable values. Even fonts playing "tricks"
   will typically only use a few extra locations in a given case. So it will
   generally not be a burden to give each location a name. (Or, failing that,
   to give each "primary" location a name.)
 * If the designspace of a font changes during development, named locations
   mean that only a small number of lines in the feature file need to change.
 * Location literals are comma-separated lists of tag, value pairs, and
   variable values are whitespace-separated lists of location, value pairs.
   This means that even with careful formatting a variable value specified
   using location literals can be difficult to read and understand, while
   variable values specified with named locations will be shorter and easier to
   read.
 * Named locations are likely to decrease the time it takes to parse a feature
   file compared with repeated location literals (although this may only be a
   factor for very large files).

The particular choice of names is up to the designer. Abbreviated instance
names, as in the above example, are one possibility. Another is the use of "m",
"d", and "M" for minimum, default, and maximum for each axis, so for example:

```fea
locationDef wght=0d, opsz=60d @mM
```

as these yield short and distinctive names for the middle and "corners" of the 
designspace.

Most of the subsequent examples will use named locations.

### "Single" variable values

In most contexts of a feature file where a single number is specified in a
non-variable font, and where a variable value is now permitted, one can now
supply a "single" variable value. A variable value is specified as a set of
whitespace-separated location, value pairs enclosed in parentheses, where the
location and the value are separated by a colon. For example (for a font with
one `wght` axis):

```fea
   (50 wght=0d:47 wght=1000d:54)
```

or this example using named locations:

```fea
   (50 @mm:47 @mM:48 @Mm:54 @MM:51)
```

Note that when the location and colon are *omitted* from a pair, the value will
treated as that of the *default location* of the variable font.

This syntax is used in almost all places where a variable value can be
specified other than a "metric" and an anchor, and can optionally be used in
those contexts as well.

## Kerning metrics

A valid kerning metric can consist of either one or four values.  When
specifying one value a single variable value is appropriate, as in:

```fea
valueRecordDef (50 @mm:47 @mM:48 @Mm:54 @MM:51) KERN_A;
```

When specifying four values there are two choices of syntax. One choice is to
treat any or all of the four values as a single variable value:

```fea
valueRecordDef <0 0 (20 @ExtraLight:30 @ExtraBlack:10) 0> KERN_B;

```

With the other syntax, the ordering of the parentheses and brackets is
reversed, so that each distinct location is used once in the metric:

```fea
valueRecordDef (<-80 0 -160 0> wght=200u:<-80 0 -140 0> @ExtraBlack:<-80 0 -180 0>) KERN_C;
```

The first syntax is easier when:

 * Only some of the metrics are variable, or
 * Different metrics use different locations

And the second syntax is easier when all metrics use the same locations.

## Anchors

Anchor specifiers provide two analogous choices. Either some or all of the two
position values can be variable

```fea
<anchor (120 @ExtraLight:115 @ExtraBlack:125) (-20 @ExtraLight:-10 @ExtraBlack:-30)>
```

or there can be a single parenthetical with internal angle brackets containing two
values:

```fea
<anchor (<120 20> @ExtraLight:<115 -10> @ExtraBlack:<125 -30>)>
```

As with variable metrics, the first syntax is easier when:

 * Only one of the positions is variable, or
 * Different positions use different locations

And the second syntax is easier when both positions use the same locations.

# Style and readability

The size (measured in characters) of a variable value specifier in a feature
file can be quite long depending on the font. As a consequence, statements
that would fit comfortably on "a line" (e.g. 80 characters in a terminal
window) in a static font might take up multiple lines in a variable font.
And if such statements are formatted in the typical way the result could
be difficult to read.

The first tool for reducing this problem is the use of named locations.
Even if you choose longer names, the distinctiveness of a word, as opposed
to a list of axis tag, position pairs, will improve readability. And shorter
names will lead to shorter location strings.

Beyond that, the key to solving the readability problem in practice is that all
"whitespace"—space characters, tabs, newlines—in a feature file outside of
quotes (or, in the case of `include` statements, parentheses) is equivalent.
Therefore, as with many programming languages that use semicolon-terminated
statements, one can use newlines and spaces freely to format a statement
across multiple lines. Here is one example with nine relevant locations:

```fea
locationDef  wght=    0d,  opsz=  8d  @CEL;   # Caption ExtraLight
locationDef  wght=  394d,  opsz=  8d  @CR ;   # Caption Regular
locationDef  wght= 1000d,  opsz=  8d  @CBl;   # Caption Black
locationDef  wght=    0d,  opsz= 20d  @TEL;   # Text    ExtraLight
locationDef  wght= 1000d,  opsz= 20d  @TBl;   # Text    Black
locationDef  wght=    0d,  opsz= 60d  @DEL;   # Display ExtraLight
locationDef  wght=  394d,  opsz= 60d  @DR ;   # Display Regular
locationDef  wght= 1000d,  opsz= 60d  @DBl;   # Display Black

pos  l    periodcentered'  (    <-150  37 -290 0>   @TEL:<-127  55 -250 0>   @TBl:< -82  21 -178 0>
                            @DR:<-123  48 -238 0>   @DEL:<-114  55 -222 0>   @DBl:< -72  32 -148 0>
                            @CR:<-152  34 -294 0>   @CEL:<-128  51 -246 0>   @CBl:<-112   7 -224 0>)  l;
pos  L    periodcentered'  (    <-204  36 -261 0>   @TEL:<-190  55 -250 0>   @TBl:<-230  21 -290 0>
                            @DR:<-211  48 -219 0>   @DEL:<-219  55 -232 0>   @DBl:<-213  32 -242 0>
                            @CR:<-238  34 -274 0>   @CEL:<-216  51 -246 0>   @CBl:<-280   7 -322 0>)  L;
pos  L.sc periodcentered'  (    <-251 -31 -310 0>   @TEL:<-233 -11 -310 0>   @TBl:<-229 -43 -300 0>
                            @DR:<-217 -27 -258 0>   @DEL:<-224 -18 -272 0>   @DBl:<-204 -43 -262 0>
                            @CR:<-284 -21 -304 0>   @CEL:<-285  -6 -296 0>   @CBl:<-298 -46 -332 0>)  L.sc;
```

This formatting exemplifies several helpful conventions.

* The formatting across these multiple similar lines has been unified, so that
  corresponding elements line up in columns and reading one line help with
  reading the others.
* This font has two axes, a weight axis with ExtraLight, Regular, and Black
  positions, and an optical size axis with Caption, Text, and Display
  positions. The locations have short abbreviations corresponding to those full
  names. (There is no need to name the default location.)
* The metrics are arranged in a 3x3 matrics corresponding to the three locations
  per axis, but with default, and therefore most salient, value moved to first
  location. (A more "geometric" arrangement with the default in the middle would
  be another sensible choice.)
* Extra spaces have been added between semantically distinct columns, and between
  metrics, to make those divisions stand out visually.

The longest of these lines is 107 characters, which is considerably longer
than 80.  Using one or two columns of metric instead of three would allow all
lines to fit within 80 characters but would reduce the association between
position of the location among the rows and columns and position of the
location in the designspace. There will always be trade-offs between readability,
tool conventions, and effort put into the formatting of a feature file, with
each team arriving at its own "sweet spot" among them.

## Recommended practices for generated files

In typical development workflows at least some feature files are
auto-generated. This is most common for pair kerning values and mark features,
because those are often available in font source files. In many cases an
auto-generated feature file will never need to be inspected. However, not every
toolchain is perfect and sometimes it is necessary to verify that the contents
of a generated file match the sources and/or match the tables of the compiled
font. It is therefore best to try to make such files at least somewhat legible.

Our primary recommendation for readable files is to use named locations.
Ideally the designer should be able to control or influence the choice of
names. A variable value specified with named locations easily recognized by the
designer are far easier to read than the alternatives.

Beyond that we recommend following the guidance of the previous section as
much as is practical, with the understanding that the bar for readability
of auto-generated files is generally lower.
