# CID-keyed UFOs in afdko

## **Introduction**

With the correct data included in a UFO you can create a hybrid 
CID-keyed/name-keyed source file that can be used in most of the 
afdko tools that understand CID-keyed fonts like `makeotf`, `tx`, and `mergefonts`. 

Some examples:

Convert an existing CID-keyed OTF to CID-keyed UFO

: `tx -ufo -o cid-keyed.ufo cid-keyed.otf`

Subset a CID-keyed UFO using CID ranges with the `-g` flag

: `tx -g /0,/959-/978,/59120-/59130 -ufo -o cid-keyed-subset.ufo cid-keyed.ufo`

Create a name-keyed source using `-decid`
: `tx -t1 -o name-keyed.pfa -decid -fd 1 cid-keyed.ufo`

Merge multiple CID-keyed UFOs into a single UFO
: `mergefonts merged-cid-keyed.ufo part1.map part1.ufo part2.map part2.ufo part3.map part3.ufo`


## **Requirements**

The following keys are required in `lib.plist` to make a UFO CID-keyed

* `com.adobe.type.ROS`
* `com.adobe.type.postscriptCIDMap`
* `com.adobe.type.postscriptFDArray`


### com.adobe.type.ROS

The value for this key must be written in the form {Registry}-{Ordering}-{Supplement}. 
Information about available ROS can be found at
https://github.com/adobe-type-tools/cmap-resources

Some typical ROSes are

* Adobe-Identity-0
* Adobe-CNS1-7
* Adobe-GB1-5
* Adobe-Japan1-7
* Adobe-KR-9

All except Adobe-Identity-0 have a fixed CID order that must be followed, but
the glyphs in the UFO can be named anything that is legal in the UFO format. 
The UFO glyphOrder may also be any order you choose. If you run `tx` alone you 
will see the order as saved in the UFO, but when converting with `tx -t1` or `tx -cff`
or building an OTF with `makeotf` the glyphs will be rearranged into CID order.

### com.adobe.type.postscriptCIDMap

The glyph name to CID mapping must be a dict with the UFO glyph names as keys and 
their corresponding CID numbers as values. When converting an existing CID-keyed 
source (OTF, CFF, Type1) to UFO with a command like `tx -ufo -o source.ufo source.otf` 
the glyph names will default to `cidXXXXX` because no names can be 
stored in those source formats. If you change the names in the UFO be sure to update 
the CID mapping and groups.plist to use the new names.


### com.adobe.type.postscriptFDArray

The FDArray is a list of font dictionaries containing hinting data. All keys are
optional and will use default values if not specified, but there must be at 
least one dict in the array and a corresponding group in groups.plist.

```
    <key>com.adobe.type.postscriptFDArray</key>
    <array>
    <dict>
    </dict>
    </array>
```

An example of one FontDict in Python dumped from Source Han Serif JP ExtraLight
```
 {'FontMatrix': [0.001, 0, 0, 0.001, 0, 0],
  'FontName': 'SourceHanSerifJP-ExtraLight-HWidth',
  'PaintType': 0,
  'PrivateDict': {'ExpansionFactor': 0.06,
                  'postscriptBlueFuzz': 1,
                  'postscriptBlueScale': 0.039625,
                  'postscriptBlueShift': 7,
                  'postscriptBlueValues': [-18, 0, 521, 538, 746, 764, 794, 794],
                  'postscriptOtherBlues': [-283, -283, -273, -273, -176, -176],
                  'postscriptStdHW': [34],
                  'postscriptStdVW': [50],
                  'postscriptStemSnapH': [34, 45, 59],
                  'postscriptStemSnapV': [28, 50, 60]}}]
```

### FDArraySelect groups in groups.plist 

In addition to the lib.plist keys there must be a groups.plist file 
containing one group for each FontDict in `com.adobe.type.postscriptFDArray`. 
Each group must be named in the following format

`FDArraySelect.{required-fdarray-index}.{optional-descriptive-name}`

The following keys are equivalent to the parser

* `FDArraySelect.0.SourceHanSerif-ExtraLight-Alphabetic`
* `FDArraySelect.0`

The optional descriptive name is only for user convenience such as when viewing and
creating groups in a font editor. When dumping with `tx` or  building an OTF 
with `makeotf` the names for each font dict will come from the `FontName` key 
in each dict in `com.adobe.type.postscriptFDArray`.


## **Using Python with CID-keyed UFOs**

Any Python library that supports UFO can handle CID-keyed UFOs since the
CID-specific data is added to the existing lib.plist and groups.plist files.
For example, using `fontParts` to make an existing UFO CID-keyed:

```
from fontParts.world import OpenFont

font = OpenFont('cid-keyed-source.ufo')

# Set the ROS
font.lib['com.adobe.type.ROS'] = 'Adobe-Japan1-7'

# Set up a preliminary CID mapping based on the current glyph order
font.lib['com.adobe.type.postscriptCIDMap'] = {name: i for i, name in enumerate(font.lib['public.glyphOrder'])}

# Set up a basic FDArray until you are ready to add hinting data
fontname = font.info.familyName.replace(" ", "")
font.lib['com.adobe.type.postscriptFDArray'] = [{'FontName': f'{fontname}-FD0'}]

# Create a single FontDict that corresponds to the one in the FDArray above
font.groups[f'FDArraySelect.0.{fontname}-FD0'] = font.glyphOrder

font.save()
```

---

#### Document Version History

Version 1.0 — April 10 2023
