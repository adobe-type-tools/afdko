
// Generated from FeatLexer.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  FeatLexer : public antlr4::Lexer {
public:
  enum {
    ANON = 1, ANON_v = 2, COMMENT = 3, WHITESPACE = 4, INCLUDE = 5, FEATURE = 6, 
    TABLE = 7, SCRIPT = 8, LANGUAGE = 9, LANGSYS = 10, SUBTABLE = 11, LOOKUP = 12, 
    LOOKUPFLAG = 13, NOTDEF = 14, RIGHT_TO_LEFT = 15, IGNORE_BASE_GLYPHS = 16, 
    IGNORE_LIGATURES = 17, IGNORE_MARKS = 18, USE_MARK_FILTERING_SET = 19, 
    MARK_ATTACHMENT_TYPE = 20, EXCLUDE_DFLT = 21, INCLUDE_DFLT = 22, EXCLUDE_dflt = 23, 
    INCLUDE_dflt = 24, USE_EXTENSION = 25, BEGINVALUE = 26, ENDVALUE = 27, 
    ENUMERATE = 28, ENUMERATE_v = 29, EXCEPT = 30, IGNORE = 31, SUBSTITUTE = 32, 
    SUBSTITUTE_v = 33, REVERSE = 34, REVERSE_v = 35, BY = 36, FROM = 37, 
    POSITION = 38, POSITION_v = 39, PARAMETERS = 40, FEATURE_NAMES = 41, 
    CV_PARAMETERS = 42, CV_UI_LABEL = 43, CV_TOOLTIP = 44, CV_SAMPLE_TEXT = 45, 
    CV_PARAM_LABEL = 46, DEF_AXIS_UNIT = 47, CV_CHARACTER = 48, SIZEMENUNAME = 49, 
    CONTOURPOINT = 50, ANCHOR = 51, ANCHOR_DEF = 52, VALUE_RECORD_DEF = 53, 
    LOCATION_DEF = 54, MARK = 55, MARK_CLASS = 56, CURSIVE = 57, MARKBASE = 58, 
    MARKLIG = 59, MARKLIG_v = 60, LIG_COMPONENT = 61, KNULL = 62, BASE = 63, 
    HA_BTL = 64, VA_BTL = 65, HA_BSL = 66, VA_BSL = 67, GDEF = 68, GLYPH_CLASS_DEF = 69, 
    ATTACH = 70, LIG_CARET_BY_POS = 71, LIG_CARET_BY_IDX = 72, HEAD = 73, 
    FONT_REVISION = 74, HHEA = 75, ASCENDER = 76, DESCENDER = 77, LINE_GAP = 78, 
    CARET_OFFSET = 79, NAME = 80, NAMEID = 81, OS_2 = 82, FS_TYPE = 83, 
    FS_TYPE_v = 84, OS2_LOWER_OP_SIZE = 85, OS2_UPPER_OP_SIZE = 86, PANOSE = 87, 
    TYPO_ASCENDER = 88, TYPO_DESCENDER = 89, TYPO_LINE_GAP = 90, WIN_ASCENT = 91, 
    WIN_DESCENT = 92, X_HEIGHT = 93, CAP_HEIGHT = 94, WEIGHT_CLASS = 95, 
    WIDTH_CLASS = 96, VENDOR = 97, UNICODE_RANGE = 98, CODE_PAGE_RANGE = 99, 
    FAMILY_CLASS = 100, STAT = 101, ELIDED_FALLBACK_NAME = 102, ELIDED_FALLBACK_NAME_ID = 103, 
    DESIGN_AXIS = 104, AXIS_VALUE = 105, FLAG = 106, LOCATION = 107, AXIS_EAVN = 108, 
    AXIS_OSFA = 109, VHEA = 110, VERT_TYPO_ASCENDER = 111, VERT_TYPO_DESCENDER = 112, 
    VERT_TYPO_LINE_GAP = 113, VMTX = 114, VERT_ORIGIN_Y = 115, VERT_ADVANCE_Y = 116, 
    LCBRACE = 117, RCBRACE = 118, LBRACKET = 119, RBRACKET = 120, LPAREN = 121, 
    RPAREN = 122, HYPHEN = 123, PLUS = 124, SEMI = 125, EQUALS = 126, MARKER = 127, 
    COMMA = 128, COLON = 129, STRVAL = 130, LNAME = 131, GCLASS = 132, AXISUNIT = 133, 
    CID = 134, ESCGNAME = 135, NAMELABEL = 136, EXTNAME = 137, POINTNUM = 138, 
    NUMEXT = 139, NUMOCT = 140, NUM = 141, A_WHITESPACE = 142, A_LABEL = 143, 
    A_LBRACE = 144, A_CLOSE = 145, A_LINE = 146, I_WHITESPACE = 147, I_RPAREN = 148, 
    IFILE = 149, I_LPAREN = 150
  };

  enum {
    Anon = 1, AnonContent = 2, Include = 3, Ifile = 4
  };

  explicit FeatLexer(antlr4::CharStream *input);

  ~FeatLexer() override;


   std::string anon_tag;

   /* All the TSTART/TCHR characters are grouped together, so just
    * look for the string and if its there verify that the characters
    * on either side are from the appropriate set (in case there are
    * "extra" characters).
    */

   bool verify_anon(const std::string &line) {
       auto p = line.find(anon_tag);
       if ( p == std::string::npos )
           return false;
       --p;
       if ( ! ( line[p] == ' ' || line[p] == '\t' || line[p] == '}' ) )
           return false;
       p += anon_tag.size() + 1;
       if ( ! ( line[p] == ' ' || line[p] == '\t' || line[p] == ';' ) )
           return false;
       return true;
   }


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  void action(antlr4::RuleContext *context, size_t ruleIndex, size_t actionIndex) override;

  bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.
  void A_LABELAction(antlr4::RuleContext *context, size_t actionIndex);

  // Individual semantic predicate functions triggered by sempred() above.
  bool A_CLOSESempred(antlr4::RuleContext *_localctx, size_t predicateIndex);

};

