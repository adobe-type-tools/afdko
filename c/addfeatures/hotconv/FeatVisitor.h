/* Copyright 2021 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 * This software is licensed as OpenSource, under the Apache License, Version 2.0.
 * This license is available at: http://opensource.org/licenses/Apache-2.0.
 */

#ifndef ADDFEATURES_HOTCONV_FEATVISITOR_H_
#define ADDFEATURES_HOTCONV_FEATVISITOR_H_

#include <assert.h>

#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "antlr4-runtime.h"
#include "BaseErrorListener.h"

#include "FeatLexer.h"
#include "FeatParserBaseVisitor.h"
#include "FeatCtx.h"
#include "varsupport.h"


/* Include handling:
 *
 * There is one FeatVisitor object per top-level or included file, with the
 * lexer, parser, and tree of that file. The top_ep variable stores the
 * FeatParser method pointer to the parsing function for the context of the
 * include directive in the "parent" file, which is passed in at construction
 * time.
 */

typedef antlr4::ParserRuleContext *(FeatParser::*FeatParsingEntry)();

class FeatVisitor : public FeatParserBaseVisitor {
    friend class FeatCtx;
 public:
    typedef std::function<antlr4::ParserRuleContext *(FeatParser&)> EntryPoint;

    FeatVisitor() = delete;
    FeatVisitor(FeatCtx *fc, const char *pathname,
                FeatVisitor *parent = nullptr,
                EntryPoint ep = &FeatParser::file,
                int depth = 0)
                : fc(fc), pathname(pathname), parent(parent),
                  top_ep(ep), depth(depth) {}
    virtual ~FeatVisitor();

    // Main entry functions
    void Parse(bool do_includes = true);
    void Translate();

    // Console message reporting utilities
    std::string newFileMsg();
    std::string tokenPositionMsg(bool full = false);
    void currentTokStr(std::string &ts);

    // Functions to mark the "current" token before calling into FeatCtx
    inline antlr4::Token *TOK(antlr4::Token *t) { current_msg_token = t; return t; }
    template <class T, typename std::enable_if<std::is_base_of<antlr4::tree::TerminalNode, T>::value>::type* = nullptr>
        inline T* TOK(T* t) { if (t) current_msg_token = t->getSymbol(); return t; }
    template <class T, typename std::enable_if<std::is_base_of<antlr4::ParserRuleContext, T>::value>::type* = nullptr>
        inline T* TOK(T* t) { if (t) current_msg_token = t->getStart(); return t; }

 private:
    // Separate stages for parsing included files and extracting data from
    // the tree
    enum Stage { vInclude = 1, vExtract } stage;

