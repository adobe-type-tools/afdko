/* Copyright 2021 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 * This software is licensed as OpenSource, under the Apache License, Version 2.0.
 * This license is available at: http://opensource.org/licenses/Apache-2.0.
 */

#include "FeatCtx.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <set>
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

const std::array<int, 4> MetricsInfo::kernorder = {2, 3, 1, 0};

const int FeatCtx::kMaxCodePageValue {32};
const int FeatCtx::kCodePageUnSet {-1};

const int MAX_NUM_LEN {3};        // For glyph ranges
const int AALT_INDEX {-1};        // used as feature index for sorting alt glyphs in rule for aalt feature

#define USE_LANGSYS_MSG                                                   \
    "use \"languagesystem\" statement(s) at beginning of file instead to" \
    " specify the language system(s) this feature should be registered under"

void GPat::ClassRec::makeUnique(hotCtx g, bool report) {
    sort();
    uint16_t glen = glyphs.size(), i = 0;
    int32_t lastgid = -1;
    while (i < glen) {
        int32_t gid = glyphs[i];
        if (gid == lastgid) {
            if (report) {
                g->ctx.feat->dumpGlyph(gid, -1, false);
                g->logger->log(sINFO, "Removing duplicate glyph <%s>",
                               g->getNote());
            }
            glyphs.erase(glyphs.begin() + i);
            glen--;
        } else {
            i++;
        }
    }
}

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

#if HOT_DEBUG

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
    g->logger->msg(msgType, buf.data());
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
    g->logger->msg(msgType, buf.data());
    v->current_msg_token = tmpt;
    current_visitor = tmpv;
}

const char *FeatCtx::tokstr() {
    assert(current_visitor != NULL);
    current_visitor->currentTokStr(tokenStringBuffer);
    return tokenStringBuffer.c_str();
}

void FeatCtx::setIDText() {
    if (curr.feature == TAG_STAND_ALONE)
        g->error_id_text = "standalone";
    else
        g->error_id_text = std::string("feature '") + TAG_STR(curr.feature) + std::string("'");
    if (IS_NAMED_LAB(curr.label)) {
        NamedLkp *curr = lab2NamedLkp(currNamedLkp);
        g->error_id_text += std::string(" lookup '") + curr->name + std::string("'");
    }
}

/* Emit report on any deprecated feature file syntax encountered */

