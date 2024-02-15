/* Copyright 2022 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SHARED_INCLUDE_SLOGGER_H_
#define SHARED_INCLUDE_SLOGGER_H_

#ifndef CTL_CDECL
#define CTL_CDECL
#endif /* CTL_CDECL */

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// Should match python logging level values
enum {
    sDEBUG = 10,
    sINFO = 20,
    sINDENT = 24,
    sFLUSH = 25,
    sWARNING = 30,
    sERROR = 40,
    sFATAL = 50,
    sINHERIT = 99999
};

enum {
    log_context_new,
    log_context_unused,
    log_context_used
};

void CTL_CDECL sLog(int level, const char *fmt, ...);
void sLogMsg(int level, const char *msg);
void svLog(int level, const char *fmt, va_list ap);

#ifdef __cplusplus
}

#include <memory>

class slogger {
 public:
    static std::shared_ptr<slogger> (*getLogger)(const char *name);
    static std::shared_ptr<slogger> extc_logger;
    virtual void msg(int level, const char *msg) = 0;
    virtual void CTL_CDECL log(int level, const char *fmt, ...);
    virtual void vlog(int level, const char *fmt, va_list ap);
    virtual int set_context(const char *key, int level, const char *str) = 0;
    virtual int clear_context(const char *key) = 0;
};

#endif

#endif  // SHARED_INCLUDE_SLOGGER_H_
