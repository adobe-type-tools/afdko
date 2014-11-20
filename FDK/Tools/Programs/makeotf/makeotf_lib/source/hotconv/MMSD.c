/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 *	Multiple master supplementary data table.
 */

#include "MMSD.h"

/* ---------------------------- Table Definition --------------------------- */

typedef struct {
	DCL_OFFSET(char *, longLabel);
	DCL_OFFSET(char *, shortLabel);
} AxisRec;
#define AXIS_SIZE   (uint16 * 2)

typedef struct {
	unsigned short nAxes;
	unsigned short axisSize;
	DCL_OFFSET_ARRAY(AxisRec, axis);            /* [nAxes] */
} AxisTbl;
#define AXIS_TBL_SIZE(h)    (uint16 * 2 + AXIS_SIZE * (h->tbl.axis_.nAxes))

typedef struct {
	DCL_OFFSET(char *, nameSuffix);
} InstanceRec;
#define INSTANCE_SIZE   (uint16 * 1)

typedef struct {
	unsigned short nInstances;
	unsigned short instanceSize;
	DCL_OFFSET_ARRAY(InstanceRec, instance);    /* [nIntances] */
} InstanceTbl;
#define INSTANCE_TBL_SIZE(h) \
	(uint16 * 2 + INSTANCE_SIZE * (h->tbl.instance_.nInstances))

typedef struct {
	Fixed point;
	Fixed delta;
} ActionRec;

typedef struct {
	unsigned char axis;
	unsigned char flags;
	ActionRec action[2];
} StyleRec;
#define STYLE_SIZE  (uint8 * 2 + 2 * (int32 * 2))

typedef struct {
	unsigned short nStyles;
	unsigned short styleSize;
	DCL_OFFSET_ARRAY(StyleRec, style);      /* [nStyles] */
} StyleTbl;
#define STYLE_TBL_SIZE(h)   (uint16 * 2 + STYLE_SIZE * (h->tbl.style_.nStyles))

typedef struct {
	Fixed version;
	unsigned short flags;
#define MMSD_USE_FOR_SUBST  (1 << 0)  /* May be used for substitution */
#define MMSD_CANT_INSTANCE  (1 << 1)  /* Can't make instance */
	DCL_OFFSET(AxisTbl, axis);
	DCL_OFFSET(InstanceTbl, instance);
	DCL_OFFSET(StyleTbl, style);
} MMSDTbl;
#define HEADER_SIZE (int32 * 1 + uint16 * 4)

