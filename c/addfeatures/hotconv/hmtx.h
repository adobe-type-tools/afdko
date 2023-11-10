/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_HMTX_H_
#define ADDFEATURES_HOTCONV_HMTX_H_

#include <vector>

#include "common.h"

#define hmtx_ TAG('h', 'm', 't', 'x')

/* Standard functions */
void hmtxNew(hotCtx g);
int hmtxFill(hotCtx g);
void hmtxWrite(hotCtx g);
void hmtxReuse(hotCtx g);
void hmtxFree(hotCtx g);

/* Supplementary Functions */
class hmtx {
 public:
    hmtx() = delete;
    explicit hmtx(hotCtx g) : g(g) {}
    int getNLongMetrics() { return filled ? advanceWidth.size() : -1; }
    int Fill();
    void Write();
 private:
    std::vector<uFWord> advanceWidth;
    std::vector<FWord> lsb;
    bool filled {false};
    hotCtx g {nullptr};
};

#endif  // ADDFEATURES_HOTCONV_HMTX_H_
