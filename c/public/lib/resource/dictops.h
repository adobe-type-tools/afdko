/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef DICTOPS_H
#define DICTOPS_H

/*
 * CFF dictionary operator definitions.
 * 
 * There are several kinds of fonts that are supported by CFF that have
 * different dict organizations:
 *
 *                  Top     Private     FDs     PDs
 *                  ---     -------     ---     ---
 * name-keyed       x       x
 * synthetic        x
 * CID-keyed        x                   x       x
 * chameleon        x       
 *
 * The Top dict, so named because of its position in the dictionary hierarchy,
 * is also known as the Font dict in all but CID-keyed fonts. CID-keyed fonts
 * contain an array called FDArray in their top dict called FDArray whose
 * elements are font dicts (FDs) which each contain a Private sub-dictionary
 * (PD). Synthetic fonts just have a Top dict containing a reference to another
 * font.
 *
 * This file defines the dict operators that can appear in these dicts. In
 * order to aid font recognition and parsing, the following restrictions are 
 * imposed upon dict op ordering:
 *
 * Synthetic/top:   Must begin with cff_SyntheticBase.
 * CID/top:         Must begin with cff_ROS.
 * Chameleon/top:   Must begin with cff_Chameleon.
 * Private/PD:      cff_OtherBlues must follow cff_BlueValues and 
 *                  cff_FamilyOtherBlues must follow cff_FamilyBlues.
 *
 * If the top dict doesn't begin with one of the operators listed above it is
 * assumed to define a name-keyed font. Chameleon fonts don't define a Private
 * dict but instead use the Private operator to specify the size and offset of
 * the Chameleon font descriptor for the font.
 *
 * Comments indicate in which dicts an op may appear along with a default
 * value, if any, within parentheses.
 */

/* One byte operators (0-31) */
#define cff_version                 0   /* Top/FD */
#define cff_Notice                  1   /* Top/FD */
#define cff_FullName                2   /* Top/FD */
#define cff_FamilyName              3   /* Top/FD */
#define cff_Weight                  4   /* Top/FD */
#define cff_FontBBox                5   /* Top/FD */
#define cff_BlueValues              6   /* Private/PD (empty array) */
#define cff_OtherBlues              7   /* Private/PD */
#define cff_FamilyBlues             8   /* Private/PD */
#define cff_FamilyOtherBlues        9   /* Private/PD */
#define cff_StdHW                   10  /* Private/PD */
#define cff_StdVW                   11  /* Private/PD */
#define cff_escape                  12  /* All. Shared with T2 op */
#define cff_UniqueID                13  /* Top/FD */
#define cff_XUID                    14  /* Top/FD */
#define cff_charset                 15  /* Top/FD (0) */
#define cff_Encoding                16  /* Top/FD (0) */
#define cff_CharStrings             17  /* Top/FD */
#define cff_Private                 18  /* Top/FD */
#define cff_Subrs                   19  /* Private/PD */
#define cff_defaultWidthX           20  /* Private/PD (0) */
#define cff_nominalWidthX           21  /* Private/PD (0) */
#define cff_vsindex                 22
#define cff_blend                   23
#define cff_VarStore                24
#define cff_maxstack                25
#define cff_reserved26              26
#define cff_reserved27              27
#define cff_shortint                28  /* All. Shared with T2 ops */
#define cff_longint                 29  /* All */
#define cff_BCD                     30  /* All */  
#define cff_BlendLE                 31
#define cff_reserved255             255
#define cff_LAST_ONE_BYTE_OP        cff_BlendLE

/* Make escape operator value; may be redefined to suit implementation */
#ifndef cff_ESC
#define cff_ESC(op)                 (cff_escape<<8|(op))
#endif

