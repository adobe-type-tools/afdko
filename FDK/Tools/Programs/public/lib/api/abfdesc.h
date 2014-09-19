/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/*
 * Abstract Font Descriptor.
 */

#ifndef ABFDESC_H
#define ABFDESC_H

/* An abstract font descriptor is composed of a single fixed-length
   "abfFontDescHeader" followed by one or more variable-length
   "abfFontDescElement"s (see below). */

typedef struct                      /* Font descriptor element */
    {
    /* The following "flags" field is used to represent font parameters having
       values of fixed size. 

       Bits in the field indicate the presence (set) or absence (clear) for
       corresponding font parameter value(s) in the "values" array. Clear bits
       signify that the font parameter was either missing from the font or its
       value was equal to the default value for that font parameter (specified
       below).

       For correct correspondence with their values in the "values" array the
       fields must be considered in least significant to most significant bit
       order. All bits correspond to 1 value except the FontMatrix where it
       is an array of 6 values.

       Comments show default values: */
    unsigned short flags;
#define ABF_DESC_CharstringType     (1<<0)  /* 2 */
#define ABF_DESC_PaintType          (1<<1)  /* 0 */
#define ABF_DESC_BlueScale          (1<<2)  /* 0.039625 */
#define ABF_DESC_BlueShift          (1<<3)  /* 7 */
#define ABF_DESC_BlueFuzz           (1<<4)  /* 1 */
#define ABF_DESC_StdHW              (1<<5)  /* none */
#define ABF_DESC_StdVW              (1<<6)  /* none */
#define ABF_DESC_ForceBold          (1<<7)  /* 0=false */
#define ABF_DESC_LanguageGroup      (1<<8)  /* 0 */
#define ABF_DESC_ExpansionFactor    (1<<9)  /* 0.06 */
#define ABF_DESC_initialRandomSeed  (1<<10) /* 0 */
#define ABF_DESC_defaultWidthX      (1<<11) /* 0 */
#define ABF_DESC_nominalWidthX      (1<<12) /* 0 */
#define ABF_DESC_lenSubrArray       (1<<13) /* 0 (local subr count) */
#define ABF_DESC_FontMatrix         (1<<14) /* [.001 0 0 .001 0 0] */

    /* The following fields are used to represent font parameters having
       values of variable size.

       The values stored in the fields represent the number of elements in the
       "values" array that are used to represent each font parameter. If the
       value is zero the corresponding font value is not stored in the "values"
       array indicating that the font parameter was missing from the font.

       For correct correspondence with their values in the "values" array the
       fields must be considered in order starting with "BlueValues".

       Comments show [max element count]: */
    unsigned char BlueValues;       /* [14] */
    unsigned char OtherBlues;       /* [10] */
    unsigned char FamilyBlues;      /* [14] */
    unsigned char FamilyOtherBlues; /* [10] */
    unsigned char StemSnapH;        /* [12] */
    unsigned char StemSnapV;        /* [12] */

    /* The values array contains the values corresponding to the font
       parameters with fixed and variable size values described above. The
       actual number of elements in the array is provided in the "valueCnt"
       field. */
    unsigned long valueCnt;
    float values[1];                /* [valueCnt] */
    } abfFontDescElement;

typedef struct                  /* Font descriptor header */
    {
    unsigned short length;      /* Total size in bytes of descriptor */
    unsigned short FDElementCnt;/* Count of "abfFontDescElement"s */
    float FontBBox[4];
    float StrokeWidth;
    float lenGSubrArray;        /* Global subr count */
    } abfFontDescHeader;

#define ABF_GET_FIRST_DESC(hdr) \
    (abfFontDescElement *)((char *)(hdr) + sizeof(abfFontDescHeader))

/* The ABF_GET_FIRST_DESC() macro returns a pointer to the first
   abfFontDescElement in the font descriptor when supplied with a
   abfFontDescHeader pointer. */

#define ABF_GET_NEXT_DESC(elem) \
    (abfFontDescElement *)((char *)(elem) + sizeof(abfFontDescElement) + \
     (((abfFontDescElement *)(elem))->valueCnt - 1)*sizeof(float))

/* The ABF_GET_NEXT_DESC() macro returns a pointer to the next
   abfFontDescElement in the font descriptor when supplied with an
   abfFontDescElement pointer. The client must ensure that ABF_GET_NEXT_DESC()
   is not called more times than there are abfFontDescElement's in the font
   descriptor. The number of elements is available via the "FDElementCnt" field
   in the abfFontDescHeader. */

#endif /* ABFDESC_H */
