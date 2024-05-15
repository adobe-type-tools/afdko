/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Macintosh resource reading support.
 */

#include <string.h>
#include <ctype.h>

#include "res.h"
#include "opt.h"
#include "sfnt.h"

/* Macintosh resource structures */
typedef struct
{
    uint32_t dataOffset;
    uint32_t mapOffset;
    uint32_t dataLength;
    uint32_t mapLength;
} Header;
#define HEADER_SIZE 256

typedef struct
{
    uint32_t length;
} Data;

typedef struct
{
    uint8_t reserved1[16];
    uint32_t reserved2;
    uint16_t reserved3;
    uint16_t attrs;
    uint16_t typeListOffset;
    uint16_t nameListOffset;
} Map;
#define MAP_SIZE (SIZEOF(Map, reserved1) +      \
                  SIZEOF(Map, reserved2) +      \
                  SIZEOF(Map, reserved3) +      \
                  SIZEOF(Map, attrs) +          \
                  SIZEOF(Map, typeListOffset) + \
                  SIZEOF(Map, nameListOffset))

typedef struct
{
    uint32_t type;
    uint16_t cnt;
    uint16_t refListOffset;
} Type;
#define TYPE_SIZE (SIZEOF(Type, type) + \
                   SIZEOF(Type, cnt) +  \
                   SIZEOF(Type, refListOffset))
typedef struct
{
    uint16_t cnt;
    Type *type;
} TypeList;

typedef struct
{
    uint16_t id;
    uint16_t nameOffset;
    uint8_t attrs;
    uint8_t dataOffset[3];
    uint32_t reserved;
} RefList;
#define REFLIST_SIZE (SIZEOF(RefList, id) +         \
                      SIZEOF(RefList, nameOffset) + \
                      SIZEOF(RefList, attrs) +      \
                      SIZEOF(RefList, dataOffset) + \
                      SIZEOF(RefList, reserved))

typedef struct
{
    uint8_t length;
} Name;

/* Client resource info */
typedef struct
{
    uint32_t type;
    uint16_t id;
    int8_t *name;
    uint8_t attrs;
#define res_SYSHEAP     (1<<6) /* Set if read into system heap */
#define res_PURGEABLE   (1<<5) /* Set if purgeable */
#define res_LOCKED      (1<<4) /* Set if locked */
#define res_PROTECTED   (1<<3) /* Set if protected */
#define res_PRELOAD     (1<<2) /* Set if to be preloaded */
#define res_CHANGED     (1<<1) /* Set if to be written to resource file */
    uint32_t offset; /* Offset to start of resource data (excludes length) */
    uint32_t length; /* Length of resource data */
} res_Info;

/* Client resource map */
typedef struct
{
    uint16_t attrs;
#define res_MAP_READONLY (1<<7) /* Set if file is read-only */
#define res_MAP_COMPACT  (1<<6) /* Set to compact file on update */
#define res_MAP_CHANGED  (1<<5) /* Set to write map on update */
    int16_t cnt;     /* Number of resources */
    res_Info *res; /* Array of resources */
} res_Map;

/* Free resource map */
static void res_MapFree(res_Map *map) {
    if (map != NULL) {
        if (map->res != NULL) {
            int i;

            for (i = 0; i < map->cnt; i++)
                if (map->res[i].name != NULL)
                    sMemFree(map->res[i].name);
            sMemFree(map->res);
        }
        sMemFree(map);
    }
}

