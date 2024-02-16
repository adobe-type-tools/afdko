/* Copyright 2024 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#include "hotlogger.h"

#include <string>

#include "FeatCtx.h"

void hotlogger::msg(int level, const char *msg) {
    std::string m {g->ctx.feat->msgPrefix()};
    m += msg;
    wrapped->msg(level, m.c_str());
    g->note.clear();

    if (level == sFATAL)
        g->cb.fatal(g->cb.ctx);
    else if (level == sERROR)
        g->hadError = true;
}
