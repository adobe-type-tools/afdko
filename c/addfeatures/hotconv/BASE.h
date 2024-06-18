/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_BASE_H_
#define ADDFEATURES_HOTCONV_BASE_H_

#include "common.h"

#define BASE_ TAG('B', 'A', 'S', 'E')

/* Standard functions */
void BASENew(hotCtx g);
int BASEFill(hotCtx g);
void BASEWrite(hotCtx g);
void BASEReuse(hotCtx g);
void BASEFree(hotCtx g);

/*
class BASE {
 public:
 private:
    struct BaseScriptInfo {
        int16_t dfltBaselineInx {0};
        std::vector<int16_t> coordInx;
    };
    struct ScriptInfo {
        Tag script {0};
        int16_t baseScriptInx {0};
    };
    struct AxisInfo {
        std::vector<Tag> baseline;
        std::vector<ScriptInfo> script;
    };
    Axis horizAxis;
    Axis vertAxis;
*/

/* Supplementary functions */

// Tags in baselineTag must be sorted
void BASESetBaselineTags(hotCtx g, bool vert, std::vector<Tag> &baselineTag);

// Number of coords must match number of baselineTags from call above
void BASEAddScript(hotCtx g, bool doVert, Tag script, Tag dfltBaseline,
                   std::vector<int16_t> &coords);

#endif  // ADDFEATURES_HOTCONV_BASE_H_
