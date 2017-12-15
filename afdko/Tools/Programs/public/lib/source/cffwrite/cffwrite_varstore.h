//
//  cffwrite_varstore.h
//  cffwrite
//
//  Created by rroberts on 4/12/17.
//
//

#ifndef cffwrite_varstore_h
#define cffwrite_varstore_h

#include "cffwrite.h"
#include "cffwrite_dict.h"
#include "varread.h"


void cfwDictFillVarStore(cfwCtx g, DICT *dst, abfTopDict *top);

#endif /* cffwrite_varstore_h */
