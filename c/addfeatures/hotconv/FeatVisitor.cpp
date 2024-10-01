/* Copyright 2021 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 * This software is licensed as OpenSource, under the Apache License, Version 2.0.
 * This license is available at: http://opensource.org/licenses/Apache-2.0.
 */

#include "FeatVisitor.h"

#include <math.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include "BASE.h"
#include "GDEF.h"
#include "GPOS.h"
#include "GSUB.h"
#include "hmtx.h"
#include "name.h"
#include "OS_2.h"
#include "otl.h"
#include "STAT.h"

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
        fc->featMsg(sFATAL, "Can't include [%s]; maximum include levels <%d> reached",
                    pathname.c_str(), MAX_INCL);
        return;
    }
    if ( parent == nullptr ) {
        stream.open(pathname);
        if ( !stream.is_open() ) {
            fc->featMsg(sFATAL, "Specified feature file '%s' not found", pathname.c_str());
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
            fc->featMsg(sERROR, "Included feature file '%s' not found", pathname.c_str());
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
    v.fc->g->logger->msg(sERROR, msg.c_str());
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

    Tag tag = getTag(ctx->A_LABEL());
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

    Tag stag = getTag(ctx->script);
    Tag ltag = getTag(ctx->lang);
    fc->addLangSys(stag, ltag, true, ctx->lang);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitValueRecordDef(FeatParser::ValueRecordDefContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    MetricsInfo mi;
    getValueLiteral(ctx->valueLiteral(), mi);
    fc->addValueDef(TOK(ctx->label())->getText(), std::move(mi));

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitLocationDef(FeatParser::LocationDefContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    uint32_t loc_idx = getLocationLiteral(ctx->locationLiteral());
    fc->addLocationDef(TOK(ctx->LNAME())->getText(), loc_idx);

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitAnchorDef(FeatParser::AnchorDefContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    assert(ctx->anchorLiteral() != nullptr);
    fc->addAnchorDef(TOK(ctx->name)->getText(), std::move(getAnchorLiteral(ctx->anchorLiteral())));
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitGlyphClassAssign(FeatParser::GlyphClassAssignContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    getGlyphClassAsCurrentGC(ctx->glyphClass(), ctx->gclass(), false);
    TOK(ctx);
    fc->finishCurrentGC();

    return nullptr;
}

// ---------------- Statements (in feature and lookup blocks) -----------------

antlrcpp::Any FeatVisitor::visitFeatureUse(FeatParser::FeatureUseContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    fc->aaltAddFeatureTag(getTag(ctx->tag()));
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitScriptAssign(FeatParser::ScriptAssignContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    fc->startScriptOrLang(FeatCtx::scriptTag, getTag(ctx->tag()));
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitLangAssign(FeatParser::LangAssignContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    int lang_change = fc->startScriptOrLang(FeatCtx::languageTag, getTag(ctx->tag()));

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
            uint16_t umfIndex = fc->g->ctx.GDEFp->addMarkSetClass(getGlyphClass(e->glyphClass(), true));
            v = fc->setLkpFlagAttribute(v, otlUseMarkFilteringSet, umfIndex);
        } else {
            assert(e->MARK_ATTACHMENT_TYPE() != nullptr);
            uint16_t macIndex = fc->g->ctx.GDEFp->addGlyphMarkClass(getGlyphClass(e->glyphClass(), true));
            if ( macIndex > kMaxMarkAttachClasses )
                fc->featMsg(sERROR, "No more than 15 different class names can be used with the \"lookupflag MarkAttachmentType\". This would be a 16th.");
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
        GPat::SP targ = getLookupPattern(lp, true);
        targ->ignore_clause = true;
        if ( is_sub )
            fc->addSub(std::move(targ), nullptr, sub_type);
        else
            fc->addPos(std::move(targ), 0, false);
    }

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitSubstitute(FeatParser::SubstituteContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    GPat::SP targ, repl;
    int type = 0;

    if ( ctx->EXCEPT() != nullptr ) {
        type = GSUBChain;
        fc->syntax.numExcept++;
        for (auto i : ctx->lookupPattern()) {
            if ( i == ctx->startpat || i == ctx->endpat )
                continue;
            targ = getLookupPattern(i, true);
            targ->ignore_clause = true;
            fc->addSub(std::move(targ), nullptr, type);
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
    fc->addSub(std::move(targ), std::move(repl), type);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitMark_statement(FeatParser::Mark_statementContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    GPat::ClassRec cr;
    fc->anchorMarkInfo.clear();

    if ( ctx->glyph() != nullptr ) {
        cr.addGlyph(getGlyph(ctx->glyph(), false));
    } else {
        assert(ctx->glyphClass() != nullptr);
        cr = getGlyphClass(ctx->glyphClass(), false);
    }

    translateAnchor(ctx->anchor(), 0);

    fc->addMark(TOK(ctx->gclass())->getText(), cr);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitPosition(FeatParser::PositionContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    bool enumerate = ctx->enumtok() != nullptr;
    int type = 0;
    GPat::SP gp;

    fc->anchorMarkInfo.clear();

    if ( ctx->startpat != nullptr )
        gp = concatenatePattern(std::move(gp), ctx->startpat);

    if ( TOK(ctx->valueRecord()) != nullptr ) {
        if ( gp == nullptr ) {
            fc->featMsg(sERROR, "Glyph or glyph class must precede a value record.");
            return nullptr;
        }
        assert(gp->patternLen() > 0);
        type = GPOSSingle;
        getValueRecord(ctx->valueRecord(), gp->classes.back().metricsInfo);
        for (auto vp : ctx->valuePattern()) {
            gp = concatenatePatternElement(std::move(gp), vp->patternElement());
            if ( vp->valueRecord() )
                getValueRecord(vp->valueRecord(), gp->classes.back().metricsInfo);
        }
    } else if ( ctx->LOOKUP().size() != 0 ) {
        if ( gp == nullptr || gp->patternLen() == 0 ) {
            TOK(ctx->LOOKUP(0));
            fc->featMsg(sERROR, "Glyph or glyph class must precede a lookup reference in a contextual rule.");
            return nullptr;
        }
        type = GPOSChain;
        for (auto l : ctx->label()) {
            gp->classes.back().lookupLabels.push_back(fc->getLabelIndex(TOK(l)->getText()));
            gp->lookup_node = true;
            if ( gp->classes.back().lookupLabels.size() > 255 )
                fc->featMsg(sFATAL, "Too many lookup references in one glyph position.");
        }
        for (auto lpe : ctx->lookupPatternElement())
            gp->addClass(getLookupPatternElement(lpe, true));
    } else if ( ctx->cursiveElement() != nullptr ) {
        type = GPOSCursive;
        gp = concatenatePatternElement(std::move(gp), ctx->cursiveElement()->patternElement());
        gp->classes.back().basenode = true;
        for (auto a : ctx->cursiveElement()->anchor())
            translateAnchor(a, 0);
        if ( ctx->endpat != nullptr )
            gp = concatenatePattern(std::move(gp), ctx->endpat);
    } else if ( ctx->MARKBASE() != nullptr ) {
        type = GPOSMarkToBase;
        gp = concatenatePattern(std::move(gp), ctx->midpat, true);
        auto sz = gp->patternLen();
        bool addedMark = false;
        for (auto mbe : ctx->baseToMarkElement()) {
            translateAnchor(mbe->anchor(), 0);
            fc->addMarkClass(TOK(mbe->gclass())->getText());
            if ( mbe->MARKER() != nullptr ) {
                GPat::ClassRec cr {fc->lookupGlyphClass(mbe->gclass()->getText())};
                cr.marknode = true;
                gp->addClass(std::move(cr));
                addedMark = true;
            }
        }
        if (addedMark)
            gp->classes[sz-1].marked = true;
        if ( ctx->endpat != nullptr )
            gp = concatenatePattern(std::move(gp), ctx->endpat);
    } else if ( ctx->markligtok() != nullptr ) {
        type = GPOSMarkToLigature;
        int componentIndex = 0;
        gp = concatenatePattern(std::move(gp), ctx->midpat, true);
        auto sz = gp->patternLen();
        bool addedMark = false;
        for (auto lme : ctx->ligatureMarkElement()) {
            bool isNULL = translateAnchor(lme->anchor(), componentIndex);
            if ( lme->MARK() ) {
                fc->addMarkClass(TOK(lme->gclass())->getText());
            } else if ( !isNULL )
                fc->featMsg(sERROR, "In mark to ligature, non-null anchor must be followed by a mark class.");
            if ( lme->LIG_COMPONENT() != nullptr )
                componentIndex++;
            if ( lme->MARKER() != nullptr ) {
                GPat::ClassRec cr {fc->lookupGlyphClass(lme->gclass()->getText())};
                cr.marknode = true;
                gp->addClass(std::move(cr));
                addedMark = true;
            }
        }
        if (addedMark)
            gp->classes[sz-1].marked = true;
        if ( ctx->endpat != nullptr )
            gp = concatenatePattern(std::move(gp), ctx->endpat);
    } else {
        assert(ctx->MARK() != nullptr);
        type = GPOSMarkToMark;
        gp = concatenatePattern(std::move(gp), ctx->midpat, true);
        auto sz = gp->patternLen();
        bool addedMark = false;
        for (auto mbe : ctx->baseToMarkElement()) {
            translateAnchor(mbe->anchor(), 0);
            fc->addMarkClass(TOK(mbe->gclass())->getText());
            if ( mbe->MARKER() != nullptr ) {
                GPat::ClassRec cr {fc->lookupGlyphClass(mbe->gclass()->getText())};
                cr.marknode = true;
                gp->addClass(std::move(cr));
                addedMark = true;
            }
        }
        if (addedMark)
            gp->classes[sz-1].marked = true;
        if ( ctx->endpat != nullptr )
            gp = concatenatePattern(std::move(gp), ctx->endpat);
    }
    assert(gp != nullptr);
    fc->addPos(std::move(gp), type, enumerate);

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitParameters(FeatParser::ParametersContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    size_t s = ctx->fixedNum().size();
    if ( s > MAX_FEAT_PARAM_NUM ) {
        TOK(ctx->fixedNum(0));
        fc->featMsg(sERROR, "Too many parameter values.");
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
        fc->featMsg(sERROR, "platform id must be %d or %d",
                    HOT_NAME_MS_PLATFORM, HOT_NAME_MAC_PLATFORM);
    }

    fc->addSizeNameString(v[0], v[1], v[2], fc->unescString(TOK(ctx->STRVAL())->getText()));
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitFeatureNames(FeatParser::FeatureNamesContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::nameEntryFile;

    if ( stage == vExtract ) {
        fc->sawFeatNames = true;
        fc->featNameID =  fc->g->ctx.name->reserveUserID();;
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
        fc->startTable(getTag(ctx->BASE(0)));
    }

    for (auto i : ctx->baseStatement())
        visitBaseStatement(i);

    if ( stage == vExtract ) {
        if ( fc->axistag_count != 0 )
            fc->featMsg(sERROR, fc->axistag_visitor, fc->axistag_token,
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
        fc->featMsg(sERROR, fc->axistag_visitor, fc->axistag_token,
                    "BaseTagList without corresponding BaseScriptList");

    fc->axistag_vert = ctx->VA_BTL() != nullptr;
    fc->axistag_count = ctx->tag().size();
    fc->axistag_token = ctx->getStart();
    fc->axistag_visitor = this;

    TOK(ctx);
    if ( fc->axistag_vert ) {
        if ( fc->sawBASEvert )
            fc->featMsg(sERROR, "VertAxis.BaseTagList must only be specified once");
        fc->sawBASEvert = true;
    } else {
        if ( fc->sawBASEhoriz )
            fc->featMsg(sERROR, "HorizAxis.BaseTagList must only be specified once");
        fc->sawBASEhoriz = true;
    }

    std::vector<Tag> tv;
    tv.reserve(fc->axistag_count);
    for (auto t : ctx->tag())
        tv.push_back(getTag(t));
    fc->g->ctx.BASEp->setBaselineTags(fc->axistag_vert, tv);

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitAxisScripts(FeatParser::AxisScriptsContext *ctx) {
    if ( stage != vExtract )
        return nullptr;
    assert(ctx->HA_BSL() != nullptr || ctx->VA_BSL() != nullptr);

    if ( ctx->HA_BSL() != nullptr && fc->axistag_vert )
        fc->featMsg(sERROR, "expecting \"VertAxis.BaseScriptList\"");
    else if ( ctx->VA_BSL() != nullptr && !fc->axistag_vert )
        fc->featMsg(sERROR, "expecting \"HorizAxis.BaseScriptList\"");

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
        fc->startTable(getTag(ctx->GDEF(0)));
    }

    for (auto i : ctx->gdefStatement())
        visitGdefStatement(i);

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitGdefGlyphClass(FeatParser::GdefGlyphClassContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    GPat::ClassRec gc[4];
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

    GPat::SP gp = getLookupPattern(ctx->lookupPattern(), false);
    if (gp->patternLen() != 1)
        fc->featMsg(sERROR,
                    "Only one glyph|glyphClass may be present per"
                    " AttachTable statement");

    for (auto n : ctx->NUM()) {
        int s = getNum<int16_t>(TOK(n)->getText(), 10);
        for (GID gid : gp->classes[0].glyphs) {
            if ( fc->g->ctx.GDEFp->addAttachEntry(gid, s) )
                fc->featMsg(sWARNING, "Skipping duplicate contour index %d", s);
        }
    }
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitGdefLigCaretPos(FeatParser::GdefLigCaretPosContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    GPat::SP gp = getLookupPattern(ctx->lookupPattern(), false);
    if (gp->patternLen() != 1)
        fc->featMsg(sERROR, "Only one glyph|glyphClass may be present per"
                              " LigatureCaret statement");

    ValueVector vv;
    vv.reserve(ctx->singleValueLiteral().size());
    for (auto cvl : ctx->singleValueLiteral()) {
        vv.emplace_back();
        getSingleValueLiteral(cvl, vv.back());
    }

    for (GID gid : gp->classes[0].glyphs)
        fc->g->ctx.GDEFp->addLigCaretCoords(gid, vv);

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitGdefLigCaretIndex(FeatParser::GdefLigCaretIndexContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    GPat::SP gp = getLookupPattern(ctx->lookupPattern(), false);
    if (gp->patternLen() != 1)
        fc->featMsg(sERROR, "Only one glyph|glyphClass may be present per"
                              " LigatureCaretByIndex statement");

    std::vector<uint16_t> pv;
    pv.reserve(ctx->NUM().size());
    for (auto n : ctx->NUM())
        pv.push_back(getNum<uint16_t>(TOK(n)->getText(), 10));

    for (GID gid : gp->classes[0].glyphs)
        fc->g->ctx.GDEFp->addLigCaretPoints(gid, pv);

    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_head(FeatParser::Table_headContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::headFile;
    if ( stage == vExtract ) {
        fc->startTable(getTag(ctx->HEAD(0)));
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
        fc->featMsg(sERROR, "Font revision numbers must be positive");

    fc->setFontRev(s);
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_hhea(FeatParser::Table_hheaContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::hheaFile;
    if ( stage == vExtract ) {
        fc->startTable(getTag(ctx->HHEA(0)));
    }

    for (auto i : ctx->hheaStatement())
        visitHheaStatement(i);

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitHhea(FeatParser::HheaContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    if (ctx->NUM() != nullptr) {
        int16_t v = getNum<int16_t>(TOK(ctx->NUM())->getText(), 10);
        if ( TOK(ctx->ASCENDER()) != nullptr )
            fc->g->font.hheaAscender = v;
        else if ( TOK(ctx->DESCENDER()) != nullptr )
            fc->g->font.hheaDescender = v;
        else {
            assert(TOK(ctx->LINE_GAP()) != nullptr);
            fc->g->font.hheaLineGap = v;
        }
    } else {
        assert(ctx->singleValueLiteral() != nullptr);
        VarValueRecord vvr;
        getSingleValueLiteral(ctx->singleValueLiteral(), vvr);
        if ( TOK(ctx->CARET_OFFSET()) != nullptr )
            fc->g->font.hheaCaretOffset = std::move(vvr);
        else if ( TOK(ctx->CARET_SLOPE_RISE()) != nullptr )
            fc->g->font.hheaCaretSlopeRise = std::move(vvr);
        else {
            assert(ctx->CARET_SLOPE_RUN() != nullptr);
            fc->g->font.hheaCaretSlopeRun = std::move(vvr);
        }
    }
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_vhea(FeatParser::Table_vheaContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::vheaFile;
    if ( stage == vExtract ) {
        fc->startTable(getTag(ctx->VHEA(0)));
    }

    for (auto i : ctx->vheaStatement())
        visitVheaStatement(i);

    include_ep = tmp_ep;
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitVhea(FeatParser::VheaContext *ctx) {
    if ( stage != vExtract )
        return nullptr;

    assert(ctx->singleValueLiteral() != nullptr);
    VarValueRecord vvr;
    getSingleValueLiteral(ctx->singleValueLiteral(), vvr);
    if ( TOK(ctx->VERT_TYPO_ASCENDER()) != nullptr )
        fc->g->font.VertTypoAscender = std::move(vvr);
    else if ( TOK(ctx->VERT_TYPO_DESCENDER()) != nullptr )
        fc->g->font.VertTypoDescender = std::move(vvr);
    else if (TOK(ctx->VERT_TYPO_LINE_GAP()) != nullptr)
        fc->g->font.VertTypoLineGap = std::move(vvr);
    else if (TOK(ctx->CARET_OFFSET()) != nullptr)
        fc->g->font.vheaCaretOffset = std::move(vvr);
    else if (TOK(ctx->CARET_SLOPE_RISE()) != nullptr)
        fc->g->font.vheaCaretSlopeRise = std::move(vvr);
    else {
        assert(TOK(ctx->CARET_SLOPE_RUN()) != nullptr);
        fc->g->font.vheaCaretSlopeRun = std::move(vvr);
    }
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_name(FeatParser::Table_nameContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::nameFile;
    if ( stage == vExtract ) {
        fc->startTable(getTag(ctx->NAME(0)));
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
        fc->featMsg(sFATAL, "name table should be defined before "
                              "STAT table with nameids above 255");
    if ( fc->sawCVParams && v[0] > 255)
        fc->featMsg(sFATAL, "name table should be defined before "
                              "GSUB cvParameters with nameids above 255");
    if ( fc->sawFeatNames && v[0] > 255)
        fc->featMsg(sFATAL, "name table should be defined before "
                              "GSUB featureNames with nameids above 255");
    if ( ctx->genNum().size() > 1 &&
         v[1] != HOT_NAME_MS_PLATFORM && v[1] != HOT_NAME_MAC_PLATFORM ) {
        TOK(ctx->genNum(1));
        fc->featMsg(sFATAL, "platform id must be %d or %d",
                              HOT_NAME_MS_PLATFORM, HOT_NAME_MAC_PLATFORM);
    }
    fc->addNameString(v[1], v[2], v[3], v[0], fc->unescString(TOK(ctx->STRVAL())->getText()));
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_vmtx(FeatParser::Table_vmtxContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::vmtxFile;
    if ( stage == vExtract ) {
        fc->startTable(getTag(ctx->VMTX(0)));
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
    VarValueRecord vvr;
    getSingleValueLiteral(ctx->singleValueLiteral(), vvr);
    TOK(ctx);
    if ( ctx->VERT_ORIGIN_Y() != nullptr )
        hotAddVertOriginY(fc->g, gid, vvr);
    else {
        assert(ctx->VERT_ADVANCE_Y() != nullptr);
        hotAddVertAdvanceY(fc->g, gid, vvr);
    }
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_STAT(FeatParser::Table_STATContext *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::statFile;
    if ( stage == vExtract ) {
        fc->sawSTAT = true;
        fc->startTable(getTag(ctx->STAT(0)));
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
        STATAddDesignAxis(fc->g, getTag(ctx->tag()),
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
        fc->featMsg(sERROR, "platform id must be %d or %d",
                    HOT_NAME_MS_PLATFORM, HOT_NAME_MAC_PLATFORM);
    }

    (fc->*(fc->addNameFn))(v[0], v[1], v[2], fc->unescString(TOK(ctx->STRVAL())->getText()));
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
            fc->featMsg(sERROR, "AxisValue missing location statement");
        if ( fc->featNameID == 0 )
            fc->featMsg(sERROR, "AxisValue missing name entry");
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

    Tag t = getTag(ctx->tag());
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
        fc->featMsg(sERROR, "AxisValue with unsupported multiple location statements");
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
            fc->featMsg(sERROR, "ElidedFallbackName already defined.");
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
        fc->featMsg(sERROR, "ElidedFallbackName already defined.");
    return nullptr;
}

antlrcpp::Any FeatVisitor::visitTable_OS_2(FeatParser::Table_OS_2Context *ctx) {
    EntryPoint tmp_ep = include_ep;
    include_ep = &FeatParser::os_2File;
    if ( stage == vExtract ) {
        fc->startTable(getTag(ctx->OS_2(0)));
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
        VarValueRecord vvr;
        getSingleValueLiteral(ctx->num, vvr);
        if ( ctx->TYPO_ASCENDER() != nullptr )
            fc->g->font.TypoAscender = std::move(vvr);
        else if ( ctx->TYPO_DESCENDER() != nullptr )
            fc->g->font.TypoDescender = std::move(vvr);
        else if ( ctx->TYPO_LINE_GAP() != nullptr )
            fc->g->font.TypoLineGap = std::move(vvr);
        else if ( ctx->WIN_ASCENT() != nullptr )
            fc->g->font.win.ascent = std::move(vvr);
        else if ( ctx->WIN_DESCENT() != nullptr )
            fc->g->font.win.descent = std::move(vvr);
        else if ( ctx->X_HEIGHT() != nullptr )
            fc->g->font.win.XHeight = std::move(vvr);
        else if ( ctx->CAP_HEIGHT() != nullptr )
            fc->g->font.win.CapHeight = std::move(vvr);
        else if ( ctx->SUBSCRIPT_X_SIZE() != nullptr )
            fc->g->font.win.SubscriptXSize = std::move(vvr);
        else if ( ctx->SUBSCRIPT_X_OFFSET() != nullptr )
            fc->g->font.win.SubscriptXOffset = std::move(vvr);
        else if ( ctx->SUBSCRIPT_Y_SIZE() != nullptr )
            fc->g->font.win.SubscriptYSize = std::move(vvr);
        else if ( ctx->SUBSCRIPT_Y_OFFSET() != nullptr )
            fc->g->font.win.SubscriptYOffset = std::move(vvr);
        else if ( ctx->SUPERSCRIPT_X_SIZE() != nullptr )
            fc->g->font.win.SuperscriptXSize = std::move(vvr);
        else if ( ctx->SUPERSCRIPT_X_OFFSET() != nullptr )
            fc->g->font.win.SuperscriptXOffset = std::move(vvr);
        else if ( ctx->SUPERSCRIPT_Y_SIZE() != nullptr )
            fc->g->font.win.SuperscriptYSize = std::move(vvr);
        else if ( ctx->SUPERSCRIPT_Y_OFFSET() != nullptr )
            fc->g->font.win.SuperscriptYOffset = std::move(vvr);
        else if ( ctx->STRIKEOUT_SIZE() != nullptr )
            fc->g->font.win.StrikeoutSize = std::move(vvr);
        else {
            assert(ctx->STRIKEOUT_POSITION() != nullptr);
            fc->g->font.win.StrikeoutPosition = std::move(vvr);
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
        fc->addVendorString(fc->unescString(TOK(ctx->STRVAL())->getText()));
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
    Tag script = getTag(ctx->script);
    Tag db = getTag(ctx->db);

    std::vector<VarValueRecord> sv;
    sv.reserve(cnt);

    if ( TOK(ctx)->singleValueLiteral().size() != cnt )
        fc->featMsg(sERROR, "The number of coordinates is not equal to "
                            "the number of baseline tags");

    for (auto svl : ctx->singleValueLiteral()) {
        VarValueRecord vvr;
        getSingleValueLiteral(svl, vvr);
        sv.emplace_back(std::move(vvr));
    }

    fc->g->ctx.BASEp->addScript(vert, script, db, sv);
}

bool FeatVisitor::translateAnchor(FeatParser::AnchorContext *ctx,
                                  int componentIndex) {
    if ( ctx->KNULL() != nullptr ) {
        fc->addAnchorByValue(std::make_shared<AnchorMarkInfo>(), componentIndex);
        return true;
    } else if ( ctx->name != NULL ) {
        fc->addAnchorByName(TOK(ctx->name)->getText(), componentIndex);
    } else {
        assert(ctx->anchorLiteral() != nullptr);
        fc->addAnchorByValue(std::make_shared<AnchorMarkInfo>(std::move(getAnchorLiteral(ctx->anchorLiteral()))), componentIndex);
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
    if (ctx->singleValueLiteral().size() != 0) {
        assert(ctx->singleValueLiteral().size() == 1 ||
               ctx->singleValueLiteral().size() == 4);
        for (auto svl : ctx->singleValueLiteral()) {
            VarValueRecord vvr;
            getSingleValueLiteral(svl, vvr);
            mi.metrics.emplace_back(std::move(vvr));
        }
    } else {
        assert(ctx->locationMultiValueLiteral().size() != 0);
        for (auto lmvl : ctx->locationMultiValueLiteral())
            addLocationMultiValue(lmvl, mi);
    }
}

void FeatVisitor::getSingleValueLiteral(FeatParser::SingleValueLiteralContext *ctx,
                                        VarValueRecord &vvr) {
    if (ctx->NUM() != nullptr) {
        vvr.addValue(getNum<int16_t>(TOK(ctx->NUM())->getText(), 10));
    } else {
        assert(ctx->parenLocationValue() != nullptr);
        getParenLocationValue(ctx->parenLocationValue(), vvr);
    }
}

void FeatVisitor::getParenLocationValue(FeatParser::ParenLocationValueContext *ctx,
                                        VarValueRecord &vvr) {
    for (auto vll : ctx->locationValueLiteral())
        addLocationValueLiteral(vll, vvr);
    vvr.verifyDefault(fc->g->logger);
}

void FeatVisitor::addLocationValueLiteral(FeatParser::LocationValueLiteralContext *ctx,
                                          VarValueRecord &vvr) {
    uint32_t locIndex = getLocationSpecifier(ctx->locationSpecifier());
    int16_t num = getNum<int16_t>(TOK(ctx->NUM())->getText(), 10);
    vvr.addLocationValue(locIndex, num, fc->g->logger);
}

void FeatVisitor::addLocationMultiValue(FeatParser::LocationMultiValueLiteralContext *ctx,
                                        MetricsInfo &mi) {
    if (mi.metrics.size() == 0)
        mi.metrics.resize(4);
    assert(mi.metrics.size() == 4);
    assert(ctx->NUM().size() == 4);
    uint32_t locIndex = getLocationSpecifier(ctx->locationSpecifier());
    for (size_t i = 0; i < 4; i++) {
        int16_t num = getNum<int16_t>(TOK(ctx->NUM(i))->getText(), 10);
        mi.metrics[i].addLocationValue(locIndex, num, fc->g->logger);
    }
}

AnchorMarkInfo FeatVisitor::getAnchorLiteral(FeatParser::AnchorLiteralContext *ctx) {
    AnchorMarkInfo am;
    getAnchorLiteralXY(ctx->anchorLiteralXY(), am);
    if ( ctx->cp != nullptr ) {
        if (am.getXValueRecord().isVariable() || am.getYValueRecord().isVariable()) {
            TOK(ctx->cp);
            fc->featMsg(sERROR, "Cannot mix variable values with contour point in anchor");
        } else {
            am.setContourpoint(getNum<uint16_t>(TOK(ctx->cp)->getText(), 10));
        }
    }
    return am;
}

void FeatVisitor::getAnchorLiteralXY(FeatParser::AnchorLiteralXYContext *ctx,
                                     AnchorMarkInfo &am) {
    if (ctx->xval != nullptr) {
        assert(ctx->yval != nullptr);
        getSingleValueLiteral(ctx->xval, am.getXValueRecord());
        getSingleValueLiteral(ctx->yval, am.getYValueRecord());
    } else {
        assert(ctx->anchorMultiValueLiteral().size() != 0);
        for (auto amvl : ctx->anchorMultiValueLiteral())
            addAnchorMultiValue(amvl, am);
    }
}

void FeatVisitor::addAnchorMultiValue(FeatParser::AnchorMultiValueLiteralContext *ctx,
                                      AnchorMarkInfo &am) {
    assert(ctx->NUM().size() == 2);
    uint32_t locIndex = getLocationSpecifier(ctx->locationSpecifier());
    int16_t num = getNum<int16_t>(TOK(ctx->NUM(0))->getText(), 10);
    am.getXValueRecord().addLocationValue(locIndex, num, fc->g->logger);
    num = getNum<int16_t>(TOK(ctx->NUM(1))->getText(), 10);
    am.getYValueRecord().addLocationValue(locIndex, num, fc->g->logger);
}

uint32_t FeatVisitor::getLocationSpecifier(FeatParser::LocationSpecifierContext *ctx, bool errorOnNull) {
    if (ctx == nullptr) {
        if (errorOnNull)
            fc->featMsg(sERROR, "Missing location specifier");
        return 0;   // default location
    }
    if (ctx->LNAME() != nullptr)
        return fc->getLocationDef(TOK(ctx->LNAME())->getText());
    else
        return getLocationLiteral(ctx->locationLiteral());
}

uint32_t FeatVisitor::getLocationLiteral(FeatParser::LocationLiteralContext *ctx) {
    uint16_t ac = fc->getAxisCount();
    if (ac == 0) {
        TOK(ctx);
        fc->featMsg(sERROR, "Location literal in non-variable font");
        return 0;
    }
    std::vector<int16_t> l(ac, 0);
    for (auto all : ctx->axisLocationLiteral()) {
        if (!addAxisLocationLiteral(all, l))
            return 0;
    }
    return fc->locationToIndex(std::make_shared<VarLocation>(l));
}

bool FeatVisitor::addAxisLocationLiteral(FeatParser::AxisLocationLiteralContext *ctx,
                                         std::vector<var_F2dot14> &l) {
    Tag tag = getTag(ctx->tag());
    int16_t axisIndex = fc->axisTagToIndex(tag);
    if (axisIndex < 0) {
        fc->featMsg(sERROR, "Axis not found in font");
        return false;
    }
    assert(axisIndex < (int16_t)l.size());

    std::string unit = TOK(ctx->AXISUNIT())->getText();
    Fixed v = getFixed<Fixed>(ctx->fixedNum());
    if (unit == "d") {
        assert(fc->g->ds != nullptr);
        v = fc->g->ds->userizeCoord(axisIndex, v);
        assert(fc->g->ctx.axes != nullptr);
        fc->g->ctx.axes->normalizeCoord(axisIndex, v, v);
    } else if (unit == "u") {
        assert(fc->g->ctx.axes != nullptr);
        fc->g->ctx.axes->normalizeCoord(axisIndex, v, v);
    } else {
        assert(unit == "n");
    }

    var_F2dot14 f2v = FIXED_TO_F2DOT14(v);
    int8_t ladjust {0};
    assert(ctx->HYPHEN() == nullptr || ctx->PLUS() == nullptr);
    if (ctx->HYPHEN())
        ladjust = -1;
    else if (ctx->PLUS())
        ladjust = 1;
    f2v += ladjust;

    f2v = fc->validAxisLocation(f2v, ladjust);
    if (l[axisIndex] != 0) {
        TOK(ctx->tag());
        fc->featMsg(sERROR, "Already set location for axis '%c%c%c%c'.", TAG_ARG(tag));
        return false;
    }
    l[axisIndex] = f2v;
    return true;
}

GPat::SP FeatVisitor::getLookupPattern(FeatParser::LookupPatternContext *ctx,
                                       bool markedOK) {
    GPat::SP gp = std::make_unique<GPat>();
    assert(stage == vExtract);

    for (auto &pe : ctx->lookupPatternElement()) {
        gp->addClass(getLookupPatternElement(pe, markedOK));
        if (gp->classes.back().lookupLabels.size() > 0)
            gp->lookup_node = true;
    }
    return gp;
}

GPat::ClassRec FeatVisitor::getLookupPatternElement(FeatParser::LookupPatternElementContext *ctx,
                                                    bool markedOK) {
    GPat::ClassRec cr = getPatternElement(ctx->patternElement(), markedOK);
    for (auto l : ctx->label()) {
        cr.lookupLabels.push_back(fc->getLabelIndex(TOK(l)->getText()));
        if ( cr.lookupLabels.size() > 255 )
            fc->featMsg(sFATAL, "Too many lookup references in one glyph position.");
    }
    return cr;
}

GPat::SP FeatVisitor::concatenatePattern(GPat::SP gp,
                                         FeatParser::PatternContext *ctx,
                                         bool isBaseNode) {
    bool first = true;
    assert(stage == vExtract);

    if ( gp == nullptr )
        gp = std::make_unique<GPat>();

    for (auto &pe : ctx->patternElement()) {
        GPat::ClassRec cr = getPatternElement(pe, true);
        cr.basenode = first && isBaseNode;
        first = false;
        gp->addClass(std::move(cr));
    }
    return gp;
}

GPat::SP FeatVisitor::concatenatePatternElement(GPat::SP gp,
                                                FeatParser::PatternElementContext *ctx) {
    assert(stage == vExtract);

    if (gp == nullptr)
        gp = std::make_unique<GPat>();

    gp->addClass(getPatternElement(ctx, true));
    return gp;
}

GPat::ClassRec FeatVisitor::getPatternElement(FeatParser::PatternElementContext *ctx,
                                              bool markedOK) {
    GPat::ClassRec cr;

    if ( ctx->glyph() != nullptr ) {
        cr.glyphs.emplace_back(getGlyph(ctx->glyph(), false));
    } else {
        assert(ctx->glyphClass() != nullptr);
        cr = getGlyphClass(ctx->glyphClass(), false);
    }
    if ( ctx->MARKER() != nullptr ) {
        if ( markedOK )
            cr.marked = true;
        else {
            TOK(ctx->MARKER());
            fc->featMsg(sERROR, "cannot mark a replacement glyph pattern");
        }
    }
    return cr;
}

GPat::ClassRec FeatVisitor::getGlyphClass(FeatParser::GlyphClassContext *ctx,
                                          bool dontcopy) {
    GPat::ClassRec cr;
    assert(stage == vExtract);

    getGlyphClassAsCurrentGC(ctx, nullptr, dontcopy);
    TOK(ctx);
    fc->finishCurrentGC(cr);
    return cr;
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
                                           FeatParser::GclassContext *target_gc,
                                           bool dontcopy) {
    assert(stage == vExtract);
    if ( ctx->gclass() != nullptr && dontcopy ) {
        fc->openAsCurrentGC(TOK(ctx->gclass())->getText());
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
        assert(ctx->gclass() != nullptr);
        fc->addGlyphClassToCurrentGC(TOK(ctx->gclass())->getText());
    }
    fc->curGC.gclass = true;
}

void FeatVisitor::addGCLiteralToCurrentGC(FeatParser::GcLiteralContext *ctx) {
    assert(stage == vExtract);
    for (auto &gcle : ctx->gcLiteralElement()) {
        if ( gcle->gclass() != nullptr ) {
            fc->addGlyphClassToCurrentGC(TOK(gcle->gclass())->getText());
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
        stag = getTag(start);
    if ( end != NULL )
        etag = getTag(end);

    if ( stag != etag )
        fc->featMsg(sERROR, "End tag %c%c%c%c does not match "
                              "start tag %c%c%c%c.",
                    TAG_ARG(etag), TAG_ARG(stag));

    return stag;
}

Tag FeatVisitor::getTag(FeatParser::TagContext *t) {
    if (t->STRVAL() != nullptr)
        return fc->str2tag(fc->unescString(TOK(t->STRVAL())->getText()));
    else
        return fc->str2tag(TOK(t)->getText());
}

Tag FeatVisitor::getTag(antlr4::tree::TerminalNode *t) {
    return fc->str2tag(TOK(t)->getText());
}

void FeatVisitor::checkLabel(FeatParser::LabelContext *start,
                             FeatParser::LabelContext *end) {
    if ( start == nullptr || end == nullptr ||
         start->getText() != end->getText() )
        fc->featMsg(sERROR, "End label %s does not match start label %s.",
                    TOK(end)->getText().c_str(), start->getText().c_str());
}

template <typename T>
T FeatVisitor::getNum(const std::string &str, int base) {
    char *end;

    int64_t v = strtoll(str.c_str(), &end, base);
    if ( end == str.c_str() )
        fc->featMsg(sERROR, "Could not parse numeric string");

    using NL = std::numeric_limits<T>;
    if ( v < NL::min() || v > NL::max() ) {
        fc->featMsg(sERROR, "Number not in range [%ld, %ld]",
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
        fc->featMsg(sERROR, "Could not parse numeric string");

    using NL = std::numeric_limits<T>;
    if ( v < NL::min() || v > NL::max() ) {
        if ( param )
            fc->featMsg(sERROR, "Number not in range [%ld, %ld]",
                        (long) NL::min(), (long) NL::max());
        else
            fc->featMsg(sERROR, "Number not in range [-32768.0, 32767.99998]");
    }
    return (T) v;
}
