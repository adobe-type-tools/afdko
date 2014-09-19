/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*****************************************************************************/
/* Header file for fontinfo lookup program                                   */
/*****************************************************************************/

                                    /*****************************************/
                                    /* These keywords are used for colored   */
                                    /* and uncolored, Mac and Sun versions   */
                                    /*****************************************/
#define ADOBECOPYRIGHT        0
#define ASCENT                1 + ADOBECOPYRIGHT
#define ASCENTDESCENTPATH     1 + ASCENT
#define BASEFONTPATH          1 + ASCENTDESCENTPATH
#define CHARACTERSET          1 + BASEFONTPATH
#define CHARACTERSETFILETYPE  1 + CHARACTERSET
#define CHARACTERSETLAYOUT    1 + CHARACTERSETFILETYPE
#define COMPOSITES            1 + CHARACTERSETLAYOUT
#define COPYRIGHTNOTICE       1 + COMPOSITES
#define CUBELIBRARY           1 + COPYRIGHTNOTICE
#define DESCENT               1 + CUBELIBRARY
#define DOMINANTH             1 + DESCENT
#define DOMINANTV             1 + DOMINANTH
#define ENCODING              1 + DOMINANTV
#define FAMILYBLUESPATH       1 + ENCODING
#define FAMILYNAME            1 + FAMILYBLUESPATH
#define FONTMATRIX            1 + FAMILYNAME
#define FONTNAME              1 + FONTMATRIX
#define FULLNAME              1 + FONTNAME
#define ISFIXEDPITCH          1 + FULLNAME
#define ITALICANGLE           1 + ISFIXEDPITCH
#define PCFILENAMEPREFIX      1 + ITALICANGLE
#define RNDSTEMUP             1 + PCFILENAMEPREFIX
#define SCALEPERCENT          1 + RNDSTEMUP
#define STEMSNAPH             1 + SCALEPERCENT
#define STEMSNAPV             1 + STEMSNAPH
#define SYNTHETICBASE         1 + STEMSNAPV
#define TRADEMARK             1 + SYNTHETICBASE
#define UNDERLINEPOSITION     1 + TRADEMARK
#define UNDERLINETHICKNESS    1 + UNDERLINEPOSITION
#define VERSION               1 + UNDERLINETHICKNESS
#define WEIGHT                1 + VERSION

                                    /*****************************************/
                                    /* These keywords are used just for      */
                                    /* coloring...                           */
                                    /*****************************************/

#define ASCHEIGHT             1 + WEIGHT
#define ASCOVERSHOOT          1 + ASCHEIGHT
#define AUXHSTEMS             1 + ASCOVERSHOOT
#define AUXVSTEMS             1 + AUXHSTEMS
#define BASELINE5             1 + AUXVSTEMS
#define BASELINE5OVERSHOOT    1 + BASELINE5
#define BASELINE6             1 + BASELINE5OVERSHOOT
#define BASELINE6OVERSHOOT    1 + BASELINE6
#define BASELINEYCOORD        1 + BASELINE6OVERSHOOT
#define BASELINEOVERSHOOT     1 + BASELINEYCOORD
#define CAPHEIGHT             1 + BASELINEOVERSHOOT
#define CAPOVERSHOOT          1 + CAPHEIGHT
#define DESCHEIGHT            1 + CAPOVERSHOOT
#define DESCOVERSHOOT         1 + DESCHEIGHT
#define FIGHEIGHT             1 + DESCOVERSHOOT
#define FIGOVERSHOOT          1 + FIGHEIGHT
#define FLEXEXISTS            1 + FIGOVERSHOOT
#define FLEXOK                1 + FLEXEXISTS
#define FORCEBOLD             1 + FLEXOK
#define HCOUNTERCHARS         1 + FORCEBOLD
#define HEIGHT5               1 + HCOUNTERCHARS
#define HEIGHT5OVERSHOOT      1 + HEIGHT5
#define HEIGHT6               1 + HEIGHT5OVERSHOOT
#define HEIGHT6OVERSHOOT      1 + HEIGHT6
#define LCHEIGHT              1 + HEIGHT6OVERSHOOT
#define LCOVERSHOOT           1 + LCHEIGHT
#define ORDINALBASELINE       1 + LCOVERSHOOT
#define ORDINALOVERSHOOT      1 + ORDINALBASELINE
#define ORIGEMSQUAREUNITS     1 + ORDINALOVERSHOOT
#define OVERSHOOTPTSIZE       1 + ORIGEMSQUAREUNITS
#define SUPERIORBASELINE      1 + OVERSHOOTPTSIZE
#define SUPERIOROVERSHOOT     1 + SUPERIORBASELINE
#define ZONETHRESHOLD         1 + SUPERIOROVERSHOOT
#define BLUEFUZZ			  1 + ZONETHRESHOLD
#define FLEXSTRICT            1 + BLUEFUZZ

                                    /*****************************************/
                                    /* These keywords are for compatibility  */
                                    /* with the blackbox (old code) world... */
                                    /*****************************************/
