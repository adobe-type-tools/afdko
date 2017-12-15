/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

/*
 * Multiple master  table format definition.
 */

#ifndef FORMAT_MMSD_H
#define FORMAT_MMSD_H

#define MMSD_VERSION VERSION(1,0)

typedef struct
        {
        Card8 * longLabel;
        Card8 * shortLabel;
        } MMSDAxisRec;

typedef struct
        {
        Card16 nAxes;
        Card16 axisSize;
        MMSDAxisRec*  axis;                        /* [nAxes] */
        } MMSDAxisTbl;

typedef struct
        {
        Card8 * nameSuffix;
        } MMSDInstanceRec;

typedef struct
        {
        Card16 nInstances;
        Card16 instanceSize;
        MMSDInstanceRec *instance;        /* [nIntances] */
        } MMSDInstanceTbl;

typedef struct
        {
        Fixed point;
        Fixed delta;
        } MMSDActionRec;

typedef struct
        {
        Card8 axis;
        Card8 flags;
        MMSDActionRec action[2];
        } MMSDStyleRec;

typedef struct
        {
        Card16 nStyles;
        Card16 styleSize;
        MMSDStyleRec * style;              /* [nStyles] */
        } MMSDStyleTbl;

typedef struct _MMSDTbl
	{
	Fixed version;
	Card16 flags;
#define MMSD_USE_FOR_SUBST      (1<<0)  /* May be used for substitution */
#define MMSD_CANT_INSTANCE      (1<<1)  /* Can't make instance */
	Card16 axisOffset;
	Card16 instanceOffset;
	Card16 styleOffset;
	MMSDAxisTbl axis;
	MMSDInstanceTbl instance;
	MMSDStyleTbl style;
	} MMSDTbl;

#endif /* FORMAT_MMSD_H */
