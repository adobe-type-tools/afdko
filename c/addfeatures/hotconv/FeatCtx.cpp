/* Copyright 2021 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 * This software is licensed as OpenSource, under the Apache License, Version 2.0. 
 * This license is available at: http://opensource.org/licenses/Apache-2.0.
 */

#include "FeatCtx.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "antlr4-runtime.h"
#include "FeatVisitor.h"
#include "FeatParser.h"
#include "otl.h"
#include "GPOS.h"
#include "GSUB.h"
#include "GDEF.h"
#include "OS_2.h"
#include "BASE.h"
#include "STAT.h"
#include "name.h"

#define kDEFAULT_BASECLASS_NAME "FDK_BASE_CLASS"
#define kDEFAULT_LIGATURECLASS_NAME "FDK_LIGATURE_CLASS"
#define kDEFAULT_MARKCLASS_NAME "FDK_DEFAULT_MARK_CLASS"
#define kDEFAULT_COMPONENTCLASS_NAME "FDK_DEFAULT_COMPONENT_CLASS"

const int FeatCtx::kMaxCodePageValue {32};
const int FeatCtx::kCodePageUnSet {-1};

const int MAX_NUM_LEN {3};        // For glyph ranges
const int AALT_INDEX {-1};        // used as feature index for sorting alt glyphs in rule for aalt feature

#define USE_LANGSYS_MSG                                                   \
    "use \"languagesystem\" statement(s) at beginning of file instead to" \
    " specify the language system(s) this feature should be registered under"

bool FeatCtx::LangSys::operator<(const FeatCtx::LangSys &b) const {
    return std::tie(script, lang) < std::tie(b.script, b.lang);
}

bool FeatCtx::LangSys::operator==(const FeatCtx::LangSys &b) const {
    return std::tie(script, lang) == std::tie(b.script, b.lang);
}

bool FeatCtx::State::operator==(const FeatCtx::State &b) const {
    return memcmp(this, &b, sizeof(State)) == 0;
}

bool FeatCtx::AALT::FeatureRecord::operator==(const FeatCtx::AALT::FeatureRecord &b) const {
    return feature == b.feature;
}

bool FeatCtx::AALT::FeatureRecord::operator==(const Tag &b) const {
    return feature == b;
}

FeatCtx::FeatCtx(hotCtx g) : g(g) {
    memset(&cvParameters, 0, sizeof(cvParameters));
    dnaINIT(g->DnaCTX, cvParameters.charValues, 10, 10);
}

FeatCtx::~FeatCtx() {
    dnaFREE(cvParameters.charValues);
    freeBlocks();
}

// --------------------------- Main entry point ------------------------------

void FeatCtx::fill(void) {
    const char *featpathname = g->cb.featTopLevelFile(g->cb.ctx);
    if ( featpathname == nullptr )
        return;

    root_visitor = new FeatVisitor(this, featpathname);

    root_visitor->Parse();

    // Report any parsing errors and quit before moving on to other
    // errors and warnings
    hotQuitOnError(g);

    root_visitor->Translate();

    reportOldSyntax();

    aaltCreate();

    if ( gFlags & (seenIgnoreClassFlag | seenMarkClassFlag) )
        createDefaultGDEFClasses();

    reportUnusedaaltTags();

    hotQuitOnError(g);
}

// ------------------------ GNode memory management --------------------------

void FeatCtx::addBlock() {
    auto &bl = blockList;
    if (bl.first == nullptr) {
        /* Initial allocation */
        bl.first = bl.curr = (BlockNode *) MEM_NEW(g, sizeof(BlockNode));
        bl.curr->data = (GNode *) MEM_NEW(g, sizeof(GNode) * bl.intl);
    } else {
        /* Incremental allocation */
        bl.curr->next = (BlockNode *) MEM_NEW(g, sizeof(BlockNode));
        bl.curr = bl.curr->next;
        bl.curr->data = (GNode *) MEM_NEW(g, sizeof(GNode) * bl.incr);
    }
    bl.curr->next = nullptr;
    bl.cnt = 0;
}

void FeatCtx::freeBlocks() {
    BlockNode *p, *pNext;

    for (p = blockList.first; p != nullptr; p = pNext) {
        pNext = p->next;
        MEM_FREE(g, p->data);
        MEM_FREE(g, p);
    }
}

GNode *FeatCtx::newNodeFromBlock() {
    auto &bl = blockList;
    if ( bl.first == nullptr || bl.cnt == (bl.curr == bl.first ? bl.intl : bl.incr) )
        addBlock();
    return bl.curr->data + bl.cnt++;
}

#if HOT_DEBUG

void FeatCtx::nodeStats() {
    BlockNode *p;
    auto &bl = blockList;

    fprintf(stderr,
            "### GNode Stats\n"
            "nAdded2FreeList: %ld, "
            "nNewFromBlockList: %ld, "
            "nNewFromFreeList: %ld.\n",
            nAdded2FreeList, nNewFromBlockList, nNewFromFreeList);
    fprintf(stderr, "%ld not freed\n",
            nNewFromBlockList + nNewFromFreeList - nAdded2FreeList);

    fprintf(stderr, "### BlockList:");

    for (p = bl.first; p != NULL; p = p->next) {
        long blSize = p == bl.first ? bl.intl : bl.incr;
        if (p->next != NULL) {
            fprintf(stderr, " %ld ->", blSize);
        } else {
            fprintf(stderr, " %ld/%ld\n", bl.cnt, blSize);
        }
    }
}

void FeatCtx::tagDump(Tag tag) {
    if (tag == TAG_UNDEF) {
        fprintf(stderr, "****");
    } else {
        fprintf(stderr, "%c%c%c%c", TAG_ARG(tag));
    }
}

void FeatCtx::stateDump(State &st) {
    fprintf(stderr, "scr='");
    tagDump(st.script);
    fprintf(stderr, "' lan='");
    tagDump(st.language);
    fprintf(stderr, "' fea='");
    tagDump(st.feature);
    fprintf(stderr, "' tbl='");
    tagDump(st.tbl);
    fprintf(stderr, "' lkpTyp=%d lkpFlg=%d label=%X\n",
            st.lkpType, st.lkpFlag, st.label);
}

#endif /* HOT_DEBUG */

/* Returns a glyph node, uninitialized except for flags */
static void initAnchor(AnchorMarkInfo *anchor) {
    anchor->x = 0;
    anchor->y = 0;
    anchor->contourpoint = 0;
    anchor->format = 0;
    anchor->markClass = NULL;
    anchor->markClassIndex = 0;
    anchor->componentIndex = 0;
    return;
}

// Returns a glyph node, uninitialized except for flags
GNode *FeatCtx::newNode() {
    GNode *ret;
    if (freelist != nullptr) {
#if HOT_DEBUG
        nNewFromFreeList++;
#endif
        /* Return first item from freelist */
        ret = freelist;
        freelist = freelist->nextSeq;
    } else {
        /* Return new item from da */
#if HOT_DEBUG
        nNewFromBlockList++;
#endif
        ret = newNodeFromBlock();
    }

    ret->flags = 0;
    ret->nextSeq = NULL;
    ret->nextCl = NULL;
    ret->lookupLabelCount = 0;
    ret->metricsInfo = METRICSINFOEMPTYPP;
    ret->aaltIndex = 0;
    ret->markClassName = NULL;
    initAnchor(&ret->markClassAnchorInfo);
    return ret;
}

GNode *FeatCtx::setNewNode(GID gid) {
    GNode *n = newNode();
    n->gid = gid;
    return n;
}

#if HOT_DEBUG

void FeatCtx::checkFreeNode(GNode *node) {
    GNode *testNode = freelist;
    long cnt = 0;
    while ( testNode != NULL ) {
        if ( testNode == node )
            printf("Node duplication in free list %lu. gid: %d!\n", (unsigned long)node, node->gid);
        testNode = testNode->nextSeq;
        cnt++;
        if ( cnt > nAdded2FreeList ) {
            printf("Endless loop in checkFreeList.\n");
            break;
        }
    }
}

#endif

/* Add node to freelist (Insert at beginning) */

void FeatCtx::recycleNode(GNode *node) {
#if HOT_DEBUG
    nAdded2FreeList++;
#endif
#if HOT_DEBUG
    checkFreeNode(node);
#endif
    node->nextSeq = freelist;
    freelist = node;
}

#define ITERATIONLIMIT 100000

/* Add nodes to freelist */

void FeatCtx::recycleNodes(GNode *node) {
    GNode *nextSeq;
    long iterations = 0;

    for (; node != nullptr; node = nextSeq) {
        nextSeq = node->nextSeq;
        GNode *nextCl;

        /* Recycle class */
        for (GNode *cl = node; cl != nullptr; cl = nextCl) {
            nextCl = cl->nextCl;
            recycleNode(cl);
            if (iterations++ > ITERATIONLIMIT) {
                fprintf(stderr, "Makeotf [WARNING]: Many cycles in featRecycleNode. Possible error.\n");
                return;
            }
        }
    }
}

// ---------------------------- Console messages -----------------------------

std::string FeatCtx::msgPrefix() {
    if (current_visitor != nullptr)
        return current_visitor->tokenPositionMsg();
    else
        return "";
}

void FeatCtx::featMsg(int msgType, const char *fmt, ...) {
    va_list ap, cap;
    std::vector<char> buf;
    buf.resize(128);

    va_start(ap, fmt);
    va_copy(cap, ap);
    int l = vsnprintf(buf.data(), buf.size(), fmt, ap) + 1;
    va_end(ap);
    if ( l > (int32_t)buf.size() ) {
        buf.resize(l);
        vsnprintf(buf.data(), buf.size(), fmt, cap);
    }
    va_end(cap);
    hotMsg(g, msgType, buf.data());
}

void FeatCtx::featMsg(int msgType, FeatVisitor *v,
                      antlr4::Token *t, const char *fmt, ...) {
    va_list ap;
    std::vector<char> buf;
    buf.resize(128);
    FeatVisitor *tmpv = current_visitor;
    antlr4::Token *tmpt = v->current_msg_token;

    va_start(ap, fmt);
    int l = VSPRINTF_S(buf.data(), 128, fmt, ap) + 1;
    if ( l > 128 ) {
        buf.resize(l);
        VSPRINTF_S(buf.data(), l, fmt, ap);
    }

    if ( v != tmpv ) {
        current_visitor = v;
        v->need_file_msg = tmpv->need_file_msg = true;
    }

    v->current_msg_token = t;
    hotMsg(g, msgType, buf.data());
    v->current_msg_token = tmpt;
    current_visitor = tmpv;
}

const char *FeatCtx::tokstr() {
    assert(current_visitor != NULL);
    current_visitor->currentTokStr(tokenStringBuffer);
    return tokenStringBuffer.c_str();
}

void FeatCtx::setIDText() {
    int len;
    if (curr.feature == TAG_STAND_ALONE)
        len = snprintf(g->error_id_text, ID_TEXT_SIZE, "standalone");
    else
        len = snprintf(g->error_id_text, ID_TEXT_SIZE, "feature '%c%c%c%c'",
                       TAG_ARG(curr.feature));
    if (IS_NAMED_LAB(curr.label)) {
        char* p = g->error_id_text + len;
        NamedLkp *curr = lab2NamedLkp(currNamedLkp);
        snprintf(p, ID_TEXT_SIZE-len, " lookup '%s'", curr->name.c_str());
    }
}

/* Emit report on any deprecated feature file syntax encountered */

void FeatCtx::reportOldSyntax() {
    if (syntax.numExcept > 0) {
        int one = syntax.numExcept == 1;

        featMsg(hotNOTE,
               "There %s %hd instance%s of the deprecated \"except\" syntax in the"
               " feature file. Though such statements are processed correctly by"
               " this parser for backward compatibility, please update them to the"
               " newer \"ignore substitute\" syntax. For example, change \"except a @LET"
               " sub a by a.end;\" to \"ignore sub a @LET; sub a' by a.end;\". (Note that"
               " the second rule is now required to be marked to identify it as a Chain"
               " Contextual and not a Single Substitution rule.)",
               one ? "is" : "are",
               syntax.numExcept,
               one ? "" : "s");
    }
}

// ---------------------------- Glyphs and Classes -----------------------------

/* Map feature file glyph name to gid; emit error message and return notdef if
   not found (in order to continue until hotQuitOnError() called) */
GID FeatCtx::mapGName2GID(const char *gname, bool allowNotdef) {
    GID gid;
    const char *realname;

    // if (IS_CID(g)) {
    //     zzerr("glyph name specified for a CID font");
    // }
    if ( gname[0] == '\\' )
        gname++;

    gid = mapName2GID(g, gname, &realname);

    /* Return the glyph if found in the font. When allowNotdef is set, we
     * always return and callers should check for GID_UNDEF as we can't return
     * GID_NOTDEF in this case. */
    if (gid != GID_UNDEF || allowNotdef == 1) {
        return gid;
    }

    if (realname != NULL && strcmp(gname, realname) != 0) {
        featMsg(hotERROR, "Glyph \"%s\" (alias \"%s\") not in font",
                realname, gname);
    } else {
        featMsg(hotERROR, "Glyph \"%s\" not in font.", gname);
    }
    return GID_NOTDEF;
}

GID FeatCtx::cid2gid(const std::string &cidstr) {
    GID gid = 0; /* Suppress optimizer warning */
    if (!IS_CID(g)) {
        featMsg(hotERROR, "CID specified for a non-CID font");
    } else {
        int t = strtoll(cidstr.c_str() + 1, NULL, 10); /* Skip initial '\' */
        if (t < 0 || t > 65535)
            featMsg(hotERROR, "CID not in range 0 .. 65535");
        else if ((gid = mapCID2GID(g, t)) == GID_UNDEF)
            featMsg(hotERROR, "CID not found in font");
    }
    return gid; /* Suppress compiler warning */
}

/* Get count of number of nodes in glyph class */

int FeatCtx::getGlyphClassCount(GNode *gc) {
    int cnt = 0;
    for (; gc != NULL; gc = gc->nextCl) {
        cnt++;
    }
    return cnt;
}

/* If gid not in cl, return address of end insertion point, else NULL */

static int glyphInClass(GID gid, GNode **cl, GNode ***lastClass) {
    GNode **p = cl;

    for (p = cl; *p != NULL; p = &((*p)->nextCl)) {
        if ((*p)->gid == gid) {
            *lastClass = p;
            return 1;
        }
    }

    *lastClass = p;
    return 0;
}