void FeatCtx::reportOldSyntax() {
    if (syntax.numExcept > 0) {
        int one = syntax.numExcept == 1;

        featMsg(sINFO,
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

    // if (IS_ROS(g)) {
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
        featMsg(sERROR, "Glyph \"%s\" (alias \"%s\") not in font",
                realname, gname);
    } else {
        featMsg(sERROR, "Glyph \"%s\" not in font.", gname);
    }
    return GID_NOTDEF;
}

GID FeatCtx::cid2gid(const std::string &cidstr) {
    GID gid = 0; /* Suppress optimizer warning */
    if (!IS_ROS(g)) {
        featMsg(sERROR, "CID specified for a non-CID font");
    } else {
        int t = strtoll(cidstr.c_str() + 1, NULL, 10); /* Skip initial '\' */
        if (t < 0 || t > 65535)
            featMsg(sERROR, "CID not in range 0 .. 65535");
        else if ((gid = mapCID2GID(g, t)) == GID_UNDEF)
            featMsg(sERROR, "CID not found in font");
    }
    return gid; /* Suppress compiler warning */
}

void FeatCtx::sortGlyphClass(GPat::SP &gp, bool unique, bool reportDups, uint16_t idx) {
    assert(idx < gp->patternLen());
    if (unique)
        // makeClassUnique also sorts
        gp->makeClassUnique(g, idx, reportDups);
    else
        gp->sortClass(idx);
}

void FeatCtx::resetCurrentGC() {
    assert(curGCName.empty());
    curGC.reset();
}

void FeatCtx::defineCurrentGC(const std::string &gcname) {
    resetCurrentGC();
    auto search = namedGlyphClasses.find(gcname);
    if ( search != namedGlyphClasses.end() ) {
        featMsg(sWARNING, "Glyph class %s redefined", gcname.c_str());
        namedGlyphClasses.erase(search);
    }
    curGCName = gcname;
}

bool FeatCtx::openAsCurrentGC(const std::string &gcname) {
    resetCurrentGC();
    curGCName = gcname;
    auto search = namedGlyphClasses.find(gcname);
    if ( search != namedGlyphClasses.end() ) {
        curGC = search->second;
        return true;
    } else {
        return false;
    }
}

void FeatCtx::finishCurrentGC() {
    if ( !curGCName.empty() )
        namedGlyphClasses.insert_or_assign(curGCName, std::move(curGC));

    curGCName.clear();
}

void FeatCtx::finishCurrentGC(GPat::ClassRec &cr) {
    cr = curGC;
    finishCurrentGC();
}

/* Add glyph to end of current glyph class */

void FeatCtx::addGlyphToCurrentGC(GID gid) {
    curGC.addGlyph(gid);
}

void FeatCtx::addGlyphClassToCurrentGC(const GPat::ClassRec &src, bool init) {
    if (init)
        curGC = src;
    else
        curGC.concat(src);
}

void FeatCtx::addGlyphClassToCurrentGC(const std::string &subGCName) {
    // XXX consider checking curGCName against subGCName and reporting that
    // error specifically
    auto search = namedGlyphClasses.find(subGCName);

    if ( search == namedGlyphClasses.end() ) {
        featMsg(sERROR, "glyph class not defined");
        addGlyphToCurrentGC(GID_NOTDEF);
        return;
    }

    addGlyphClassToCurrentGC(search->second);
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
#define INVALID_RANGE featMsg(sFATAL, "Invalid glyph range [%s-%s]", \
                              firstName.c_str(), lastName.c_str())
    if (IS_ROS(g)) {
        if (first <= last) {
            for (GID i = first; i <= last; i++) {
                addGlyphToCurrentGC(i);
            }
        } else {
            featMsg(sFATAL, "Bad GID range: %u thru %u", first, last);
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

const GPat::ClassRec &FeatCtx::lookupGlyphClass(const std::string &gcname) {
    auto search = namedGlyphClasses.find(gcname);
    if ( search == namedGlyphClasses.end() ) {
        featMsg(sFATAL, "glyph class not defined");
        return GPat::ClassRec {};
    }
    return search->second;
}

/* Return 1 if targ and repl have same number of glyphs in class, else
   emit error message and return 0 */

bool FeatCtx::compareGlyphClassCount(int targc, int replc, bool isSubrule) {
    if (targc == replc) {
        return true;
    }
    featMsg(sERROR,
            "Target glyph class in %srule doesn't have the same"
            " number of elements as the replacement class; the target has %d,"
            " the replacement, %d",
            isSubrule ? "sub-" : "", targc, replc);
    return false;
}

void FeatCtx::dumpGlyph(GID gid, int ch, bool print) {
    char msg[512];
    int len;
    if (IS_ROS(g)) {
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
        g->note.append(msg);
    }
}

#define DUMP_CH(ch, print)             \
    do {                               \
        if (print)                     \
            fprintf(stderr, "%c", ch); \
        else                           \
            g->note.push_back(ch);     \
    } while (0)

/* If !print, add to g->note */

void FeatCtx::dumpGlyphClass(const GPat::ClassRec &cr, int ch, bool print) {
    DUMP_CH('[', print);
    bool first = true;
    for (GID gid : cr.glyphs) {
        if (!first)
            DUMP_CH(' ', print);
        first = false;
        dumpGlyph(gid, (char)(-1), print);
    }
    DUMP_CH(']', print);
    if (ch >= 0)
        DUMP_CH(ch, print);
}

/* If !print, add to g->note */

void FeatCtx::dumpPattern(const GPat &pat, int ch, bool print) {
    DUMP_CH('{', print);
    bool first = true;
    for (auto &cr : pat.classes) {
        if (!first)
            DUMP_CH(' ', print);
        first = false;
        if (cr.is_glyph()) {
            dumpGlyph(cr.glyphs[0], -1, print);
        } else {
            dumpGlyphClass(cr, -1, print);
        }
        if (cr.marked) {
            DUMP_CH('\'', print);
        }
    }
    DUMP_CH('}', print);
    if (ch >= 0)
        DUMP_CH(ch, print);
}

// ----------------------------------- Tags ------------------------------------

std::string FeatCtx::unescString(const std::string &s) {
    if (s.length() < 2 || s[0] != '"' || s.back() != '"') {
        featMsg(sERROR, "Malformed quoted string '%s'", s.c_str());
        return "";
    }
    return s.substr(1, s.length() - 2);
/*
    std::string r;
    if (s.length() < 2 || s[0] != '"' || s.back() != '"') {
        featMsg(sERROR, "Malformed quoted string '%s'", s.c_str());
        return "";
    }

    r.reserve(s.length() - 2);
    bool escaping = false;
    auto e = s.end() - 1;
    for (auto i = s.begin() + 1; i != e; i++) {
        if (*i != '\\' || escaping) {
            r.push_back(*i);
            escaping = false;
        } else {
            escaping = true;
        }
    }
    return r;
*/
}

Tag FeatCtx::str2tag(const std::string &tagName) {
    if (tagName.length() == 0) {
        featMsg(sERROR, "Empty tag");
        return 0;
    }

    if ( tagName.length() > 4 )
        featMsg(sERROR, "Tag %s exceeds 4 characters", tagName.c_str());
    if ( tagName == "dflt" ) {
        return dflt_;
    } else {
        char buf[4];
        size_t l = tagName.length();
        for (size_t i = 0; i < 4; i++) {
            char c {' '};
            if (i < l) {
                c = tagName[i];
                if (c < 0x20 || c > 0x7E) {
                    featMsg(sERROR, "Invalid character value %hhx in tag string", c);
                    c = 0;
                }
            }
            buf[i] = c;
        }
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
            featMsg(sWARNING, "'dflt' is not a valid tag for a script statement; using 'DFLT'.");
        }
        t = &curr.script;
    } else if (type == languageTag) {
        ta = &language;
        if (tag == DFLT_) {
            tag = dflt_;
            featMsg(sWARNING, "'DFLT' is not a valid tag for a language statement; using 'dflt'.");
        }
        t = &curr.language;
    } else if (type == tableTag) {
        ta = &table;
    } else if (type != featureTag) {
        featMsg(sFATAL, "[internal] unrecognized tag type");
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
        featMsg(sERROR,
                "\"script\" and \"language\" statements "
                "are not allowed in 'aalt' or 'size' features; " USE_LANGSYS_MSG);
        return -1;
    } else if ((tag != TAG_STAND_ALONE) && (curr.feature == TAG_STAND_ALONE)) {
        featMsg(sERROR,
                "\"script\" and \"language\" statements "
                "are not allowed within standalone lookup blocks; ");
    }
    /*
    if (h->fFlags & FF_LANGSYS_MODE) {
        featMsg(sWARNING,
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
                featMsg(sERROR, "script behavior already specified");

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
            featMsg(sWARNING, "'DFLT' is not a valid tag for a language statement; using 'dflt'.");
        }

        /* Once we have seen a script or a language statement other */
        /* than 'dflt' any further rules in the feature should not  */
        /* be added to the default list.                            */
        if ((fFlags & langSysMode) && (tag != dflt_))
            fFlags &= ~langSysMode;

        if (tag == curr.language)
            return 0; /* Statement has no effect */

        if (tag == dflt_)
            featMsg(sERROR, "dflt must precede language-specific behavior");

        if ( !tagAssign(tag, languageTag, true) )
            featMsg(sERROR, "language-specific behavior already specified");
    }
    return 1;
}

/* Include dflt rules in lang-specific behavior if includeDFLT != 0 */

void FeatCtx::includeDFLT(bool includeDFLT, int langChange, bool seenOD) {
    if (seenOD && !seenOldDFLT) {
        seenOldDFLT = true;
        featMsg(sWARNING, "Use of includeDFLT and excludeDFLT tags has been deprecated. It will work, but please use 'include_dflt' and 'exclude_dflt' tags instead.");
    }
    /* Set value */
    if (langChange) {
        include_dflt = includeDFLT;
    } else if (includeDFLT != include_dflt) {
        featMsg(sERROR,
            "can't change whether a language should include dflt rules once "
            "this has already been indicated");
    }

    if (includeDFLT) {
        /* Include dflt rules for current script and explicit language    */
        /* statement. Languages which don't exclude_dflt get the feature- */
        /* level defaults at the end of the feature.                      */
        for (auto &i : DFLTLkps)
            callLkp(i, true);
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
        featMsg(sERROR,
                "languagesystem must be specified before all"
                " feature blocks");
        return;
    }
    if (!(gFlags & seenLangSys)) {
        gFlags |= seenLangSys;
    } else if (script == DFLT_) {
        if (gFlags & seenNonDFLTScriptLang)
            featMsg(sERROR, "All references to the script tag DFLT must precede all other script references.");
    } else {
        gFlags |= seenNonDFLTScriptLang;
    }

    if (script == dflt_) {
        featMsg(sWARNING, "'dflt' is not a valid script tag for a languagesystem statement; using 'DFLT'.");
        script = DFLT_;
    }

    if ( langctx != NULL )
        current_visitor->TOK(langctx);

    if (language == DFLT_) {
        featMsg(sWARNING, "'DFLT' is not a valid language tag for a languagesystem statement; using 'dflt'.");
        language = dflt_;
    }

    /* First check if already exists */
    if ( langSysMap.find({script, language}) != langSysMap.end() ) {
        featMsg(sWARNING, "Duplicate specification of language system");
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
                    g->ctx.GSUBp->FeatureBegin(ls.first.script, ls.first.lang, curr.feature);
                    seenGSUB = 1;
                }
                g->ctx.GSUBp->LookupBegin(lkp.lkpType, lkp.lkpFlag,
                                          (Label)(lkp.label | REF_LAB),
                                          lkp.useExtension, lkp.markSetIndex);
                g->ctx.GSUBp->LookupEnd();
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
            g->ctx.GSUBp->FeatureEnd();
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
            featMsg(sWARNING, "feature already defined: %s", tokstr());
        }
    }

    fFlags = 0;
    gFlags |= seenFeature;

    lookup.clear();
    script.clear();
    if (langSysMap.size() == 0) {
        featMsg(sWARNING,
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
            featMsg(sFATAL, "[internal] label not found\n");
        }
        curr->useExtension = true;
    } else {
        /* feature block scope */
        if (curr.feature == aalt_) {
            aalt.useExtension = true;
        } else {
            featMsg(sERROR,
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
            g->ctx.GSUBp->LookupEnd();
        g->error_id_text.clear();
        g->ctx.GSUBp->FeatureEnd();
    } else if ( st.tbl == GPOS_ ) {
        if ( st.lkpType != 0 )
            g->ctx.GPOSp->LookupEnd();
        g->error_id_text.clear();
        g->ctx.GPOSp->FeatureEnd();
    }
}

void FeatCtx::addFeatureParam(const std::vector<uint16_t> &params) {
    switch (curr.feature) {
        case size_:
            prepRule(GPOS_, GPOSFeatureParam, nullptr, nullptr);

            g->ctx.GPOSp->AddParameters(params);

            wrapUpRule();

            break;

        default:
            featMsg(sERROR,
                    "A feature parameter is supported only for the 'size' feature.");
    }
}

void FeatCtx::subtableBreak() {
    bool retval = false;

    if (curr.feature == aalt_ || curr.feature == size_) {
        featMsg(sERROR, "\"subtable\" use not allowed in 'aalt' or 'size' feature");
        return;
    }

    if (curr.tbl == GSUB_) {
        retval = g->ctx.GSUBp->SubtableBreak();
    } else if (curr.tbl == GPOS_) {
        retval = g->ctx.GPOSp->SubtableBreak();
    } else {
        featMsg(sWARNING, "Statement not expected here");
        return;
    }

    if (retval)
        featMsg(sWARNING, "subtable break is supported only in pair kerning lookups");
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
            featMsg(sERROR, "\"lookup\" use not allowed in 'aalt' feature");
            return;
        }
        if (curr.feature == size_) {
            featMsg(sERROR,
                    "\"lookup\" use not allowed anymore in 'size'"
                    " feature; " USE_LANGSYS_MSG);
            return;
        }
    }

    DF(2, (stderr, "# at start of named lookup %s\n", name.c_str()));
    if (name2NamedLkp(name) != NULL)
        featMsg(sFATAL, "lookup name \"%s\" already defined", name.c_str());

    currNamedLkp = getNextNamedLkpLabel(name, isTopLevel);
    /* State will be recorded at end of block section, below */
}

