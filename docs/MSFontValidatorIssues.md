# Issues with OpenType/CFF and TrueType fonts in MS Font Validator

Version 1.2 — October 2019

#### Copyright
Copyright 2016 Adobe Systems Incorporated. All rights reserved.

## Introduction
The following is a list of inappropriate error messages issued by the MS Font Validator version 1.0.1286.23890 for Adobe OpenType/CFF and TrueType fonts. These messages should be ignored when checking OpenType fonts that have CFF or TrueType font data. Note that these issues may be fixed in newer versions of the MS Font Validator.

### 1. Messages caused by the absence of a table that is TrueType specific
#### 1.a. OpenType CFF fonts do not need or use these tables:
	W0022 Recommended table is missing gasp
	W0022 Recommended table is missing kern
	W0022 Recommended table is missing hdmx
	W0022 Recommended table is missing VDMX
	E6059 Could not perform rasterization Unable to get data from rasterizer. 'glyf' table is not valid.

### 2. Messages caused by layout tables
#### 2.a. The following message is caused by the presence of the GPOS ‘size’ feature, which correctly uses a non-NULL FeatureParams pointer. This feature is registered.
	E5400 The FeatureParams field is not null FeatureList, FeatureRecord[30](size), FeatureTable

#### 2.b. The following message is causes by the presence of stylistic set features. These feature tags are registered.
	W5300 The FeatureRecord tag is valid, but unregistered FeatureList, FeatureRecord[<nnn>], tag = 'ss01'

### 3. Messages caused by OS/2 table version 4
#### 3.a. Presence of this table version causes the following messages:
	E2127 The table length does not match the expected length for this version
	E2132 The version number is invalid 4
	W0051 Cannot perform test due to other errors in this table OS/2 table appears to be corrupt. No further tests will be performed
	W2106 The version number is valid, but less than 3 2

### 4. Messages caused by differences in approach by Adobe and MS
#### 4.a. Adobe does not require that a font contain all the glyphs in a code page in order to declare support for that code page. As a result the following message may be seen:
	W2101 A CodePage bit is set in ulCodePageRange, but the font is missing some of the printable characters from that codepage bit #<n>...

#### 4.b. The first release of the Adobe OpenType fonts contained PUA (Private Use Area) Unicode values. Future versions of the same fonts will contain the same PUA values in order to avoid problems between versions. However, we agree that it is best to avoid using PUA values when possible, and new fonts will not have them. As a result, the following message may be seen:
	W0307 Characters are mapped in the Unicode Private Use Area

#### 4.c. The head table contains a value which indicates the lowest size at which the font can be read. This makes sense for TrueType fonts, where the itself controls many details of hinting, and the designer can usefully determine the threshold of legibility. However, in the CFF model, the hints are only suggestions, and the rasterizer may change over time. The Adobe Type Department chooses to set this value to 3 as being so low that it will not interfere with future improvements in rasterization and anti-aliasing. As a result, the following warning will be seen:
	W1305 The lowestRecPPEM value may be unreasonably small lowestRecPPEM = 3

#### 4.d. A font which is not an italic style of another face in the same family can nevertheless have large italic angle. Examples are script fonts and italic designs which are the only font in the family. In this case the Adobe Type Department prefers to enter the real italic angle in the post table italicAngle field. This way, the insertion bar is correctly slanted in most programs. However, this will cause the following messages:
	E1316 The macStyle italic bit is clear, but the post table italic angle is nonzero
	E2308 The italicAngle is nonzero, but the head.macStyle italic bit is not set

#### 4.e. The Adobe Type Department prefers to set the hhea table Ascender/Descender to the same value as the OS/2 table sTypoAscender/Descender, rather than the OS/2 table usWinAscent/Descent. We also prefer to set the LineGap to 20% of the em-square, although other choices are reasonable. As a result, the following messages are usually shown:
	W1404 The LineGap value is less than the recommended value LineGap = 200, recommended = <n>
	W1405 Ascender is different than OS/2.usWinAscent. Different line heights on Windows and Apple hhea.Ascender = <n1>, OS/2.usWinAscent = <n2>
	W1406 Descender is different than OS/2.usWinDescent. Different line heights on Windows and Apple hhea.Descender = <n1>, OS/2.usWinDescent = <n2>

### 5. Messages caused by MS Validator bugs
#### 5.a. When the last glyph(s) of a TrueType font are non-marking, the loca table offsets are calculated incorrectly; MS Validator will produce an error informing that there are more glyph entries in the loca table than there are glyphs in the glyf table. This error can be ignored if the number of glyphs mentioned in the details matches the total number of consecutive non-marking glyphs at the end of the font's glyph set.
	E1703 Loca entry points outside the glyf range | Number of glyphs with the error = X

---

#### Document Version History

Version 1.0 First version. Read Roberts 4/24/2007  
Version 1.1 Added section #5. Miguel Sousa 8/31/2015  
Version 1.2 Converted to Markdown + minor edits. Josh Hadley 10/2019  