/* Return address of last nextCl. Preserves everything */
/* but sets nextSeq of each copied GNode to NULL       */
GNode **FeatCtx::copyGlyphClass(GNode **dst, GNode *src) {
    GNode **newDst = dst;
    for (; src != NULL; src = src->nextCl) {
        *newDst = newNode();
        **newDst = *src;
        (*newDst)->nextSeq = NULL;
        newDst = &(*newDst)->nextCl;
    }
    return newDst;
}

/* Sort a glyph class; head node retains flags of original head node */
/* nextSeq, flags, markClassName, and markCnt of head now are zeroed. */

void FeatCtx::sortGlyphClass(GNode **list, bool unique, bool reportDups) {
    unsigned long i;
    GNode *p = *list;
    /* Preserve values that are kept with the head node only, and then zero them in the head node. */
    GNode *nextTarg = p->nextSeq;
    short flags = (*list)->flags;
    MetricsInfo metricsInfo = p->metricsInfo;
    char *markClassName = p->markClassName;
    p->markClassName = NULL;
    p->metricsInfo = METRICSINFOEMPTYPP;
    p->nextSeq = NULL;
    p->flags = 0;

    /* Copy over pointers */
    std::vector<GNode *> sortTmp;

    for (; p != NULL; p = p->nextCl)
        sortTmp.push_back(p);

    struct {
        bool operator()(GNode *a, GNode *b) const { return a->gid < b->gid; }
    } cmpNode;
    std::sort(sortTmp.begin(), sortTmp.end(), cmpNode);

    /* Move pointers around */
    for (i = 0; i < sortTmp.size() - 1; i++)
        sortTmp[i]->nextCl = sortTmp[i + 1];
    sortTmp[i]->nextCl = NULL;

    *list = sortTmp[0];

    /* Check for duplicates */
    if (unique && !g->hadError) {
        for (p = *list; p->nextCl != NULL;) {
            if (p->gid == p->nextCl->gid) {
                GNode *tmp = p->nextCl;

                p->nextCl = tmp->nextCl;
                tmp->nextCl = NULL;
                tmp->nextSeq = NULL;

                /* If current_visitor is null we are being called after the
                 * feature files have been closed. In this case, we are
                 * sorting GDEF classes, and duplicates don't need a
                 * warning. */
                if ( current_visitor != nullptr && reportDups ) {
                    dumpGlyph(tmp->gid, '\0', 0);
                    featMsg(hotNOTE, "Removing duplicate glyph <%s>",
                            g->note.array);
                }
                recycleNodes(tmp);
            } else {
                p = p->nextCl;
            }
        }
    }

    /*restore head node values to the new head node.*/
    p = *list;
    p->flags = flags;
    p->nextSeq = nextTarg;
    p->metricsInfo = metricsInfo;
    p->markClassName = markClassName;
}

void FeatCtx::resetCurrentGC() {
    assert(curGCHead == nullptr && curGCTailAddr == NULL && curGCName.empty());
    curGCTailAddr = &curGCHead;
}

void FeatCtx::defineCurrentGC(const std::string &gcname) {
    resetCurrentGC();
    auto search = namedGlyphClasses.find(gcname);
    if ( search != namedGlyphClasses.end() ) {
        featMsg(hotWARNING, "Glyph class %s redefined", gcname.c_str());
        recycleNodes(search->second);
        namedGlyphClasses.erase(search);
    }
    curGCName = gcname;
}

bool FeatCtx::openAsCurrentGC(const std::string &gcname) {
    resetCurrentGC();
    auto search = namedGlyphClasses.find(gcname);
    if ( search != namedGlyphClasses.end() ) {
        GNode *nextClass = curGCHead = search->second;
        while ( nextClass->nextCl != NULL )
            nextClass = nextClass->nextCl;
        curGCTailAddr = &nextClass->nextCl;
        // If the class is found don't set curGCName as it doesn't need to
        // be stored on Close.
        return true;
    } else {
        curGCName = gcname;
        return false;
    }
}

GNode *FeatCtx::finishCurrentGC() {
    if ( !curGCName.empty() && curGCHead != nullptr )
        namedGlyphClasses.insert({curGCName, curGCHead});

    GNode *ret = curGCHead;
    curGCName.clear();
    curGCHead = nullptr;
    curGCTailAddr = nullptr;
    return ret;
}

/* Add glyph to end of current glyph class */

void FeatCtx::addGlyphToCurrentGC(GID gid) {
    *curGCTailAddr = setNewNode(gid);
    curGCTailAddr = &(*curGCTailAddr)->nextCl;
}

void FeatCtx::addGlyphClassToCurrentGC(GNode *src) {
    curGCTailAddr = copyGlyphClass(curGCTailAddr, src);
}

void FeatCtx::addGlyphClassToCurrentGC(const std::string &subGCName) {
    // XXX consider checking curGCName against subGCName and reporting that
    // error specifically
    auto search = namedGlyphClasses.find(subGCName);

    if ( search == namedGlyphClasses.end() ) {
        featMsg(hotERROR, "glyph class not defined");
        addGlyphToCurrentGC(GID_NOTDEF);
        return;
    }

    curGCTailAddr = copyGlyphClass(curGCTailAddr, search->second);
}

static long getNum(const char *str, int length) {
    char buf[MAX_NUM_LEN + 1];
    STRCPY_S(buf, sizeof(buf), str);
    assert(length < MAX_NUM_LEN);
    buf[length] = '\0';
    return strtol(buf, NULL, 10);
}

/* Glyph names in range differ by a single letter */

void FeatCtx::addAlphaRangeToCurrentGC(GID first, GID last,
                                       const char *firstName, const char *p,
                                       char q) {
    int l = strlen(firstName);
    char *gname = (char *) MEM_NEW(g, l+1);  // Extra shouldn't be needed
    char *ptr;

    STRCPY_S(gname, l+1, firstName);
    ptr = &gname[p - firstName];

    for (; *ptr <= q; (*ptr)++) {
        GID gid = (*ptr == *p) ? first : (*ptr == q) ? last : mapGName2GID(gname, false);
        addGlyphToCurrentGC(gid);
    }
    MEM_FREE(g, gname);
}

/* Glyph names in range differ by a decimal number */

void FeatCtx::addNumRangeToCurrentGC(GID first, GID last, const char *firstName,
                                     const char *p1, const char *p2,
                                     const char *q1, int numLen) {
    /* --- Range is valid number range --- */
    long i, l = strlen(firstName)+1;
    long firstNum = getNum(p1, numLen);
    long lastNum = getNum(q1, numLen);
    char *gname = (char *) MEM_NEW(g, l+3);  // Extra shouldn't be needed
    char *preNum = (char *) MEM_NEW(g, l);
    char fmt[128];

    for (i = firstNum; i <= lastNum; i++) {
        GID gid;
        fmt[0] = '\0';
        if (i == firstNum) {
            gid = first;
        } else if (i == lastNum) {
            gid = last;
        } else {
            if (i == firstNum + 1) {
                snprintf(fmt, sizeof(fmt), "%%s%%0%dd%%s", numLen);
                /* Part of glyph name before number; */
                /* p2 marks post-number              */
                STRCPY_S(preNum, l, firstName);
                assert(p1 - firstName < l);
                preNum[p1 - firstName] = '\0';
            }
            snprintf(gname, l+3, fmt, preNum, i, p2);
            gid = mapGName2GID(gname, false);
        }
        addGlyphToCurrentGC(gid);
    }
    MEM_FREE(g, gname);
    MEM_FREE(g, preNum);
}

/* Add glyph range to end of current glyph class */

void FeatCtx::addRangeToCurrentGC(GID first, GID last, const std::string &firstName,
                                  const std::string &lastName) {
#define INVALID_RANGE featMsg(hotFATAL, "Invalid glyph range [%s-%s]", \
                              firstName.c_str(), lastName.c_str())
    if (IS_CID(g)) {
        if (first <= last) {
            for (GID i = first; i <= last; i++) {
                addGlyphToCurrentGC(i);
            }
        } else {
            featMsg(hotFATAL, "Bad GID range: %u thru %u", first, last);
        }
    } else {
        if (firstName.length() != lastName.length()) {
            INVALID_RANGE;
        }

        /* Enforce glyph range rules */
        const char *fn = firstName.c_str(), *p1, *p2;
        const char *ln = lastName.c_str(), *q1, *q2;

        for (p1 = fn, q1 = ln; *p1 != '\0' && *p1 == *q1; p1++, q1++) {
            // noop
        }

        if (*p1 == '\0') {
            INVALID_RANGE; /* names are same */
        }
        for (p2 = p1, q2 = q1;
             *p2 != '\0' && *p2 != *q2;
             p2++, q2++) {
        }
        /* Both ends of the first difference are now marked. */
        /* The remainder should be the same: */
        if (strcmp(p2, q2) != 0) {
            INVALID_RANGE;
        }

        /* A difference of up to 3 digits is allowed; 1 for letters */
        switch (p2 - p1) {
            int i;

            case 1:
                if (isalpha(*p1) && isalpha(*q1) && *q1 > *p1 && *q1 - *p1 < 26) {
                    addAlphaRangeToCurrentGC(first, last, fn, p1, *q1);
                    return;
                }

            case 2:
            case 3:
                /* All differences should be digits */
                for (i = 0; i < p2 - p1; i++) {
                    if (!isdigit(p1[i]) || !isdigit(q1[i])) {
                        INVALID_RANGE;
                    }
                }
                /* Search for largest enclosing number */
                for (; p1 > firstName && isdigit(*(p1 - 1)); p1--, q1--) {
                }
                for (; isdigit(*p2); p2++, q2++) {
                }
                if (p2 - p1 > MAX_NUM_LEN) {
                    INVALID_RANGE;
                } else {
                    addNumRangeToCurrentGC(first, last, fn, p1, p2, q1, p2 - p1);
                    return;
                }
                break;

            default:
                INVALID_RANGE;
        }
    }
}

GNode *FeatCtx::lookupGlyphClass(const std::string &gcname) {
    auto search = namedGlyphClasses.find(gcname);
    if ( search == namedGlyphClasses.end() ) {
        featMsg(hotERROR, "glyph class not defined");
        return nullptr;
    }
    return search->second;
}

/* Return 1 if targ and repl have same number of glyphs in class, else
   emit error message and return 0 */

bool FeatCtx::compareGlyphClassCount(GNode *targ, GNode *repl, bool isSubrule) {
    int nTarg = getGlyphClassCount(targ);
    int nRepl = getGlyphClassCount(repl);

    if (nTarg == nRepl) {
        return true;
    }
    featMsg(hotERROR,
            "Target glyph class in %srule doesn't have the same"
            " number of elements as the replacement class; the target has %d,"
            " the replacement, %d",
            isSubrule ? "sub-" : "", nTarg, nRepl);
    return false;
}

/* Duplicates node (copies GID) num times to create a class */
void FeatCtx::extendNodeToClass(GNode *node, int num) {
    if (node->nextCl != NULL) {
        featMsg(hotFATAL, "[internal] can't extend glyph class");
    }

    if (!(node->flags & FEAT_GCLASS)) {
        node->flags |= FEAT_GCLASS;
    }

    for (int i = 0; i < num; i++) {
        node->nextCl = setNewNode(node->gid);
        node = node->nextCl;
    }
}

/* Gets length of pattern sequence (ignores any classes) */
unsigned int FeatCtx::getPatternLen(GNode *pat) {
    unsigned int len = 0;
    for (; pat != NULL; pat = pat->nextSeq) {
        len++;
    }
    return len;
}

/* Make a copy of src pattern. If num != -1, copy up to num nodes only       */
/* (assumes they exist); set the last nextSeq to NULL. Preserves all flags. */
/* Return address of last nextSeq (so that client may add on to the end).   */

GNode **FeatCtx::copyPattern(GNode **dst, GNode *src, int num) {
    int i = 0;

    for (; (num == -1) ? src != NULL : i < num; i++, src = src->nextSeq) {
        copyGlyphClass(dst, src);
        dst = &(*dst)->nextSeq;
    }
    return dst;
}

void FeatCtx::dumpGlyph(GID gid, int ch, bool print) {
    char msg[512];
    int len;
    if (IS_CID(g)) {
        len = snprintf(msg, sizeof(msg), "\\%hd", mapGID2CID(gid));
    } else {
        mapGID2Name(g, gid, msg, sizeof(msg));
        len = strlen(msg);
    }
    if (ch >= 0) {
        msg[len++] = ch;
    }
    msg[len] = '\0';

    if (print) {
        fprintf(stderr, "%s", msg);
    } else {
        strncpy(dnaEXTEND(g->note, len), msg, len);
    }
}

#define DUMP_CH(ch, print)             \
    do {                               \
        if (print)                     \
            fprintf(stderr, "%c", ch); \
        else                           \
            *dnaNEXT(g->note) = (ch);  \
    } while (0)

/* If !print, add to g->notes */

void FeatCtx::dumpGlyphClass(GNode *gc, int ch, bool print) {
    GNode *p = gc;

    DUMP_CH('[', print);
    for (; p != NULL; p = p->nextCl) {
        dumpGlyph(p->gid, (char)(p->nextCl != NULL ? ' ' : -1), print);
    }
    DUMP_CH(']', print);
    if (ch >= 0) {
        DUMP_CH(ch, print);
    }
}

/* If !print, add to g->notes */

void FeatCtx::dumpPattern(GNode *pat, int ch, bool print) {
    DUMP_CH('{', print);
    for (; pat != NULL; pat = pat->nextSeq) {
        if (pat->nextCl == NULL) {
            dumpGlyph(pat->gid, -1, print);
        } else {
            dumpGlyphClass(pat, -1, print);
        }
        if (pat->flags & FEAT_MARKED) {
            DUMP_CH('\'', print);
        }
        if (pat->nextSeq != NULL) {
            DUMP_CH(' ', print);
        }
    }
    DUMP_CH('}', print);
    if (ch >= 0) {
        DUMP_CH(ch, print);
    }
}

// ----------------------------------- Tags ------------------------------------

Tag FeatCtx::str2tag(const std::string &tagName) {
    if ( tagName.length() > 4 )
        featMsg(hotERROR, "Tag %s exceeds 4 characters", tagName.c_str());
    if ( tagName == "dflt" ) {
        return dflt_;
    } else {
        int i;
        char buf[5];
        memset(buf, 0, sizeof(buf));
        STRCPY_S(buf, sizeof(buf), tagName.c_str());
        for (i = 3; buf[i] == '\0'; i--)
            buf[i] = ' ';
        return TAG(buf[0], buf[1], buf[2], buf[3]);
    }
}