void FeatCtx::endLookup() {
    if ( curr.feature == aalt_ || curr.feature == size_)
        return;

    NamedLkp *c = lab2NamedLkp(currNamedLkp);

    if (c == nullptr) {
        featMsg(sFATAL, "[internal] label not found\n");
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
            featMsg(sERROR, "must specify non-zero MarkAttachmentType value");
        } else if (val & attr) {
            featMsg(sERROR,
                    "MarkAttachmentType already specified in this statement");
        } else {
            val |= (markAttachClassIndex & 0xFF) << 8;
        }
    } else if (attr == otlUseMarkFilteringSet) {
        if (val & attr) {
            featMsg(sERROR,
                    "UseMarkSetType already specified in this statement");
        }
        curr.markSetIndex = markAttachClassIndex;
        val |= attr;
    } else {
        if (val & attr) {
            featMsg(sWARNING,
                    "\"%s\" repeated in this statement; ignoring", tokstr());
        } else {
            val |= attr;
        }
    }
    return val;
}

void FeatCtx::setLkpFlag(uint16_t flag) {
    if (curr.feature == aalt_ || curr.feature == size_)
        featMsg(sERROR, "\"lookupflag\" use not allowed in 'aalt' or 'size' feature");
    curr.lkpFlag = flag;
    /* if UseMarkSet, then the markSetIndex is set in setLkpFlagAttribute() */
}

void FeatCtx::callLkp(State &st, bool fromDFLT) {
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
            g->logger->msg(sFATAL, "undefined label");
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
    prepRule(st.tbl, st.lkpType, nullptr, nullptr, fromDFLT);
    /* No actual rules will be fed into GPOS/GSUB */
    wrapUpRule();
    currNamedLkp = LAB_UNDEF; /* As in:  } <name>;        */
    endOfNamedLkpOrRef = true;
}

void FeatCtx::useLkp(const std::string &name) {
    NamedLkp *lkp = name2NamedLkp(name);

    if (curr.feature == aalt_) {
        featMsg(sERROR, "\"lookup\" use not allowed in 'aalt' feature");
        return;
    } else {
        AALT::FeatureRecord t { curr.feature, false };
        auto it = std::find(std::begin(aalt.features), std::end(aalt.features), t);
        if ( it != std::end(aalt.features) )
            it->used = true;
    }

    if (curr.feature == size_) {
        featMsg(sERROR,
                "\"lookup\" use not allowed anymore in 'size'"
                " feature; " USE_LANGSYS_MSG);
        return;
    }

    if (lkp == NULL) {
        featMsg(sERROR, "lookup name \"%s\" not defined", name.c_str());
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
        featMsg(sFATAL,
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
        featMsg(sFATAL, "lookup name \"%s\" not defined", name.c_str());
    }
    return curr->state.label;
}

Label FeatCtx::getNextAnonLabel() {
    if (anonLabelCnt > FEAT_ANON_LKP_END) {
        featMsg(sFATAL,
                "[internal] maximum number of lookups reached: %d",
                FEAT_ANON_LKP_END - FEAT_ANON_LKP_BEG + 1);
    }
    return anonLabelCnt++;
}

// ---------------------------------- Tables -----------------------------------

void FeatCtx::startTable(Tag tag) {
    if ( !tagAssign(tag, tableTag, true) )
        featMsg(sERROR, "table already specified");
}

void FeatCtx::setGDEFGlyphClassDef(GPat::ClassRec &simple, GPat::ClassRec &ligature,
                                   GPat::ClassRec &mark, GPat::ClassRec &component) {
    gFlags |= seenGDEFGC;
    g->ctx.GDEFp->setGlyphClass(simple, ligature, mark, component);
}

void FeatCtx::createDefaultGDEFClasses() {
    if ( !(gFlags & seenGDEFGC) ) {
        GPat::ClassRec classes[4];
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
            finishCurrentGC(classes[i]);
            classes[i].makeUnique(g);
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
            featMsg(sWARNING, "head FontRevision entry <%s> should have 3 fractional decimal places. Stored as <%.3f>", rev.c_str(), version);
        }
    } else {
        featMsg(sWARNING, "head FontRevision entry <%ld> should have 3 fractional decimal places; it now has none.", major);
    }

    /* limit of 32767 as anything higher sets the sign bit to negative */
    major = MIN(major, 32767);

    /* Convert to Fixed */
    g->font.version.otf = (Fixed)((major + minor) * 65536.0);
}

/* Add name string to name table. */

void FeatCtx::addNameString(int32_t platformId, int32_t platspecId,
                            int32_t languageId, int32_t nameId,
                            const std::string &str) {
    bool nameError = false;
    if ((nameId == 2) || (nameId == 6) || ((nameId >= 26) && (nameId <= 255)))
        nameError = true;
    else if ((nameId > 0) && ((nameId < 7) && (!(g->convertFlags & HOT_OVERRIDE_MENUNAMES)))) {
        nameError = true;
    }
    if (nameError) {
        featMsg(sWARNING,
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
                   (uint16_t)platformId, (uint16_t)platspecId,
                   (uint16_t)languageId, (uint16_t)nameId,
                   str.c_str())) {
        featMsg(sERROR, "Bad string");
    }
}

/* Add 'size' feature submenuname name string to name table. */

void FeatCtx::addSizeNameString(int32_t platformId, int32_t platspecId,
                                int32_t languageId, const std::string &str) {
    uint16_t nameID;

    /* We only need to reserve a name ID *once*; after the first time, */
    /* all subsequent sizemenunames will share the same nameID.        */
    if (featNameID == 0) {
        nameID = g->ctx.name->reserveUserID();
        g->ctx.GPOSp->SetSizeMenuNameID(nameID);
        featNameID = nameID;
    } else {
        nameID = featNameID;
    }

    addNameString(platformId, platspecId, languageId, nameID, str);
}

/* Add 'name for feature string to name table. */

void FeatCtx::addFeatureNameString(int32_t platformId, int32_t platspecId,
                                   int32_t languageId, const std::string &str) {
    uint16_t nameID;

    /* We only need to reserve a name ID *once*; after the first time, */
    /* all subsequent sizemenunames will share the same nameID.        */
    if (featNameID == 0) {
        nameID = g->ctx.name->reserveUserID();
        g->ctx.GSUBp->SetFeatureNameID(curr.feature, nameID);
        featNameID = nameID;
    } else {
        nameID = featNameID;
    }

    addNameString(platformId, platspecId, languageId, nameID, str);
}

void FeatCtx::addFeatureNameParam() {
    prepRule(GSUB_, GSUBFeatureNameParam, nullptr, nullptr);

    g->ctx.GSUBp->AddFeatureNameParam(featNameID);

    wrapUpRule();
}

void FeatCtx::addUserNameString(int32_t platformId, int32_t platspecId,
                                int32_t languageId, const std::string &str) {
    uint16_t nameID;

    /* We only need to reserve a name ID *once*. */
    if (featNameID == 0) {
        nameID = g->ctx.name->reserveUserID();
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
        featMsg(sWARNING, "Vendor name too short. Padded automatically to 4 characters.");
    }
    if (str.size() > 4) {
        featMsg(sERROR, "Vendor name too long. Max is 4 characters.");
    }

    setVendId_str(g, str.c_str());
}

