/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Multiple master  table format definition.
 */

#ifndef FORMAT_MMSD_H
#define FORMAT_MMSD_H

#define MMSD_VERSION VERSION(1, 0)

typedef struct
{
    uint8_t* longLabel;
    uint8_t* shortLabel;
} MMSDAxisRec;

typedef struct
{
    uint16_t nAxes;
    uint16_t axisSize;
    MMSDAxisRec* axis; /* [nAxes] */
} MMSDAxisTbl;

typedef struct
{
    uint8_t* nameSuffix;
} MMSDInstanceRec;

typedef struct
{
    uint16_t nInstances;
    uint16_t instanceSize;
    MMSDInstanceRec* instance; /* [nIntances] */
} MMSDInstanceTbl;

typedef struct
{
    Fixed point;
    Fixed delta;
} MMSDActionRec;

typedef struct
{
    uint8_t axis;
    uint8_t flags;
    MMSDActionRec action[2];
} MMSDStyleRec;

typedef struct
{
    uint16_t nStyles;
    uint16_t styleSize;
    MMSDStyleRec* style; /* [nStyles] */
} MMSDStyleTbl;

typedef struct _MMSDTbl {
    Fixed version;
    uint16_t flags;
#define MMSD_USE_FOR_SUBST (1 << 0) /* May be used for substitution */
#define MMSD_CANT_INSTANCE (1 << 1) /* Can't make instance */
    uint16_t axisOffset;
    uint16_t instanceOffset;
    uint16_t styleOffset;
    MMSDAxisTbl axis;
    MMSDInstanceTbl instance;
    MMSDStyleTbl style;
} MMSDTbl;

#endif /* FORMAT_MMSD_H */