    // Antlr 4 error reporting class
    struct FeatErrorListener : public antlr4::BaseErrorListener {
        FeatErrorListener() = delete;
        explicit FeatErrorListener(FeatVisitor &v) : v(v) {};
        void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *symbol,
                         size_t line, size_t charPositionInLine,
                         const std::string &msg, std::exception_ptr e) override;
        FeatVisitor &v;
    };

    // Grammar node visitors

    // Blocks
    antlrcpp::Any visitFeatureBlock(FeatParser::FeatureBlockContext *ctx) override;
    antlrcpp::Any visitAnonBlock(FeatParser::AnonBlockContext *ctx) override;
    antlrcpp::Any visitLookupBlockTopLevel(FeatParser::LookupBlockTopLevelContext *ctx) override;

    // Top-level statements
    antlrcpp::Any visitInclude(FeatParser::IncludeContext *ctx) override;
    antlrcpp::Any visitLangsysAssign(FeatParser::LangsysAssignContext *ctx) override;
    antlrcpp::Any visitValueRecordDef(FeatParser::ValueRecordDefContext *ctx) override;
    antlrcpp::Any visitLocationDef(FeatParser::LocationDefContext *ctx) override;
    antlrcpp::Any visitAnchorDef(FeatParser::AnchorDefContext *ctx) override;
    antlrcpp::Any visitGlyphClassAssign(FeatParser::GlyphClassAssignContext *ctx) override;

    // Statements (in feature and lookup blocks)
    antlrcpp::Any visitFeatureUse(FeatParser::FeatureUseContext *ctx) override;
    antlrcpp::Any visitScriptAssign(FeatParser::ScriptAssignContext *ctx) override;
    antlrcpp::Any visitLangAssign(FeatParser::LangAssignContext *ctx) override;
    antlrcpp::Any visitLookupflagAssign(FeatParser::LookupflagAssignContext *ctx) override;
    antlrcpp::Any visitIgnoreSubOrPos(FeatParser::IgnoreSubOrPosContext *ctx) override;
    antlrcpp::Any visitSubstitute(FeatParser::SubstituteContext *ctx) override;
    antlrcpp::Any visitMark_statement(FeatParser::Mark_statementContext *ctx) override;
    antlrcpp::Any visitPosition(FeatParser::PositionContext *ctx) override;
    antlrcpp::Any visitParameters(FeatParser::ParametersContext *ctx) override;
    antlrcpp::Any visitSizemenuname(FeatParser::SizemenunameContext *ctx) override;
    antlrcpp::Any visitFeatureNames(FeatParser::FeatureNamesContext *ctx) override;
    antlrcpp::Any visitSubtable(FeatParser::SubtableContext *ctx) override;

    // Feature-specific
    antlrcpp::Any visitLookupBlockOrUse(FeatParser::LookupBlockOrUseContext *ctx) override;
    antlrcpp::Any visitCvParameterBlock(FeatParser::CvParameterBlockContext *ctx) override;
    antlrcpp::Any visitCvParameter(FeatParser::CvParameterContext *ctx) override;

    // Tables
    antlrcpp::Any visitTable_BASE(FeatParser::Table_BASEContext *ctx) override;
    antlrcpp::Any visitAxisTags(FeatParser::AxisTagsContext *ctx) override;
    antlrcpp::Any visitAxisScripts(FeatParser::AxisScriptsContext *ctx) override;

    antlrcpp::Any visitTable_GDEF(FeatParser::Table_GDEFContext *ctx) override;
    antlrcpp::Any visitGdefGlyphClass(FeatParser::GdefGlyphClassContext *ctx) override;
    antlrcpp::Any visitGdefAttach(FeatParser::GdefAttachContext *ctx) override;
    antlrcpp::Any visitGdefLigCaretPos(FeatParser::GdefLigCaretPosContext *ctx) override;
    antlrcpp::Any visitGdefLigCaretIndex(FeatParser::GdefLigCaretIndexContext *ctx) override;

    antlrcpp::Any visitTable_head(FeatParser::Table_headContext *ctx) override;
    antlrcpp::Any visitHead(FeatParser::HeadContext *ctx) override;

    antlrcpp::Any visitTable_hhea(FeatParser::Table_hheaContext *ctx) override;
    antlrcpp::Any visitHhea(FeatParser::HheaContext *ctx) override;

    antlrcpp::Any visitTable_vhea(FeatParser::Table_vheaContext *ctx) override;
    antlrcpp::Any visitVhea(FeatParser::VheaContext *ctx) override;

    antlrcpp::Any visitTable_name(FeatParser::Table_nameContext *ctx) override;
    antlrcpp::Any visitNameID(FeatParser::NameIDContext *ctx) override;

    antlrcpp::Any visitTable_vmtx(FeatParser::Table_vmtxContext *ctx) override;
    antlrcpp::Any visitVmtx(FeatParser::VmtxContext *ctx) override;

    antlrcpp::Any visitTable_STAT(FeatParser::Table_STATContext *ctx) override;
    antlrcpp::Any visitDesignAxis(FeatParser::DesignAxisContext *ctx) override;
    antlrcpp::Any visitNameEntry(FeatParser::NameEntryContext *ctx) override;
    antlrcpp::Any visitAxisValue(FeatParser::AxisValueContext *ctx) override;
    antlrcpp::Any visitAxisValueFlags(FeatParser::AxisValueFlagsContext *ctx) override;
    antlrcpp::Any visitAxisValueLocation(FeatParser::AxisValueLocationContext *ctx) override;
    antlrcpp::Any visitElidedFallbackName(FeatParser::ElidedFallbackNameContext *ctx) override;
    antlrcpp::Any visitElidedFallbackNameID(FeatParser::ElidedFallbackNameIDContext *ctx) override;

    antlrcpp::Any visitTable_OS_2(FeatParser::Table_OS_2Context *ctx) override;
    antlrcpp::Any visitOs_2(FeatParser::Os_2Context *ctx) override;


    // Translating visitors
    void translateBaseScript(FeatParser::BaseScriptContext *ctx, bool vert, size_t cnt);
    bool translateAnchor(FeatParser::AnchorContext *ctx, int componentIndex);


    // Retrieval visitors
    void getValueRecord(FeatParser::ValueRecordContext *ctx, MetricsInfo &mi);
    void getValueLiteral(FeatParser::ValueLiteralContext *ctx, MetricsInfo &mi);
    void getSingleValueLiteral(FeatParser::SingleValueLiteralContext *ctx,
                               VarValueRecord &vvr);
    void getParenLocationValue(FeatParser::ParenLocationValueContext *ctx,
                               VarValueRecord &vvr);
    void addLocationValueLiteral(FeatParser::LocationValueLiteralContext *ctx,
                                 VarValueRecord &vvr);
    uint32_t getLocationSpecifier(FeatParser::LocationSpecifierContext *ctx);
    uint32_t getLocationLiteral(FeatParser::LocationLiteralContext *ctx);
    bool addAxisLocationLiteral(FeatParser::AxisLocationLiteralContext *ctx,
                                std::vector<var_F2dot14> &l);
    GPat::SP getLookupPattern(FeatParser::LookupPatternContext *ctx, bool markedOK);
    GPat::ClassRec getLookupPatternElement(FeatParser::LookupPatternElementContext *ctx,
                                           bool markedOK);
    GPat::SP concatenatePattern(GPat::SP gp, FeatParser::PatternContext *ctx,
                             bool isBaseNode = false);
    GPat::SP concatenatePatternElement(GPat::SP loc, FeatParser::PatternElementContext *ctx);
    GPat::ClassRec getPatternElement(FeatParser::PatternElementContext *ctx, bool markedOK);
    GPat::ClassRec getGlyphClass(FeatParser::GlyphClassContext *ctx, bool dontcopy);
    GID getGlyph(FeatParser::GlyphContext *ctx, bool allowNotDef);


    // Utility
    void getGlyphClassAsCurrentGC(FeatParser::GlyphClassContext *ctx,
                                  antlr4::tree::TerminalNode *target_gc,
                                  bool dontcopy);
    void addGCLiteralToCurrentGC(FeatParser::GcLiteralContext *ctx);
    Tag checkTag(FeatParser::TagContext *start, FeatParser::TagContext *end);
    Tag getTag(FeatParser::TagContext *t);
    Tag getTag(antlr4::tree::TerminalNode *t);
    void checkLabel(FeatParser::LabelContext *start, FeatParser::LabelContext *end);

    template <typename T> T getNum(const std::string &str, int base = 0);
    template <typename T> T getFixed(FeatParser::FixedNumContext *ctx, bool param = false);


    // "Remove" default inherited visitors