bool FeatCtx::tagAssign(Tag tag, enum TagType type, bool checkIfDef) {
    TagArray *ta = NULL;
    Tag *t = NULL;

    if (type == featureTag) {
        ta = &feature;
        t = &curr.feature;
    } else if (type == scriptTag) {
        ta = &script;
        if (tag == dflt_) {
            tag = DFLT_;
            featMsg(hotWARNING, "'dflt' is not a valid tag for a script statement; using 'DFLT'.");
        }
        t = &curr.script;
    } else if (type == languageTag) {
        ta = &language;
        if (tag == DFLT_) {
            tag = dflt_;
            featMsg(hotWARNING, "'DFLT' is not a valid tag for a language statement; using 'dflt'.");
        }
        t = &curr.language;
    } else if (type == tableTag) {
        ta = &table;
    } else if (type != featureTag) {
        featMsg(hotFATAL, "[internal] unrecognized tag type");
        return false;
    }

    if (checkIfDef && ta->find(tag) != ta->end()) {
        if (type == featureTag) {
            assert(t != NULL);
            *t = tag;
        }
        return false;
    }

    ta->emplace(tag);
    if (t != NULL)
        *t = tag;

    return true;
}

// --------------------------- Scripts and Languages ---------------------------

int FeatCtx::startScriptOrLang(TagType type, Tag tag) {
    if (curr.feature == aalt_ || curr.feature == size_) {
        featMsg(hotERROR,
                "\"script\" and \"language\" statements "
                "are not allowed in 'aalt' or 'size' features; " USE_LANGSYS_MSG);
        return -1;
    } else if ((tag != TAG_STAND_ALONE) && (curr.feature == TAG_STAND_ALONE)) {
        featMsg(hotERROR,
                "\"script\" and \"language\" statements "
                "are not allowed within standalone lookup blocks; ");
    }
    /*
    if (h->fFlags & FF_LANGSYS_MODE) {
        featMsg(hotWARNING,
                "If you don't want feature '%c%c%c%c' registered under all"
                " the language systems you specified earlier on in the feature"
                " file by the \"languagesystem\" keyword, you must start this"
                " feature with an explicit \"script\" statement",
                TAG_ARG(h->curr.feature));
        return -1;
    }
    */
    fFlags |= seenScriptLang;

    if (type == scriptTag) {
        if (tag == curr.script && curr.language == dflt_)
            return 0;

        /* Once we have seen a script or a language statement other  */
        /* than 'dflt' any further rules in the feature should not   */
        /* be added to the default list.                             */
        fFlags &= ~langSysMode;

        if (tag != curr.script) {
            if ( !tagAssign(tag, scriptTag, true) )
                featMsg(hotERROR, "script behavior already specified");

            language.clear();
            DFLTLkps.clear(); /* reset the script-specific default list to 0 */
        }
        if (curr.language != dflt_)
            tagAssign(dflt_, languageTag, false);

        include_dflt = true;
        curr.lkpFlag = 0;
        curr.markSetIndex = 0;
    } else {
        assert(type == languageTag);
        if (tag == DFLT_) {
            tag = dflt_;
            featMsg(hotWARNING, "'DFLT' is not a valid tag for a language statement; using 'dflt'.");
        }

        /* Once we have seen a script or a language statement other */
        /* than 'dflt' any further rules in the feature should not  */
        /* be added to the default list.                            */
        if ((fFlags & langSysMode) && (tag != dflt_))
            fFlags &= ~langSysMode;

        if (tag == curr.language)
            return 0; /* Statement has no effect */

        if (tag == dflt_)
            featMsg(hotERROR, "dflt must precede language-specific behavior");

        if ( !tagAssign(tag, languageTag, true) )
            featMsg(hotERROR, "language-specific behavior already specified");
    }
    return 1;
}

/* Include dflt rules in lang-specific behavior if includeDFLT != 0 */

void FeatCtx::includeDFLT(bool includeDFLT, int langChange, bool seenOD) {
    if (seenOD && !seenOldDFLT) {
        seenOldDFLT = true;
        featMsg(hotWARNING, "Use of includeDFLT and excludeDFLT tags has been deprecated. It will work, but please use 'include_dflt' and 'exclude_dflt' tags instead.");
    }
    /* Set value */
    if (langChange) {
        include_dflt = includeDFLT;
    } else if (includeDFLT != include_dflt) {
        featMsg(hotERROR,
            "can't change whether a language should include dflt rules once "
            "this has already been indicated");
    }

    if (includeDFLT) {
        /* Include dflt rules for current script and explicit language    */
        /* statement. Languages which don't exclude_dflt get the feature- */
        /* level defaults at the end of the feature.                      */
        for (auto &i : DFLTLkps)
            callLkp(i);
    } else {
        /* Defaults have been explicitly excluded from the this language.  */
        /* Find the matching  script-lang lang-sys, and set the flag so it */
        /* won't add the feature level defaults at the end of the feature. */
        auto ls = langSysMap.find({curr.script, curr.language});
        if ( ls != langSysMap.end() )
            ls->second = true;
    }
}

/* Add a language system that all features (that do not contain language or
   script statements) would be registered under */

void FeatCtx::addLangSys(Tag script, Tag language, bool checkBeforeFeature,
                         FeatParser::TagContext *langctx) {
    if (checkBeforeFeature && gFlags & seenFeature) {
        featMsg(hotERROR,
                "languagesystem must be specified before all"
                " feature blocks");
        return;
    }
    if (!(gFlags & seenLangSys)) {
        gFlags |= seenLangSys;
    } else if (script == DFLT_) {
        if (gFlags & seenNonDFLTScriptLang)
            featMsg(hotERROR, "All references to the script tag DFLT must precede all other script references.");
    } else {
        gFlags |= seenNonDFLTScriptLang;
    }

    if (script == dflt_) {
        featMsg(hotWARNING, "'dflt' is not a valid script tag for a languagesystem statement; using 'DFLT'.");
        script = DFLT_;
    }

    if ( langctx != NULL )
        current_visitor->TOK(langctx);

    if (language == DFLT_) {
        featMsg(hotWARNING, "'DFLT' is not a valid language tag for a languagesystem statement; using 'dflt'.");
        language = dflt_;
    }

    /* First check if already exists */
    if ( langSysMap.find({script, language}) != langSysMap.end() ) {
        featMsg(hotWARNING, "Duplicate specification of language system");
        return;
    }

    langSysMap.emplace(LangSys{script, language}, false);

#if HOT_DEBUG
    if (!checkBeforeFeature) {
        DF(2, (stderr, "languagesystem '%c%c%c%c' '%c%c%c%c' ;\n",
               TAG_ARG(script), TAG_ARG(language)));
    }
#endif
}

/* Register current feature under remaining language systems */

void FeatCtx::registerFeatureLangSys() {
    for (auto &ls : langSysMap) {
        bool seenGSUB = false, seenGPOS = false;

        /* If we have seen an explicit language statement for this script, */
        /* then the default lookups have already been included ( or        */
        /* excluded) by the includeDDflt function.                         */
        if (ls.second) {
            ls.second = false;
            continue;
        }

        for (const auto &lkp : lookup) {
            if (lkp.tbl == GSUB_) {
                if (!seenGSUB) {
                    GSUBFeatureBegin(g, ls.first.script, ls.first.lang, curr.feature);
                    seenGSUB = 1;
                }
                GSUBLookupBegin(g, lkp.lkpType, lkp.lkpFlag,
                                (Label)(lkp.label | REF_LAB),
                                lkp.useExtension, lkp.markSetIndex);
                GSUBLookupEnd(g, curr.feature);
            } else {
                /* lkp->tbl == GPOS_ */
                if (!seenGPOS) {
                    g->ctx.GPOSp->FeatureBegin(ls.first.script, ls.first.lang, curr.feature);
                    seenGPOS = true;
                }
                g->ctx.GPOSp->LookupBegin(lkp.lkpType, lkp.lkpFlag,
                                          (Label)(lkp.label | REF_LAB),
                                          lkp.useExtension, lkp.markSetIndex);
                g->ctx.GPOSp->LookupEnd();
            }
        }
        if (seenGSUB) {
            GSUBFeatureEnd(g);
        }
        if (seenGPOS) {
            g->ctx.GPOSp->FeatureEnd();
        }
    }
}

// --------------------------------- Features ----------------------------------

void FeatCtx::startFeature(Tag tag) {
    /* Allow interleaving features */
    if ( !tagAssign(tag, featureTag, true) ) {
        if (tag != TAG_STAND_ALONE) {
            /* This is normal for standalone lookup blocks- we use the same feature tag for each. */
            featMsg(hotWARNING, "feature already defined: %s", tokstr());
        }
    }

    fFlags = 0;
    gFlags |= seenFeature;

    lookup.clear();
    script.clear();
    if (langSysMap.size() == 0) {
        featMsg(hotWARNING,
                "[internal] Feature block seen before any language system statement. You should place languagesystem statements before any feature definition");
        addLangSys(DFLT_, dflt_, false);
    }
    tagAssign(langSysMap.cbegin()->first.script, scriptTag, false);

    language.clear();
    tagAssign(langSysMap.cbegin()->first.lang, languageTag, false);

    include_dflt = true;
    DFLTLkps.clear();

    curr.lkpFlag = 0;
    curr.markSetIndex = 0;
}

void FeatCtx::endFeature() {
    if ( curr.feature != aalt_ ) {
        closeFeatScriptLang(curr);
        registerFeatureLangSys();
        prev.tbl = TAG_UNDEF;
    }
}

/* Indicate current feature or labeled lookup block to be created with
   extension lookupTypes. */

void FeatCtx::flagExtension(bool isLookup) {
    if ( isLookup ) {
        /* lookup block scope */
        NamedLkp *curr = lab2NamedLkp(currNamedLkp);
        if ( curr == nullptr ) {
            featMsg(hotFATAL, "[internal] label not found\n");
        }
        curr->useExtension = true;
    } else {
        /* feature block scope */
        if (curr.feature == aalt_) {
            aalt.useExtension = true;
        } else {
            featMsg(hotERROR,
                    "\"useExtension\" allowed in feature-scope only"
                    " for 'aalt'");
        }
    }
}

void FeatCtx::closeFeatScriptLang(State &st) {
    if ( st.tbl == TAG_UNDEF )
        return;

    if ( st.tbl == GSUB_ ) {
        if ( st.lkpType != 0 )
            GSUBLookupEnd(g, st.feature);
        g->error_id_text[0] = '\0';
        GSUBFeatureEnd(g);
    } else if ( st.tbl == GPOS_ ) {
        if ( st.lkpType != 0 )
            g->ctx.GPOSp->LookupEnd();
        g->error_id_text[0] = '\0';
        g->ctx.GPOSp->FeatureEnd();
    }
}

void FeatCtx::addFeatureParam(const std::vector<uint16_t> &params) {
    switch (curr.feature) {
        case size_:
            prepRule(GPOS_, GPOSFeatureParam, NULL, NULL);

            g->ctx.GPOSp->AddParameters(params);

            wrapUpRule();

            break;

        default:
            featMsg(hotERROR,
                    "A feature parameter is supported only for the 'size' feature.");
    }
}

void FeatCtx::subtableBreak() {
    bool retval = false;

    if (curr.feature == aalt_ || curr.feature == size_) {
        featMsg(hotERROR, "\"subtable\" use not allowed in 'aalt' or 'size' feature");
        return;
    }

    if (curr.tbl == GSUB_) {
        retval = GSUBSubtableBreak(g);
    } else if (curr.tbl == GPOS_) {
        retval = g->ctx.GPOSp->SubtableBreak();
    } else {
        featMsg(hotWARNING, "Statement not expected here");
        return;
    }

    if (retval)
        featMsg(hotWARNING, "subtable break is supported only in class kerning lookups");
}

// --------------------------------- Lookups -----------------------------------

void FeatCtx::startLookup(const std::string &name, bool isTopLevel) {
    if (isTopLevel) {
        /* This is a standalone lookup, used outside of a feature block. */
        /* Add it to the dummy feature 'A\0\0A'                           */
        startFeature(TAG_STAND_ALONE);
        startScriptOrLang(scriptTag, TAG_STAND_ALONE);
    } else {
        if (curr.feature == aalt_) {
            featMsg(hotERROR, "\"lookup\" use not allowed in 'aalt' feature");
            return;
        }
        if (curr.feature == size_) {
            featMsg(hotERROR,
                    "\"lookup\" use not allowed anymore in 'size'"
                    " feature; " USE_LANGSYS_MSG);
            return;
        }
    }

    DF(2, (stderr, "# at start of named lookup %s\n", name.c_str()));
    if (name2NamedLkp(name) != NULL)
        featMsg(hotFATAL, "lookup name \"%s\" already defined", name.c_str());

    currNamedLkp = getNextNamedLkpLabel(name, isTopLevel);
    /* State will be recorded at end of block section, below */
}

void FeatCtx::endLookup() {
    if ( curr.feature == aalt_ || curr.feature == size_)
        return;

    NamedLkp *c = lab2NamedLkp(currNamedLkp);

    if (c == nullptr) {
        featMsg(hotFATAL, "[internal] label not found\n");
    }
    DF(2, (stderr, "# at end of named lookup %s\n", c->name.c_str()));

    if (c->isTopLevel)
        endFeature();

    endOfNamedLkpOrRef = true;

    /* Store state for future lookup references to it */
    c->state = curr;

    currNamedLkp = LAB_UNDEF;
}

/* Validate and set a particular flag/subcomponent of the lookupflag. markType
   only used if attr is otlMarkAttachmentType */

uint16_t FeatCtx::setLkpFlagAttribute(uint16_t val, unsigned int attr,
                                      uint16_t markAttachClassIndex) {
    if (attr > 1) {
        /* 1 is the RTL flag - does not need to set this */
        gFlags |= seenIgnoreClassFlag;
    }

    if (attr == otlMarkAttachmentType) {
        if (markAttachClassIndex == 0) {
            featMsg(hotERROR, "must specify non-zero MarkAttachmentType value");
        } else if (val & attr) {
            featMsg(hotERROR,
                    "MarkAttachmentType already specified in this statement");
        } else {
            val |= (markAttachClassIndex & 0xFF) << 8;
        }
    } else if (attr == otlUseMarkFilteringSet) {
        if (val & attr) {
            featMsg(hotERROR,
                    "UseMarkSetType already specified in this statement");
        }
        curr.markSetIndex = markAttachClassIndex;
        val |= attr;
    } else {
        if (val & attr) {
            featMsg(hotWARNING,
                    "\"%s\" repeated in this statement; ignoring", tokstr());
        } else {
            val |= attr;
        }
    }
    return val;
}

