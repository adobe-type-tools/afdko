/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
    
#include "MVAR.h"

#include "GDEF.h"

// mvar pointer is managed by hot.cpp
void MVARNew(hotCtx g) {}
void MVARReuse(hotCtx g) {}
void MVARFree(hotCtx g) {}

int MVARFill(hotCtx g) {
    return (g->ctx.MVAR != nullptr && g->ctx.MVAR->hasValues()) ? 1 : 0;
}

void MVARWrite(hotCtx g) {
    if (g->ctx.MVAR == nullptr || !g->ctx.MVAR->hasValues())
        return;
    hotVarWriter vw {g};
    g->ctx.MVAR->write(vw);
}

void MVARAddValue(hotCtx g, ctlTag tag, const VarValueRecord &vvr) {
    g->ctx.MVAR->addValue(tag, *(g->ctx.locMap), vvr, g->logger);
}
