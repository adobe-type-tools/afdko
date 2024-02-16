/* Copyright 2024 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef ADDFEATURES_HOTCONF_HOTLOGGER_H_
#define ADDFEATURES_HOTCONF_HOTLOGGER_H_

#include "slogger.h"
#include "common.h"

class hotlogger : public slogger {
 public:
    hotlogger() = delete;
    hotlogger(hotCtx g, std::shared_ptr<slogger> l) : g(g) {
        if (l != nullptr)
            wrapped = l;
        else
            wrapped = slogger::getLogger("hotconv");
    }
    void msg(int level, const char *msg) override;
    int set_context(const char *key, int level, const char *str) override {
        return wrapped->set_context(key, level, str);
    }
    int clear_context(const char *key) override {
        return wrapped->clear_context(key);
    }
 private:
    hotCtx g;
    std::shared_ptr<slogger> wrapped;
};

#endif  // ADDFEATURES_HOTCONF_HOTLOGGER_H_