void FeatCtx::setLkpFlag(uint16_t flag) {
    if (curr.feature == aalt_ || curr.feature == size_)
        featMsg(hotERROR, "\"lookupflag\" use not allowed in 'aalt' or 'size' feature");
    curr.lkpFlag = flag;
    /* if UseMarkSet, then the markSetIndex is set in setLkpFlagAttribute() */
}

void FeatCtx::callLkp(State &st) {
    Label lab = st.label;

#if HOT_DEBUG
    if (g->font.debug & HOT_DB_FEAT_2) {
        if (curr.tbl == GSUB_) {
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "# call lkp ");
        if (IS_REF_LAB(lab)) {
            fprintf(stderr, "REF->");
        }
        if (IS_NAMED_LAB(lab)) {
            fprintf(stderr, "<%s>", lab2NamedLkp(lab)->name.c_str());
        } else if (IS_ANON_LAB(lab)) {
            fprintf(stderr, "<ANON>");
        } else {
            hotMsg(g, hotFATAL, "undefined label");
        }
        fprintf(stderr, "[label=%x]", lab);
        /* fprintf(stderr, " : %s:%d:%d", st->tbl == GPOS_ ? "GPOS" : "GSUB", st->lkpType, st->lkpFlag); */
        fprintf(stderr, "(but with s'%c%c%c%c' l'%c%c%c%c' f'%c%c%c%c') :\n",
                TAG_ARG(curr.script), TAG_ARG(curr.language),
                TAG_ARG(curr.feature));
    }
#endif

    /* Use the scr, lan, fea that's *currently* set.                   */
    /* Use the *original* lkpFlag and, of course, tbl and lkpType.     */
    /* Copy the lookup label, set its reference bit.                   */
    /* Simulate the first rule in the original block simply by calling */
    /* prepRule() !                                                    */

    currNamedLkp = (Label)(lab | REF_LAB); /* As in:  lookup <name> {  */
    curr.lkpFlag = st.lkpFlag;
    curr.markSetIndex = st.markSetIndex;
    prepRule(st.tbl, st.lkpType, NULL, NULL);
    /* No actual rules will be fed into GPOS/GSUB */
    wrapUpRule();
    currNamedLkp = LAB_UNDEF; /* As in:  } <name>;        */
    endOfNamedLkpOrRef = true;
}

void FeatCtx::useLkp(const std::string &name) {
    NamedLkp *lkp = name2NamedLkp(name);

    if (curr.feature == aalt_) {
        featMsg(hotERROR, "\"lookup\" use not allowed in 'aalt' feature");
        return;
    } else {
        AALT::FeatureRecord t { curr.feature, false };
        auto it = std::find(std::begin(aalt.features), std::end(aalt.features), t);
        if ( it != std::end(aalt.features) )
            it->used = true;
    }

    if (curr.feature == size_) {
        featMsg(hotERROR,
                "\"lookup\" use not allowed anymore in 'size'"
                " feature; " USE_LANGSYS_MSG);
        return;
    }

    if (lkp == NULL) {
        featMsg(hotERROR, "lookup name \"%s\" not defined", name.c_str());
    } else {
        callLkp(lkp->state);
    }
}

FeatCtx::NamedLkp *FeatCtx::name2NamedLkp(const std::string &lkpName) {
    for (auto &it : namedLkp) {
        if ( it.name == lkpName )
            return &it;
    }
    return nullptr;
}

FeatCtx::NamedLkp *FeatCtx::lab2NamedLkp(Label lab) {
    Label baselab = (Label)(lab & ~REF_LAB);

    if ( !IS_NAMED_LAB(baselab) || baselab >= (Label) namedLkp.size() )
        return nullptr;
    else
        return &namedLkp[baselab];
}

Label FeatCtx::getNextNamedLkpLabel(const std::string &str, bool isa) {
    if (namedLkp.size() >= FEAT_NAMED_LKP_END) {
        featMsg(hotFATAL,
                "[internal] maximum number of named lookups reached:"
                " %d",
                FEAT_NAMED_LKP_END + 1);
    }
    namedLkp.push_back({str, isa});
    return namedLkp.size()-1;
}

Label FeatCtx::getLabelIndex(const std::string &name) {
    NamedLkp *curr = name2NamedLkp(name);
    if (curr == NULL) {
        featMsg(hotFATAL, "lookup name \"%s\" not defined", name.c_str());
    }
    return curr->state.label;
}

Label FeatCtx::getNextAnonLabel() {
    if (anonLabelCnt > FEAT_ANON_LKP_END) {
        featMsg(hotFATAL,
                "[internal] maximum number of lookups reached: %d",
                FEAT_ANON_LKP_END - FEAT_ANON_LKP_BEG + 1);
    }
    return anonLabelCnt++;
}

// ---------------------------------- Tables -----------------------------------

void FeatCtx::startTable(Tag tag) {
    if ( !tagAssign(tag, tableTag, true) )
        featMsg(hotERROR, "table already specified");
}

void FeatCtx::setGDEFGlyphClassDef(GNode *simple, GNode *ligature, GNode *mark,
                                   GNode *component) {
    gFlags |= seenGDEFGC;
    g->ctx.GDEFp->setGlyphClass(simple, ligature, mark, component);
}

void FeatCtx::createDefaultGDEFClasses() {
    if ( !(gFlags & seenGDEFGC) ) {
        GNode *classes[4] = {nullptr};
        const char *classnames[4] = { kDEFAULT_BASECLASS_NAME,
                                      kDEFAULT_LIGATURECLASS_NAME,
                                      kDEFAULT_MARKCLASS_NAME,
                                      kDEFAULT_COMPONENTCLASS_NAME };

        /* The source glyph classes are all named classes. Named class      */
        /* glyphs get recycled when featFree() is called so we need to      */
        /* make a copy of these classes here and recycle the copy after in  */
        /* GDEF.c::createGlyphClassDef(). This is because                   */
        /* createGlyphClassDef deletes glyphs from within the class lists   */
        /* to eliminate duplicates. If we operate on a named class list,    */
        /* then any deleted duplicated glyphs gets deleted again when       */
        /* hashFree() is called. Also, the hash element head points to the  */
        /* original first glyph, which may be sorted further down the list. */

        for (int i = 0; i < 4; ++i) {
            openAsCurrentGC(classnames[i]);
            if (curGCHead != NULL) {
                copyGlyphClass(classes + i, curGCHead);
                sortGlyphClass(classes + i, true, false);
            }
            finishCurrentGC();
        }

        g->ctx.GDEFp->setGlyphClass(classes[0], classes[1], classes[2], classes[3]);
    }
}

void FeatCtx::setFontRev(const std::string &rev) {
    char *fraction = 0;
    double minor = 0;

    long major = strtol(rev.c_str(), &fraction, 10);

    if ((fraction != 0) && (strlen(fraction) > 0)) {
        short strLen = strlen(fraction);
        minor = strtod(fraction, NULL);

        if (strLen != 4) {
            double version = major + minor;
            featMsg(hotWARNING, "head FontRevision entry <%s> should have 3 fractional decimal places. Stored as <%.3f>", rev.c_str(), version);
        }
    } else {
        featMsg(hotWARNING, "head FontRevision entry <%ld> should have 3 fractional decimal places; it now has none.", major);
    }

    /* limit of 32767 as anything higher sets the sign bit to negative */
    major = MIN(major, 32767);

    /* Convert to Fixed */
    g->font.version.otf = (Fixed)((major + minor) * 65536.0);
}

/* Add name string to name table. */

void FeatCtx::addNameString(long platformId, long platspecId,
                            long languageId, long nameId, const std::string &str) {
    bool nameError = false;
    if ((nameId == 2) || (nameId == 6) || ((nameId >= 26) && (nameId <= 255)))
        nameError = true;
    else if ((nameId > 0) && ((nameId < 7) && (!(g->convertFlags & HOT_OVERRIDE_MENUNAMES)))) {
        nameError = true;
    }
    if (nameError) {
        featMsg(hotWARNING,
               "name id %ld cannot be set from the feature file. "
               "ignoring record", nameId);
        return;
    }

    /* Add defaults */
    if (platformId == -1) {
        platformId = HOT_NAME_MS_PLATFORM;
    }
    if (platformId == HOT_NAME_MS_PLATFORM) {
        if (platspecId == -1) {
            platspecId = HOT_NAME_MS_UGL;
        }
        if (languageId == -1) {
            languageId = HOT_NAME_MS_ENGLISH;
        }
    } else if (platformId == HOT_NAME_MAC_PLATFORM) {
        if (platspecId == -1) {
            platspecId = HOT_NAME_MAC_ROMAN;
        }
        if (languageId == -1) {
            languageId = HOT_NAME_MAC_ENGLISH;
        }
    }

    if (hotAddName(g,
                   (unsigned short)platformId, (unsigned short)platspecId,
                   (unsigned short)languageId, (unsigned short)nameId,
                   str.c_str())) {
        featMsg(hotERROR, "Bad string");
    }
}

/* Add 'size' feature submenuname name string to name table. */

void FeatCtx::addSizeNameString(long platformId, long platspecId,
                                long languageId, const std::string &str) {
    unsigned short nameID;

    /* We only need to reserve a name ID *once*; after the first time, */
    /* all subsequent sizemenunames will share the same nameID.        */
    if (featNameID == 0) {
        nameID = nameReserveUserID(g);
        g->ctx.GPOSp->SetSizeMenuNameID(nameID);
        featNameID = nameID;
    } else {
        nameID = featNameID;
    }

    addNameString(platformId, platspecId, languageId, nameID, str);
}

/* Add 'name for feature string to name table. */

void FeatCtx::addFeatureNameString(long platformId, long platspecId,
                                   long languageId, const std::string &str) {
    unsigned short nameID;

    /* We only need to reserve a name ID *once*; after the first time, */
    /* all subsequent sizemenunames will share the same nameID.        */
    if (featNameID == 0) {
        nameID = nameReserveUserID(g);
        GSUBSetFeatureNameID(g, curr.feature, nameID);
        featNameID = nameID;
    } else {
        nameID = featNameID;
    }

    addNameString(platformId, platspecId, languageId, nameID, str);
}

void FeatCtx::addFeatureNameParam() {
    prepRule(GSUB_, GSUBFeatureNameParam, NULL, NULL);

    GSUBAddFeatureMenuParam(g, &featNameID);

    wrapUpRule();
}

void FeatCtx::addUserNameString(long platformId, long platspecId,
                                long languageId, const std::string &str) {
    unsigned short nameID;

    /* We only need to reserve a name ID *once*. */
    if (featNameID == 0) {
        nameID = nameReserveUserID(g);
        featNameID = nameID;
    } else {
        nameID = featNameID;
    }

    addNameString(platformId, platspecId, languageId, nameID, str);
}

/* Add vendor name to OS/2 table. */
/* ------------------------------------------------------------------- */
/* ------------------------------------------------------------------- */
void FeatCtx::addVendorString(std::string str) {
    bool tooshort = false;

    while (str.size() < 4) {
        str += ' ';
        tooshort = true;
    }
    if (tooshort) {
        featMsg(hotWARNING, "Vendor name too short. Padded automatically to 4 characters.");
    }

    if (str.size() > 4) {
        featMsg(hotERROR, "Vendor name too long. Max is 4 characters.");
    }

    setVendId_str(g, str.c_str());
}

// --------------------------------- Anchors -----------------------------------

void FeatCtx::addAnchorDef(const std::string &name, const AnchorDef &a) {
    auto ret = anchorDefs.insert(std::make_pair(name, a));

    if ( !ret.second )
        featMsg(hotFATAL, "Named anchor definition '%s' is a a duplicate of an earlier named anchor definition.", name.c_str());
}

void FeatCtx::addAnchorByName(const std::string &name, int componentIndex) {
    auto search = anchorDefs.find(name);
    if ( search == anchorDefs.end() ) {
        featMsg(hotERROR, "Named anchor reference '%s' is not in list of named anchors.", name.c_str());
        return;
    }

    addAnchorByValue(search->second, false, componentIndex);
}

void FeatCtx::addAnchorByValue(const AnchorDef &a, bool isNull, int componentIndex) {
    AnchorMarkInfo am;
    initAnchor(&am);
    am.x = a.x;
    am.y = a.y;
    if ( isNull ) {
        am.format = 0;
    } else if ( a.hasContour ) {
        am.format = 2;
        am.contourpoint = a.contourpoint;
    } else {
        am.format = 1;
    }
    am.componentIndex = componentIndex;
    anchorMarkInfo.push_back(am);
}

/* Make a copy of a string */

static void copyStr(hotCtx g, char **dst, const char *src) {
    if (src == NULL) {
        *dst = NULL;
    } else {
        int l = strlen(src);
        *dst = (char *) MEM_NEW(g, l + 1);
        STRCPY_S(*dst, l+1, src);
    }
}

/* Add new mark class definition */
void FeatCtx::addMark(const std::string &name, GNode *targ) {
    GNode *curNode;

    /* save this anchor info in all glyphs of this mark class */
    curNode = targ;
    while (curNode != NULL) {
        curNode->markClassAnchorInfo = anchorMarkInfo.back();
        curNode = curNode->nextCl;
    }

    bool found = openAsCurrentGC(name);
    addGlyphClassToCurrentGC(targ);

    if ( curGCHead->flags & FEAT_USED_MARK_CLASS )
        featMsg(hotERROR, "You cannot add glyphs to a mark class after the mark class has been used in a position statement. %s.", name.c_str());

    if ( !found ) {
        /* this is the first time this mark class name has been referenced; save the class name in the head node. */
        copyStr(g, &curGCHead->markClassName, name.c_str());
    }

    finishCurrentGC();  // Save new entry if needed

    /* add mark glyphs to default base class */
    openAsCurrentGC(kDEFAULT_MARKCLASS_NAME);
    addGlyphClassToCurrentGC(targ);
    finishCurrentGC();

    recycleNodes(targ);

    gFlags |= seenMarkClassFlag;
}

// --------------------------------- Metrics -----------------------------------

void FeatCtx::addValueDef(const std::string &name, const MetricsInfo &mi) {
    auto ret = valueDefs.insert(std::make_pair(name, mi));

    if ( !ret.second )
        featMsg(hotERROR, "Named value record definition '%s' is a a duplicate of an earlier named value record definition.", name.c_str());
}

