/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Feature name table format definition.
 */

#ifndef FORMAT_FEAT_H
#define FORMAT_FEAT_H

#define feat_VERSION VERSION(1, 0)

typedef struct
{
    uint16_t setting;
    uint16_t nameId;
} SettingName;
#define SETTING_NAME_SIZE (SIZEOF(SettingName, setting) + \
                           SIZEOF(SettingName, nameId))

typedef struct
{
    uint16_t feature;
    uint16_t nSettings;
    uint32_t settingOffset;
    uint16_t featureFlags;
#define FLAG_EXCLUSIVE (1 << 15)
    uint16_t nameId;
    DCL_ARRAY(SettingName, setting);
} FeatureName;
#define FEATURE_NAME_SIZE (SIZEOF(FeatureName, feature) +       \
                           SIZEOF(FeatureName, nSettings) +     \
                           SIZEOF(FeatureName, settingOffset) + \
                           SIZEOF(FeatureName, featureFlags) +  \
                           SIZEOF(FeatureName, nameId))

typedef struct
{
    Fixed version;
    uint16_t nNames;
    uint16_t nSets;     /* Unused */
    uint32_t setOffset; /* Unused */
    DCL_ARRAY(FeatureName, feature);
} featTbl;
#define TBL_HDR_SIZE (SIZEOF(featTbl, version) + \
                      SIZEOF(featTbl, nNames) +  \
                      SIZEOF(featTbl, nSets) +   \
                      SIZEOF(featTbl, setOffset))

#endif /* FORMAT_FEAT_H */
