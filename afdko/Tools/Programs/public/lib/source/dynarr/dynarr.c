/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Dynamic array support.
 */

#include "dynarr.h"

#include <string.h>

typedef dnaDCL (void, dnaGeneric);

struct dnaCtx_ { /* Library context */
	ctlMemoryCallbacks mem;
};

/* Validate client and create context. */
dnaCtx dnaNew(ctlMemoryCallbacks *mem_cb, CTL_CHECK_ARGS_DCL) {
	dnaCtx h;

	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(DNA_VERSION)) {
		return NULL;
	}

	/* Allocate context */
	h = (dnaCtx)mem_cb->manage(mem_cb, NULL, sizeof(struct dnaCtx_));
	if (h == NULL) {
		return NULL;
	}

	memset(h, 0, sizeof(*h));

	/* Initialize context */
	h->mem = *mem_cb;

	return h;
}

/* Free library context. */
void dnaFree(dnaCtx h) {
	if (h == NULL) {
		return;
	}

	h->mem.manage(&h->mem, h, 0);
}

/* Initialize dynamic array. */
void dnaInit(dnaCtx h, void *object, size_t init, size_t incr, int check) {
	dnaGeneric *da = (dnaGeneric*)object;

	if (check && da->size != 0) {
		return; /* Already initialized */
	}
	da->ctx     = h;
	da->array   = (void *)init; /* Used once then overwritten by array ptr */
	da->cnt     = 0;
	da->size    = 0;
	/* 64-bit warning fixed by cast here */
	da->incr    = (long)incr;
	da->func    = NULL;
}

/* Grow dynamic array to accomodate index. Return -1 on allocation failure else
   0. If the client has arranged to trap allocation failures in the memory
   callback functions by using setjmp/longjmp, for example, the -1 return will
   never occur and the macro wrappers provided will be safe to use. */
long dnaGrow(void *object, size_t elemsize, long index) {
	void *new_ptr;
	size_t new_size;
	dnaGeneric *da = (dnaGeneric*)object;
	dnaCtx h = da->ctx;

	if (index < da->size || elemsize == 0) {
		return 0;   /* Use existing allocation */
	}
	else if (da->size == 0) {
		/* Initial allocation */
		size_t init = (size_t)da->array;
		size_t new_mem_size;
		new_size = ((size_t)index < init) ? init :
		    init + ((index - init) + da->incr) / da->incr * da->incr;
		new_mem_size = new_size * elemsize;
		if (new_mem_size / elemsize == new_size)	/* check math overflow */
			new_ptr = h->mem.manage(&h->mem, NULL, new_mem_size);
		else
			new_ptr = NULL;
	}
	else {
		/* Incremental allocation */
		new_size = da->size +
		    ((index - da->size) + da->incr) / da->incr * da->incr;
		if (new_size * elemsize >= new_size)	/* check math overflow */
			new_ptr = h->mem.manage(&h->mem, da->array, new_size * elemsize);
		else
			new_ptr = NULL;
	}

	if (new_ptr == NULL) {
		return -1;  /* Allocation failed */
	}
	if (da->func != NULL) {
		/* Initialize newly allocated elements */
		/* 64-bit warning fixed by cast here */
		da->func(h->mem.ctx,
		         (long)(new_size - da->size), (char *)new_ptr + da->size * elemsize);
	}

	da->array = new_ptr;
	/* 64-bit warning fixed by cast here */
	da->size = (long)new_size;
	return 0;
}

/* Set da cnt. Return -1 on error else 0. */
long dnaSetCnt(void *object, size_t elemsize, long cnt) {
	dnaGeneric *da = (dnaGeneric*)object;
	if (cnt > da->size) {
		if (dnaGrow(object, elemsize, cnt - 1)) {
			return -1;
		}
	}
	da->cnt = cnt;
	return 0;
}

/* Index da. Return -1 on error else index. */
long dnaIndex(void *object, size_t elemsize, long index) {
	dnaGeneric *da = (dnaGeneric*)object;
	if (index >= da->size) {
		if (dnaGrow(object, elemsize, index)) {
			return -1;
		}
	}
	return index;
}

/* Index da and record max index+1 in da.cnt. Return -1 on error else index. */
long dnaMax(void *object, size_t elemsize, long index) {
	dnaGeneric *da = (dnaGeneric*)object;
	if (index >= da->size) {
		if (dnaGrow(object, elemsize, index)) {
			return -1;
		}
	}
	if (index + 1 > da->cnt) {
		da->cnt = index + 1;
	}
	return index;
}

/* Grow da by one element. Return -1 on error else da->cnt. */
long dnaNext(void *object, size_t elemsize) {
	dnaGeneric *da = (dnaGeneric*)object;
	if (da->cnt >= da->size) {
		if (dnaGrow(object, elemsize, da->cnt)) {
			return -1;
		}
	}
	return da->cnt++;
}

/* Extend da by length elements. Return -1 on error else first element of
   extension. */
long dnaExtend(void *object, size_t elemsize, long length) {
	dnaGeneric *da = (dnaGeneric*)object;
	long index = da->cnt + length - 1;
	long retval = da->cnt;
	if (index >= da->size) {
		if (dnaGrow(object, elemsize, index)) {
			return -1;
		}
	}
	da->cnt = index + 1;
	return retval;
}

/* Free dynamic array object. */
void dnaFreeObj(void *object) {
	dnaGeneric *da = (dnaGeneric*)object;
	if (da->size != 0) {
		dnaCtx h = da->ctx;
		h->mem.manage(&h->mem, da->array, 0);
		da->size = 0;
	}
}

/* Get version numbers of libraries. */
void dnaGetVersion(ctlVersionCallbacks *cb) {
	if (cb->called & 1 << DNA_LIB_ID) {
		return; /* Already enumerated */
	}
	cb->getversion(cb, DNA_VERSION, "dynarr");

	/* Record this call */
	cb->called |= 1 << DNA_LIB_ID;
}