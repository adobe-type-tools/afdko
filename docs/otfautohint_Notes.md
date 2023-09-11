# Notes on the OTF autohinter

`otfautohint` contains a Python port of the "Automatic Coloring" code
originally written in C by Bill Paxton. That code was most recently distributed
as "psautohint" in a PyPI package of the same name.

Changes Summary:
* [Name-Change] The name-change better reflects how hinted input is now published, 
  and lets the new version coexist with the old version when needed.
* [AFDKO Tools Updated] Other tools in AFDKO have been updated to call oftautohint 
  instead of psautohint, and the dependency on the latter's repository has been removed.
* [Stopping Psautohint Development] We expect to stop development of psautohint after the 4.0.0 release.
* [Improvements] The new code fixes a number of bugs and in our judgement produces 
  better results on average. It also uses a slightly different encoding in
  UFO glif files. Accordingly, users should expect that running the new code
  results in many differences compared with psautohint, but this should be a one-time change.
* [Variable CFF Hinting] The new code also supports hinting of variable CFF-based fonts,
  including directly hinting a built CFF2 font. Glyphs that include overlap will typically 
  be hinted close to how they would have been hinted with overlap removed.
* [Hinting Time] Because psautohint was partly written in (very old) C code,
   and otfautohint is written entirely in Python, the new code takes significantly
   longer to hint an individual glyph. However, we have also enhanced otfautohint
   to hint different glyphs on multiple CPU cores by default. As a result the tool
   will be 5-8 times slower when running on a single core but will typically be
   slightly faster when running on 8 cores.

Most of the algorithms are unchanged but not all. Some notable differences are:

1.  Because all code is now in Python there is no role for the `bez` charstring
    interchange format. CharStrings are now directly unpacked into `glyphData`
    objects. These are capable of representing any (decompiled) CharString with
    the caveats that the particular spline operator (e.g. `rCurveTo` vs
    `curveTo`) is not recorded.
2.  Intermediate hint state is now maintained in `hintState` objects, one per
    glyph, rather than in globals.
3.  The C code algorithms for vertical and horizontal hinting were mostly
    implemented as separate functions. In the port there is typically one
    function shared by both dimensions. The `glyphData/pt` 2-tuple object has
    special `a` and `o` value accessors and a class-level switch as part of
    this unification. (`a` is meant to suggest "aligned" and `o` is meant to
    suggest "opposite", referring to the relation between the value and the
    chosen alignment; `a` is `x` and `o` is `y` when horizontal alignment is
    chosen, and vice-versa when vertical alignment is chosen.) Some bug fixes
    and improvements that were only added to one dimension in the past now work
    in both.
4.  In the C code spline characteristics such as bounding boxes and measures of
    flatness were calculated by approximating spline curves with line segments.
    In the Python code these calculations are closed-form and implemented with
    fontTools' quadratic and cubic root finders.
5.  The C code had special functions for handling a spline with an inflection
    point. The new code copies and splits such splines at their inflection
    points, analyzes the copies, and copies the resulting analysis back to the
    inflected splines.
