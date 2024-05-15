/* Copyright 2017 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Dictionary support.
 */

#include "cffwrite_varstore.h"

#include <stdio.h>

#include <cassert>
#include <vector>

#include "ctutil.h"
#include "cffwrite_dict.h"

class cfwVarWriter : public VarWriter {
 public:
    cfwVarWriter() = delete;
    explicit cfwVarWriter(DICT *dict) : dict(dict) {}
    void w1(char o) override {
        char *arg = dnaEXTEND(*dict, 1);
        arg[1] = (unsigned char)o;
    }
    void w2(int16_t o) override {
        char *arg = dnaEXTEND(*dict, 2);
        arg[0] = (unsigned char)(o >> 8);
        arg[1] = (unsigned char)o;
    }
    void w3(int32_t o) override { assert(false); }
    void w4(int32_t o) override {
        char *arg = dnaEXTEND(*dict, 4);
        arg[0] = (unsigned char)(o >> 24);
        arg[1] = (unsigned char)(o >> 16);
        arg[2] = (unsigned char)(o >> 8);
        arg[3] = (unsigned char)o;
    }
    void w(size_t count, char *data) { assert(false); }
    DICT *dict;
};

void cfwDictFillVarStore(cfwCtx g, DICT *dst, abfTopDict *top) {
    cfwVarWriter vw {dst};

    vw.w2((uint16_t)top->varStore->getSize());
    top->varStore->write(vw);
}