void FeatCtx::getValueDef(const std::string &name, MetricsInfo &mi) {
    auto search = valueDefs.find(name);
    if ( search == valueDefs.end() )
        featMsg(hotERROR, "Named value reference '%s' is not in list of named value records.", name.c_str());
    else
        mi = search->second;
}

// ------------------------------ Substitutions --------------------------------

void FeatCtx::prepRule(Tag newTbl, int newlkpType, GNode *targ, GNode *repl) {
    int accumDFLTLkps = 1;

    curr.tbl = newTbl;
    curr.lkpType = newlkpType;

    /* Proceed in language system mode for this feature if (1) languagesystem */
    /* specified at global scope and (2) this feature did not start with an  */
    /* explicit script or language statement                                 */
    if (    !(fFlags & langSysMode)
         && gFlags & seenLangSys
         && !(fFlags & seenScriptLang) ) {
        fFlags |= langSysMode;

        /* We're now poised at the start of the first rule of this feature.  */
        /* Start registering rules under the first language system (register */
        /* under the rest of the language systems at end of feature).        */
        auto fls = langSysMap.cbegin();
        assert(fls !=langSysMap.cend());
        tagAssign(fls->first.script, scriptTag, false);
        tagAssign(fls->first.lang, languageTag, false);
    }

    /* Set the label */
    if (currNamedLkp != LAB_UNDEF) {
        /* This is a named lkp or a reference (to an anon/named) lkp */
        if (curr.label != currNamedLkp) {
            curr.label = currNamedLkp;
        } else if (curr.script == prev.script &&
                   curr.feature == prev.feature) {
            /* Store lkp state only once per lookup. If the script/feature  */
            /* has changed but the named label hasn't, accumDFLTLkps should */
            /* be 1.                                                        */
            accumDFLTLkps = 0;
        }
    } else {
        /* This is an anonymous rule */
        if (prev.script == curr.script &&
            prev.language == curr.language &&
            prev.feature == curr.feature &&
            prev.tbl == curr.tbl &&
            prev.lkpType == curr.lkpType) {
            /* I don't test against lookupFlag and markSetIndex, as I do     */
            /* not want to silently start a new lookup because these changed */
            if ( endOfNamedLkpOrRef ) {
                curr.label = getNextAnonLabel();
            } else {
                curr.label = prev.label;
                accumDFLTLkps = 0; /* Store lkp state only once per lookup */
            }
        } else {
            curr.label = getNextAnonLabel();
        }
    }

    /* --- Everything is now in current state for the new rule --- */

    /* Check that rules in a named lkp block */
    /* are of the same tbl, lkpType, lkpFlag */
    if (currNamedLkp != LAB_UNDEF && IS_NAMED_LAB(curr.label) &&
        !IS_REF_LAB(curr.label) && prev.label == curr.label) {
        if (curr.tbl != prev.tbl || curr.lkpType != prev.lkpType) {
            featMsg(hotFATAL,
                    "Lookup type different from previous "
                    "rules in this lookup block");
        } else if (curr.lkpFlag != prev.lkpFlag) {
            featMsg(hotFATAL,
                    "Lookup flags different from previous "
                    "rules in this block");
        } else if (curr.markSetIndex != prev.markSetIndex) {
            featMsg(hotFATAL,
                    "Lookup flag UseMarkSetIndex different from previous "
                    "rules in this block");
        }
    }

    /* Reset DFLTLkp accumulator at feature change or start of DFLT run */
    if ((curr.feature != prev.feature) || (curr.script != prev.script))
        DFLTLkps.clear();

    /* stop accumulating script specific defaults once a language other than 'dflt' is seen */
    if (accumDFLTLkps && curr.language != dflt_)
        accumDFLTLkps = 0;

    if (accumDFLTLkps)
        /* Save for possible inclusion later in lang-specific stuff */
        DFLTLkps.push_back(curr);

    /* Store, if applicable, for aalt creation */
    if ((!IS_REF_LAB(curr.label)) && (repl != NULL))
        storeRuleInfo(targ, repl);

    /* If h->prev != h->curr (ignoring markSet) */
    if (prev.script != curr.script ||
        prev.language != curr.language ||
        prev.feature != curr.feature ||
        prev.tbl != curr.tbl ||
        prev.lkpType != curr.lkpType ||
        prev.label != curr.label) {
        bool useExtension = false;

        closeFeatScriptLang(prev);

        /* Determine whether extension lookups are to be used */
        if (currNamedLkp != LAB_UNDEF && IS_NAMED_LAB(curr.label)) {
            NamedLkp *lkp = lab2NamedLkp(currNamedLkp);
            if (lkp == NULL) {
                featMsg(hotFATAL, "[internal] label not found\n");
            }
            useExtension = lkp->useExtension;
        }

        /* Initiate calls to GSUB/GPOS */
        if (curr.tbl == GSUB_) {
            GSUBFeatureBegin(g, curr.script, curr.language, curr.feature);
            GSUBLookupBegin(g, curr.lkpType, curr.lkpFlag,
                            curr.label, useExtension, curr.markSetIndex);
        } else if (curr.tbl == GPOS_) {
            g->ctx.GPOSp->FeatureBegin(curr.script, curr.language, curr.feature);
            g->ctx.GPOSp->LookupBegin(curr.lkpType, curr.lkpFlag,
                            curr.label, useExtension, curr.markSetIndex);
        }

        /* If LANGSYS mode, snapshot lookup info for registration under */
        /* other language systems at end of feature                     */
        if (fFlags & langSysMode)
            lookup.emplace_back(curr.tbl, curr.lkpType, curr.lkpFlag,
                                curr.markSetIndex, curr.label, useExtension);
        setIDText();

        /* --- COPY CURRENT TO PREVIOUS STATE: */
        prev = curr;
    } else {
        /* current state sate is the same for everything except maybe lkpFlag and markSetIndex */
        if (curr.lkpFlag != prev.lkpFlag) {
            featMsg(hotFATAL,
                    "Lookup flags different from previous "
                    "rules in this block");
        } else if (curr.markSetIndex != prev.markSetIndex) {
            featMsg(hotFATAL,
                    "Lookup flag UseMarkSetIndex different from previous "
                    "rules in this block");
        }
    }
}

void FeatCtx::addGSUB(int lkpType, GNode *targ, GNode *repl) {
    if (aaltCheckRule(lkpType, targ, repl))
        return;

    prepRule(GSUB_, lkpType, targ, repl);

    GSUBRuleAdd(g, targ, repl);

    wrapUpRule();
}

/* Validate targ and repl for GSUBSingle. targ and repl come in with nextSeq
   NULL and repl not marked. If valid return 1, else emit error message(s) and
   return 0 */

bool FeatCtx::validateGSUBSingle(GNode *targ, GNode *repl, bool isSubrule) {
    if (!isSubrule && targ->flags & FEAT_MARKED) {
        featMsg(hotERROR, "Target must not be marked in this rule");
        return false;
    }

    if (is_glyph(targ)) {
        if (!is_glyph(repl)) {
            featMsg(hotERROR, "Replacement in %srule must be a single glyph", isSubrule ? "sub-" : "");
            return false;
        }
    } else if (repl->nextCl != NULL &&
               !compareGlyphClassCount(targ, repl, isSubrule)) {
        return false;
    }

    return true;
}

/* Return 1 if node is an unmarked pattern of 2 or more glyphs; glyph classes
   may be present */
static bool isUnmarkedSeq(GNode *node) {
    if (node == nullptr || node->flags & FEAT_HAS_MARKED || node->nextSeq == nullptr) {
        return false;
    }
    return true;
}

/* Return 1 if node is an unmarked pattern of 2 or more glyphs, and no glyph
   classes are present */
static bool isUnmarkedGlyphSeq(GNode *node) {
    if (!isUnmarkedSeq(node)) {
        return false;
    }

    for (; node != nullptr; node = node->nextSeq) {
        if (node->nextCl != nullptr) {
            return true; /* A glyph class is present */
        }
    }
    return true;
}

/* Validate targ and repl for GSUBMultiple. targ comes in with nextSeq NULL and
   repl NULL or with nextSeq non-NULL (and unmarked). If valid return 1, else
   emit error message(s) and return 0 */

bool FeatCtx::validateGSUBMultiple(GNode *targ, GNode *repl, bool isSubrule) {
    if (!isSubrule && targ->flags & FEAT_MARKED) {
        featMsg(hotERROR, "Target must not be marked in this rule");
        return false;
    }

    if (!((isSubrule || is_glyph(targ)) && isUnmarkedGlyphSeq(repl)) && (repl != NULL || targ->flags & FEAT_LOOKUP_NODE)) {
        featMsg(hotERROR, "Invalid multiple substitution rule");
        return false;
    }
    return true;
}

/* Validate targ and repl for GSUBAlternate. repl is not marked. If valid
   return 1, else emit error message(s) and return 0 */

bool FeatCtx::validateGSUBAlternate(GNode *targ, GNode *repl, bool isSubrule) {
    if (!isSubrule && targ->flags & FEAT_MARKED) {
        featMsg(hotERROR, "Target must not be marked in this rule");
        return false;
    }

    if (!is_unmarked_glyph(targ)) {
        featMsg(hotERROR,
                "Target of alternate substitution %srule must be a"
                " single unmarked glyph",
                isSubrule ? "sub-" : "");
        return false;
    }
    if (!is_class(repl)) {
        featMsg(hotERROR,
                "Replacement of alternate substitution %srule must "
                "be a glyph class",
                isSubrule ? "sub-" : "");
        return false;
    }
    return true;
}

/* Validate targ and repl for GSUBLigature. targ comes in with nextSeq
   non-NULL and repl is unmarked. If valid return 1, else emit error message(s)
   and return 0 */

bool FeatCtx::validateGSUBLigature(GNode *targ, GNode *repl, bool isSubrule) {
    if (!isSubrule && targ->flags & FEAT_HAS_MARKED) {
        featMsg(hotERROR, "Target must not be marked in this rule");
        return false;
    }

    if (!(is_glyph(repl))) {
        featMsg(hotERROR, "Invalid ligature %srule replacement", isSubrule ? "sub-" : "");
        return false;
    }
    return true;
}

/* Analyze GSUBChain targ and repl. Return 1 if valid, else 0 */

bool FeatCtx::validateGSUBReverseChain(GNode *targ, GNode *repl) {
    int state;
    GNode *p;
    GNode *input = NULL; /* first node  of input sequence */
    // GNode *m = NULL;
    int nMarked = 0;

    if (repl == NULL) {
        /* Except clause */
        if (targ->flags & FEAT_HAS_MARKED) {
            /* Mark backtrack */
            for (p = targ; p != NULL && !(p->flags & FEAT_MARKED);
                 p = p->nextSeq) {
                p->flags |= FEAT_BACKTRACK;
            }
            for (; p != NULL && p->flags & FEAT_MARKED; p = p->nextSeq) {
                p->flags |= FEAT_INPUT;
            }
        } else {
            /* If clause is unmarked: first char is INPUT, rest LOOKAHEAD */
            targ->flags |= FEAT_INPUT;
            p = targ->nextSeq;
        }
        /* Mark rest of glyphs as lookahead */
        for (; p != NULL; p = p->nextSeq) {
            if (p->flags & FEAT_MARKED) {
                featMsg(hotERROR,
                        "ignore clause may have at most one run "
                        "of marked glyphs");
                return false;
            } else {
                p->flags |= FEAT_LOOKAHEAD;
            }
        }
        return true;
    }

    /* At most one run of marked positions supported, for now */
    for (p = targ; p != NULL; p = p->nextSeq) {
        if (p->flags & FEAT_MARKED) {
            ++nMarked;
        } else if (p->nextSeq != NULL && p->nextSeq->flags & FEAT_MARKED && nMarked > 0) {
            featMsg(hotERROR, "Reverse contextual GSUB rule may must have one and only one glyph or class marked for replacement");
            return false;
        }
    }

    /* If nothing is marked, mark everything [xxx?] */
    if (nMarked == 0) {
        // m = targ;
        for (p = targ; p != NULL; p = p->nextSeq) {
            p->flags |= FEAT_MARKED;
            nMarked++;
        }
    }
    /* m now points to beginning of marked run */

#if 0
    targ->flags |= (nMarked == 1) ? FEAT_HAS_SINGLE_MARK
                                  : FEAT_HAS_LIGATURE_MARK;
#endif

    if (repl->nextSeq != NULL) {
        featMsg(hotERROR, "Reverse contextual GSUB replacement sequence may have only one glyph or class");
        return false;
    }

    if (nMarked != 1) {
        featMsg(hotERROR, "Reverse contextual GSUB rule may must have one and only one glyph or class marked for replacement");
        return false;
    }

    /* Divide into backtrack, input, and lookahead (xxx ff interface?). */
    /* For now, input is marked glyphs                                  */
    state = FEAT_BACKTRACK;
    for (p = targ; p != NULL; p = p->nextSeq) {
        if (p->flags & FEAT_MARKED) {
            if (input == NULL) {
                input = p;
            }
            state = FEAT_INPUT;
        } else if (state != FEAT_BACKTRACK) {
            state = FEAT_LOOKAHEAD;
        }
        p->flags |= state;
    }

    if (!compareGlyphClassCount(input, repl, false)) {
        return false;
    }

    return true;
}


/* Analyze GSUBChain targ and repl. Return 1 if valid, else 0 */