6.  The mask distribution algorithm (which is equivalent to what used to be
    called "hint substitution" is implemeneted somewhat differently.

There are also some features that are not (yet) ported:

1.  The C code for hint substitution "promoted" hints to earlier splines under
    some circumstances. This was not included because testing did not indicate
    it was of noticable benefit in current renderers.
2.  Previous versions of psautohint would clean up duplicate `moveTo` operators
    and warn about duplicate subpaths or unusually large glyphs. It is now more
    appropriate to check for these characteristics using sanitizers at earlier
    stages in the development process.
3.  Under some circumstances the C autohinter would divide curved splines at *t*
    == .5 when the ``allow-changes`` option was active. The primary reason for
    doing so was when a single spline "wanted" conflicting hints at its start
    and end points. This is a relatively rare circumstance and the Adobe
    maintainers are evaluating what, if anything, should be done in this case.
4.  The C autohinter would add a new hint mask before each subpath in the
    "percent" and "perthousand" glyphs but did not offer this option for other
    glyphs. This code was not ported.

Most functions are now documented in-line. Adapter code in `autohint.py` calls
`hint()` on `glyphHinter` in `hinter.py`, which in turn calls into the
dimension-specific `hhinter` and `vhinter` objects in the same file.

The Python code is slower when hinting individual glyphs, often 5 or more times
as slow or more compared with the C code on the same machine. It also uses more
memory.  However, by default glyphs are now hinted in parallel using the Python
`multiprocessing` module when multiple CPU cores are available. As part of this
change the glyphs are also unpacked just before hinting and updated right after
hinting in order to lower the total memory used by the process at a given time.
As a result the overall hinting process is often slightly faster on
contemporary machines.

# Some notes on hinting glyphs in a variable font

## Stem ordering

The initial CFF2 specification, and all revisions at the time of writing,
require that stems are defined in increasing order by their (lower) starting
locations and then, if two stems start at the same place, their ending
locations (which are higher except in the case of ghost hints).  Duplicate
stems are disallowed.  Stems that would be specified out of order in a
particular master (relative to the default master ordering) must therefore be
removed.

<!---
As we hope to eliminate these restrictions at a future point the variable font
autohinter supports two modes: a default mode in which stems are removed until
all are in order across all masters and an experimental mode in which stems
that change order are retained but treated as conflicting with all stems they
"cross" anywhere in design space. 
--->

As long as a design space is defined by interpolation only (rather than
extrapolation) the extremes of stem ordering are represented by the (sorted)
orders in the individual masters. Consider the bottom edge of stem *i* in a
variable glyph with *n* masters. It's location at some point in design space
can be represented as

c<sub>1</sub>\*s<sub>*i*1</sub> + c<sub>2</sub>\*s<sub>*i*2</sub> + ... + c<sub>*n*</sub>\*s<sub>*in*</sub>

where s<sub>*ik*</sub> is the position of the edge in that glyph in master *k*
and each `c` value is some interpolation coefficient, so that c<sub>1</sub> +
c<sub>2</sub> + ... + c<sub>*n*</sub> == 1 and 0 <= c<sub>*k*</sub> <= 1

The signed distance between the bottom edges of two stems *i* and *j* is accordingly

c<sub>1</sub>\*s<sub>*i*1</sub> + c<sub>2</sub>\*s<sub>*i*2</sub> + ... + c<sub>*n*</sub>\*s<sub>*in*</sub> - c<sub>1</sub>\*s<sub>*j*1</sub> + c<sub>2</sub>\*s<sub>*j*2</sub> + ... + c<sub>*n*</sub>\*s<sub>*jn*</sub>

or

c<sub>1</sub>\*(s<sub>*i*1</sub> - s<sub>*j*1</sub>) + c<sub>2</sub>\*(s<sub>*i*2</sub> - s<sub>*j*2</sub>) + ... + c<sub>*n*</sub>\*(s<sub>*in*</sub> - s<sub>*jn*</sub>)

The minimum/maximum distance in the space will therefore be *s*<sub>*ik*</sub>
- *s*<sub>*jk*</sub> for whatever master *k* minimizes/maximizes this value.
The question of whether *i* and *j* change order anywhere in design space is
therefore equivalent to the question of whether *i* and *j* change order in any
master, which is further equivalent to the question of whether they appear in a
different order in any master besides the default.

## Stem overlap

Suppose that stems *i* and *j* are in the same order across all masters with
stem *i* before stem *j* (so either s<sub>*ik*</sub> < s<sub>*jk*</sub> or
(s<sub>*ik*</sub> == s<sub>*jk*</sub> and e<sub>*ik*</sub> <
e<sub>*jk*</sub>)). Whether *i* overlaps with *j* in a given master *k* is
defined by whether s<sub>*jk*</sub> - e<sub>*ik*</sub> < O<sub>m</sub> (the
overlap margin). Therefore, by reasoning analogous to the above, the question
of whether two consistently ordered stems overlap in design space is equivalent
to whether they overlap in at least one master.

Any two stems that change order between two masters overlap at some point in design
space interpolated between those masters. The question of whether two stems overlap
in the general is therefore equivalent to:

1.  Do the stems change order in any master relative to the default master? If yes,
    then they overlap.
2.  If no, then is there any master *k* in which s<sub>*jk*</sub> -
    e<sub>*ik*</sub> < O<sub>m</sub>? If yes, then they overlap.
