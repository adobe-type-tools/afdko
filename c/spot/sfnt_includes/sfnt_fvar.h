/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Font variation table format definition.
 */

#ifndef FORMAT_FVAR_H
#define FORMAT_FVAR_H

#define fvar_VERSION VERSION(1, 0)

typedef struct
{
    uint32_t axisTag;
    Fixed minValue;
    Fixed defaultValue;
    Fixed maxValue;
    uint16_t flags;
    uint16_t nameId;
} Axis;
#define AXIS_SIZE (SIZEOF(Axis, axisTag) +      \
                   SIZEOF(Axis, minValue) +     \
                   SIZEOF(Axis, defaultValue) + \
                   SIZEOF(Axis, maxValue) +     \
                   SIZEOF(Axis, flags) +        \
                   SIZEOF(Axis, nameId))

typedef struct
{
    uint16_t nameId;
    uint16_t flags;
    uint16_t psNameId; /* This field may not be present in font */
    Fixed *coord;
} Instance;
#define INSTANCE_SIZE(axes) (SIZEOF(Instance, nameId) +   \
                             SIZEOF(Instance, flags) +    \
                             SIZEOF(Instance, psNameId) + \
                             SIZEOF(Instance, coord[0]) * (axes))

typedef struct
{
    Fixed version;
    uint16_t offsetToData;
    uint16_t countSizePairs;
    uint16_t axisCount;
    uint16_t axisSize;
    uint16_t instanceCount;
    uint16_t instanceSize;
    Axis *axis;
    Instance *instance;
} fvarTbl;
#define HEADER_SIZE (SIZEOF(fvarTbl, version) +        \
                     SIZEOF(fvarTbl, offsetToData) +   \
                     SIZEOF(fvarTbl, countSizePairs) + \
                     SIZEOF(fvarTbl, axisCount) +      \
                     SIZEOF(fvarTbl, axisSize) +       \
                     SIZEOF(fvarTbl, instanceCount) +  \
                     SIZEOF(fvarTbl, instanceSize))

#endif /* FORMAT_FVAR_H */