bool FeatCtx::validateGSUBChain(GNode *targ, GNode *repl) {
    int state;
    GNode *p;
    int nMarked = 0;
    GNode *m = NULL; /* Suppress optimizer warning */
    int hasDirectLookups = (targ->flags & FEAT_LOOKUP_NODE);

    if (targ->flags & FEAT_IGNORE_CLAUSE) {
        /* Except clause */
        if (targ->flags & FEAT_HAS_MARKED) {
            /* Mark backtrack */
            for (p = targ; p != NULL && !(p->flags & FEAT_MARKED);
                 p = p->nextSeq) {
                p->flags |= FEAT_BACKTRACK;
            }
            for (; p != NULL && p->flags & FEAT_MARKED; p = p->nextSeq) {
                p->flags |= FEAT_INPUT;
            }
        } else {
            /* If clause is unmarked: first char is INPUT, rest LOOKAHEAD */
            targ->flags |= FEAT_INPUT;
            p = targ->nextSeq;
        }
        /* Mark rest of glyphs as lookahead */
        for (; p != NULL; p = p->nextSeq) {
            if (p->flags & FEAT_MARKED) {
                featMsg(hotERROR,
                        "ignore clause may have at most one run "
                        "of marked glyphs");
                return false;
            } else {
                p->flags |= FEAT_LOOKAHEAD;
            }
        }
        return 1;
    } else if ((repl == NULL) && (!hasDirectLookups)) {
        featMsg(hotERROR, "contextual substitution clause must have a replacement rule or direct lookup reference.");
        return false;
    }

    if (hasDirectLookups) {
        if (repl != NULL) {
            featMsg(hotERROR, "contextual substitution clause cannot both have a replacement rule and a direct lookup reference.");
            return false;
        }
        if (!(targ->flags & FEAT_HAS_MARKED)) {
            featMsg(hotERROR, "The  direct lookup reference in a contextual substitution clause must be marked as part of a contextual input sequence.");
            return false;
        }
    }

    /* At most one run of marked positions supported, for now */
    for (p = targ; p != NULL; p = p->nextSeq) {
        if (p->flags & FEAT_MARKED) {
            if (++nMarked == 1) {
                m = p;
            }
        } else if (p->lookupLabelCount > 0) {
            featMsg(hotERROR, "The  direct lookup reference in a contextual substitution clause must be marked as part of a contextual input sequence.");
            return false;
        } else if (p->nextSeq != NULL && p->nextSeq->flags & FEAT_MARKED && nMarked > 0) {
            featMsg(hotERROR, "Unsupported contextual GSUB target sequence");
            return false;
        }
    }

    /* If nothing is marked, mark everything [xxx?] */
    if (nMarked == 0) {
        m = targ;
        for (p = targ; p != NULL; p = p->nextSeq) {
            p->flags |= FEAT_MARKED;
            nMarked++;
        }
    }
    /* m now points to beginning of marked run */

    if (repl) {
        if (nMarked == 1) {
            if (is_glyph(m) && is_class(repl)) {
                featMsg(hotERROR, "Contextual alternate rule not yet supported");
                return false;
            }

            if (repl->nextSeq != NULL) {
                if (!validateGSUBMultiple(m, repl, 1)) {
                    return false;
                }
            } else if (!validateGSUBSingle(m, repl, 1)) {
                return false;
            }
        } else if (nMarked > 1) {
            if (repl->nextSeq != NULL) {
                featMsg(hotERROR, "Unsupported contextual GSUB replacement sequence");
                return false;
            }

            /* Ligature run may contain classes, but only with a single repl */
            if (!validateGSUBLigature(m, repl, 1)) {
                return false;
            }
        }
    }

    /* Divide into backtrack, input, and lookahead (xxx ff interface?). */
    /* For now, input is marked glyphs                                  */
    state = FEAT_BACKTRACK;
    for (p = targ; p != NULL; p = p->nextSeq) {
        if (p->flags & FEAT_MARKED) {
            state = FEAT_INPUT;
        } else if (state != FEAT_BACKTRACK) {
            state = FEAT_LOOKAHEAD;
        }
        p->flags |= state;
    }

    return true;
}

void FeatCtx::addSub(GNode *targ, GNode *repl, int lkpType) {
    GNode *nextNode = targ;

    for (nextNode = targ; nextNode != NULL; nextNode = nextNode->nextSeq) {
        if (nextNode->flags & FEAT_MARKED) {
            targ->flags |= FEAT_HAS_MARKED;
            if ((lkpType != GSUBReverse) && (lkpType != GSUBChain)) {
                lkpType = GSUBChain;
            }
            break;
        }
    }

    if (lkpType == GSUBChain || (targ->flags & FEAT_IGNORE_CLAUSE)) {
        /* Chain sub exceptions (further analyzed below).                */
        /* "sub f i by fi;" will be here if there was an "except" clause */

        if (!g->hadError) {
            if (validateGSUBChain(targ, repl)) {
                lkpType = GSUBChain;
                addGSUB(GSUBChain, targ, repl);
            }
        }
    } else if (lkpType == GSUBReverse) {
        if (validateGSUBReverseChain(targ, repl)) {
            addGSUB(GSUBReverse, targ, repl);
        }
    } else if (lkpType == GSUBAlternate) {
        if (validateGSUBAlternate(targ, repl, 0)) {
            addGSUB(GSUBAlternate, targ, repl);
        }
    } else if (targ->nextSeq == NULL && (repl == NULL || repl->nextSeq != NULL)) {
        if (validateGSUBMultiple(targ, repl, 0)) {
            addGSUB(GSUBMultiple, targ, repl);
        }
    } else if (targ->nextSeq == NULL && repl->nextSeq == NULL) {
        if (validateGSUBSingle(targ, repl, 0)) {
            addGSUB(GSUBSingle, targ, repl);
        }
    } else {
        GNode *next;

        if (validateGSUBLigature(targ, repl, 0)) {
            /* add glyphs to lig and component classes, in case we need to
            make a default GDEF table. Note that we may make a lot of
            duplicated. These get weeded out later. The components are
            linked by the next->nextSeq fields. For each component*/
            openAsCurrentGC(kDEFAULT_COMPONENTCLASS_NAME); /* looks up class, making if needed. Sets h->gcInsert to address of nextCl of last node, and returns it.*/
            next = targ;
            while (next != NULL) {
                if (next->nextCl != NULL) {
                    /* the current target node is a glyph class. Need to add all members of the class to the kDEFAULT_COMPONENTCLASS_NAME. */
                    addGlyphClassToCurrentGC(next);
                } else {
                    addGlyphToCurrentGC(next->gid); /* adds new node at h->gcInsert, sets h->gcInsert to address of new node's nextCl */
                }
                next = next->nextSeq;
            }
            finishCurrentGC();
            openAsCurrentGC(kDEFAULT_LIGATURECLASS_NAME);
            next = repl;
            while (next != NULL) {
                if (next->nextCl != NULL) {
                    addGlyphClassToCurrentGC(next);
                } else {
                    addGlyphToCurrentGC(next->gid);
                }
                next = next->nextSeq;
            }
            finishCurrentGC();
            addGSUB(GSUBLigature, targ, repl);
        }
    }
}

void FeatCtx::wrapUpRule() {
    prev = curr;
    endOfNamedLkpOrRef = false;
}

// -------------------------------- Positions ----------------------------------

/* Add mark class reference to current anchorMarkInfo for the rule. */
void FeatCtx::addMarkClass(const std::string &markClassName) {
    if ( !openAsCurrentGC(markClassName) ) {
        featMsg(hotERROR, "MarkToBase or MarkToMark rule references a mark class (%s) that has not yet been defined", markClassName.c_str());
        return;
    }
    curGCHead->flags |= FEAT_USED_MARK_CLASS;
    anchorMarkInfo.back().markClass = curGCHead;
    finishCurrentGC();
}

void FeatCtx::addGPOS(int lkpType, GNode *targ) {
    prepRule(GPOS_, (targ->flags & FEAT_HAS_MARKED) ? GPOSChain : lkpType, targ, NULL);

    std::string locDesc = current_visitor->tokenPositionMsg(true);
    g->ctx.GPOSp->RuleAdd(lkpType, targ, locDesc, anchorMarkInfo);

    wrapUpRule();
}

/* Analyze featValidateGPOSChain targ metrics. Return 1 if valid, else 0 */
/* Also sets flags in backtrack and look-ahead sequences */

bool FeatCtx::validateGPOSChain(GNode *targ, int lkpType) {
    int state;
    GNode *p;
    int nMarked = 0;
    int nNodesWithMetrics = 0;
    bool seenTerminalMetrics = false; /* special case for kern -like syntax in a contextual sub statement. */
    int nBaseGlyphs = 0;
    int nLookupRefs = 0;
    GNode *m = NULL;        /* Suppress optimizer warning */
    GNode *lastNode = NULL; /* Suppress optimizer warning */

    /* At most one run of marked positions supported, for now */
    for (p = targ; p != NULL; p = p->nextSeq) {
        lastNode = p;

        if (p->flags & FEAT_MARKED) {
            if (++nMarked == 1) {
                m = p;
            }
            if (p->metricsInfo.cnt != -1) {
                nNodesWithMetrics++;
            }
            if (p->lookupLabelCount > 0) {
                nLookupRefs++;
            }
        } else {
            if (p->lookupLabelCount > 0) {
                featMsg(hotERROR, "Lookup references are allowed only in the input sequence: this is the sequence of marked glyphs.");
            }

            if (p->flags & FEAT_IS_MARK_NODE) {
                featMsg(hotERROR, "The final mark class must be marked as part of the input sequence: this is the sequence of marked glyphs.");
            }

            if ((p->nextSeq != NULL) && (p->nextSeq->flags & FEAT_MARKED) && (nMarked > 0)) {
                featMsg(hotERROR, "Unsupported contextual GPOS target sequence: only one run of marked glyphs  is supported.");
                return false;
            }

            /* We actually do allow  a value records after the last glyph node, if there is only one marked glyph */
            if (p->metricsInfo.cnt != -1) {
                if (nMarked == 0) {
                    featMsg(hotERROR, "Positioning cannot be applied in the backtrack glyph sequence, before the marked glyph sequence.");
                    return false;
                }
                if ((p->nextSeq != NULL) || (nMarked > 1)) {
                    featMsg(hotERROR, "Positioning values are allowed only in the marked glyph sequence, or after the final glyph node when only one glyph node is marked.");
                    return false;
                }

                if ((p->nextSeq == NULL) && (nMarked == 1)) {
                    seenTerminalMetrics = true;
                }

                nNodesWithMetrics++;
            }
        }

        if (p->flags & FEAT_IS_BASE_NODE) {
            nBaseGlyphs++;
            if ((lkpType == GPOSCursive) && (!(p->flags & FEAT_MARKED))) {
                featMsg(hotERROR, "The base glyph or glyph class must be marked as part of the input sequence in a contextual pos cursive statement.");
            }
        }
    }

    /* m now points to beginning of marked run */

    /* Check for special case of a single marked node, with one or more lookahead nodes, and a single value record attached to the last node */
    if (seenTerminalMetrics) {
        m->metricsInfo = lastNode->metricsInfo;
        lastNode->metricsInfo = METRICSINFOEMPTYPP;
    }

    if (targ->flags & FEAT_IGNORE_CLAUSE) {
        /* An ignore clause is always contextual. If not marked, then mark the first glyph in the sequence */
        if (nMarked == 0) {
            targ->flags |= FEAT_MARKED;
            nMarked = 1;
        }
    } else if ((nNodesWithMetrics == 0) && (nBaseGlyphs == 0) && (nLookupRefs == 0)) {
        featMsg(hotERROR, "Contextual positioning rule must specify a positioning value or a mark attachment rule or a direct lookup reference.");
        return false;
    }

    /* Divide into backtrack, input, and lookahead (xxx ff interface?). */
    /* For now, input is marked glyphs                                  */
    state = FEAT_BACKTRACK;
    for (p = targ; p != NULL; p = p->nextSeq) {
        if (p->flags & FEAT_MARKED) {
            state = FEAT_INPUT;
        } else if (state != FEAT_BACKTRACK) {
            state = FEAT_LOOKAHEAD;
        }
        p->flags |= state;
    }

    return true;
}

/* ----------- ----------- -------------------------
   Targ        Metrics     Lookup type
   ----------- ----------- -------------------------
   g           m m m m     Single                   g: single glyph
   c           m m m m     Single (range)           c: glyph class (2 or more)
                                                    x: g or c
   g g         m           Pair (specific)
   g c         m           Pair (class)
   c g         m           Pair (class)
   c c         m           Pair (class)

   x x' x x'   m m         Chain ctx (not supported)
   ----------- ----------- -------------------------

   Add a pos rule from the feature file.
    Can assume targ is at least one node. If h->metrics.cnt == 0
   then targ is an "ignore position" pattern. */

void FeatCtx::addBaseClass(GNode *targ, const std::string &defaultClassName) {
    /* Find base node in a possibly contextual sequence, */
    /* and add it to the default base glyph class        */
    GNode *nextNode = targ;

    /* If it is contextual, first find the base glyph */
    if (targ->flags & FEAT_HAS_MARKED) {
        while (nextNode != NULL) {
            if (nextNode->flags & FEAT_IS_BASE_NODE) {
                break;
            }
            nextNode = nextNode->nextSeq;
        }
    }

    if (nextNode->flags & FEAT_IS_BASE_NODE) {
        openAsCurrentGC(defaultClassName);
        if (nextNode->nextCl != NULL) {
            addGlyphClassToCurrentGC(nextNode);
        } else {
            addGlyphToCurrentGC(nextNode->gid);
        }
        finishCurrentGC();
    }
}