// --------------------------------- Anchors -----------------------------------

void FeatCtx::addAnchorDef(const std::string &name, AnchorMarkInfo &&a) {
    auto ret = anchorDefs.insert({name, std::move(a)});

    if ( !ret.second )
        featMsg(sFATAL, "Named anchor definition '%s' is a a duplicate of an earlier named anchor definition.", name.c_str());
}

void FeatCtx::addAnchorByName(const std::string &name, int componentIndex) {
    auto search = anchorDefs.find(name);
    if ( search == anchorDefs.end() ) {
        featMsg(sERROR, "Named anchor reference '%s' is not in list of named anchors.", name.c_str());
        return;
    }

    addAnchorByValue(std::make_shared<AnchorMarkInfo>(search->second), componentIndex);
}

void FeatCtx::addAnchorByValue(std::shared_ptr<AnchorMarkInfo> a, int componentIndex) {
    assert(a != nullptr);
    a->setComponentIndex(componentIndex);
    anchorMarkInfo.emplace_back(a);
}

/* Add new mark class definition */
void FeatCtx::addMark(const std::string &name, GPat::ClassRec &cr) {
    cr.markClassName = name;
    assert(anchorMarkInfo.size() > 0);
    for (auto &gr : cr.glyphs)
        gr.markClassAnchorInfo = anchorMarkInfo.back();

    bool found = openAsCurrentGC(name);
    addGlyphClassToCurrentGC(cr, !found);

    if ( curGC.used_mark_class )
        featMsg(sERROR, "You cannot add glyphs to a mark class after the mark class has been used in a position statement. %s.", name.c_str());

    finishCurrentGC();  // Save new entry if needed

    /* add mark glyphs to default base class */
    openAsCurrentGC(kDEFAULT_MARKCLASS_NAME);
    addGlyphClassToCurrentGC(cr);
    finishCurrentGC();

    gFlags |= seenMarkClassFlag;
}

// --------------------------------- Metrics -----------------------------------

void FeatCtx::addValueDef(const std::string &name, const MetricsInfo &&mi) {
    auto ret = valueDefs.insert(std::make_pair(name, std::move(mi)));

    if ( !ret.second )
        featMsg(sERROR, "Named value record definition '%s' is a duplicate of an earlier named value record definition.", name.c_str());
}

void FeatCtx::getValueDef(const std::string &name, MetricsInfo &mi) {
    auto search = valueDefs.find(name);
    if ( search == valueDefs.end() )
        featMsg(sERROR, "Named value reference '%s' is not in list of named value records.", name.c_str());
    else
        mi = search->second;
}

// ------------------------------ Substitutions --------------------------------

void FeatCtx::prepRule(Tag newTbl, int newlkpType, const GPat::SP &targ,
                       const GPat::SP &repl, bool fromDFLT) {
    bool accumDFLTLkps = !fromDFLT;

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
            /* be true.                                                     */
            accumDFLTLkps = false;
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
                accumDFLTLkps = false; /* Store lkp state only once per lookup */
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
            featMsg(sFATAL,
                    "Lookup type different from previous "
                    "rules in this lookup block");
        } else if (curr.lkpFlag != prev.lkpFlag) {
            featMsg(sFATAL,
                    "Lookup flags different from previous "
                    "rules in this block");
        } else if (curr.markSetIndex != prev.markSetIndex) {
            featMsg(sFATAL,
                    "Lookup flag UseMarkSetIndex different from previous "
                    "rules in this block");
        }
    }

    /* Reset DFLTLkp accumulator at feature change or start of DFLT run */
    if ((curr.feature != prev.feature) || (curr.script != prev.script))
        DFLTLkps.clear();

    /* stop accumulating script specific defaults once a language other than 'dflt' is seen */
    if (accumDFLTLkps && curr.language != dflt_)
        accumDFLTLkps = false;

    if (accumDFLTLkps)
        /* Save for possible inclusion later in lang-specific stuff */
        DFLTLkps.push_back(curr);

    /* Store, if applicable, for aalt creation */
    if ((!IS_REF_LAB(curr.label)) && (repl != nullptr))
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
                featMsg(sFATAL, "[internal] label not found\n");
            }
            useExtension = lkp->useExtension;
        }

        /* Initiate calls to GSUB/GPOS */
        if (curr.tbl == GSUB_) {
            g->ctx.GSUBp->FeatureBegin(curr.script, curr.language,
                                       curr.feature);
            g->ctx.GSUBp->LookupBegin(curr.lkpType, curr.lkpFlag, curr.label,
                                      useExtension, curr.markSetIndex);
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
            featMsg(sFATAL,
                    "Lookup flags different from previous "
                    "rules in this block");
        } else if (curr.markSetIndex != prev.markSetIndex) {
            featMsg(sFATAL,
                    "Lookup flag UseMarkSetIndex different from previous "
                    "rules in this block");
        }
    }
}

void FeatCtx::addGSUB(int lkpType, GPat::SP targ, GPat::SP repl) {
    if (aaltCheckRule(lkpType, targ, repl))
        return;

    prepRule(GSUB_, lkpType, targ, repl);

    g->ctx.GSUBp->RuleAdd(std::move(targ), std::move(repl));

    wrapUpRule();
}

bool FeatCtx::validateGSUBSingle(const GPat::ClassRec &targcr,
                                 const GPat::SP &repl, bool isSubrule) {
    if (targcr.is_glyph()) {
        if (!repl->is_glyph()) {
            featMsg(sERROR, "Replacement in %srule must be a single glyph",
                    isSubrule ? "sub-" : "");
            return false;
        }
    } else if (repl->is_mult_class() &&
               !compareGlyphClassCount(targcr.classSize(), repl->classSize(),
                                       isSubrule)) {
        return false;
    }

    return true;
}

/* Validate targ and repl for GSUBSingle. targ and repl come in with nextSeq
   NULL and repl not marked. If valid return 1, else emit error message(s) and
   return 0 */

bool FeatCtx::validateGSUBSingle(const GPat::SP &targ, const GPat::SP &repl) {
    if (targ->has_marked) {
        featMsg(sERROR, "Target must not be marked in this rule");
        return false;
    }

    return targ->classes.size() > 0 &&
           validateGSUBSingle(targ->classes[0], repl, false);
}

/* Return 1 if node is an unmarked pattern of 2 or more glyphs; glyph classes
   may be present */
static bool isUnmarkedSeq(const GPat::SP &gp) {
    return !(gp == nullptr || gp->has_marked || gp->patternLen() < 2);
}

/* Return 1 if node is an unmarked pattern of 2 or more glyphs, and no glyph
   classes are present */
static bool isUnmarkedGlyphSeq(const GPat::SP &gp) {
    if (!isUnmarkedSeq(gp))
        return false;

    for (auto &cr : gp->classes) {
        if (!cr.is_glyph())
            return false; /* A glyph class is present */
    }
    return true;
}

/* Validate targ and repl for GSUBMultiple. targ comes in with nextSeq NULL and
   repl NULL or with nextSeq non-NULL (and unmarked). If valid return 1, else
   emit error message(s) and return 0 */

bool FeatCtx::validateGSUBMultiple(const GPat::ClassRec &targ,
                                   const GPat::SP &repl, bool isSubrule) {
    if (!((isSubrule || targ.is_glyph()) && isUnmarkedGlyphSeq(repl)) &&
        (repl != NULL || targ.has_lookups())) {
        featMsg(sERROR, "Invalid multiple substitution rule");
        return false;
    }
    return true;
}

bool FeatCtx::validateGSUBMultiple(const GPat::SP &targ, const GPat::SP &repl) {
    if (targ->has_marked) {
        featMsg(sERROR, "Target must not be marked in this rule");
        return false;
    }
    return targ->patternLen() > 0 &&
           validateGSUBMultiple(targ->classes[0], repl, false);
}

/* Validate targ and repl for GSUBAlternate. repl is not marked. If valid
   return 1, else emit error message(s) and return 0 */