/* Read resource map */
static res_Map *res_MapRead(long origin) {
    int i;
    int j;
    int k;
    int cnt;
    Header header;
    Map map;
    TypeList typeList;
    RefList *refList;
    res_Map *new;

    /* Read header */
    IN1(header.dataOffset);
    IN1(header.mapOffset);
    IN1(header.dataLength);
    IN1(header.mapLength);

    header.dataOffset += origin;
    header.mapOffset += origin;

    /* Read map */
    SEEK_ABS(header.mapOffset +
             SIZEOF(Map, reserved1) +
             SIZEOF(Map, reserved2) +
             SIZEOF(Map, reserved3));

    IN1(map.attrs);
    IN1(map.typeListOffset);
    IN1(map.nameListOffset);

    /* Read type list */
    IN1(typeList.cnt);

    typeList.type = sMemNew(sizeof(Type) * (typeList.cnt + 1));
    cnt = 0;
    for (i = 0; i <= typeList.cnt; i++) {
        Type *type = &typeList.type[i];

        IN1(type->type);
        IN1(type->cnt);
        IN1(type->refListOffset);

        cnt += type->cnt + 1;
    }

    /* Read reference list */
    refList = sMemNew(sizeof(RefList) * cnt);
    k = 0;
    for (i = 0; i <= typeList.cnt; i++) {
        Type *type = &typeList.type[i];

        SEEK_ABS(header.mapOffset + map.typeListOffset + type->refListOffset);
        for (j = 0; j <= type->cnt; j++) {
            RefList *ref = &refList[k++];

            IN1(ref->id);
            IN1(ref->nameOffset);
            IN1(ref->attrs);
            IN1(ref->dataOffset[0]);
            IN1(ref->dataOffset[1]);
            IN1(ref->dataOffset[2]);
            IN1(ref->reserved);
        }
    }

    /* Build client map */
    new = sMemNew(sizeof(res_Map));
    new->attrs = map.attrs;
    new->cnt = cnt;
    new->res = sMemNew(sizeof(res_Info) * cnt);

    /* Build client resource info */
    k = 0;
    for (i = 0; i <= typeList.cnt; i++) {
        Type *type = &typeList.type[i];

        for (j = 0; j <= type->cnt; j++) {
            res_Info *res = &new->res[k];
            RefList *ref = &refList[k++];

            res->type = type->type;
            res->id = ref->id;
            res->attrs = ref->attrs;
            res->offset = ((uint32_t)ref->dataOffset[0] << 16 |
                           ref->dataOffset[1] << 8 |
                           ref->dataOffset[2]) +
                          HEADER_SIZE;
            res->offset += origin;
            if (ref->nameOffset == (uint16_t)0xffff)
                res->name = NULL;
            else {
                /* Read name */
                Name name;

                SEEK_ABS(header.mapOffset + map.nameListOffset +
                         ref->nameOffset);
                IN1(name.length);

                res->name = sMemNew(name.length + 1);
                IN_BYTES(name.length, (uint8_t *)res->name);
                res->name[name.length] = '\0';
            }
        }
    }

    /* Read data lengths */
    for (i = 0; i < cnt; i++) {
        Data data;

        SEEK_ABS(new->res[i].offset);
        IN1(data.length);

        new->res[i].length = data.length;
        new->res[i].offset += SIZEOF(Data, length);
    }

    sMemFree(refList);

    return new;
}

static IdList ids;     /* sfnt id list */
static int allIds = 0; /* Flags to dump all ids in suitcase */

/* sfnt resource id list argument scanner */
int resIdScan(int argc, char *argv[], int argi, opt_Option *opt) {
    if (argi == 0)
        return 0; /* No initialization required */

    if (argi == argc)
        opt_Error(opt_Missing, opt, NULL);
    else {
        int8_t *arg = argv[argi++];

        allIds = strcmp(arg, "all") == 0;
        if (!allIds) {
            da_INIT_ONCE(ids, 5, 2);
            ids.cnt = 0;
            if (parseIdList(arg, &ids))
                opt_Error(opt_Format, opt, arg);
        }
    }
    return argi;
}

/* Macintosh resource reader */
void resRead(long origin) {
    int i;
    int nsfnts;
    int isfnt;
    res_Map *map = res_MapRead(origin);

    if (allIds) {
        /* Dump every sfnt in suitcase */
        for (i = 0; i < map->cnt; i++) {
            res_Info *res = &map->res[i];
            if (res->type == sfnt_) {
                sfntRead(res->offset, res->id);
                sfntDump();
                sfntFree_spot((i == (map->cnt - 1)));
            }
        }
        res_MapFree(map);
        return;
    }

    /* Count number of sfnt resources */
    nsfnts = 0;
    for (i = 0; i < map->cnt; i++)
        if (map->res[i].type == sfnt_) {
            nsfnts++;
            isfnt = i; /* Save its index */
        }

    if (opt_Present("-r") || (nsfnts != 1 && ids.cnt == 0)) {
        /* Dump resource map if explicitly requested (-r) or if more than 1
           sfnt in file and no sfnt ids requested */

        fprintf(OUTPUTBUFF,
                "### Macintosh resource map\n"
                "Type   Id  Attr  Offset        Length         Name\n"
                "---- ----- ---- -------- ------------------ --------\n");

        for (i = 0; i < map->cnt; i++) {
            res_Info *res = &map->res[i];
            fprintf(OUTPUTBUFF, "%c%c%c%c %5hu  %02hx  %08x %7u (%08x) %s\n",
                    TAG_ARG(res->type),
                    res->id, (uint16_t)res->attrs, res->offset, res->length, res->length,
                    (res->name == NULL) ? "--none--" : (char *)res->name);
        }
    }

    if (ids.cnt > 0) {
        /* Dump requested ids */
        for (i = 0; i < ids.cnt; i++) {
            int j;
            int id = ids.array[i];

            for (j = 0; j < map->cnt; j++) {
                res_Info *res = &map->res[j];

                if (res->type == sfnt_ && res->id == id) {
                    sfntRead(res->offset, id);
                    sfntDump();
                    sfntFree_spot((j == (map->cnt - 1)));
                    goto next;
                }
            }
            spotWarning(SPOT_MSG_NOSUCHSFNT, id);
        next:;
        }
    } else if (nsfnts == 1) {
        /* Only one sfnt so dump it */
        sfntRead(map->res[isfnt].offset, map->res[isfnt].id);
        sfntDump();
        sfntFree_spot(1);
    }
    res_MapFree(map);
}