void FeatCtx::addPos(GNode *targ, int type, bool enumerate) {
    int glyphCount = 0;
    int markedCount = 0;
    int lookupLabelCnt = 0;
    GNode *next_targ = NULL;
    GNode *copyHeadNode;

    if (enumerate) {
        targ->flags |= FEAT_ENUMERATE;
    }

    /* count glyphs, figure out if is single, pair, or context. */
    next_targ = targ;
    while (next_targ != NULL) {
        glyphCount++;
        if (next_targ->flags & FEAT_MARKED) {
            markedCount++;
        }
        if (next_targ->lookupLabelCount > 0) {
            lookupLabelCnt++;
            if (!(next_targ->flags & FEAT_MARKED)) {
                featMsg(hotERROR, "the glyph which precedes the 'lookup' keyword must be marked as part of the contextual input sequence");
            }
        }
        next_targ = next_targ->nextSeq;
    }

    /* The ignore statement can only be used with contextual lookups. */
    /* If no glyph is marked, then mark the first.                    */
    if (targ->flags & FEAT_IGNORE_CLAUSE) {
        type = GPOSChain;
        if (markedCount == 0) {
            markedCount = 1;
            targ->flags |= FEAT_MARKED;
        }
    }

    if (markedCount > 0) {
        targ->flags |= FEAT_HAS_MARKED; /* used later to decide if stuff is contextual */
    }
    if ((glyphCount == 2) && (markedCount == 0) && (type == GPOSSingle)) {
        type = GPOSPair;
    } else if (enumerate) {
        featMsg(hotERROR, "\"enumerate\" only allowed with pair positioning,");
    }

    if (type == GPOSSingle) {
        addGPOS(GPOSSingle, targ);
        /* These nodes are recycled in GPOS.c */
    } else if (type == GPOSPair) {
        next_targ = targ->nextSeq;
        if (targ->nextCl != NULL) {
            /* In order to sort and remove duplicates, I need to copy the   */
            /* original class definition. This is because class definitions */
            /* may be used for sequences as well as real classes, and       */
            /* sorting and removing duplicates from the original class is a */
            /* bad idea.                                                    */
            copyGlyphClass(&copyHeadNode, targ);
            targ = copyHeadNode;
            sortGlyphClass(&targ, true, true);
            targ->nextSeq = next_targ; /* featGlyphClassCopy zeros the  nextSeq field in all nodes.*/
        }
        if (next_targ->nextCl != NULL) {
            /* In order to sort and remove duplicates, I need to copy the    */
            /* original class definition. This is because class definitions  */
            /* may be used for sequences as well as real classes, and        */
            /* sorting and removing duplicates from the original class is a  */
            /* bad idea. */
            copyGlyphClass(&copyHeadNode, next_targ);
            next_targ = copyHeadNode;
            sortGlyphClass(&next_targ, true, true);
            targ->nextSeq = next_targ; /* featGlyphClassSort may change which node in the next_targ class is the head node.  */
        }
        /* addGPOSPair(targ, second, enumerate); */
        addGPOS(GPOSPair, targ);
        /* These nodes are recycled in GPOS.c due to some complicated copying of nodes. */
    } else if (type == GPOSCursive) {
        if (anchorMarkInfo.size() != 2) {
            featMsg(hotERROR, "The 'cursive' statement requires two anchors. This has %ld. Skipping rule.", anchorMarkInfo.size());
        } else if ((!(targ->flags & FEAT_HAS_MARKED)) && ((!(targ->flags & FEAT_IS_BASE_NODE)) || (targ->nextSeq != NULL))) {
            featMsg(hotERROR, "This statement has contextual glyphs around the cursive statement, but no glyphs are marked as part of the input sequence. Skipping rule.");
        } else {
            addGPOS(GPOSCursive, targ);
        }
        /* These nodes are recycled in GPOS.c */
    } else if (type == GPOSMarkToBase) {
        addBaseClass(targ, kDEFAULT_BASECLASS_NAME);
        if ((!(targ->flags & FEAT_HAS_MARKED)) && ((!(targ->flags & FEAT_IS_BASE_NODE)) || ((targ->nextSeq != NULL) && (targ->nextSeq->nextSeq != NULL)))) {
            featMsg(hotERROR, "This statement has contextual glyphs around the base-to-mark statement, but no glyphs are marked as part of the input sequence. Skipping rule.");
        }
        addGPOS(GPOSMarkToBase, targ);
        /* These nodes are recycled in GPOS.c */
    } else if (type == GPOSMarkToLigature) {
        addBaseClass(targ, kDEFAULT_LIGATURECLASS_NAME);
        if ((!(targ->flags & FEAT_HAS_MARKED)) && ((!(targ->flags & FEAT_IS_BASE_NODE)) || ((targ->nextSeq != NULL) && (targ->nextSeq->nextSeq != NULL)))) {
            featMsg(hotERROR, "This statement has contextual glyphs around the ligature statement, but no glyphs are marked as part of the input sequence. Skipping rule.");
        }

        /* With mark to ligatures, we may see the same mark class on         */
        /* different components, which leads to duplicate GID's in the       */
        /* contextual mark node. As a result, we need to sort and get rid of */
        /* duplicates.                                                       */

        if (targ->flags & FEAT_HAS_MARKED) {
            /* find the mark node */
            GNode *markClassNode = targ;
            GNode *prevNode = NULL;

            while (markClassNode != NULL) {
                if (markClassNode->flags & FEAT_IS_MARK_NODE) {
                    break;
                }
                prevNode = markClassNode;
                markClassNode = markClassNode->nextSeq;
            }
            if ((markClassNode != NULL) && (markClassNode->flags & FEAT_IS_MARK_NODE)) {
                copyGlyphClass(&copyHeadNode, markClassNode);
                markClassNode = copyHeadNode;
                sortGlyphClass(&markClassNode, true, false); /* changes value of markClassNode. I specify to NOT warn of duplicates, because they can happen with correct syntax. */
                prevNode->nextSeq = markClassNode;
            }
        }

        addGPOS(GPOSMarkToLigature, targ);
        /* These nodes are recycled in GPOS.c */
    } else if (type == GPOSMarkToMark) {
        addBaseClass(targ, kDEFAULT_MARKCLASS_NAME);
        if ((!(targ->flags & FEAT_HAS_MARKED)) && ((!(targ->flags & FEAT_IS_BASE_NODE)) || ((targ->nextSeq != NULL) && (targ->nextSeq->nextSeq != NULL)))) {
            featMsg(hotERROR, "This statement has contextual glyphs around the mark-to-mark statement, but no glyphs are marked as part of the input sequence. Skipping rule.");
        }
        addGPOS(GPOSMarkToMark, targ);
        /* These nodes are recycled in GPOS.c */
    } else if (type == GPOSChain) {
        /* is contextual */
        if (markedCount == 0) {
            featMsg(hotERROR, "The 'lookup' keyword can be used only in a contextual statement. At least one glyph in the sequence must be marked. Skipping rule.");
        } else {
            validateGPOSChain(targ, type);
            addGPOS(GPOSChain, targ);
        }
        /* These nodes are recycled in GPOS.c, as they are used in the fill phase, some time after this function returns. */
    } else {
        featMsg(hotERROR, "This rule type is not recognized..");
    }
}

// ------------------------------ CV Parameters --------------------------------

void FeatCtx::clearCVParameters() {
    sawCVParams = true;
    cvParameters.FeatUILabelNameID = 0;
    cvParameters.FeatUITooltipTextNameID = 0;
    cvParameters.SampleTextNameID = 0;
    cvParameters.NumNamedParameters = 0;
    cvParameters.FirstParamUILabelNameID = 0;
    cvParameters.charValues.cnt = 0;
}

void FeatCtx::addCVNameID(int labelID) {
    switch (labelID) {
        case cvUILabelEnum: {
            if (cvParameters.FeatUILabelNameID != 0) {
                featMsg(hotERROR, "A Character Variant parameter table can have only one FeatUILabelNameID entry.");
            }
            cvParameters.FeatUILabelNameID = featNameID;
            break;
        }

        case cvToolTipEnum: {
            if (cvParameters.FeatUITooltipTextNameID != 0) {
                featMsg(hotERROR, "A Character Variant parameter table can have only one SampleTextNameID entry.");
            }
            cvParameters.FeatUITooltipTextNameID = featNameID;
            break;
        }

        case cvSampleTextEnum: {
            if (cvParameters.SampleTextNameID != 0) {
                featMsg(hotERROR, "A Character Variant parameter table can have only one SampleTextNameID entry.");
            }
            cvParameters.SampleTextNameID = featNameID;
            break;
        }

        case kCVParameterLabelEnum: {
            if ( cvParameters.FirstParamUILabelNameID == 0 )
                cvParameters.FirstParamUILabelNameID = featNameID;
            else if ( cvParameters.FirstParamUILabelNameID +
                      cvParameters.NumNamedParameters != featNameID )
                featMsg(hotERROR, "Character variant AParamUILabelNameID statements must be contiguous.");
            cvParameters.NumNamedParameters++;
            break;
        }
    }

    featNameID = 0;
}

void FeatCtx::addCVParametersCharValue(unsigned long uv) {
    unsigned long *uvp = dnaNEXT(cvParameters.charValues);
    *uvp = uv;
}

void FeatCtx::addCVParam() {
    prepRule(GSUB_, GSUBCVParam, NULL, NULL);

    GSUBAddCVParam(g, &cvParameters);

    wrapUpRule();
}

// --------------------------------- Ranges ------------------------------------

/* Add Unicode and CodePage ranges to  OS/2 table. */
/* ------------------------------------------------------------------- */
/* ------------------------------------------------------------------- */
#define SET_BIT_ARR(a, b) (a[(b) / 32] |= 1UL << (b) % 32)

const short kValidCodePageNumbers[kLenCodePageList] = {
    1252,                    /*  bit 0  Latin 1 */
    1250,                    /*  bit 1  Latin 2: Eastern Europe */
    1251,                    /*  bit 2  Cyrillic */
    1253,                    /*  bit 3  Greek */
    1254,                    /*  bit 4  Turkish */
    1255,                    /*  bit 5  Hebrew */
    1256,                    /*  bit 6  Arabic */
    1257,                    /*  bit 7  Windows Baltic */
    1258,                    /*  bit 8  Vietnamese */
    FeatCtx::kCodePageUnSet, /*  bit 9  Reserved for Alternate ANSI */
    FeatCtx::kCodePageUnSet, /*  bit 10  Reserved for Alternate ANSI */
    FeatCtx::kCodePageUnSet, /*  bit 11  Reserved for Alternate ANSI */
    FeatCtx::kCodePageUnSet, /*  bit 12  Reserved for Alternate ANSI */
    FeatCtx::kCodePageUnSet, /*  bit 13  Reserved for Alternate ANSI */
    FeatCtx::kCodePageUnSet, /*  bit 14 Reserved for Alternate ANSI */
    FeatCtx::kCodePageUnSet, /*  bit 15  Reserved for Alternate ANSI */
    874,                     /*  bit 16  Thai */
    932,                     /*  bit 17  JIS/Japan */
    936,                     /*  bit 18  Chinese: Simplified chars--PRC and Singapore */
    949,                     /*  bit 19  Korean Wansung */
    950,                     /*  bit 20  Chinese: Traditional chars--Taiwan and Hong Kong */
    1361,                    /*  bit 21  Korean Johab */
    FeatCtx::kCodePageUnSet, /*  bit 22-28  Reserved for Alternate ANSI & OEM */
    FeatCtx::kCodePageUnSet, /*  bit 23  Reserved for Alternate ANSI & OEM */
    FeatCtx::kCodePageUnSet, /*  bit 24  Reserved for Alternate ANSI & OEM */
    FeatCtx::kCodePageUnSet, /*  bit 25  Reserved for Alternate ANSI & OEM */
    FeatCtx::kCodePageUnSet, /*  bit 26  Reserved for Alternate ANSI & OEM */
    FeatCtx::kCodePageUnSet, /*  bit 27  Reserved for Alternate ANSI & OEM */
    FeatCtx::kCodePageUnSet, /*  bit 28  Reserved for Alternate ANSI & OEM */
    FeatCtx::kCodePageUnSet, /*  bit 29  Macintosh Character Set (US Roman) */
    FeatCtx::kCodePageUnSet, /*  bit 30  OEM Character Set */
    FeatCtx::kCodePageUnSet, /*  bit 31  Symbol Character Set */
    FeatCtx::kCodePageUnSet, /*  bit 32  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 33  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 34  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 35  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 36  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 37  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 38  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 39  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 40  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 41  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 42  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 43  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 44  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 45  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 46  Reserved for OEM */
    FeatCtx::kCodePageUnSet, /*  bit 47  Reserved for OEM */
    869,                     /*  bit 48  IBM Greek */
    866,                     /*  bit 49  MS-DOS Russian */
    865,                     /*  bit 50  MS-DOS Nordic */
    864,                     /*  bit 51  Arabic */
    863,                     /*  bit 52  MS-DOS Canadian French */
    862,                     /*  bit 53  Hebrew */
    861,                     /*  bit 54  MS-DOS Icelandic */
    860,                     /*  bit 55  MS-DOS Portuguese */
    857,                     /*  bit 56  IBM Turkish */
    855,                     /*  bit 57  IBM Cyrillic; primarily Russian */
    852,                     /*  bit 58  Latin 2 */
    775,                     /*  bit 59  MS-DOS Baltic */
    737,                     /*  bit 60  Greek; former 437 G */
    708,                     /*  bit 61  Arabic; ASMO 708 */
    850,                     /*  bit 62  WE/Latin 1 */
    437,                     /*  bit 63  US */
};

static int validateCodePage(short pageNum) {
    int validPageBitIndex = FeatCtx::kCodePageUnSet;

    for (int i = 0; i < kLenCodePageList; i++) {
        if (pageNum == kValidCodePageNumbers[i]) {
            validPageBitIndex = i;
            break;
        }
    }

    return validPageBitIndex;
}

void FeatCtx::setUnicodeRange(short unicodeList[kLenUnicodeList]) {
    unsigned long unicodeRange[4] { 0, 0, 0, 0 };
    short i = 0;

    while (i < kLenUnicodeList) {
        short bitnum = unicodeList[i];

        if (bitnum != kCodePageUnSet) {
            if ((bitnum >= 0) && (bitnum < kLenUnicodeList)) {
                SET_BIT_ARR(unicodeRange, bitnum);
            } else {
                featMsg(hotERROR, "OS/2 Bad Unicode block value <%d>. All values must be in [0 ...127] inclusive.", bitnum);
            }
        } else {
            break;
        }
        i++;
    }

    OS_2SetUnicodeRanges(g, unicodeRange[0], unicodeRange[1],
                         unicodeRange[2], unicodeRange[3]);
}

void FeatCtx::setCodePageRange(short codePageList[kLenCodePageList]) {
    unsigned long codePageRange[2] { 0, 0 };
    short validPageBitIndex;
    int i = 0;

    while (i < kLenCodePageList) {
        short pageNumber = codePageList[i];

        if (pageNumber != kCodePageUnSet) {
            validPageBitIndex = validateCodePage(pageNumber);
            if (validPageBitIndex != kCodePageUnSet) {
                SET_BIT_ARR(codePageRange, validPageBitIndex);
            } else {
                featMsg(hotERROR, "OS/2 Code page value <%d> is not permitted according to the OpenType spec v1.4.", pageNumber);
            }
        } else {
            break;
        }

        i++;
    }

    OS_2SetCodePageRanges(g, codePageRange[0], codePageRange[1]);
}

// ---------------------------------- aalt -------------------------------------

void FeatCtx::aaltAddFeatureTag(Tag tag) {
    if (curr.feature != aalt_) {
        featMsg(hotERROR, "\"feature\" statement allowed only in 'aalt' feature");
    } else if ( tag != aalt_ ) {
        AALT::FeatureRecord t { tag, true };
        auto it = std::find(std::begin(aalt.features), std::end(aalt.features), t);
        if ( it == std::end(aalt.features) )
            aalt.features.push_back(t);
    }
}

void FeatCtx::reportUnusedaaltTags() {
    for (auto &f : aalt.features) {
        if ( !f.used ) {
            featMsg(hotWARNING,
                    "feature '%c%c%c%c', referenced in aalt feature, either is"
                    " not defined or had no rules which could be included in"
                    " the aalt feature.",
                   TAG_ARG(f.feature));
        }
    }
}

