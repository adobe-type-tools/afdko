/* Copyright 2021 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 * This software is licensed as OpenSource, under the Apache License, Version 2.0.
 * This license is available at: http://opensource.org/licenses/Apache-2.0.
 */

#include "FeatVisitor.h"

#include <math.h>

#include <algorithm>
#include <limits>
#include <memory>

#include "BASE.h"
#include "GDEF.h"
#include "GPOS.h"
#include "GSUB.h"
#include "hhea.h"
#include "name.h"
#include "OS_2.h"
#include "otl.h"
#include "STAT.h"
#include "vhea.h"

extern "C" const char *curdir();
extern "C" const char *sep();

#define MAX_INCL 50

// -------------------------------- Helper -----------------------------------

static const char* findDirName(const char *path) {
    size_t i = strlen(path);
    const char* end = nullptr;
    while (i > 0) {
        end = strchr("/\\:", path[--i]);
        if (end != nullptr)
            break;
    }
    if (end != nullptr)
        end = &path[i];
    return end;
}

static void assignDirName(const std::string &of, std::string &to) {
    const char *p = findDirName(of.c_str());
    if ( p == nullptr ) {
        to.assign(curdir());
    } else {
        to.assign(of, 0, (size_t)(p-of.c_str()));
    }
}

FeatVisitor::~FeatVisitor() {
    for (auto i : includes)
        delete i;

    // The tree pointer itself is managed by the parser
    delete parser;
    delete tokens;
    delete lexer;
    delete input;
}

// ----------------------------- Entry Points --------------------------------

void FeatVisitor::Parse(bool do_includes) {
    std::ifstream stream;

    fc->g->logger->set_context("feat_file", sINHERIT, newFileMsg().c_str());

    if ( depth >= MAX_INCL ) {
        fc->featMsg(hotFATAL, "Can't include [%s]; maximum include levels <%d> reached",
                    pathname.c_str(), MAX_INCL);
        return;
    }
    if ( parent == nullptr ) {
        stream.open(pathname);
        if ( !stream.is_open() ) {
            fc->featMsg(hotFATAL, "Specified feature file '%s' not found", pathname.c_str());
            return;
        }
        assignDirName(pathname, dirname);
    } else {
        std::string &rootdir = fc->root_visitor->dirname;
        std::string fullname;
        // Try relative to (potential) UFO fontinfo.plist file
        stream.open(rootdir + sep() + "fontinfo.plist");
        if ( stream.is_open() ) {
            stream.close();
            fullname = rootdir + sep() + ".." + sep() + pathname;
            stream.open(fullname);
        }
        // Try relative to top-level file
        if ( !stream.is_open() ) {
            fullname = rootdir + sep() + pathname;
            stream.open(fullname);
        }
        // Try relative to including file
        if ( !stream.is_open() && rootdir != parent->dirname ) {
            fullname = parent->dirname + sep() + pathname;
            stream.open(fullname);
        }
        // Try just the pathname
        if ( !stream.is_open() ) {
            fullname = pathname;
            stream.open(fullname);
        }
        if ( !stream.is_open() ) {
            fc->featMsg(hotERROR, "Included feature file '%s' not found", pathname.c_str());
            return;
        }
        assignDirName(fullname, dirname);
    }

    input = new antlr4::ANTLRInputStream(stream);
    lexer = new FeatLexer(input);
    tokens = new antlr4::CommonTokenStream(lexer);
    parser = new FeatParser(tokens);
    parser->removeErrorListeners();
    FeatErrorListener el{*this};
    parser->addErrorListener(&el);

    fc->current_visitor = this;

    tree = top_ep(*parser);

    parser->removeErrorListeners();

    if ( tree == nullptr || !do_includes ) {
        fc->current_visitor = nullptr;
        return;
    }

    stage = vInclude;
    include_ep = top_ep;
    tree->accept(this);

    fc->g->logger->clear_context("feat_file");

    fc->current_visitor = nullptr;
    return;
}

void FeatVisitor::Translate() {
    fc->current_visitor = this;
    stage = vExtract;  // This will still parse includes if not already done
    include_ep = top_ep;
    fc->g->logger->set_context("feat_file", sINHERIT, newFileMsg().c_str());
    tree->accept(this);

    int used = fc->g->logger->clear_context("feat_file");
    if (used == log_context_used) {
        auto a = this;
        while (a != nullptr) {
            a->need_file_msg = false;
            a = a->parent;
        }
    }

    fc->current_visitor = nullptr;
}

// --------------------------- Message Reporting -----------------------------

void FeatVisitor::currentTokStr(std::string &ts) {
    if ( current_msg_token == nullptr )
        return;

    ts = current_msg_token->getText();
}

std::string FeatVisitor::newFileMsg() {
    std::string r;
    r = (parent == nullptr ? "in top-level feature file '"
                           : "in feature file '")
         + pathname + "' ";
    auto a = parent;
    bool did_a = false;
    while ( a != nullptr && a->need_file_msg ) {
        if ( !did_a )
            r += "(";
        else
            r += ", ";
        did_a = true;
        r += "included from " + a->pathname;
        a->need_file_msg = false;
        a = a->parent;
    }
    if ( did_a )
        r += ")";
    r += ":";
    return r;
}

std::string FeatVisitor::tokenPositionMsg(bool full) {
    std::string r;

    if ( current_msg_token == nullptr && !full ) {
        return r;
    }

    int l = pathname.length() + 50;
    auto buf = std::make_unique<char[]>(l);
    if ( current_msg_token != nullptr ) {
        if (full) {
            snprintf(buf.get(), l, "file '%s' line %5ld char %3ld",
                    pathname.c_str(),
                    (long)current_msg_token->getLine(),
                    (long)current_msg_token->getCharPositionInLine()+1);
        } else {
            snprintf(buf.get(), l, "[line %5ld char %3ld] ",
                    (long)current_msg_token->getLine(),
                    (long)current_msg_token->getCharPositionInLine()+1);
        }
        r = (char *)buf.get();
    } else {
        assert(full);
        r = "unknown";
    }
    return r;
}

void FeatVisitor::FeatErrorListener::syntaxError(antlr4::Recognizer *,
                                                 antlr4::Token *t,
                                                 size_t, size_t,
                                                 const std::string &msg,
                                                 std::exception_ptr) {
    v.TOK(t);
    // Whatever messages the parser can produce are printed as a group
    // before processing ends if there are any errors, so here we just
    // mark them all as ERR instead of FATAL
    hotMsg(v.fc->g, hotERROR, msg.c_str());
}

// --------------------------------- Blocks -----------------------------------