/* Two byte operators */
#define cff_Copyright               cff_ESC(0)  /* Top/FD */
#define cff_isFixedPitch            cff_ESC(1)  /* Top/FD (false) */
#define cff_ItalicAngle             cff_ESC(2)  /* Top/FD (0) */
#define cff_UnderlinePosition       cff_ESC(3)  /* Top/FD (-100) */
#define cff_UnderlineThickness      cff_ESC(4)  /* Top/FD (50) */
#define cff_PaintType               cff_ESC(5)  /* Top/FD (0) */
#define cff_CharstringType          cff_ESC(6)  /* Top/FD (2) */
#define cff_FontMatrix              cff_ESC(7)  /* Top/FD (.001 0 0 .001 0 0)*/
#define cff_StrokeWidth             cff_ESC(8)  /* Top/FD (0) */
#define cff_BlueScale               cff_ESC(9)  /* Private/PD (0.039625) */
#define cff_BlueShift               cff_ESC(10) /* Private/PD (7) */
#define cff_BlueFuzz                cff_ESC(11) /* Private/PD (1) */
#define cff_StemSnapH               cff_ESC(12) /* Private/PD */
#define cff_StemSnapV               cff_ESC(13) /* Private/PD */
#define cff_ForceBold               cff_ESC(14) /* Private/PD (false) */
#define cff_reservedESC15           cff_ESC(15)
#define cff_reservedESC16           cff_ESC(16) /* Was lenIV (don't reuse) */
#define cff_LanguageGroup           cff_ESC(17) /* Private/PD */
#define cff_ExpansionFactor         cff_ESC(18) /* Private/PD (0.06) */
#define cff_initialRandomSeed       cff_ESC(19) /* Private/PD (0) */
#define cff_SyntheticBase           cff_ESC(20) /* Top/FD */
#define cff_PostScript              cff_ESC(21) /* Private/PD */
#define cff_BaseFontName            cff_ESC(22) /* Top/FD */
#define cff_BaseFontBlend           cff_ESC(23) /* Top/FD (instance UDV) */
#define cff_numMasters           cff_ESC(24)
#define cff_reservedESC25           cff_ESC(25)
#define cff_reservedESC26           cff_ESC(26)
#define cff_reservedESC27           cff_ESC(27)
#define cff_reservedESC28           cff_ESC(28)
#define cff_reservedESC29           cff_ESC(29)
#define cff_ROS                     cff_ESC(30) /* Top */
#define cff_CIDFontVersion          cff_ESC(31) /* Top (0) */
#define cff_CIDFontRevision         cff_ESC(32) /* Top (0) */
#define cff_CIDFontType             cff_ESC(33) /* Top (0) */
#define cff_CIDCount                cff_ESC(34) /* Top (8720) */
#define cff_UIDBase                 cff_ESC(35) /* Top */
#define cff_FDArray                 cff_ESC(36) /* Top */
#define cff_FDSelect                cff_ESC(37) /* Top */
#define cff_FontName                cff_ESC(38) /* FD (CID-only) */
#define cff_Chameleon               cff_ESC(39) /* Top */
#define cff_reservedESC40           cff_ESC(40) /* Was VToHOrigin (don't reuse) */
#define cff_reservedESC41           cff_ESC(41) /* Was defaultWidthY (don't reuse) */
#define cff_LAST_TWO_BYTE_OP        cff_reservedESC41
/*                                          42-255 Reserved */

/* Default operator values */
#define cff_DFLT_isFixedPitch       0           /* False */
#define cff_DFLT_ItalicAngle        0
#define cff_DFLT_UnderlinePosition  -100
#define cff_DFLT_UnderlineThickness 50
#define cff_DFLT_PaintType          0
#define cff_DFLT_CharstringType     2
#define cff_DFLT_StrokeWidth        0

#define cff_DFLT_CIDFontVersion     0.0
#define cff_DFLT_CIDFontRevision    0
#define cff_DFLT_CIDCount           8720

#define cff_DFLT_BlueScale          0.039625
#define cff_DFLT_BlueShift          7
#define cff_DFLT_BlueFuzz           1
#define cff_DFLT_ForceBold          0           /* False */
#define cff_DFLT_LanguageGroup      0
#define cff_DFLT_ExpansionFactor    0.06
#define cff_DFLT_initialRandomSeed  0
#define cff_DFLT_defaultWidthX      0
#define cff_DFLT_nominalWidthX      0

/* Predefined charsets (cff_charset operands) */
enum
    {
    cff_ISOAdobeCharset,
    cff_ExpertCharset,
    cff_ExpertSubsetCharset,
    cff_PredefCharsetCount,
    cff_CharSetfromPost,
    };

/* Predefined encodings (cff_Encoding operands) */
enum
    {
    cff_StandardEncoding,
    cff_ExpertEncoding,
    cff_PredefEncodingCount
    };

#endif /* DICTOPS_H */
