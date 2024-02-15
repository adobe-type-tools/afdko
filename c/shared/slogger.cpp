/* Copyright 2022 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "slogger.h"

#include <iostream>
#include <deque>

#define INI_VLOG_BUFSIZ 128

void slogger::vlog(int level, const char *fmt, va_list ap) {
    va_list ap_cp;
    auto buf = std::make_unique<char[]>(INI_VLOG_BUFSIZ);

    va_copy(ap_cp, ap);
    int size = std::vsnprintf(buf.get(), INI_VLOG_BUFSIZ, fmt, ap) + 1;
    if ( size > INI_VLOG_BUFSIZ ) {
        buf = std::make_unique<char[]>(size);
        vsnprintf(buf.get(), size, fmt, ap_cp);
    }
    va_end(ap_cp);
    msg(level, buf.get());
}

void CTL_CDECL slogger::log(int level, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(level, fmt, ap);
    va_end(ap);
}

struct LogContext {
    std::string key;
    std::string str;
    int level;
    bool used;
};

class deflogger : public slogger {
    void msg(int level, const char *msg) override;
    int set_context(const char *key, int level, const char *str) override;
    int clear_context(const char *key) override;
 private:
    std::deque<LogContext> contexts;
};

void deflogger::msg(int level, const char *msg) {
    for (auto &c : contexts) {
        if (!c.used) {
            std::cerr << c.str << std::endl;
            c.used = true;
        }
    }
    switch (level) {
        // NO prefix for sFLUSH
        case sINFO:
            std::cerr << "   INFO: ";
            break;
        case sINDENT:
            std::cerr << "         ";
            break;
        case sWARNING:
            std::cerr << "WARNING: ";
            break;
        case sERROR:
            std::cerr << "  ERROR: ";
            break;
        case sFATAL:
            std::cerr << "  FATAL: ";
            break;
    }
    std::cerr << msg << std::endl;
}

int deflogger::set_context(const char *key, int level, const char *str) {
    for (auto &c : contexts) {
        if (c.key == key) {
            int r = c.used ? log_context_used : log_context_unused;
            c.level = level;
            c.str = str;
            c.used = false;
            return r;
        }
    }
    contexts.push_back({key, str, level, false});
    return log_context_new;
}

int deflogger::clear_context(const char *key) {
    for (auto i = contexts.begin(); i != contexts.end() ; i++) {
        if (i->key == key) {
            int r = i->used ? log_context_used : log_context_unused;
            contexts.erase(i);
            return r;
        }
    }
    return log_context_new;
}

static std::shared_ptr<slogger> getDefLogger(const char *name) {
    return std::make_shared<deflogger>();
}

std::shared_ptr<slogger> (*slogger::getLogger)(const char *name) = getDefLogger;
std::shared_ptr<slogger> slogger::extc_logger = std::make_unique<deflogger>();

void CTL_CDECL sLog(int level, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    slogger::extc_logger->vlog(level, fmt, ap);
    va_end(ap);
}

void sLogMsg(int level, const char *msg) {
    slogger::extc_logger->msg(level, msg);
}

void svLog(int level, const char *fmt, va_list ap) {
    slogger::extc_logger->vlog(level, fmt, ap);
}