/* For the alternate glyphs in a  GSUBAlternate rule, sort them */
/* in the order of the GNode aaltIndex field, aka the order that */
/* the features were named in the aalt definition. Alts that are */
/* explicitly defined in the aalt feature have an index of -1. */
/* See aaltAddAlternates. */
void FeatCtx::aaltRuleSort(GNode **list) {
    unsigned int i;
    GNode *p = *list;
    short flags = (*list)->flags;

    /* Copy over pointers */
    std::vector<GNode *> sortTmp;

    for (; p != NULL; p = p->nextCl)
        sortTmp.push_back(p);

    struct {
        bool operator()(GNode *a, GNode *b) const { return a->aaltIndex < b->aaltIndex; }
    } cmpNode;
    std::stable_sort(sortTmp.begin(), sortTmp.end(), cmpNode);

    /* Move pointers around */
    for (i = 0; i < sortTmp.size() - 1; i++)
        sortTmp[i]->nextCl = sortTmp[i + 1];
    sortTmp[i]->nextCl = NULL;

    *list = sortTmp[0];

    (*list)->flags = flags;
}

/* Input GNodes will be copied. Ranges of 1-1, single 1-1, or 1 from n. (Only
   first glyph position in targ will be looked at)  */
/* The aaltIndex is set to non-zero here in order to facilitate later sorting */
/* of the target glyph alternates, by the order that the current feature file is */
/* named in the aalt feature. If the alt glyph is defined explicitly in the */
/* aalt feature, via a 'sub' directive, it gets an index of AALT_INDEX, aka, -1, */
/* so that it will sort before all the alts from included features. */

void FeatCtx::aaltAddAlternates(GNode *targ, GNode *repl) {
    bool range = targ->nextCl != nullptr;

    /* Duplicate repl if a single glyph: */
    if ( targ->nextCl != nullptr && repl->nextCl == nullptr )
        extendNodeToClass(repl, getGlyphClassCount(targ) - 1);

    for (; targ != nullptr; targ = targ->nextCl, repl = repl->nextCl) {
        GNode *replace;

        /* Start new accumulator for targ->gid, if it doesn't already exist */
        auto ru = aalt.rules.find(targ->gid);
        if ( ru == aalt.rules.end() ) {
            GNode *t = setNewNode(targ->gid);
            aalt.rules.insert(std::make_pair(targ->gid, AALT::RuleInfo{ t, nullptr }));
            ru = aalt.rules.find(targ->gid);
        }

        auto &ri = ru->second;
        /* Add alternates to alt set,                 */
        /* checking for uniqueness & preserving order */
        replace = repl;
        for (; replace != nullptr; replace = range ? nullptr : replace->nextCl) {
            GNode **lastClass;
            GNode *p;

            auto it = std::find(std::begin(aalt.features), std::end(aalt.features), curr.feature);
            short aaltTagIndex = it != std::end(aalt.features) ? it - aalt.features.begin() : -1;
            if (glyphInClass(replace->gid, &ri.repl, &lastClass)) {
                p = *lastClass;

                if ( aaltTagIndex < p->aaltIndex ) {
                    p->aaltIndex = aaltTagIndex;
                }
            } else {
                *lastClass = setNewNode(replace->gid);
                p = *lastClass;

                if (curr.feature == aalt_) {
                    p->aaltIndex = AALT_INDEX;
                } else {
                    p->aaltIndex = aaltTagIndex;
                }
            }
        }
    }
}

/* Create aalt at end of feature file processing */

void FeatCtx::aaltCreate() {
#if HOT_DEBUG
    long nInterSingleRules = 0;       /* Rules that interact with the alt rules; i.e. */
                                      /* its repl GID == an alt rule's targ GID       */
#endif
    Label labelSingle = LAB_UNDEF;    /* Init to suppress compiler warning */
    Label labelAlternate = LAB_UNDEF; /* Init to Suppress compiler warning */

    if (aalt.rules.size() == 0) {
        return;
    }

    // Create and sort a vector of RuleInfo pointers into the aalt.rules map
    // (the latter must not be altered during this function)
    std::vector<AALT::RuleInfo *> sortTmp;
    sortTmp.reserve(aalt.rules.size());
    for (auto &ru : aalt.rules)
        sortTmp.push_back(&ru.second);

    /* Sort single subs before alt subs, sort alt subs by targ GID */
    struct {
        bool operator()(AALT::RuleInfo *aa, AALT::RuleInfo *bb) const {
            GNode *a = aa->repl, *b = bb->repl;

            /* Sort single sub rules before alt sub rules */
            if (is_glyph(a) && is_mult_class(b)) {
                return true;
            } else if (is_mult_class(a) && is_glyph(b)) {
                return false;
            } else if (is_mult_class(a) && is_mult_class(b)) {
                // Sort alt sub rules by targ GID
                return aa->targ->gid < bb->targ->gid;
            } else {
                return aa->targ->gid < bb->targ->gid;
            }
        }
    } cmpNode;
    std::sort(sortTmp.begin(), sortTmp.end(), cmpNode);

    auto single_end = sortTmp.begin();
    while ( single_end != sortTmp.end() && is_glyph((*single_end)->repl) )
        single_end++;

    if ( single_end != sortTmp.begin() && single_end != sortTmp.end() ) {
        /* If the repl GID of a SingleSub rule is the same as an AltSub    */
        /* rule's targ GID, then the SingleSub rule sinks to the bottom of */
        /* the SingleSub rules, and becomes part of the AltSub rules       */
        /* section:                                                        */
        for (auto si = single_end-1; si >= sortTmp.begin(); si--) {
            auto search = aalt.rules.find((*si)->repl->gid);
            if ( search != aalt.rules.end() && is_mult_class(search->second.repl) ) {
                single_end--;
                if ( si != single_end ) {
                    AALT::RuleInfo *tmp = *single_end;
                    *single_end = *si;
                    *si = tmp;
                }
#if HOT_DEBUG
                nInterSingleRules++;
#endif
            }
        }
#if HOT_DEBUG
        if (nInterSingleRules) {
            DF(1, (stderr,
                   "# aalt: %ld SingleSub rule%s moved to AltSub "
                   "lookup to prevent lookup interaction\n",
                   nInterSingleRules,
                   nInterSingleRules == 1 ? "" : "s"));
        }
#endif
    }

    /* Add default lang sys if none specified */
    if (langSysMap.size() == 0) {
        addLangSys(DFLT_, dflt_, false);
        if (fFlags & langSysMode) {
            hotMsg(g, hotWARNING, "[internal] aalt language system unspecified");
        }
    }

    auto i = langSysMap.cbegin();
    assert(i != langSysMap.cend());
    GSUBFeatureBegin(g, i->first.script, i->first.lang, aalt_);

    /* --- Feed in single subs --- */
    if (sortTmp.begin() != single_end) {
        labelSingle = getNextAnonLabel();
        GSUBLookupBegin(g, GSUBSingle, 0, labelSingle, aalt.useExtension, 0);
        for (auto i = sortTmp.begin(); i != single_end; i++) {
            GSUBRuleAdd(g, (*i)->targ, (*i)->repl);
        }
        GSUBLookupEnd(g, aalt_);
    }

    /* --- Feed in alt subs --- */
    if (single_end != sortTmp.end()) {
        labelAlternate = getNextAnonLabel();
        GSUBLookupBegin(g, GSUBAlternate, 0, labelAlternate, aalt.useExtension, 0);
        for (auto i = single_end; i != sortTmp.end(); i++) {
            aaltRuleSort(&(*i)->repl);  // sort alts in order of feature def
                                        // in aalt feature
            GSUBRuleAdd(g, (*i)->targ, (*i)->repl);
        }
        GSUBLookupEnd(g, aalt_);
    }

    GSUBFeatureEnd(g);

    /* Also register these lookups under any other lang systems, if needed: */
    for (auto ls = langSysMap.cbegin(); ls != langSysMap.cend(); ls++) {
        if ( ls == langSysMap.cbegin() )
            continue;

        GSUBFeatureBegin(g, ls->first.script, ls->first.lang, aalt_);

        if (sortTmp.begin() != single_end) {
            GSUBLookupBegin(g, GSUBSingle, 0, (Label)(labelSingle | REF_LAB),
                            aalt.useExtension, 0);
            GSUBLookupEnd(g, aalt_);
        }
        if (single_end != sortTmp.end()) {
            GSUBLookupBegin(g, GSUBAlternate, 0,
                            (Label)(labelAlternate | REF_LAB),
                            aalt.useExtension, 0);
            GSUBLookupEnd(g, aalt_);
        }

        GSUBFeatureEnd(g);
    }
}

/* Return 1 if this rule is to be treated specially */

bool FeatCtx::aaltCheckRule(int type, GNode *targ, GNode *repl) {
    if (curr.feature == aalt_) {
        if (type == GSUBSingle || type == GSUBAlternate) {
            aaltAddAlternates(targ, repl);
            recycleNodes(targ);
            recycleNodes(repl);
        } else {
            featMsg(hotWARNING,
                    "Only single and alternate "
                    "substitutions are allowed within an 'aalt' feature");
        }
        return true;
    }
    return false;
}

void FeatCtx::storeRuleInfo(GNode *targ, GNode *repl) {
    if (curr.tbl == GPOS_ || repl == NULL) {
        return; /* No GPOS or except clauses */
    }
    AALT::FeatureRecord t { curr.feature, false };
    auto f = std::find(std::begin(aalt.features), std::end(aalt.features), t);
    if ( curr.feature == aalt_ || f != std::end(aalt.features) ) {
        /* Now check if lkpType is appropriate */

        switch (curr.lkpType) {
            case GSUBSingle:
            case GSUBAlternate:
                break;

            case GSUBChain:
                /* Go to first marked glyph (guaranteed to be present) */
                for (; !(targ->flags & FEAT_MARKED); targ = targ->nextSeq) {
                }
                if (targ->nextSeq != NULL && targ->nextSeq->flags & FEAT_MARKED) {
                    return; /* Ligature sub-substitution */
                }
                break;

            default:
                return;
        }

        if (curr.feature != aalt_) {
            assert(f != std::end(aalt.features));
            f->used = true;
        }
        aaltAddAlternates(targ, repl);
    }
}

/* Creates the cross product of pattern pat, and returns the array of *n
 * resulting sequences. pat is left intact; the client is responsible for
   recycling the result. */

GNode **FeatCtx::makeCrossProduct(GNode *pat, unsigned *n) {
    GNode *cl;

    prod.clear();

    /* Add each glyph class in pat to the cross product */
    for (cl = pat; cl != NULL; cl = cl->nextSeq) {
        long i;
        long nProd;

        if (cl == pat)
            prod.push_back(nullptr);

        nProd = prod.size();

        for (i = 0; i < nProd; i++) {
            GNode *p;
            int cntCl = 0;
            int lenSrc = 0;

            /* h->prod.array[i] is the source. Don't store its address as a */
            /* pointer in a local variable since dnaINDEX below may         */
            /* obsolete it!                                                 */
            for (p = cl; p != NULL; p = p->nextCl) {
                GNode **r;

                if (p == cl) {
                    for (r = &prod[i]; *r != NULL; r = &(*r)->nextSeq) {
                        lenSrc++;
                    }
                } else {
                    size_t inxTarg = nProd * cntCl + i;
                    if ( inxTarg >= prod.size() )
                        prod.resize(inxTarg+1);

                    r = copyPattern(&prod[inxTarg], prod[i], lenSrc);
                }
                *r = setNewNode(p->gid);
                cntCl++;
            }
        }
    }

    *n = prod.size();
    return prod.data();
}

// -------------------------------- C Hooks -----------------------------------

inline FeatCtx *hctofc(hotCtx g) {
    assert(g->ctx.feat != nullptr);
    return (FeatCtx *) g->ctx.feat;
}

void featNew(hotCtx g) {
    assert(g->ctx.feat == nullptr);
    FeatCtx *hfp = new FeatCtx(g);
    g->ctx.feat = (void *) hfp;
}

void featFree(hotCtx g) {
    delete hctofc(g);
    g->ctx.feat = nullptr;
}

// Prior to Antlr 4 the old code reset the context here but with an object
// that's an invitation for bugs so just reallocate.
void featReuse(hotCtx g) {
    featFree(g);
    featNew(g);
}

void featFill(hotCtx g) { hctofc(g)->fill(); }

GNode *featSetNewNode(hotCtx g, GID gid) {
    return hctofc(g)->setNewNode(gid);
}

void featRecycleNodes(hotCtx g, GNode *node) {
    hctofc(g)->recycleNodes(node);
}

GNode **featGlyphClassCopy(hotCtx g, GNode **dst, GNode *src) {
    return hctofc(g)->copyGlyphClass(dst, src);
}

void featGlyphDump(hotCtx g, GID gid, int ch, int print) {
    hctofc(g)->dumpGlyph(gid, ch, print);
}

void featGlyphClassDump(hotCtx g, GNode *gc, int ch, int print) {
    hctofc(g)->dumpGlyphClass(gc, ch, print);
}

void featPatternDump(hotCtx g, GNode *pat, int ch, int print) {
    hctofc(g)->dumpPattern(pat, ch, print);
}

GNode **featPatternCopy(hotCtx g, GNode **dst, GNode *src, int num) {
    return hctofc(g)->copyPattern(dst, src, num);
}

void featExtendNodeToClass(hotCtx g, GNode *node, int num) {
    hctofc(g)->extendNodeToClass(node, num);
}

int featGetGlyphClassCount(hotCtx g, GNode *gc) {
    return hctofc(g)->getGlyphClassCount(gc);
}

unsigned int featGetPatternLen(hotCtx g, GNode *pat) {
    return hctofc(g)->getPatternLen(pat);
}

void featGlyphClassSort(hotCtx g, GNode **list, int unique, int reportDups) {
    hctofc(g)->sortGlyphClass(list, unique, reportDups);
}

std::string featMsgPrefix(hotCtx g) {
    return hctofc(g)->msgPrefix();
}

GNode **featMakeCrossProduct(hotCtx g, GNode *pat, unsigned *n) {
    return hctofc(g)->makeCrossProduct(pat, n);
}

Label featGetNextAnonLabel(hotCtx g) {
    return hctofc(g)->getNextAnonLabel();
}

int featValidateGPOSChain(hotCtx g, GNode *targ, int lookupType) {
    return hctofc(g)->validateGPOSChain(targ, lookupType);
}
