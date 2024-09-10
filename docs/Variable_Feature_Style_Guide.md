# Style guide for variable font-specific features

## Introduction

As of version 1.27, the [OpenType Feature File
Specification](./OpenTypeFeatureFileSpecification.md) includes new syntax for
direct encoding of variable values in a variable font. Prior to this revision
the creation of a variable font using AFDKO (and most other font development
toolkits) has started with the generation of a number of relatively complete
"static" fonts, each with its own feature file hierarchy. Those static fonts
would then be merged to form a variable font.

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
specification linked above. This document provides a basic introduction to them
along with advice about how to choose between expressions when alternatives are
available.

**Note:** As of this draft the additional support for variable values (kerning,
anchors, etc) is tentatively complete and being prepared for general community
review. Other planned additions to support "Feature Variations" (such as
substitutions that are limited to certain axis ranges) have not yet been
specified or implemented. This document will be updated as the specification,
implementation, reviewing, and finalizing continues.)

## The basics of a single variable value

Generally speaking, a variable value consists of a set of designspace locations
paired with values. In a merging workflow the location is assigned to a source
master in a designspace file and the value is some value in the master, such as
the *x* coordinate of a point on a glyph, a kerning value, or the TypoAscender
value in the OS/2 table.

The added syntax allows a variable value to be declared all at once, often in a
context where a single number would be written when building a static font. The
second part of a pair—the value—will typically just be a number (and therefore
does not need much explanation). Locations are more complicated, and the best
way of specifying a location may depend on a font designer's process.

### Designspace locations and unit suffixes

A designspace location is itself a set of pairs, each consisting of an axis
specifier—almost always a three- or four-letter "tag"—and a position on that
axis.  There are scales for specifying an axis position, each with its own
"unit letter":

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
are typically both the most intuitive and the most stable measure for
specifying axis locations.

A full designspace location is a comma-separated list of axis tag, axis value
pairs separated by an equals sign (`=`). Each axis value must have a unit.  For
example, in a variable font with a `wght` axis and an `opsz` axis, this would
fully specify a location:

```
wght=1000d, opsz=0n
```

(Note that `0n` always represents the default location of an axis, just as
`-1n` always represents the minimum and `1n` always represents the maximum.)

There is one extra "trick" with axis position specifiers: When the unit letter
is followed by a plus sign (`+`) like this:

```
wght=600d+, opsz=0n+
```

the axis position specified will be the minimum F2dot14 increment above the
position specified with the number and unit. This makes it easier to specify
locations that are "right next to each other". Following the unit letter with a
minus sign/hyphen (`-`) similarly yields the minimum F2dot14 decremented
position.

### Named locations

The `locationDef` keyword can be used to give a name to a location. For example,
this statement gives the name `@DisplayBlack` to a weight of 1000 and an optical
size of 60 (both in design units):

```
locationDef wght=1000d, opsz=60d @DisplayBlack;
```

Note that all named locations must start with an at-sign (`@`) in both the
definition and wherever it is used.

We strongly encourage the use of named locations in feature files, for these
reasons:

 * For typical variable fonts only a small number (typically less than 30) of
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

```
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

```
   (50 wght=0:47 wght=1000:54)
```

or this example using named locations:

```
   (50 @mm:47 @mM:48 @Mm:54 @MM:51)
```

Note that when the location and colon are *omitted* from a pair, the value will
treated as that of the *default location* of the variable font.

This syntax is used in almost all places where a variable value can be
specified other than a "metric" and an anchor, and can optionally be used in
those contexts as well.

## Kerning metrics

A valid kerning metric can consist of either one or four values.  When
specifying one value a single variable value is appropriate. When specifying
four values there are two choices of syntax. One choice is to treat any or all
of the four values as a single variable value:

```
valueRecordDef <0 0 (20 @ExtraLight:30 @ExtraBlack:10) 0> KERN_A;

```

With the other syntax, the ordering of the parentheses and brackets is
reversed, so that each distinct location is used once in the metric:

```
valueRecordDef (<-80 0 -160 0> wght=200u:<-80 0 -140 0> @ExtraBlack:<-80 0 -180 0>) KERN_B;
```

The first syntax is easier when:

 * Not all of the metrics are variable, or
 * Different metrics use different locations

And the second syntax is easier when all metrics use the same locations.

## Anchors

Anchor specifiers provide two analogous choices. Either some or all of the two
position values can be variable

```
<anchor (120 @ExtraLight:115 @ExtraBlack:125) (-20 @ExtraLight:-10 @ExtraBlack:-30)>
```

or there can be a single parenthetical with internal angle brackets containing two
values:

```
<anchor (<120 20> @ExtraLight:<115 -10> @ExtraBlack:<125 -30>)>
```

As with variable metrics, the first syntax is easier when:

 * Only one of the positions is variable, or
 * Different positions use different locations

And the second syntax is easier when both positions use the same locations.

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

Beyond that, when a value has more locations and values than fit on a typical
line (say 80 or 100 characters), putting some thought into formatting can be
very helpful. This can be as simple as adding a newline and some spaces for
basic alignment, as in:

```
pos at underscore (@CEL:-40 @CR:-40 @CBl:-70 @TEL:-80 -80 @TBl:-80 @DEL:-80
                   @DR:-80 @DBl:-80);
```