struct MMSDCtx_ {
	MMSDTbl tbl;    /* Table data */
	hotCtx g;       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void MMSDNew(hotCtx g) {
	MMSDCtx h = MEM_NEW(g, sizeof(struct MMSDCtx_));

	h->tbl.axis_.axis_ = NULL;
	h->tbl.instance_.instance_ = NULL;
	h->tbl.style_.style_ = NULL;

	/* Link contexts */
	h->g = g;
	g->ctx.MMSD = h;
}

int MMSDFill(hotCtx g) {
	int i;
	Offset offset;
	MMSDCtx h = g->ctx.MMSD;

	if (!IS_MM(g)) {
		return 0;
	}

	h->tbl.version = VERSION(1, 0);
	h->tbl.flags = 0;
	if (g->font.flags & HOT_USE_FOR_SUBST) {
		h->tbl.flags |= MMSD_USE_FOR_SUBST;
	}
	if (g->font.flags & HOT_CANT_INSTANCE) {
		h->tbl.flags |= MMSD_CANT_INSTANCE;
	}

	/* Fill axis table */
	h->tbl.axis_.nAxes = (unsigned short)g->font.mm.axis.cnt;
	h->tbl.axis_.axisSize = AXIS_SIZE;
	h->tbl.axis_.axis_ = MEM_NEW(g, sizeof(AxisRec) * h->tbl.axis_.nAxes);
	for (i = 0; i < h->tbl.axis_.nAxes; i++) {
		AxisRec *dst = &h->tbl.axis_.axis_[i];
		MMAxis *src = &g->font.mm.axis.array[i];
		dst->longLabel_ = src->longLabel;
		dst->shortLabel_ = src->shortLabel;
	}

	if (g->font.mm.instance.cnt != 0) {
		/* Fill instance table */
		h->tbl.instance_.nInstances = (unsigned short)g->font.mm.instance.cnt;
		h->tbl.instance_.instanceSize = INSTANCE_SIZE;
		h->tbl.instance_.instance_ =
		    MEM_NEW(g, sizeof(InstanceRec) * h->tbl.instance_.nInstances);
		for (i = 0; i < h->tbl.instance_.nInstances; i++) {
			h->tbl.instance_.instance_[i].nameSuffix_ =
			    g->font.mm.instance.array[i].suffix;
		}
	}
	else {
		h->tbl.instance_.nInstances = 0;
	}

	if (g->font.mm.style.cnt != 0) {
		/* Fill style table */
		h->tbl.style_.nStyles = (unsigned short)g->font.mm.style.cnt;
		h->tbl.style_.styleSize = STYLE_SIZE;
		h->tbl.style_.style_ =
		    MEM_NEW(g, sizeof(StyleRec) * h->tbl.style_.nStyles);
		for (i = 0; i < h->tbl.style_.nStyles; i++) {
			StyleRec *dst = &h->tbl.style_.style_[i];
			MMStyle *src = &g->font.mm.style.array[i];

			dst->axis = (unsigned char)src->axis;
			dst->flags = (unsigned char)src->flags;
			dst->action[0].point = src->action[0].point;
			dst->action[0].delta = src->action[0].delta;
			dst->action[1].point = src->action[1].point;
			dst->action[1].delta = src->action[1].delta;
		}
	}
	else {
		h->tbl.style_.nStyles = 0;
	}

	/* Fill offsets */
	h->tbl.axis = offset = HEADER_SIZE;
	offset += AXIS_TBL_SIZE(h);

	if (h->tbl.instance_.nInstances != 0) {
		h->tbl.instance = offset;
		offset += INSTANCE_TBL_SIZE(h);
	}
	else {
		h->tbl.instance = 0;
	}

	if (h->tbl.style_.nStyles != 0) {
		h->tbl.style = offset;
		offset += STYLE_TBL_SIZE(h);
	}
	else {
		h->tbl.style = 0;
	}

	/* Calculate string offsets */
	for (i = 0; i < h->tbl.axis_.nAxes; i++) {
		AxisRec *axis = &h->tbl.axis_.axis_[i];

		axis->longLabel = offset;
		offset += strlen(axis->longLabel_) + 1;
		axis->shortLabel = offset;
		offset += strlen(axis->shortLabel_) + 1;
	}
	if (h->tbl.instance != 0) {
		for (i = 0; i < h->tbl.instance_.nInstances; i++) {
			InstanceRec *instance = &h->tbl.instance_.instance_[i];

			instance->nameSuffix = offset;
			offset += strlen(instance->nameSuffix_) + 1;
		}
	}

	return 1;
}

void MMSDWrite(hotCtx g) {
	int i;
	MMSDCtx h = g->ctx.MMSD;

	/* Write header */
	OUT4(h->tbl.version);
	OUT2(h->tbl.flags);
	OUT2(h->tbl.axis);
	OUT2(h->tbl.instance);
	OUT2(h->tbl.style);

	/* Write axis table */
	OUT2(h->tbl.axis_.nAxes);
	OUT2(h->tbl.axis_.axisSize);
	for (i = 0; i < h->tbl.axis_.nAxes; i++) {
		AxisRec *axis = &h->tbl.axis_.axis_[i];
		OUT2(axis->longLabel);
		OUT2(axis->shortLabel);
	}

	if (h->tbl.instance != 0) {
		/* Write instance table */
		OUT2(h->tbl.instance_.nInstances);
		OUT2(h->tbl.instance_.instanceSize);
		for (i = 0; i < h->tbl.instance_.nInstances; i++) {
			OUT2(h->tbl.instance_.instance_[i].nameSuffix);
		}
	}

	if (h->tbl.style != 0) {
		/* Write style table */
		OUT2(h->tbl.style_.nStyles);
		OUT2(h->tbl.style_.styleSize);
		for (i = 0; i < h->tbl.style_.nStyles; i++) {
			StyleRec *style = &h->tbl.style_.style_[i];
			OUT1(style->axis);
			OUT1(style->flags);
			OUT4(style->action[0].point);
			OUT4(style->action[0].delta);
			OUT4(style->action[1].point);
			OUT4(style->action[1].delta);
		}
	}

	/* Write string table */
	for (i = 0; i < h->tbl.axis_.nAxes; i++) {
		AxisRec *axis = &h->tbl.axis_.axis_[i];
		hotWritePString(g, axis->longLabel_);
		hotWritePString(g, axis->shortLabel_);
	}
	if (h->tbl.instance != 0) {
		for (i = 0; i < h->tbl.instance_.nInstances; i++) {
			hotWritePString(g, h->tbl.instance_.instance_[i].nameSuffix_);
		}
	}
}

void MMSDReuse(hotCtx g) {
	MMSDCtx h = g->ctx.MMSD;
	MEM_FREE(g, h->tbl.axis_.axis_);
	MEM_FREE(g, h->tbl.instance_.instance_);
	MEM_FREE(g, h->tbl.style_.style_);
}

void MMSDFree(hotCtx g) {
	MEM_FREE(g, g->ctx.MMSD);
}