#define APPLEFONDID           1 + FLEXSTRICT
#define APPLENAME             1 + APPLEFONDID
#define BITSIZES75            1 + APPLENAME
#define BITSIZES96            1 + BITSIZES75
#define FONTVENDOR            1 + BITSIZES96
#define MSMENUNAME            1 + FONTVENDOR
#define MSWEIGHT              1 + MSMENUNAME
#define NCEXISTS              1 + MSWEIGHT
#define PAINTTYPE             1 + NCEXISTS
#define PGFONTIDNAME          1 + PAINTTYPE
#define PRIMOGENITALFONTNAME  1 + PGFONTIDNAME
#define PI                    1 + PRIMOGENITALFONTNAME
#define POLYCONDITIONPROC     1 + PI
#define POLYCONDITIONTYPE     1 + POLYCONDITIONPROC
#define POLYFLEX              1 + POLYCONDITIONTYPE
#define POLYFONT              1 + POLYFLEX
#define SERIF                 1 + POLYFONT
#define SOURCE                1 + SERIF
#define STROKEWIDTH           1 + SOURCE
#define STROKEWIDTHHEAVY      1 + STROKEWIDTH
#define VCOUNTERCHARS         1 + STROKEWIDTHHEAVY
#define VENDORSFONTID         1 + VCOUNTERCHARS
#define VPMENUNAME            1 + VENDORSFONTID
#define VPSTYLE               1 + VPMENUNAME
#define VPTYPEFACEID          1 + VPSTYLE

                                    /*****************************************/
                                    /* These keywords are just used for      */
                                    /* multiple master fonts...              */
                                    /*****************************************/
#define AXISLABELS1           1 + VPTYPEFACEID
#define AXISLABELS2           1 + AXISLABELS1
#define AXISLABELS3           1 + AXISLABELS2
#define AXISLABELS4           1 + AXISLABELS3
#define AXISMAP1              1 + AXISLABELS4
#define AXISMAP2              1 + AXISMAP1
#define AXISMAP3              1 + AXISMAP2
#define AXISMAP4              1 + AXISMAP3
#define AXISTYPE1             1 + AXISMAP4
#define AXISTYPE2             1 + AXISTYPE1
#define AXISTYPE3             1 + AXISTYPE2
#define AXISTYPE4             1 + AXISTYPE3
#define FORCEBOLDTHRESHOLD    1 + AXISTYPE4
#define HINTSDIR              1 + FORCEBOLDTHRESHOLD
#define MASTERDESIGNPOSITIONS 1 + HINTSDIR
#define MASTERDIRS            1 + MASTERDESIGNPOSITIONS
#define NUMAXES               1 + MASTERDIRS
#define PHANTOMVECTORS        1 + NUMAXES
#define PRIMARYINSTANCES      1 + PHANTOMVECTORS
#define REGULARINSTANCE       1 + PRIMARYINSTANCES
#define STYLEBOLD             1 + REGULARINSTANCE
#define STYLECONDENSED        1 + STYLEBOLD
#define STYLEEXTENDED         1 + STYLECONDENSED

/* additionals */
#define LANGUAGEGROUP         1 + STYLEEXTENDED



#define KWTABLESIZE           1 + LANGUAGEGROUP

#define MAXOVERSHOOTBANDS     6
#define MAXUNDERSHOOTBANDS    6
#define MAXBANDPAIRS          (MAXOVERSHOOTBANDS + MAXUNDERSHOOTBANDS)