antlrcpp::Any FeatVisitor::visitFeatureBlock(FeatParser::FeatureBlockContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::featureFile;
    if ( stage == vExtract ) {
        Tag t = checkTag(TOK(ctx->starttag), ctx->endtag);
        fc->startFeature(t);
        if ( ctx->USE_EXTENSION() != nullptr )
            fc->flagExtension(false);
    }

    for (auto i : ctx->featureStatement())
        visitFeatureStatement(i);

    if ( stage == vExtract ) {
        TOK(ctx->endtag);
        fc->endFeature();
    }
    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitAnonBlock(FeatParser::AnonBlockContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    Tag tag = fc->str2tag(TOK(ctx->A_LABEL())->getText());
    std::string buf;
    for (auto al : ctx->A_LINE())
        buf += al->getText();

    TOK(ctx);
    fc->g->cb.featAddAnonData(fc->g->cb.ctx, buf.c_str(), buf.size(), tag);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitLookupBlockTopLevel(FeatParser::LookupBlockTopLevelContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::statementFile;
    if ( stage == vExtract ) {
        checkLabel(ctx->startlabel, ctx->endlabel);
        fc->startLookup(TOK(ctx->startlabel)->getText(), true);
        if ( ctx->USE_EXTENSION() != nullptr )
            fc->flagExtension(true);
    }

    for (auto i : ctx->statement())
        visitStatement(i);

    if ( stage == vExtract ) {
        TOK(ctx->endlabel);
        fc->endLookup();
    }
    include_ep = tmp_ep;
    return nullptr;
}

// --------------------------- Top-level statements ---------------------------

antlrcpp::Any FeatVisitor::visitInclude(FeatParser::IncludeContext *ctx) {
    assert(ctx->IFILE() != nullptr);
    FeatVisitor *inc;

    if ( includes.size() == current_include ) {
        std::string fname = TOK(ctx->IFILE())->getText();
        // (dubiously) strip leading and trailing spaces from the filename
        while ( !fname.empty() && std::isspace(*fname.begin()) )
            fname.erase(fname.begin());
        while ( !fname.empty() && std::isspace(*fname.rbegin()) )
            fname.erase(fname.length()-1);
        inc = new FeatVisitor(fc, fname.c_str(), this, include_ep, depth+1);
        inc->Parse(true);
        includes.emplace_back(inc);
        fc->current_visitor = this;
    } else {
        assert(includes.size() > current_include);
        inc = includes[current_include];
    }

    if ( stage == vExtract ) {
        inc->Translate();
        fc->current_visitor = this;
    }

    fc->g->logger->set_context("feat_file", sINHERIT, newFileMsg().c_str());

    current_include++;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitLangsysAssign(FeatParser::LangsysAssignContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    Tag stag = fc->str2tag(TOK(ctx->script)->getText());
    Tag ltag = fc->str2tag(ctx->lang->getText());
    fc->addLangSys(stag, ltag, true, ctx->lang);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitValueRecordDef(FeatParser::ValueRecordDefContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    MetricsInfo mi = METRICSINFOEMPTYPP;
    getValueLiteral(ctx->valueLiteral(), mi);
    fc->addValueDef(TOK(ctx->label())->getText(), mi);

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitAnchorDef(FeatParser::AnchorDefContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    FeatCtx::AnchorDef a;
    a.x = getNum<int16_t>(TOK(ctx->xval)->getText(), 10);
    a.y = getNum<int16_t>(TOK(ctx->yval)->getText(), 10);
    if ( ctx->cp != nullptr ) {
        a.contourpoint = getNum<uint16_t>(TOK(ctx->cp)->getText(), 10);
        a.hasContour = true;
    }

    fc->addAnchorDef(TOK(ctx->name)->getText(), a);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitGlyphClassAssign(FeatParser::GlyphClassAssignContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    getGlyphClassAsCurrentGC(ctx->glyphClass(), ctx->GCLASS(), false);
    TOK(ctx);
    fc->finishCurrentGC();

    return nullptr;
}

// ---------------- Statements (in feature and lookup blocks) -----------------

antlrcpp::Any FeatVisitor::visitFeatureUse(FeatParser::FeatureUseContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    fc->aaltAddFeatureTag(fc->str2tag(TOK(ctx->tag())->getText()));
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitScriptAssign(FeatParser::ScriptAssignContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    fc->startScriptOrLang(FeatCtx::scriptTag, fc->str2tag(TOK(ctx->tag())->getText()));
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitLangAssign(FeatParser::LangAssignContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    int lang_change = fc->startScriptOrLang(FeatCtx::languageTag, fc->str2tag(TOK(ctx->tag())->getText()));

    bool old_format = false, include_dflt = true;
    if ( ctx->EXCLUDE_dflt() || ctx->EXCLUDE_DFLT() )
        include_dflt = false;
    if ( ctx->EXCLUDE_DFLT() || ctx->INCLUDE_DFLT() )
        old_format = true;

    TOK(ctx);
    if ( lang_change != -1 )
        fc->includeDFLT(include_dflt, lang_change, old_format);

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitLookupflagAssign(FeatParser::LookupflagAssignContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    uint16_t v = 0;
    GNode *gc = nullptr;
    for (auto e : ctx->lookupflagElement()) {
        if ( e->RIGHT_TO_LEFT() != nullptr )
            v = fc->setLkpFlagAttribute(v, otlRightToLeft, 0);
        else if ( e->IGNORE_BASE_GLYPHS() != nullptr )
            v = fc->setLkpFlagAttribute(v, otlIgnoreBaseGlyphs, 0);
        else if ( e->IGNORE_LIGATURES() != nullptr )
            v = fc->setLkpFlagAttribute(v, otlIgnoreLigatures, 0);
        else if ( e->IGNORE_MARKS() != nullptr )
            v = fc->setLkpFlagAttribute(v, otlIgnoreMarks, 0);
        else if ( e->USE_MARK_FILTERING_SET() != nullptr ) {
            gc = getGlyphClass(e->glyphClass(), true);
            uint16_t umfIndex = fc->g->ctx.GDEFp->addMarkSetClass(gc);
            v = fc->setLkpFlagAttribute(v, otlUseMarkFilteringSet, umfIndex);
        } else {
            assert(e->MARK_ATTACHMENT_TYPE() != nullptr);
            gc = getGlyphClass(e->glyphClass(), true);
            uint16_t macIndex = fc->g->ctx.GDEFp->addGlyphMarkClass(gc);
            if ( macIndex > kMaxMarkAttachClasses )
                fc->featMsg(hotERROR, "No more than 15 different class names can be used with the \"lookupflag MarkAttachmentType\". This would be a 16th.");
            v = fc->setLkpFlagAttribute(v, otlMarkAttachmentType, macIndex);
        }
    }
    if ( ctx->lookupflagElement().size() == 0 ) {
        assert(ctx->NUM() != nullptr);
        v = getNum<uint16_t>(TOK(ctx->NUM())->getText(), 10);
    }
    fc->setLkpFlag(v);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitIgnoreSubOrPos(FeatParser::IgnoreSubOrPosContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    int sub_type = ctx->revtok() != nullptr ? GSUBReverse : GSUBChain;
    bool is_sub = ctx->postok() == nullptr;

    for (auto lp : ctx->lookupPattern()) {
        GNode *targ = getLookupPattern(lp, true);
        targ->flags |= FEAT_IGNORE_CLAUSE;
        if ( is_sub )
            fc->addSub(targ, nullptr, sub_type);
        else
            fc->addPos(targ, 0, 0);
    }

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitSubstitute(FeatParser::SubstituteContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    GNode *targ, *repl = nullptr;
    int type = 0;

    if ( ctx->EXCEPT() != nullptr ) {
        type = GSUBChain;
        fc->syntax.numExcept++;
        for (auto i : ctx->lookupPattern()) {
            if ( i == ctx->startpat || i == ctx->endpat )
                continue;
            targ = getLookupPattern(i, true);
            targ->flags |= FEAT_IGNORE_CLAUSE;
            fc->addSub(targ, nullptr, type);
        }
    }
    if ( ctx->revtok() != nullptr ) {
        type = GSUBReverse;
        assert(ctx->subtok() == nullptr);
        assert(ctx->startpat != nullptr);

        targ = getLookupPattern(ctx->startpat, true);
        if ( ctx->endpat != nullptr )
            repl = getLookupPattern(ctx->endpat, false);
    } else {
        assert(ctx->subtok() != nullptr);
        assert(ctx->startpat != nullptr);
        if ( ctx->FROM() != nullptr )
            type = GSUBAlternate;
        targ = getLookupPattern(ctx->startpat, true);
        if ( ctx->endpat != nullptr )
            repl = getLookupPattern(ctx->endpat, false);
    }
    TOK(ctx);
    fc->addSub(targ, repl, type);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitMark_statement(FeatParser::Mark_statementContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    GNode *targ = nullptr;
    fc->anchorMarkInfo.clear();

    if ( ctx->glyph() != nullptr ) {
        targ = fc->setNewNode(getGlyph(ctx->glyph(), false));
    } else {
        assert(ctx->glyphClass() != nullptr);
        targ = getGlyphClass(ctx->glyphClass(), false);
    }

    translateAnchor(ctx->anchor(), 0);

    fc->addMark(TOK(ctx->GCLASS())->getText(), targ);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitPosition(FeatParser::PositionContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    bool enumerate = ctx->enumtok() != nullptr;
    int type = 0;
    GNode *head = nullptr, *tail = nullptr, **marks;

    fc->anchorMarkInfo.clear();

    if ( ctx->startpat != nullptr )
        tail = concatenatePattern(&head, ctx->startpat);

    if ( TOK(ctx->valueRecord()) != nullptr ) {
        if ( tail == nullptr ) {
            fc->featMsg(hotERROR, "Glyph or glyph class must precede a value record.");
            return nullptr;
        }
        type = GPOSSingle;
        getValueRecord(ctx->valueRecord(), tail->metricsInfo);
        for (auto vp : ctx->valuePattern()) {
            tail = concatenatePatternElement(&tail, vp->patternElement());
            if ( vp->valueRecord() )
                getValueRecord(vp->valueRecord(), tail->metricsInfo);
        }
    } else if ( ctx->LOOKUP().size() != 0 ) {
        if ( tail == nullptr ) {
            TOK(ctx->LOOKUP(0));
            fc->featMsg(hotERROR, "Glyph or glyph class must precede a lookup reference in a contextual rule.");
            return nullptr;
        }
        type = GPOSChain;
        for (auto l : ctx->label()) {
            tail->lookupLabels[tail->lookupLabelCount++] = fc->getLabelIndex(TOK(l)->getText());
            if ( tail->lookupLabelCount > 255 )
                fc->featMsg(hotFATAL, "Too many lookup references in one glyph position.");
        }
        for (auto lpe : ctx->lookupPatternElement()) {
            tail->nextSeq = getLookupPatternElement(lpe, true);
            tail = tail->nextSeq;
        }
    } else if ( ctx->cursiveElement() != nullptr ) {
        type = GPOSCursive;
        tail = concatenatePatternElement(tail == nullptr ? &head : &tail,
                                         ctx->cursiveElement()->patternElement());
        tail->flags |= FEAT_IS_BASE_NODE;
        for (auto a : ctx->cursiveElement()->anchor())
            translateAnchor(a, 0);
        if ( ctx->endpat != nullptr )
            tail = concatenatePattern(&tail, ctx->endpat);
    } else if ( ctx->MARKBASE() != nullptr ) {
        type = GPOSMarkToBase;
        tail = concatenatePattern(tail == nullptr ? &head : &tail, ctx->midpat, FEAT_IS_BASE_NODE);
        marks = &tail->nextSeq;
        for (auto mbe : ctx->baseToMarkElement()) {
            translateAnchor(mbe->anchor(), 0);
            fc->addMarkClass(TOK(mbe->GCLASS())->getText());
            if ( mbe->MARKER() != nullptr )
                marks = fc->copyGlyphClass(marks, fc->lookupGlyphClass(mbe->GCLASS()->getText()));
        }
        if ( tail->nextSeq != nullptr ) {
            tail->flags |= FEAT_MARKED;
            tail = tail->nextSeq;
            tail->flags |= FEAT_IS_MARK_NODE;
        }
        if ( ctx->endpat != nullptr )
            tail = concatenatePattern(&tail, ctx->endpat);
    } else if ( ctx->markligtok() != nullptr ) {
        type = GPOSMarkToLigature;
        int componentIndex = 0;
        tail = concatenatePattern(tail == nullptr ? &head : &tail, ctx->midpat, FEAT_IS_BASE_NODE);
        marks = &tail->nextSeq;
        for (auto lme : ctx->ligatureMarkElement()) {
            bool isNULL = translateAnchor(lme->anchor(), componentIndex);
            if ( lme->MARK() ) {
                fc->addMarkClass(TOK(lme->GCLASS())->getText());
            } else if ( !isNULL )
                fc->featMsg(hotERROR, "In mark to ligature, non-null anchor must be followed by a mark class.");
            if ( lme->LIG_COMPONENT() != nullptr )
                componentIndex++;
            if ( lme->MARKER() != nullptr )
                marks = fc->copyGlyphClass(marks, fc->lookupGlyphClass(lme->GCLASS()->getText()));
        }
        if ( tail->nextSeq != nullptr ) {
            tail->flags |= FEAT_MARKED;
            tail = tail->nextSeq;
            tail->flags |= FEAT_IS_MARK_NODE;
        }
        if ( ctx->endpat != nullptr )
            tail = concatenatePattern(&tail, ctx->endpat);
    } else {
        assert(ctx->MARK() != nullptr);
        type = GPOSMarkToMark;
        tail = concatenatePattern(tail == nullptr ? &head : &tail, ctx->midpat, FEAT_IS_BASE_NODE);
        marks = &tail->nextSeq;
        for (auto mbe : ctx->baseToMarkElement()) {
            translateAnchor(mbe->anchor(), 0);
            fc->addMarkClass(TOK(mbe->GCLASS())->getText());
            if ( mbe->MARKER() != nullptr )
                marks = fc->copyGlyphClass(marks, fc->lookupGlyphClass(mbe->GCLASS()->getText()));
        }
        if ( tail->nextSeq != nullptr ) {
            tail->flags |= FEAT_MARKED;
            tail = tail->nextSeq;
            tail->flags |= FEAT_IS_MARK_NODE;
        }
        if ( ctx->endpat != nullptr )
            tail = concatenatePattern(&tail, ctx->endpat);
    }
    assert(head != nullptr);
    fc->addPos(head, type, enumerate);

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitParameters(FeatParser::ParametersContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    size_t s = ctx->fixedNum().size();
    if ( s > MAX_FEAT_PARAM_NUM ) {
        TOK(ctx->fixedNum(0));
        fc->featMsg(hotERROR, "Too many parameter values.");
        s = MAX_FEAT_PARAM_NUM;
    }
    fc->featNameID = 0;
    std::vector<uint16_t> p(s);
    for (size_t i=0; i < s; ++i)
        p[i] = getFixed<uint16_t>(ctx->fixedNum(i), true);
    fc->addFeatureParam(p);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitSizemenuname(FeatParser::SizemenunameContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    long v[3] {-1, -1, -1};
    assert(ctx->genNum().size() == 0 || ctx->genNum().size() == 1 ||
           ctx->genNum().size() == 3);

    for (size_t i = 0; i < ctx->genNum().size(); ++i)
        v[i] = getNum<uint16_t>(TOK(ctx->genNum(i))->getText());

    if ( ctx->genNum().size() > 0 &&
         v[0] != HOT_NAME_MS_PLATFORM && v[0] != HOT_NAME_MAC_PLATFORM ) {
        TOK(ctx->genNum(0));
        fc->featMsg(hotERROR, "platform id must be %d or %d",
                    HOT_NAME_MS_PLATFORM, HOT_NAME_MAC_PLATFORM);
    }

    fc->addSizeNameString(v[0], v[1], v[2], TOK(ctx->STRVAL())->getText());
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitFeatureNames(FeatParser::FeatureNamesContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::nameEntryFile;

    if ( stage == vExtract ) {
        fc->sawFeatNames = true;
        fc->featNameID = nameReserveUserID(fc->g);
        fc->addNameFn = &FeatCtx::addFeatureNameString;
    }

    for (auto i : ctx->nameEntryStatement())
        visitNameEntryStatement(i);

    if ( stage == vExtract ) {
        fc->addFeatureNameParam();
    }

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitSubtable(FeatParser::SubtableContext *ctx) {
    if ( stage != vExtract )
        return nullptr;
    TOK(ctx);
    fc->subtableBreak();
    return nullptr;
}

// ---------------------------- Feature-specific -----------------------------

antlrcpp::Any FeatVisitor::visitLookupBlockOrUse(FeatParser::LookupBlockOrUseContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::statementFile;
    if ( stage == vExtract ) {
        if ( ctx->RCBRACE() == nullptr ) {
            fc->useLkp(TOK(ctx->startlabel)->getText());
            include_ep = tmp_ep;
            return nullptr;
        }
        checkLabel(ctx->startlabel, ctx->endlabel);
        fc->startLookup(TOK(ctx->startlabel)->getText(), false);
        if ( ctx->USE_EXTENSION() != nullptr )
            fc->flagExtension(true);
    }

    for (auto i : ctx->statement())
        visitStatement(i);

    if ( stage == vExtract ) {
        TOK(ctx->endlabel);
        fc->endLookup();
    }
    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitCvParameterBlock(FeatParser::CvParameterBlockContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::cvStatementFile;
    if ( stage == vExtract ) {
        fc->clearCVParameters();
        fc->featNameID = 0;
    }

    for (auto i : ctx->cvParameterStatement())
        visitCvParameterStatement(i);

    if ( stage == vExtract ) {
        fc->addCVParam();
    }
    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitCvParameter(FeatParser::CvParameterContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    int en = 0;
    if ( ctx->CV_UI_LABEL() != nullptr )
        en = FeatCtx::cvUILabelEnum;
    else if ( ctx->CV_TOOLTIP() != nullptr )
        en = FeatCtx::cvToolTipEnum;
    else if ( ctx->CV_SAMPLE_TEXT() != nullptr )
        en = FeatCtx::cvSampleTextEnum;
    else if ( ctx->CV_PARAM_LABEL() != nullptr )
        en = FeatCtx::kCVParameterLabelEnum;
    else
        assert(ctx->CV_CHARACTER() != nullptr);

    if ( en != 0 ) {
        fc->addNameFn = &FeatCtx::addFeatureNameString;
        for (auto i : ctx->nameEntryStatement())
            visitNameEntryStatement(i);
        fc->addCVNameID(en);
    } else
        fc->addCVParametersCharValue(getNum<uint32_t>(TOK(ctx->genNum())->getText(), 0));
    return nullptr;
}

// --------------------------------- Tables ----------------------------------

antlrcpp::Any FeatVisitor::visitTable_BASE(FeatParser::Table_BASEContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::baseFile;
    if ( stage == vExtract ) {
        fc->startTable(fc->str2tag(TOK(ctx->BASE(0))->getText()));
    }

    for (auto i : ctx->baseStatement())
        visitBaseStatement(i);

    if ( stage == vExtract ) {
        if ( fc->axistag_count != 0 )
            fc->featMsg(hotERROR, fc->axistag_visitor, fc->axistag_token,
                        "BaseTagList without corresponding BaseScriptList");
        fc->axistag_count = 0;
        fc->axistag_token = nullptr;
        fc->axistag_visitor = nullptr;
    }

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitAxisTags(FeatParser::AxisTagsContext *ctx) {
    if ( stage != vExtract )
        return nullptr;
    assert(ctx->HA_BTL() != nullptr || ctx->VA_BTL() != nullptr);

    if ( fc->axistag_count != 0 )
        fc->featMsg(hotERROR, fc->axistag_visitor, fc->axistag_token,
                    "BaseTagList without corresponding BaseScriptList");

    fc->axistag_vert = ctx->VA_BTL() != nullptr;
    fc->axistag_count = ctx->tag().size();
    fc->axistag_token = ctx->getStart();
    fc->axistag_visitor = this;

    TOK(ctx);
    if ( fc->axistag_vert ) {
        if ( fc->sawBASEvert )
            fc->featMsg(hotERROR, "VertAxis.BaseTagList must only be specified once");
        fc->sawBASEvert = true;
    } else {
        if ( fc->sawBASEhoriz )
            fc->featMsg(hotERROR, "HorizAxis.BaseTagList must only be specified once");
        fc->sawBASEhoriz = true;
    }

    std::vector<Tag> tv;
    tv.reserve(fc->axistag_count);
    for (auto t : ctx->tag())
        tv.push_back(fc->str2tag(TOK(t)->getText()));
    BASESetBaselineTags(fc->g, fc->axistag_vert, tv.size(), tv.data());

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitAxisScripts(FeatParser::AxisScriptsContext *ctx) {
    if ( stage != vExtract )
        return nullptr;
    assert(ctx->HA_BSL() != nullptr || ctx->VA_BSL() != nullptr);

    if ( ctx->HA_BSL() != nullptr && fc->axistag_vert )
        fc->featMsg(hotERROR, "expecting \"VertAxis.BaseScriptList\"");
    else if ( ctx->VA_BSL() != nullptr && !fc->axistag_vert )
        fc->featMsg(hotERROR, "expecting \"HorizAxis.BaseScriptList\"");

    for (auto bs : ctx->baseScript())
        translateBaseScript(bs, fc->axistag_vert, fc->axistag_count);

    fc->axistag_count = 0;
    fc->axistag_token = nullptr;
    fc->axistag_visitor = nullptr;

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_GDEF(FeatParser::Table_GDEFContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::gdefFile;
    if ( stage == vExtract ) {
        fc->startTable(fc->str2tag(TOK(ctx->GDEF(0))->getText()));
    }

    for (auto i : ctx->gdefStatement())
        visitGdefStatement(i);

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitGdefGlyphClass(FeatParser::GdefGlyphClassContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    GNode *gc[4] = {nullptr};
    assert(ctx->glyphClassOptional().size() == 4);

    for (int i = 0; i < 4; ++i) {
        auto gco = ctx->glyphClassOptional(i);
        if ( gco->glyphClass() != nullptr )
            gc[i] = getGlyphClass(gco->glyphClass(), false);
    }
    fc->setGDEFGlyphClassDef(gc[0], gc[1], gc[2], gc[3]);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitGdefAttach(FeatParser::GdefAttachContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    GNode *pat = getLookupPattern(ctx->lookupPattern(), false);
    if (pat->nextSeq != NULL)
        fc->featMsg(hotERROR,
                    "Only one glyph|glyphClass may be present per"
                    " AttachTable statement");

    for (auto n : ctx->NUM()) {
        int s = getNum<int16_t>(TOK(n)->getText(), 10);
        GNode *next = pat;
        while (next != NULL) {
            if ( fc->g->ctx.GDEFp->addAttachEntry(next, s) )
                fc->featMsg(hotWARNING, "Skipping duplicate contour index %d", s);
            next = next->nextCl;
        }
    }
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitGdefLigCaretPos(FeatParser::GdefLigCaretPosContext *ctx) {
    if ( stage != vExtract )
        return nullptr;
    translateGdefLigCaret(ctx->lookupPattern(), ctx->NUM(), 1);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitGdefLigCaretIndex(FeatParser::GdefLigCaretIndexContext *ctx) {
    if ( stage != vExtract )
        return nullptr;
    translateGdefLigCaret(ctx->lookupPattern(), ctx->NUM(), 2);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_head(FeatParser::Table_headContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::headFile;
    if ( stage == vExtract ) {
        fc->startTable(fc->str2tag(TOK(ctx->HEAD(0))->getText()));
    }

    for (auto i : ctx->headStatement())
        visitHeadStatement(i);

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitHead(FeatParser::HeadContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    const std::string s = TOK(ctx->POINTNUM())->getText();
    if ( s[0] == '-' )
        fc->featMsg(hotERROR, "Font revision numbers must be positive");

    fc->setFontRev(s);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_hhea(FeatParser::Table_hheaContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::hheaFile;
    if ( stage == vExtract ) {
        fc->startTable(fc->str2tag(TOK(ctx->HHEA(0))->getText()));
    }

    for (auto i : ctx->hheaStatement())
        visitHheaStatement(i);

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitHhea(FeatParser::HheaContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    assert(ctx->NUM() != nullptr);
    int16_t v = getNum<int16_t>(TOK(ctx->NUM())->getText(), 10);
    if ( TOK(ctx->CARET_OFFSET()) != nullptr )
        hheaSetCaretOffset(fc->g, v);
    else if ( TOK(ctx->ASCENDER()) != nullptr )
        fc->g->font.hheaAscender = v;
    else if ( TOK(ctx->DESCENDER()) != nullptr )
        fc->g->font.hheaDescender = v;
    else {
        assert(TOK(ctx->LINE_GAP()) != nullptr);
        fc->g->font.hheaLineGap = v;
    }
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_vhea(FeatParser::Table_vheaContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::vheaFile;
    if ( stage == vExtract ) {
        fc->startTable(fc->str2tag(TOK(ctx->VHEA(0))->getText()));
    }

    for (auto i : ctx->vheaStatement())
        visitVheaStatement(i);

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitVhea(FeatParser::VheaContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    assert(ctx->NUM() != nullptr);
    int16_t v = getNum<int16_t>(TOK(ctx->NUM())->getText(), 10);
    if ( TOK(ctx->VERT_TYPO_ASCENDER()) != nullptr )
        fc->g->font.VertTypoAscender = v;
    else if ( TOK(ctx->VERT_TYPO_DESCENDER()) != nullptr )
        fc->g->font.VertTypoDescender = v;
    else {
        assert(TOK(ctx->VERT_TYPO_LINE_GAP()) != nullptr);
        fc->g->font.VertTypoLineGap = v;
    }
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_name(FeatParser::Table_nameContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::nameFile;
    if ( stage == vExtract ) {
        fc->startTable(fc->str2tag(TOK(ctx->NAME(0))->getText()));
    }

    for (auto i : ctx->nameStatement())
        visitNameStatement(i);

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitNameID(FeatParser::NameIDContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    long v[4] {-1, -1, -1, -1};
    assert(ctx->genNum().size() == 1 || ctx->genNum().size() == 2 ||
           ctx->genNum().size() == 4);

    for (size_t i = 0; i < ctx->genNum().size(); ++i)
        v[i] = getNum<uint16_t>(TOK(ctx->genNum(i))->getText());

    if ( fc->sawSTAT && v[0] > 255 )
        fc->featMsg(hotFATAL, "name table should be defined before "
                              "STAT table with nameids above 255");
    if ( fc->sawCVParams && v[0] > 255)
        fc->featMsg(hotFATAL, "name table should be defined before "
                              "GSUB cvParameters with nameids above 255");
    if ( fc->sawFeatNames && v[0] > 255)
        fc->featMsg(hotFATAL, "name table should be defined before "
                              "GSUB featureNames with nameids above 255");
    if ( ctx->genNum().size() > 1 &&
         v[1] != HOT_NAME_MS_PLATFORM && v[1] != HOT_NAME_MAC_PLATFORM ) {
        TOK(ctx->genNum(1));
        fc->featMsg(hotFATAL, "platform id must be %d or %d",
                              HOT_NAME_MS_PLATFORM, HOT_NAME_MAC_PLATFORM);
    }
    fc->addNameString(v[1], v[2], v[3], v[0], TOK(ctx->STRVAL())->getText());
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_vmtx(FeatParser::Table_vmtxContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::vmtxFile;
    if ( stage == vExtract ) {
        fc->startTable(fc->str2tag(TOK(ctx->VMTX(0))->getText()));
    }

    for (auto i : ctx->vmtxStatement())
        visitVmtxStatement(i);

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitVmtx(FeatParser::VmtxContext *ctx) {
    if ( stage != vExtract )
        return nullptr;
    GID gid = getGlyph(ctx->glyph(), false);
    int16_t v = getNum<int16_t>(TOK(ctx->NUM())->getText(), 10);
    TOK(ctx);
    if ( ctx->VERT_ORIGIN_Y() != nullptr )
        hotAddVertOriginY(fc->g, gid, v);
    else {
        assert(ctx->VERT_ADVANCE_Y() != nullptr);
        hotAddVertAdvanceY(fc->g, gid, v);
    }
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_STAT(FeatParser::Table_STATContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::statFile;
    if ( stage == vExtract ) {
        fc->sawSTAT = true;
        fc->startTable(fc->str2tag(TOK(ctx->STAT(0))->getText()));
    }

    for (auto i : ctx->statStatement())
        visitStatStatement(i);

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitDesignAxis(FeatParser::DesignAxisContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::nameEntryFile;

    if ( stage == vExtract ) {
        fc->featNameID = 0;
        fc->addNameFn = &FeatCtx::addUserNameString;
    }

    for (auto i : ctx->nameEntryStatement())
        visitNameEntryStatement(i);

    if ( stage == vExtract ) {
        STATAddDesignAxis(fc->g, fc->str2tag(TOK(ctx->tag())->getText()),
                          fc->featNameID,
                          getNum<uint16_t>(TOK(ctx->NUM())->getText()));
        fc->featNameID = 0;
    }

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitNameEntry(FeatParser::NameEntryContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    long v[3] {-1, -1, -1};
    assert(ctx->genNum().size() == 0 || ctx->genNum().size() == 1 ||
           ctx->genNum().size() == 3);

    for (size_t i = 0; i < ctx->genNum().size(); ++i)
        v[i] = getNum<uint16_t>(TOK(ctx->genNum(i))->getText());

    if ( ctx->genNum().size() > 0 &&
         v[0] != HOT_NAME_MS_PLATFORM && v[0] != HOT_NAME_MAC_PLATFORM ) {
        TOK(ctx->genNum(0));
        fc->featMsg(hotERROR, "platform id must be %d or %d",
                    HOT_NAME_MS_PLATFORM, HOT_NAME_MAC_PLATFORM);
    }

    (fc->*(fc->addNameFn))(v[0], v[1], v[2], TOK(ctx->STRVAL())->getText());
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitAxisValue(FeatParser::AxisValueContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::axisValueFile;
    if ( stage == vExtract ) {
        fc->featNameID = fc->stat.flags = fc->stat.format = fc->stat.prev = 0;
        fc->stat.axisTags.clear();
        fc->stat.values.clear();
        fc->addNameFn = &FeatCtx::addUserNameString;
    }

    for (auto i : ctx->axisValueStatement())
        visitAxisValueStatement(i);

    if ( stage == vExtract ) {
        if ( fc->stat.format == 0 )
            fc->featMsg(hotERROR, "AxisValue missing location statement");
        if ( fc->featNameID == 0 )
            fc->featMsg(hotERROR, "AxisValue missing name entry");
        STATAddAxisValueTable(fc->g, fc->stat.format, fc->stat.axisTags.data(),
                              fc->stat.values.data(), fc->stat.values.size(),
                              fc->stat.flags, fc->featNameID,
                              fc->stat.min, fc->stat.max);
    }
    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitAxisValueFlags(FeatParser::AxisValueFlagsContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    if ( ctx->AXIS_OSFA().size() != 0 )
        fc->stat.flags |= 0x1;
    if ( ctx->AXIS_EAVN().size() != 0 )
        fc->stat.flags |= 0x2;

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitAxisValueLocation(FeatParser::AxisValueLocationContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    Tag t = fc->str2tag(TOK(ctx->tag())->getText());
    fc->stat.format = 1;
    Fixed v = getFixed<Fixed>(ctx->fixedNum(0));
    if ( ctx->fixedNum().size() > 1 ) {
        fc->stat.format = 3;
        fc->stat.min = getFixed<Fixed>(ctx->fixedNum(1));
    }
    if ( ctx->fixedNum().size() > 2 ) {
        fc->stat.format = 2;
        fc->stat.max = getFixed<Fixed>(ctx->fixedNum(2));
    }
    if ( fc->stat.prev != 0 && (fc->stat.prev != 1 || fc->stat.format != fc->stat.prev) )
        fc->featMsg(hotERROR, "AxisValue with unsupported multiple location statements");
    fc->stat.axisTags.push_back(t);
    fc->stat.values.push_back(v);
    fc->stat.prev = fc->stat.format;

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitElidedFallbackName(FeatParser::ElidedFallbackNameContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::nameEntryFile;
    if ( stage == vExtract ) {
        fc->featNameID = 0;
        fc->addNameFn = &FeatCtx::addUserNameString;
    }

    for (auto i : ctx->nameEntryStatement())
        visitNameEntryStatement(i);

    if ( stage == vExtract ) {
        if ( !STATSetElidedFallbackNameID(fc->g, fc->featNameID) )
            fc->featMsg(hotERROR, "ElidedFallbackName already defined.");
        fc->featNameID = 0;
    }
    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitElidedFallbackNameID(FeatParser::ElidedFallbackNameIDContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    uint16_t v = getNum<uint16_t>(TOK(ctx->genNum())->getText());
    if ( !STATSetElidedFallbackNameID(fc->g, v) )
        fc->featMsg(hotERROR, "ElidedFallbackName already defined.");
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_OS_2(FeatParser::Table_OS_2Context *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::os_2File;
    if ( stage == vExtract ) {
        fc->startTable(fc->str2tag(TOK(ctx->OS_2(0))->getText()));
    }

    for (auto i : ctx->os_2Statement())
        visitOs_2Statement(i);

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitOs_2(FeatParser::Os_2Context *ctx) {
    if ( stage != vExtract )
        return nullptr;

    if ( ctx->num != nullptr ) {
        int16_t v = getNum<int16_t>(TOK(ctx->num)->getText(), 10);
        if ( ctx->TYPO_ASCENDER() != nullptr )
            fc->g->font.TypoAscender = v;
        else if ( ctx->TYPO_DESCENDER() != nullptr )
            fc->g->font.TypoDescender = v;
        else if ( ctx->TYPO_LINE_GAP() != nullptr )
            fc->g->font.TypoLineGap = v;
        else if ( ctx->WIN_ASCENT() != nullptr )
            fc->g->font.winAscent = v;
        else if ( ctx->WIN_DESCENT() != nullptr )
            fc->g->font.winDescent = v;
        else if ( ctx->X_HEIGHT() != nullptr )
            fc->g->font.win.XHeight = v;
        else {
            assert(ctx->CAP_HEIGHT() != nullptr);
            fc->g->font.win.CapHeight = v;
        }
    } else if ( ctx->unum != nullptr ) {
        uint16_t v = getNum<uint16_t>(TOK(ctx->unum)->getText(), 10);
        if ( ctx->FS_TYPE() != nullptr || ctx->FS_TYPE_v() != nullptr )
            OS_2SetFSType(fc->g, v);
        else if ( ctx->WEIGHT_CLASS() != nullptr )
            OS_2SetWeightClass(fc->g, v);
        else if ( ctx->WIDTH_CLASS() != nullptr )
            OS_2SetWidthClass(fc->g, v);
        else if ( ctx->OS2_LOWER_OP_SIZE() != nullptr )
            OS_2LowerOpticalPointSize(fc->g, v);
        else {
            assert(ctx->OS2_UPPER_OP_SIZE() != nullptr);
            OS_2UpperOpticalPointSize(fc->g, v);
        }
    } else if ( ctx->gnum != nullptr ) {
        assert(ctx->FAMILY_CLASS() != nullptr);
        OS_2FamilyClass(fc->g, getNum<uint16_t>(TOK(ctx->gnum)->getText()));
    } else if ( ctx->STRVAL() != nullptr ) {
        assert(ctx->VENDOR() != nullptr);
        fc->addVendorString(TOK(ctx->STRVAL())->getText());
    } else if ( ctx->PANOSE() != nullptr ) {
        assert(ctx->NUM().size() == 10);
        uint8_t p[10];
        for (size_t i=0; i < 10; ++i)
            p[i] = getNum<uint8_t>(TOK(ctx->NUM(i))->getText(), 10);
        OS_2SetPanose(fc->g, (char *) p);
    } else if ( ctx->UNICODE_RANGE() != nullptr ) {
        std::vector<uint16_t> ur(kLenUnicodeList, FeatCtx::kCodePageUnSet);
        size_t s = std::min(ctx->NUM().size(), (size_t)kLenUnicodeList);
        for (size_t i=0; i < s; ++i)
            ur[i] = getNum<uint16_t>(TOK(ctx->NUM(i))->getText(), 10);
        TOK(ctx->UNICODE_RANGE());
        fc->setUnicodeRange((short *) ur.data());
    } else {
        assert(ctx->CODE_PAGE_RANGE() != nullptr);
        std::vector<uint16_t> cpr(kLenCodePageList, FeatCtx::kCodePageUnSet);
        size_t s = std::min(ctx->NUM().size(), (size_t)kLenCodePageList);
        for (size_t i=0; i < s; ++i)
            cpr[i] = getNum<uint16_t>(TOK(ctx->NUM(i))->getText(), 10);
        TOK(ctx->CODE_PAGE_RANGE());
        fc->setCodePageRange((short *) cpr.data());
    }

    return nullptr;
}

// -------------------------- Translation Visitors ----------------------------

void FeatVisitor::translateBaseScript(FeatParser::BaseScriptContext *ctx,
                                      bool vert, size_t cnt) {
    Tag script = fc->str2tag(TOK(ctx->script)->getText());
    Tag db = fc->str2tag(TOK(ctx->db)->getText());

    std::vector<int16_t> sv;
    sv.reserve(cnt);

    if ( ctx->NUM().size() != cnt ) {
        if ( ctx->NUM().size() > cnt )
            sv.reserve(ctx->NUM().size());
        TOK(ctx);
        fc->featMsg(hotERROR, "The number of coordinates is not equal to "
                              "the number of baseline tags");
    }

    for (auto n : ctx->NUM())
        sv.push_back(getNum<int16_t>(TOK(n)->getText(), 10));

    BASEAddScript(fc->g, vert, script, db, sv.data());
}

void FeatVisitor::translateGdefLigCaret(FeatParser::LookupPatternContext *pctx,
                                 std::vector<antlr4::tree::TerminalNode *> nv,
                                 unsigned short format) {
    assert(stage == vExtract);

    GNode *pat = getLookupPattern(pctx, false);
    if (pat->nextSeq != NULL)
        fc->featMsg(hotERROR, "Only one glyph|glyphClass may be present per"
                              " LigatureCaret statement");

    std::vector<uint16_t> sv;
    sv.reserve(nv.size());
    for (auto n : nv)
        sv.push_back(getNum<uint16_t>(TOK(n)->getText(), 10));

    GNode *next = pat;
    while ( next != nullptr ) {
        fc->g->ctx.GDEFp->addLigCaretEntry(next, sv, format);
        next = next->nextCl;
    }
}

bool FeatVisitor::translateAnchor(FeatParser::AnchorContext *ctx,
                                  int componentIndex) {
    FeatCtx::AnchorDef a;

    if ( ctx->KNULL() != nullptr ) {
        fc->addAnchorByValue(a, true, componentIndex);
        return true;
    } else if ( ctx->name != NULL ) {
        fc->addAnchorByName(TOK(ctx->name)->getText(), componentIndex);
    } else {
        assert(ctx->xval != nullptr && ctx->yval != nullptr);
        a.x = getNum<int16_t>(TOK(ctx->xval)->getText(), 10);
        a.y = getNum<int16_t>(TOK(ctx->yval)->getText(), 10);
        if ( ctx->cp != nullptr ) {
            a.contourpoint = getNum<uint16_t>(TOK(ctx->cp)->getText(), 10);
            a.hasContour = true;
        }
        fc->addAnchorByValue(a, false, componentIndex);
    }
    return false;
}

// --------------------------- Retrieval Visitors -----------------------------

void FeatVisitor::getValueRecord(FeatParser::ValueRecordContext *ctx,
                                 MetricsInfo &mi) {
    if ( ctx->valuename != nullptr )
        fc->getValueDef(TOK(ctx->valuename)->getText(), mi);
    else
        getValueLiteral(ctx->valueLiteral(), mi);
}

void FeatVisitor::getValueLiteral(FeatParser::ValueLiteralContext *ctx,
                                  MetricsInfo &mi) {
    mi.cnt = ctx->NUM().size();
    assert(mi.cnt == 1 || mi.cnt == 4);
    for (int16_t i = 0; i < mi.cnt; ++i)
        mi.metrics[i] = getNum<int16_t>(TOK(ctx->NUM(i))->getText(), 10);
}

GNode *FeatVisitor::getLookupPattern(FeatParser::LookupPatternContext *ctx,
                                     bool markedOK) {
    GNode *ret, **insert = &ret;
    assert(stage == vExtract);

    for (auto &pe : ctx->lookupPatternElement()) {
        *insert = getLookupPatternElement(pe, markedOK);
        if ( (*insert)->flags & FEAT_LOOKUP_NODE ) {
            (*insert)->flags &= ~FEAT_LOOKUP_NODE;
            ret->flags |= FEAT_LOOKUP_NODE;
        }
        insert = &(*insert)->nextSeq;
    }
    return ret;
}

GNode *FeatVisitor::getLookupPatternElement(FeatParser::LookupPatternElementContext *ctx,
                                            bool markedOK) {
    GNode *ret = getPatternElement(ctx->patternElement(), markedOK);
    for (auto l : ctx->label()) {
        int labelIndex = fc->getLabelIndex(TOK(l)->getText());
        ret->lookupLabels[ret->lookupLabelCount++] = labelIndex;
        if ( ret->lookupLabelCount > 255 )
            fc->featMsg(hotFATAL, "Too many lookup references in one glyph position.");
        // temporary tracking state will move to the head
        ret->flags |= FEAT_LOOKUP_NODE;
    }
    return ret;
}

GNode *FeatVisitor::concatenatePattern(GNode **loc,
                                       FeatParser::PatternContext *ctx,
                                       int flags) {
    GNode *ret = *loc, **insert = loc;
    bool first = true;
    assert(stage == vExtract);

    if ( *insert != nullptr )
        insert = &(*insert)->nextSeq;

    for (auto &pe : ctx->patternElement()) {
        ret = *insert = getPatternElement(pe, true);
        if ( first && flags != 0 )
            ret->flags |= flags;
        first = false;
        insert = &(*insert)->nextSeq;
    }
    return ret;
}

GNode *FeatVisitor::concatenatePatternElement(GNode **loc,
                                       FeatParser::PatternElementContext *ctx) {
    assert(stage == vExtract);

    if ( *loc != nullptr )
        loc = &(*loc)->nextSeq;

    *loc = getPatternElement(ctx, true);
    return *loc;
}

GNode *FeatVisitor::getPatternElement(FeatParser::PatternElementContext *ctx,
                                      bool markedOK) {
    GNode *ret;

    if ( ctx->glyph() != nullptr ) {
        ret = fc->setNewNode(getGlyph(ctx->glyph(), false));
    } else {
        assert(ctx->glyphClass() != nullptr);
        ret = getGlyphClass(ctx->glyphClass(), false);
    }
    if ( ctx->MARKER() != nullptr ) {
        if ( markedOK )
            ret->flags |= FEAT_MARKED;
        else {
            TOK(ctx->MARKER());
            fc->featMsg(hotERROR, "cannot mark a replacement glyph pattern");
        }
    }
    return ret;
}

GNode *FeatVisitor::getGlyphClass(FeatParser::GlyphClassContext *ctx,
                                  bool dontcopy) {
    assert(stage == vExtract);
    getGlyphClassAsCurrentGC(ctx, nullptr, dontcopy);
    TOK(ctx);
    return fc->finishCurrentGC();
}

GID FeatVisitor::getGlyph(FeatParser::GlyphContext *ctx, bool allowNotDef) {
    assert(stage == vExtract);
    if ( ctx->CID() != nullptr )
        return fc->cid2gid(TOK(ctx->CID())->getText());
    else {
        assert(ctx->glyphName() != nullptr);
        return fc->mapGName2GID(TOK(ctx->glyphName())->getText(), allowNotDef);
    }
}

// -------------------------------- Utility ----------------------------------

void FeatVisitor::getGlyphClassAsCurrentGC(FeatParser::GlyphClassContext *ctx,
                                          antlr4::tree::TerminalNode *target_gc,
                                           bool dontcopy) {
    assert(stage == vExtract);
    if ( ctx->GCLASS() != nullptr && dontcopy ) {
        fc->openAsCurrentGC(TOK(ctx->GCLASS())->getText());
        return;
    }

    TOK(ctx);
    if ( target_gc != nullptr )
        fc->defineCurrentGC(TOK(target_gc)->getText());
    else
        fc->resetCurrentGC();

    if ( ctx->gcLiteral() != nullptr ) {
        addGCLiteralToCurrentGC(ctx->gcLiteral());
    } else {
        assert(ctx->GCLASS() != nullptr);
        fc->addGlyphClassToCurrentGC(TOK(ctx->GCLASS())->getText());
    }
    if ( fc->curGCHead != nullptr )
        fc->curGCHead->flags |= FEAT_GCLASS;
}

void FeatVisitor::addGCLiteralToCurrentGC(FeatParser::GcLiteralContext *ctx) {
    assert(stage == vExtract);
    for (auto &gcle : ctx->gcLiteralElement()) {
        if ( gcle->GCLASS() != nullptr ) {
            fc->addGlyphClassToCurrentGC(TOK(gcle->GCLASS())->getText());
        } else {
            assert(gcle->startg != nullptr);
            /* The tokenizing stage doesn't separate a hyphen from a glyph
             * name if there are no spaces. So startg could be something like
             * "a-r". If it is then "a-r - z" is an error, so if endg is
             * defined we just assume startg is a normal glyphname and let
             * the calls handle any errors.
             *
             * XXX The grammar doesn't currently parse "a- z" as a range.
             */
            if ( gcle->endg != nullptr ) {
                GID sgid = getGlyph(gcle->startg, false);
                GID egid = getGlyph(gcle->endg, false);
                fc->addRangeToCurrentGC(sgid, egid,
                                        TOK(gcle->startg)->getText(),
                                        gcle->endg->getText());
            } else {
                auto g = gcle->startg;
                GID gid = getGlyph(g, true);
                if ( gid == GID_UNDEF ) {  // Could be a range
                    // Can't be a CID because that pattern doesn't have a hyphen
                    assert(g->CID() == nullptr && g->glyphName() != nullptr);
                    auto gn = TOK(g->glyphName())->getText();
                    auto hpos = gn.find('-');
                    if ( hpos == std::string::npos ) {
                        gid = getGlyph(g, false);  // For error
                        assert(gid == GID_NOTDEF);
                        // Add notdef so processing can continue
                        fc->addGlyphToCurrentGC(gid);
                        return;
                    }
                    auto sgname = gn.substr(0, hpos);
                    auto egname = gn.substr(hpos+1, std::string::npos);
                    gid = fc->mapGName2GID(sgname, false);
                    GID egid = fc->mapGName2GID(egname, false);
                    // XXX Letting this call handle undefined glyphs
                    fc->addRangeToCurrentGC(gid, egid, sgname, egname);
                } else
                    fc->addGlyphToCurrentGC(gid);
            }
        }
    }
}

Tag FeatVisitor::checkTag(FeatParser::TagContext *start,
                          FeatParser::TagContext *end) {
    Tag stag = TAG_UNDEF, etag = TAG_UNDEF;
    if ( start != NULL )
        stag = fc->str2tag(TOK(start)->getText());
    if ( end != NULL )
        etag = fc->str2tag(TOK(end)->getText());

    if ( stag != etag )
        fc->featMsg(hotERROR, "End tag %c%c%c%c does not match "
                              "start tag %c%c%c%c.",
                    TAG_ARG(etag), TAG_ARG(stag));

    return stag;
}

void FeatVisitor::checkLabel(FeatParser::LabelContext *start,
                             FeatParser::LabelContext *end) {
    if ( start == nullptr || end == nullptr ||
         start->getText() != end->getText() )
        fc->featMsg(hotERROR, "End label %s does not match start label %s.",
                    TOK(end)->getText().c_str(), start->getText().c_str());
}

template <typename T>
T FeatVisitor::getNum(const std::string &str, int base) {
    char *end;

    int64_t v = strtoll(str.c_str(), &end, base);
    if ( end == str.c_str() )
        fc->featMsg(hotERROR, "Could not parse numeric string");

    using NL = std::numeric_limits<T>;
    if ( v < NL::min() || v > NL::max() ) {
        fc->featMsg(hotERROR, "Number not in range [%ld, %ld]",
                    (long) NL::min(), (long) NL::max());
    }
    return (T) v;
}

template <typename T>
T FeatVisitor::getFixed(FeatParser::FixedNumContext *ctx, bool param) {
    float mult = param ? 1.0 : 65536.0;

    if ( ctx->NUM() != nullptr )
        return (T) mult * getNum<T>(TOK(ctx->NUM())->getText(), 10);

    mult = param ? 10.0 : 65536.0;

    assert(ctx->POINTNUM() != nullptr);
    auto str = TOK(ctx->POINTNUM())->getText();
    char *end;
    long v = (long)floor(0.5 + mult*strtod(str.c_str(), &end));
    if ( end == str.c_str() )
        fc->featMsg(hotERROR, "Could not parse numeric string");

    using NL = std::numeric_limits<T>;
    if ( v < NL::min() || v > NL::max() ) {
        if ( param )
            fc->featMsg(hotERROR, "Number not in range [%ld, %ld]",
                        (long) NL::min(), (long) NL::max());
        else
            fc->featMsg(hotERROR, "Number not in range [-32768.0, 32767.99998]");
    }
    return (T) v;
}
