/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "feat.h"
#include "sfnt_feat.h"

static featTbl *feat = NULL;
static int loaded = 0;

void featRead(int32_t start, uint32_t length) {
    int i;

    if (loaded)
        return;

    feat = (featTbl *)sMemNew(sizeof(featTbl));
    SEEK_ABS(start);

    /* Read header */
    IN1(feat->version);
    IN1(feat->nNames);
    IN1(feat->nSets);
    IN1(feat->setOffset);

    /* Read feature names */
    feat->feature = sMemNew(sizeof(FeatureName) * feat->nNames);
    for (i = 0; i < feat->nNames; i++) {
        FeatureName *feature = &feat->feature[i];

        IN1(feature->feature);
        IN1(feature->nSettings);
        IN1(feature->settingOffset);
        IN1(feature->featureFlags);
        IN1(feature->nameId);
    }

    /* Read setting names */
    for (i = 0; i < feat->nNames; i++) {
        int j;
        FeatureName *feature = &feat->feature[i];

        feature->setting = sMemNew(sizeof(SettingName) * feature->nSettings);
        for (j = 0; j < feature->nSettings; j++) {
            SettingName *setting = &feature->setting[j];

            IN1(setting->setting);
            IN1(setting->nameId);
        }
    }

    loaded = 1;
}

void featDump(int level, int32_t start) {
    int i;

    DL(1, (OUTPUTBUFF, "### [feat] (%08lx)\n", start));

    /* Dump header */
    DLV(2, "version  =", feat->version);
    DL(2, (OUTPUTBUFF, "nNames   =%hu\n", feat->nNames));
    DL(2, (OUTPUTBUFF, "nSets    =%hu\n", feat->nSets));
    DL(2, (OUTPUTBUFF, "setOffset=%08x\n", feat->setOffset));

    /* Dump feature names */
    DL(2, (OUTPUTBUFF,
           "--- featureNames[index]="
           "{feature,nSettings,settingOffset,featureFlags,nameId}\n"));
    for (i = 0; i < feat->nNames; i++) {
        FeatureName *feature = &feat->feature[i];

        DL(2, (OUTPUTBUFF, "[%2d]={%2hu,%2hu,%08x,%04hx,%hu}\n", i, feature->feature,
               feature->nSettings, feature->settingOffset,
               feature->featureFlags, feature->nameId));
    }

    /* Dump setting names */
    DL(2, (OUTPUTBUFF, "--- setting[feature+setting]={setting,nameId}\n"));
    for (i = 0; i < feat->nNames; i++) {
        int j;
        FeatureName *feature = &feat->feature[i];

        for (j = 0; j < feature->nSettings; j++) {
            SettingName *setting = &feature->setting[j];

            DL(2, (OUTPUTBUFF, "[%d+%d]={%hu,%hu} ", i, j,
                   setting->setting, setting->nameId));
        }
        DL(2, (OUTPUTBUFF, "\n"));
    }
}

void featFree_spot(void) {
    int i;

    if (!loaded)
        return;

    for (i = 0; i < feat->nNames; i++)
        sMemFree(feat->feature[i].setting);
    sMemFree(feat->feature);
    sMemFree(feat);
    feat = NULL;
    loaded = 0;
}
