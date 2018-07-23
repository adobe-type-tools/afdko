/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)feat.h	1.1
 * Changed:    4/13/95 10:15:21
 ***********************************************************************/

/*
 * Feature name table format definition.
 */

#ifndef FORMAT_FEAT_H
#define FORMAT_FEAT_H

#define feat_VERSION VERSION(1,0)

typedef struct
	{
	Card16 setting;
	Card16 nameId;
	} SettingName;
#define SETTING_NAME_SIZE (SIZEOF(SettingName, setting) + \
						   SIZEOF(SettingName, nameId))

typedef struct
	{
	Card16 feature;
	Card16 nSettings;
	Card32 settingOffset;
	Card16 featureFlags;
#define FLAG_EXCLUSIVE (1<<15)
	Card16 nameId;
	DCL_ARRAY(SettingName, setting);
	} FeatureName;
#define FEATURE_NAME_SIZE (SIZEOF(FeatureName, feature) + \
						   SIZEOF(FeatureName, nSettings) + \
						   SIZEOF(FeatureName, settingOffset) + \
						   SIZEOF(FeatureName, featureFlags) + \
						   SIZEOF(FeatureName, nameId))

typedef struct
	{
	Fixed version;
	Card16 nNames;
	Card16 nSets;		/* Unused */
	Card32 setOffset;	/* Unused */
	DCL_ARRAY(FeatureName, feature);
	} featTbl;
#define TBL_HDR_SIZE (SIZEOF(featTbl, version) + \
					  SIZEOF(featTbl, nNames) + \
					  SIZEOF(featTbl, nSets) + \
					  SIZEOF(featTbl, setOffset))

#endif /* FORMAT_FEAT_H */