bool FeatCtx::validateGSUBAlternate(const GPat::SP &targ,
                                    const GPat::SP &repl, bool isSubrule) {
    if (targ->has_marked) {
        featMsg(sERROR, "Target must not be marked in this rule");
        return false;
    }

    if (!targ->is_unmarked_glyph()) {
        featMsg(sERROR,
                "Target of alternate substitution %srule must be a"
                " single unmarked glyph",
                isSubrule ? "sub-" : "");
        return false;
    }
    if (!repl->is_class()) {
        featMsg(sERROR,
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

bool FeatCtx::validateGSUBLigature(const GPat::SP &targ,
                                   const GPat::SP &repl, bool isSubrule) {
    assert(isSubrule || targ != nullptr);

    if (!isSubrule && targ->has_marked) {
        featMsg(sERROR, "Target must not be marked in this rule");
        return false;
    }

    if (!repl->is_glyph()) {
        featMsg(sERROR, "Invalid ligature %srule replacement", isSubrule ? "sub-" : "");
        return false;
    }
    return true;
}

/* Analyze GSUBChain targ and repl. Return 1 if valid, else 0 */

bool FeatCtx::validateGSUBReverseChain(GPat::SP &targ, GPat::SP &repl) {
    int nMarked = 0;
    bool after_marked = false;

    if (repl == NULL) {
        /* Except clause */
        if (targ->has_marked) {
            for (auto &cr : targ->classes) {
                if (nMarked == 0 && !cr.marked) {
                    cr.backtrack = true;
                } else if (cr.marked) {
                    if (after_marked) {
                        featMsg(sERROR,
                                "ignore clause may have at most one run "
                                "of marked glyphs");
                        return false;
                    }
                    nMarked++;
                    cr.input = true;
                } else if (nMarked > 0) {
                    after_marked = true;
                    cr.lookahead = true;
                }
            }
        } else {
            bool first = true;
            for (auto &cr : targ->classes) {
                if (first) {
                    cr.input = true;
                    first = false;
                } else {
                    cr.lookahead = true;
                }
            }
        }
        return true;
    }

    nMarked = 0;
    after_marked = false;
    /* At most one run of marked positions supported, for now */
    for (auto &cr : targ->classes) {
        if (cr.marked) {
            if (after_marked) {
                featMsg(sERROR, "Reverse contextual GSUB rule must have one and only one "
                                  "glyph or class marked for replacement");
                return false;
            }
            nMarked++;
        } else if (nMarked > 0) {
            after_marked = true;
        }
    }

    /* If nothing is marked, mark everything [xxx?] */
    if (nMarked == 0) {
        for (auto &cr : targ->classes) {
            cr.marked = true;
            nMarked++;
        }
    }

    if (repl->classes.size() != 1) {
        featMsg(sERROR, "Reverse contextual GSUB replacement sequence may have only one glyph or class");
        return false;
    }

    if (nMarked != 1) {
        featMsg(sERROR, "Reverse contextual GSUB rule may must have one and only one glyph or class marked for replacement");
        return false;
    }

    /* Divide into backtrack, input, and lookahead (xxx ff interface?). */
    /* For now, input is marked glyphs                                  */
    nMarked = 0;
    GPat::ClassRec *crp = nullptr;
    for (auto &cr : targ->classes) {
        if (nMarked == 0 && !cr.marked) {
            cr.backtrack = true;
        } else if (cr.marked) {
            if (nMarked == 0)
                crp = &cr;
            nMarked++;
            cr.input = true;
        } else if (nMarked > 0) {
            cr.lookahead = true;
        }
    }

    if (!compareGlyphClassCount(crp->classSize(), repl->classSize(), false)) {
        return false;
    }

    return true;
}


/* Analyze GSUBChain targ and repl. Return 1 if valid, else 0 */

bool FeatCtx::validateGSUBChain(GPat::SP &targ, GPat::SP &repl) {
    int nMarked = 0;

    bool after_marked {false};
    if (targ->ignore_clause) {
        if (targ->has_marked) {
            for (auto &cr : targ->classes) {
                if (nMarked == 0 && !cr.marked) {
                    cr.backtrack = true;
                } else if (cr.marked) {
                    if (after_marked) {
                        featMsg(sERROR,
                                "ignore clause may have at most one run "
                                "of marked glyphs");
                        return false;
                    }
                    nMarked++;
                    cr.input = true;
                } else if (nMarked > 0) {
                    after_marked = true;
                    cr.lookahead = true;
                }
            }
        } else {
            bool first = true;
            for (auto &cr : targ->classes) {
                if (first) {
                    cr.input = true;
                    first = false;
                } else {
                    cr.lookahead = true;
                }
            }
        }
        return true;
    } else if ((repl == NULL) && (!targ->lookup_node)) {
        featMsg(sERROR, "contextual substitution clause must have a replacement rule or direct lookup reference.");
        return false;
    }

    if (targ->lookup_node) {
        if (repl != NULL) {
            featMsg(sERROR, "contextual substitution clause cannot both have a replacement rule and a direct lookup reference.");
            return false;
        }
        if (!(targ->has_marked)) {
            featMsg(sERROR, "The  direct lookup reference in a contextual substitution clause must be marked as part of a contextual input sequence.");
            return false;
        }
    }

    nMarked = 0;
    after_marked = false;
    /* At most one run of marked positions supported, for now */
    GPat::ClassRec *crp = nullptr;
    for (auto &cr : targ->classes) {
        if (cr.marked) {
            if (after_marked) {
                featMsg(sERROR, "Unsupported contextual GSUB target sequence");
                return false;
            }
            if (nMarked == 0)
                crp = &cr;
            nMarked++;
        } else {
            if (cr.lookupLabels.size() > 0) {
                featMsg(sERROR, "The direct lookup reference in a contextual substitution clause must be marked as part of a contextual input sequence.");
                return false;
            }
            if (nMarked > 0)
                after_marked = true;
        }
    }

    /* If nothing is marked, mark everything [xxx?] */
    if (nMarked == 0) {
        for (auto &cr : targ->classes) {
            if (nMarked == 0)
                crp = &cr;
            cr.marked = true;
            nMarked++;
        }
    }
    /* m now points to beginning of marked run */

    if (repl) {
        if (nMarked == 1) {
            if (crp->is_glyph() && repl->is_class()) {
                featMsg(sERROR, "Contextual alternate rule not yet supported");
                return false;
            }

            if (repl->patternLen() > 1) {
                if (!validateGSUBMultiple(*crp, repl, true)) {
                    return false;
                }
            } else if (!validateGSUBSingle(*crp, repl, true)) {
                return false;
            }
        } else if (nMarked > 1) {
            if (repl->patternLen() > 1) {
                featMsg(sERROR, "Unsupported contextual GSUB replacement sequence");
                return false;
            }

            /* Ligature run may contain classes, but only with a single repl */
            if (!validateGSUBLigature(nullptr, repl, true)) {
                return false;
            }
        }
    }

    /* Divide into backtrack, input, and lookahead (xxx ff interface?). */
    /* For now, input is marked glyphs                                  */
    nMarked = 0;
    for (auto &cr : targ->classes) {
        if (nMarked == 0 && !cr.marked) {
            cr.backtrack = true;
        } else if (cr.marked) {
            nMarked++;
            cr.input = true;
        } else if (nMarked > 0) {
            cr.lookahead = true;
        }
    }

    return true;
}

void FeatCtx::addSub(GPat::SP targ, GPat::SP repl, int lkpType) {
    for (auto &cr : targ->classes) {
        if (cr.marked) {
            targ->has_marked = true;
            if ((lkpType != GSUBReverse) && (lkpType != GSUBChain)) {
                lkpType = GSUBChain;
            }
            break;
        }
    }

    if (lkpType == GSUBChain || (targ->ignore_clause)) {
        /* Chain sub exceptions (further analyzed below).                */
        /* "sub f i by fi;" will be here if there was an "except" clause */

        if (!g->hadError) {
            if (validateGSUBChain(targ, repl)) {
                lkpType = GSUBChain;
                addGSUB(GSUBChain, std::move(targ), std::move(repl));
            }
        }
    } else if (lkpType == GSUBReverse) {
        if (validateGSUBReverseChain(targ, repl)) {
            addGSUB(GSUBReverse, std::move(targ), std::move(repl));
        }
    } else if (lkpType == GSUBAlternate) {
        if (validateGSUBAlternate(targ, repl, false)) {
            addGSUB(GSUBAlternate, std::move(targ), std::move(repl));
        }
    } else if (targ->patternLen() == 1 && (repl == NULL || repl->patternLen() > 1)) {
        if (validateGSUBMultiple(targ, repl)) {
            addGSUB(GSUBMultiple, std::move(targ), std::move(repl));
        }
    } else if (targ->patternLen() == 1 && repl->patternLen() == 1) {
        if (validateGSUBSingle(targ, repl)) {
            addGSUB(GSUBSingle, std::move(targ), std::move(repl));
        }
    } else {
        if (validateGSUBLigature(targ, repl, false)) {
            /* add glyphs to lig and component classes, in case we need to
            make a default GDEF table. Note that we may make a lot of
            duplicated. These get weeded out later. The components are
            linked by the next->nextSeq fields. For each component*/
            openAsCurrentGC(kDEFAULT_COMPONENTCLASS_NAME); /* looks up class, making if needed. Sets h->gcInsert to address of nextCl of last node, and returns it.*/
            for (auto &cr : targ->classes)
                addGlyphClassToCurrentGC(cr);
            finishCurrentGC();
            openAsCurrentGC(kDEFAULT_LIGATURECLASS_NAME);
            for (auto &cr : repl->classes)
                addGlyphClassToCurrentGC(cr);
            finishCurrentGC();
            addGSUB(GSUBLigature, std::move(targ), std::move(repl));
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
        featMsg(sERROR, "MarkToBase or MarkToMark rule references a mark class (%s) that has not yet been defined", markClassName.c_str());
        return;
    }
    curGC.used_mark_class = true;
    assert(anchorMarkInfo.size() > 0);
    anchorMarkInfo.back()->markClassName = markClassName;
    finishCurrentGC();
}

void FeatCtx::addGPOS(int lkpType, GPat::SP targ) {
    prepRule(GPOS_, (targ->has_marked) ? GPOSChain : lkpType, targ, nullptr);

    std::string locDesc = current_visitor->tokenPositionMsg(true);
    g->ctx.GPOSp->RuleAdd(lkpType, std::move(targ), locDesc, anchorMarkInfo);

    wrapUpRule();
}

/* Analyze featValidateGPOSChain targ metrics. Return 1 if valid, else 0 */
/* Also sets flags in backtrack and look-ahead sequences */

bool FeatCtx::validateGPOSChain(GPat::SP &targ, int lkpType) {
    int nMarked {0};
    int nNodesWithMetrics {0};
    int nBaseGlyphs {0};
    int nLookupRefs {0};

    /* At most one run of marked positions supported, for now */
    bool after_marked {false}, unmarked_metrics {false}, terminal_metrics {false};
    GPat::ClassRec *mcr {nullptr};
    for (auto &cr : targ->classes) {
        terminal_metrics = false;
        if (unmarked_metrics) {
            featMsg(sERROR, "Positioning values are allowed only in the marked glyph sequence, or after the final glyph node when only one glyph node is marked.");
            return false;
        }

        if (cr.marked) {
            if (nMarked++ == 0)
                mcr = &cr;
            if (after_marked) {
                featMsg(sERROR, "Unsupported contextual GPOS target sequence: only one run of marked glyphs  is supported.");
                return false;
            }
            if (cr.metricsInfo.metrics.size() > 0) {
                nNodesWithMetrics++;
            }
            if (cr.lookupLabels.size() > 0) {
                nLookupRefs++;
            }
        } else {
            if (nMarked > 0)
                after_marked = true;

            if (cr.lookupLabels.size() > 0) {
                featMsg(sERROR, "Lookup references are allowed only in the input sequence: this is the sequence of marked glyphs.");
            }

            if (cr.marknode) {
                featMsg(sERROR, "The final mark class must be marked as part of the input sequence: this is the sequence of marked glyphs.");
            }

            /* We actually do allow  a value records after the last glyph node, if there is only one marked glyph */
            if (cr.metricsInfo.metrics.size() > 0) {
                terminal_metrics = true;
                if (nMarked == 0) {
                    featMsg(sERROR, "Positioning cannot be applied in the backtrack glyph sequence, before the marked glyph sequence.");
                    return false;
                }
                if (nMarked > 1) {
                    featMsg(sERROR, "Positioning values are allowed only in the marked glyph sequence, or after the final glyph node when only one glyph node is marked.");
                    return false;
                }
                nNodesWithMetrics++;
            } else {
                terminal_metrics = false;
            }
        }

        if (cr.basenode) {
            nBaseGlyphs++;
            if ((lkpType == GPOSCursive) && (!cr.marked)) {
                featMsg(sERROR, "The base glyph or glyph class must be marked as part of the input sequence in a contextual pos cursive statement.");
            }
        }
    }
    /* Check for special case of a single marked node, with one or more lookahead nodes, and a single value record attached to the last node */
    if (terminal_metrics && nMarked == 1) {
        mcr->metricsInfo = targ->classes.back().metricsInfo;
        targ->classes.back().metricsInfo.reset();
    }

    if (targ->ignore_clause) {
        /* An ignore clause is always contextual. If not marked, then mark the first glyph in the sequence */
        if (nMarked == 0) {
            assert(targ != nullptr && targ->classes.size() > 0);
            targ->classes[0].marked = true;
            nMarked = 1;
        }
    } else if ((nNodesWithMetrics == 0) && (nBaseGlyphs == 0) && (nLookupRefs == 0)) {
        featMsg(sERROR, "Contextual positioning rule must specify a positioning value or a mark attachment rule or a direct lookup reference.");
        return false;
    }

    /* Divide into backtrack, input, and lookahead (xxx ff interface?). */
    /* For now, input is marked glyphs                                  */
    nMarked = 0;
    for (auto &cr : targ->classes) {
        if (nMarked == 0 && !cr.marked) {
            cr.backtrack = true;
        } else if (cr.marked) {
            nMarked++;
            cr.input = true;
        } else if (nMarked > 0) {
            cr.lookahead = true;
        }
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

void FeatCtx::addBaseClass(GPat::SP &targ, const std::string &defaultClassName) {
    /* Find base node in a possibly contextual sequence, */
    /* and add it to the default base glyph class        */
    assert(targ != nullptr && targ->patternLen() > 0);
    GPat::ClassRec *crp {&targ->classes[0]};

    if (targ->has_marked) {
        crp = nullptr;
        for (auto &cr : targ->classes) {
            if (cr.basenode)
                crp = &cr;
        }
    }
    assert(crp != nullptr);
    openAsCurrentGC(defaultClassName);
    addGlyphClassToCurrentGC(*crp);
    finishCurrentGC();
}

void FeatCtx::addPos(GPat::SP targ, int type, bool enumerate) {
    int glyphCount {0};
    bool marked {false};
    int lookupLabelCnt {0};

    if (enumerate)
        targ->enumerate = true;

    /* count glyphs, figure out if is single, pair, or context. */
    for (auto &cr : targ->classes) {
        glyphCount++;
        if (cr.marked)
            marked = true;
        if (cr.lookupLabels.size() > 0) {
            lookupLabelCnt++;
            if (!cr.marked) {
                featMsg(sERROR, "the glyph which precedes the 'lookup' keyword must be marked as part of the contextual input sequence");
            }
        }
    }

    /* The ignore statement can only be used with contextual lookups. */
    /* If no glyph is marked, then mark the first.                    */
    if (targ->ignore_clause) {
        type = GPOSChain;
        if (!marked) {
            marked = true;
            assert(targ->classes.size() > 0);
            targ->classes[0].marked = true;
        }
    }

    if (marked)
        targ->has_marked = true; /* used later to decide if stuff is contextual */

    if ((glyphCount == 2) && (!marked) && (type == GPOSSingle)) {
        type = GPOSPair;
    } else if (enumerate) {
        featMsg(sERROR, "\"enumerate\" only allowed with pair positioning,");
    }

    if (type == GPOSSingle) {
        addGPOS(GPOSSingle, std::move(targ));
        /* These nodes are recycled in GPOS.c */
    } else if (type == GPOSPair) {
        sortGlyphClass(targ, true, true, 0);
        sortGlyphClass(targ, true, true, 1);
        addGPOS(GPOSPair, std::move(targ));
    } else if (type == GPOSCursive) {
        if (anchorMarkInfo.size() != 2) {
            featMsg(sERROR, "The 'cursive' statement requires two anchors. This has %ld. Skipping rule.", anchorMarkInfo.size());
        } else if (!targ->has_marked && (!targ->classes[0].basenode || targ->patternLen() > 1)) {
            featMsg(sERROR, "This statement has contextual glyphs around the cursive statement, but no glyphs are marked as part of the input sequence. Skipping rule.");
        } else {
            addGPOS(GPOSCursive, std::move(targ));
        }
        /* These nodes are recycled in GPOS.c */
    } else if (type == GPOSMarkToBase) {
        addBaseClass(targ, kDEFAULT_BASECLASS_NAME);
        if (!targ->has_marked && (!targ->classes[0].basenode || targ->patternLen() > 2)) {
            featMsg(sERROR, "This statement has contextual glyphs around the base-to-mark statement, but no glyphs are marked as part of the input sequence. Skipping rule.");
        }
        addGPOS(GPOSMarkToBase, std::move(targ));
        /* These nodes are recycled in GPOS.c */
    } else if (type == GPOSMarkToLigature) {
        addBaseClass(targ, kDEFAULT_LIGATURECLASS_NAME);
        if (!targ->has_marked && (!targ->classes[0].basenode || targ->patternLen() > 2)) {
            featMsg(sERROR, "This statement has contextual glyphs around the ligature statement, but no glyphs are marked as part of the input sequence. Skipping rule.");
        }

        /* With mark to ligatures, we may see the same mark class on         */
        /* different components, which leads to duplicate GID's in the       */
        /* contextual mark node. As a result, we need to sort and get rid of */
        /* duplicates.                                                       */
        if (targ->has_marked) {
            for (int i = 0; i < targ->patternLen(); i++) {
                if (targ->classes[i].marked)
                    sortGlyphClass(targ, true, false, i);
            }
        }

        addGPOS(GPOSMarkToLigature, std::move(targ));
        /* These nodes are recycled in GPOS.c */
    } else if (type == GPOSMarkToMark) {
        addBaseClass(targ, kDEFAULT_MARKCLASS_NAME);
        if (!targ->has_marked && (!targ->classes[0].basenode || targ->patternLen() > 2)) {
            featMsg(sERROR, "This statement has contextual glyphs around the mark-to-mark statement, but no glyphs are marked as part of the input sequence. Skipping rule.");
        }
        addGPOS(GPOSMarkToMark, std::move(targ));
        /* These nodes are recycled in GPOS.c */
    } else if (type == GPOSChain) {
        /* is contextual */
        if (!marked) {
            featMsg(sERROR, "The 'lookup' keyword can be used only in a contextual statement. At least one glyph in the sequence must be marked. Skipping rule.");
        } else {
            validateGPOSChain(targ, type);
            addGPOS(GPOSChain, std::move(targ));
        }
        /* These nodes are recycled in GPOS.c, as they are used in the fill phase, some time after this function returns. */
    } else {
        featMsg(sERROR, "This rule type is not recognized..");
    }
}

// ------------------------------ CV Parameters --------------------------------

void FeatCtx::clearCVParameters() {
    sawCVParams = true;
    cvParameters.reset();
}

void FeatCtx::addCVNameID(int labelID) {
    switch (labelID) {
        case cvUILabelEnum: {
            if (cvParameters.FeatUILabelNameID != 0) {
                featMsg(sERROR, "A Character Variant parameter table can have only one FeatUILabelNameID entry.");
            }
            cvParameters.FeatUILabelNameID = featNameID;
            break;
        }

        case cvToolTipEnum: {
            if (cvParameters.FeatUITooltipTextNameID != 0) {
                featMsg(sERROR, "A Character Variant parameter table can have only one SampleTextNameID entry.");
            }
            cvParameters.FeatUITooltipTextNameID = featNameID;
            break;
        }

        case cvSampleTextEnum: {
            if (cvParameters.SampleTextNameID != 0) {
                featMsg(sERROR, "A Character Variant parameter table can have only one SampleTextNameID entry.");
            }
            cvParameters.SampleTextNameID = featNameID;
            break;
        }

        case kCVParameterLabelEnum: {
            if ( cvParameters.FirstParamUILabelNameID == 0 )
                cvParameters.FirstParamUILabelNameID = featNameID;
            else if ( cvParameters.FirstParamUILabelNameID +
                      cvParameters.NumNamedParameters != featNameID )
                featMsg(sERROR, "Character variant AParamUILabelNameID statements must be contiguous.");
            cvParameters.NumNamedParameters++;
            break;
        }
    }

    featNameID = 0;
}

void FeatCtx::addCVParametersCharValue(unsigned long uv) {
    cvParameters.charValues.push_back(uv);
}

void FeatCtx::addCVParam() {
    prepRule(GSUB_, GSUBCVParam, nullptr, nullptr);

    g->ctx.GSUBp->AddCVParam(std::move(cvParameters));

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
                featMsg(sERROR, "OS/2 Bad Unicode block value <%d>. All values must be in [0 ...127] inclusive.", bitnum);
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
                featMsg(sERROR, "OS/2 Code page value <%d> is not permitted according to the OpenType spec v1.4.", pageNumber);
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
        featMsg(sERROR, "\"feature\" statement allowed only in 'aalt' feature");
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
            featMsg(sWARNING,
                    "feature '%c%c%c%c', referenced in aalt feature, either is"
                    " not defined or had no rules which could be included in"
                    " the aalt feature.",
                   TAG_ARG(f.feature));
        }
    }
}

/* Ranges of 1-1, single 1-1, or 1 from n. (Only first glyph position in targ will
 * be looked at)
 * If the alt glyph is defined explicitly in the aalt feature, via a 'sub' directive,
 * it gets an index of AALT_INDEX, aka, -1, so that it will sort before all the alts
 * from included features. */

void FeatCtx::aaltAddAlternates(const GPat::ClassRec &targcr,
                                const GPat::ClassRec &replcr) {
    auto it = std::find(std::begin(aalt.features), std::end(aalt.features), curr.feature);
    short aaltTagIndex = it != std::end(aalt.features) ? it - aalt.features.begin() : -1;

    auto tgi = targcr.glyphs.begin();
    auto rgi = replcr.glyphs.begin();
    while (tgi != targcr.glyphs.end() && rgi != replcr.glyphs.end()) {
        /* Start new accumulator for gid if it doesn't already exist */
        auto ru = aalt.rules.find(*tgi);
        if ( ru == aalt.rules.end() ) {
            aalt.rules.insert(std::make_pair(*tgi, AALT::RuleInfo{*tgi}));
            ru = aalt.rules.find(*tgi);
        }
        auto &ri = ru->second;
        assert(ri.targid == *tgi);
        /* Add alternates to alt set,                 */
        /* checking for uniqueness & preserving order */
        bool found = false;
        for (auto &rig : ri.glyphs) {
            if (rig.rgid == *rgi) {
                found = true;
                if (aaltTagIndex < rig.aaltIndex)
                    rig.aaltIndex = aaltTagIndex;
            }
        }
        if (!found) {
            ri.glyphs.emplace_back(*rgi, curr.feature == aalt_ ? AALT_INDEX : aaltTagIndex);
        }
        if (targcr.glyphs.size() > 1)
            tgi++;
        if (targcr.glyphs.size() == 1 || replcr.glyphs.size() > 1)
            rgi++;
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

    if (aalt.rules.size() == 0)
        return;

    // These will be sorted by GID in virtue of aalt.rules being a std::map
    std::vector<AALT::RuleInfo> singles;
    std::vector<AALT::RuleInfo> alts;
    std::set<GID> altKeys;
    for (auto &ru : aalt.rules) {
        assert(ru.first == ru.second.targid);
        if (ru.second.glyphs.size() > 1) {
            alts.push_back(std::move(ru.second));
            altKeys.insert(ru.first);
        } else {
            singles.push_back(std::move(ru.second));
        }
    }
    aalt.rules.clear();

    // If the repl GID of a SingleSub rule is the same as an AltSub
    // rule's targ GID, then the SingleSub becomes part of the AltSub
    // rules section:
    for (int si = singles.size() - 1; si >= 0; si--) {
        auto ssearch = altKeys.find(singles[si].glyphs[0].rgid);
        if ( ssearch != altKeys.end() ) {
            alts.insert(alts.begin(), std::move(singles[si]));
            singles.erase(singles.begin() + si);
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

    /* Add default lang sys if none specified */
    if (langSysMap.size() == 0) {
        addLangSys(DFLT_, dflt_, false);
        if (fFlags & langSysMode) {
            g->logger->msg(sWARNING, "[internal] aalt language system unspecified");
        }
    }

    auto i = langSysMap.cbegin();
    assert(i != langSysMap.cend());
    g->ctx.GSUBp->FeatureBegin(i->first.script, i->first.lang, aalt_);

    /* --- Feed in single subs --- */
    if (singles.size() > 0) {
        labelSingle = getNextAnonLabel();
        g->ctx.GSUBp->LookupBegin(GSUBSingle, 0, labelSingle, aalt.useExtension, 0);
        for (auto &rule : singles) {
            GPat::SP targ = std::make_unique<GPat>(rule.targid);
            GPat::SP repl = std::make_unique<GPat>(rule.glyphs[0].rgid);
            g->ctx.GSUBp->RuleAdd(std::move(targ), std::move(repl));
        }
        g->ctx.GSUBp->LookupEnd();
    }

    /* --- Feed in alt subs --- */
    if (alts.size() > 0) {
        labelAlternate = getNextAnonLabel();
        g->ctx.GSUBp->LookupBegin(GSUBAlternate, 0, labelAlternate, aalt.useExtension, 0);
        for (auto &rule : alts) {
            GPat::SP targ = std::make_unique<GPat>(rule.targid);
            // sort alts in order of feature def in aalt feature
            std::sort(rule.glyphs.begin(), rule.glyphs.end());
            GPat::SP repl = std::make_unique<GPat>();
            repl->addClass(GPat::ClassRec{});
            for (auto &gr : rule.glyphs)
                repl->classes[0].glyphs.emplace_back(gr.rgid);
            g->ctx.GSUBp->RuleAdd(std::move(targ), std::move(repl));
        }
        g->ctx.GSUBp->LookupEnd();
    }

    g->ctx.GSUBp->FeatureEnd();

    /* Also register these lookups under any other lang systems, if needed: */
    for (auto ls = langSysMap.cbegin(); ls != langSysMap.cend(); ls++) {
        if ( ls == langSysMap.cbegin() )
            continue;

        g->ctx.GSUBp->FeatureBegin(ls->first.script, ls->first.lang, aalt_);

        if (singles.size() > 0) {
            g->ctx.GSUBp->LookupBegin(GSUBSingle, 0, (Label)(labelSingle | REF_LAB),
                                      aalt.useExtension, 0);
            g->ctx.GSUBp->LookupEnd();
        }
        if (alts.size() > 0) {
            g->ctx.GSUBp->LookupBegin(GSUBAlternate, 0,
                                      (Label)(labelAlternate | REF_LAB),
                                      aalt.useExtension, 0);
            g->ctx.GSUBp->LookupEnd();
        }

        g->ctx.GSUBp->FeatureEnd();
    }
}

/* Return 1 if this rule is to be treated specially */

bool FeatCtx::aaltCheckRule(int type, GPat::SP &targ, GPat::SP &repl) {
    if (curr.feature == aalt_) {
        if (type == GSUBSingle || type == GSUBAlternate) {
            assert(repl != nullptr && repl->patternLen() > 0);
            aaltAddAlternates(targ->classes[0], repl->classes[0]);
        } else {
            featMsg(sWARNING,
                    "Only single and alternate "
                    "substitutions are allowed within an 'aalt' feature");
        }
        return true;
    }
    return false;
}

void FeatCtx::storeRuleInfo(const GPat::SP &targ, const GPat::SP &repl) {
    assert(repl != nullptr && repl->patternLen() > 0);
    if (curr.tbl == GPOS_) {
        return; /* No GPOS or except clauses */
    }
    const GPat::ClassRec *crp = &(targ->classes[0]);
    AALT::FeatureRecord t { curr.feature, false };
    auto f = std::find(std::begin(aalt.features), std::end(aalt.features), t);
    if ( curr.feature == aalt_ || f != std::end(aalt.features) ) {
        bool found = false;

        switch (curr.lkpType) {
            case GSUBSingle:
            case GSUBAlternate:
                break;

            case GSUBChain:
                /* Go to first marked glyph (guaranteed to be present) */
                for (auto &cr : targ->classes) {
                    if (!found && cr.marked) {
                        crp = &cr;
                        found = true;
                    } else if (cr.marked)
                        return;  // Ligature sub-substitution;
                }
                break;

            default:
                return;
        }

        if (curr.feature != aalt_) {
            assert(f != std::end(aalt.features));
            f->used = true;
        }
        aaltAddAlternates(*crp, repl->classes[0]);
    }
}

// -------------------------------- Variable ----------------------------------

uint16_t FeatCtx::getAxisCount() {
    if (g->ctx.axes == nullptr)
        return 0;
    return g->ctx.axes->getAxisCount();
}

var_F2dot14 FeatCtx::validAxisLocation(var_F2dot14 v, int8_t adjustment) {
    if (v > F2DOT14_ONE) {
        if (adjustment > 0) {
            featMsg(sERROR, "Cannot increase location value above maximum.");
        } else {
            featMsg(sWARNING, "Normalized axis value > 1, will be rounded down.");
            v = F2DOT14_ONE;
        }
    } else if (v < F2DOT14_MINUS_ONE) {
        if (adjustment < 0) {
            featMsg(sERROR, "Cannot decrease location value below minumum.");
        } else {
            featMsg(sWARNING, "Normalized axis value < -1, will be rounded up.");
            v = F2DOT14_MINUS_ONE;
        }
    }
    return v;
}

int16_t FeatCtx::axisTagToIndex(Tag tag) {
    if (g->ctx.axes == nullptr) {
        featMsg(sERROR, "Reference to axis '%c%c%c%c' in non-variable font", TAG_ARG(tag));
        return -1;
    }
    auto i = g->ctx.axes->getAxisIndex(tag);
    if (i == -1)
        featMsg(sERROR, "Axis '%c%c%c%c' not found", TAG_ARG(tag));
    return i;
}

uint32_t FeatCtx::locationToIndex(std::shared_ptr<VarLocation> vl) {
    if (g->ctx.locMap == nullptr) {
        featMsg(sERROR, "Designspace location seen in non-variable font");
        return 0;
    }
    return g->ctx.locMap->getIndex(vl);
}

bool FeatCtx::addLocationDef(const std::string &name, uint32_t loc_idx) {
    const auto [it, success] = locationDefs.emplace(name, loc_idx);
    return success;
}

uint32_t FeatCtx::getLocationDef(const std::string &name) {
    auto search = locationDefs.find(name);
    if ( search == locationDefs.end() ) {
        featMsg(sERROR, "Named location '%s' is not in list of named location records.", name.c_str());
        return 0;
    }
    return search->second;
}

#if HOT_DEBUG
void FeatCtx::dumpLocationDefs() {
    for (auto &[name, index] : locationDefs)
        std::cerr << " " << name << ":  " << index << std::endl;
}
#endif  // HOT_DEBUG
