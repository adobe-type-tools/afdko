/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Metamorphosis table format definition.
 */

#ifndef FORMAT_MORT_H
#define FORMAT_MORT_H

#define mort_VERSION VERSION(1, 0)

/* Feature table entry */
typedef struct
{
    uint16_t feature;
    uint16_t setting;
    uint32_t enableFlags;
    uint32_t disableFlags;
} Feature;
#define FEATURE_ENTRY_SIZE (SIZEOF(Feature, feature) +     \
                            SIZEOF(Feature, setting) +     \
                            SIZEOF(Feature, enableFlags) + \
                            SIZEOF(Feature, disableFlags))

/* Type-specific tables */
typedef struct
{
    State state;
} Context;

typedef struct
{
    State state;
} Ligature;

typedef struct
{
    Lookup lookup;
#define MORT_LOOKUP_INTL 250
#define MORT_LOOKUP_INCR 100
} NonContext;

/* Mort subtable */
typedef struct _table {
    uint16_t length;
    uint16_t coverage;
#define COVERAGE_VERTICAL   (1 << 15)
#define COVERAGE_DESCENDING (1 << 14)
#define COVERAGE_CONTEXT    1
#define COVERAGE_LIGATURE   2
#define COVERAGE_NONCONTEXT 4
#define COVERAGE_TYPE       0x7
    void *type;
    struct _table *next; /* Next table in list [Not in format] */
} Table;

/* Subtable header */
typedef struct
{
    uint16_t feature;
    uint16_t setting;
    uint32_t selector;
    Table list; /* Subtable list for this feature/setting */
} Subtable;
#define SUBTABLE_HDR_SIZE (SIZEOF(Table, length) +   \
                           SIZEOF(Table, coverage) + \
                           SIZEOF(Subtable, selector))

/* Chain */
typedef struct
{
    uint32_t defaultFlags;
    uint32_t length;
    uint16_t nFeatures;
    uint16_t nSubtables;
    DCL_ARRAY(Feature, feature);
    Subtable subtable[32];
} Chain;
#define CHAIN_HDR_SIZE (SIZEOF(Chain, defaultFlags) + \
                        SIZEOF(Chain, length) +       \
                        SIZEOF(Chain, nFeatures) +    \
                        SIZEOF(Chain, nSubtables))

typedef struct
{
    Fixed version;
    uint32_t nChains;
    DCL_ARRAY(Chain, chain);
} mortTbl;

#endif /* FORMAT_MORT_H */