#ifndef NDEBUG
#define DEBSTOP assert(false); return nullptr;
    antlrcpp::Any visitCursiveElement(FeatParser::CursiveElementContext *) override { DEBSTOP }
    antlrcpp::Any visitBaseToMarkElement(FeatParser::BaseToMarkElementContext *) override { DEBSTOP }
    antlrcpp::Any visitLigatureMarkElement(FeatParser::LigatureMarkElementContext *) override { DEBSTOP }
    antlrcpp::Any visitValuePattern(FeatParser::ValuePatternContext *) override { DEBSTOP }
    antlrcpp::Any visitLookupflagElement(FeatParser::LookupflagElementContext *) override { DEBSTOP }
    antlrcpp::Any visitValueRecord(FeatParser::ValueRecordContext *) override { DEBSTOP }
    antlrcpp::Any visitValueLiteral(FeatParser::ValueLiteralContext *) override { DEBSTOP }
    antlrcpp::Any visitLocationLiteral(FeatParser::LocationLiteralContext *) override { DEBSTOP }
    antlrcpp::Any visitAxisLocationLiteral(FeatParser::AxisLocationLiteralContext *) override { DEBSTOP }
    antlrcpp::Any visitAnchor(FeatParser::AnchorContext *) override { DEBSTOP }
    antlrcpp::Any visitLookupPattern(FeatParser::LookupPatternContext *) override { DEBSTOP }
    antlrcpp::Any visitLookupPatternElement(FeatParser::LookupPatternElementContext *) override { DEBSTOP }
    antlrcpp::Any visitPattern(FeatParser::PatternContext *) override { DEBSTOP }
    antlrcpp::Any visitPatternElement(FeatParser::PatternElementContext *) override { DEBSTOP }
    antlrcpp::Any visitGlyphClassOptional(FeatParser::GlyphClassOptionalContext *) override { DEBSTOP }
    antlrcpp::Any visitGlyphClass(FeatParser::GlyphClassContext *) override { DEBSTOP }
    antlrcpp::Any visitGcLiteral(FeatParser::GcLiteralContext *) override { DEBSTOP }
    antlrcpp::Any visitGcLiteralElement(FeatParser::GcLiteralElementContext *) override { DEBSTOP }
    antlrcpp::Any visitGlyph(FeatParser::GlyphContext *) override { DEBSTOP }
    antlrcpp::Any visitGlyphName(FeatParser::GlyphNameContext *) override { DEBSTOP }
    antlrcpp::Any visitLabel(FeatParser::LabelContext *) override { DEBSTOP }
    antlrcpp::Any visitTag(FeatParser::TagContext *) override { DEBSTOP }
    antlrcpp::Any visitFixedNum(FeatParser::FixedNumContext *) override { DEBSTOP }
    antlrcpp::Any visitGenNum(FeatParser::GenNumContext *) override { DEBSTOP }
    antlrcpp::Any visitSubtok(FeatParser::SubtokContext *) override { DEBSTOP }
    antlrcpp::Any visitRevtok(FeatParser::RevtokContext *) override { DEBSTOP }
    antlrcpp::Any visitAnontok(FeatParser::AnontokContext *) override { DEBSTOP }
    antlrcpp::Any visitEnumtok(FeatParser::EnumtokContext *) override { DEBSTOP }
    antlrcpp::Any visitPostok(FeatParser::PostokContext *) override { DEBSTOP }
    antlrcpp::Any visitMarkligtok(FeatParser::MarkligtokContext *) override { DEBSTOP }
#undef DEBSTOP
#endif

    // State

    FeatCtx *fc;

    // Feature filename and directory name
    std::string pathname, dirname;

    // The including file object (or nullptr at the top-level)
    FeatVisitor *parent;

    // Set by TOK() methods
    antlr4::Token *current_msg_token {nullptr};

    // State for included files
    std::vector<FeatVisitor *> includes;
    size_t current_include {0};
    EntryPoint top_ep, include_ep;
    int depth;
    bool need_file_msg {true};

    /* Antlr 4 parse tree state
     *
     * It appears that the parse tree is only valid for the lifetime of
     * the parser and the parser is only valid for the lifetime of just
     * about everything else. Therefore we keep all of it around until the
     * object is destroyed.
     */
    antlr4::ANTLRInputStream *input {nullptr};
    FeatLexer *lexer {nullptr};
    antlr4::CommonTokenStream *tokens {nullptr};
    FeatParser *parser {nullptr};
    antlr4::tree::ParseTree *tree {nullptr};
};

#endif  // ADDFEATURES_HOTCONV_FEATVISITOR_H_
