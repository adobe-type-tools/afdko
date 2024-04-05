/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
    
#include "MVAR.h"

#include "GDEF.h"

// mvar pointer is managed by hot.cpp
void MVARNew(hotCtx g) {
    if (g->ctx.axes != nullptr && g->ctx.MVAR == nullptr)
        g->ctx.MVAR = new var_MVAR();
}

int MVARFill(hotCtx g) {
    return (g->ctx.MVAR != nullptr && g->ctx.MVAR->hasValues()) ? 1 : 0;
}

void MVARWrite(hotCtx g) {
    if (g->ctx.MVAR == nullptr || !g->ctx.MVAR->hasValues())
        return;
    g->ctx.MVAR->write(g->vw);
}

void MVARAddValue(hotCtx g, ctlTag tag, const VarValueRecord &vvr) {
    g->ctx.MVAR->addValue(tag, *(g->ctx.locMap), vvr, g->logger);
}

void MVARReuse(hotCtx g) {
    delete g->ctx.MVAR;
    g->ctx.MVAR = nullptr;
}

void MVARFree(hotCtx g) {
    delete g->ctx.MVAR;
    g->ctx.MVAR = nullptr;
}
