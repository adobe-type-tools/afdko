
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
    CV_PARAM_LABEL = 46, CV_CHARACTER = 47, SIZEMENUNAME = 48, CONTOURPOINT = 49, 
    ANCHOR = 50, ANCHOR_DEF = 51, VALUE_RECORD_DEF = 52, LOCATION_DEF = 53, 
    MARK = 54, MARK_CLASS = 55, CURSIVE = 56, MARKBASE = 57, MARKLIG = 58, 
    MARKLIG_v = 59, LIG_COMPONENT = 60, KNULL = 61, BASE = 62, HA_BTL = 63, 
    VA_BTL = 64, HA_BSL = 65, VA_BSL = 66, GDEF = 67, GLYPH_CLASS_DEF = 68, 
    ATTACH = 69, LIG_CARET_BY_POS = 70, LIG_CARET_BY_IDX = 71, HEAD = 72, 
    FONT_REVISION = 73, HHEA = 74, ASCENDER = 75, DESCENDER = 76, LINE_GAP = 77, 
    CARET_OFFSET = 78, CARET_SLOPE_RISE = 79, CARET_SLOPE_RUN = 80, NAME = 81, 
    NAMEID = 82, OS_2 = 83, FS_TYPE = 84, FS_TYPE_v = 85, OS2_LOWER_OP_SIZE = 86, 
    OS2_UPPER_OP_SIZE = 87, PANOSE = 88, TYPO_ASCENDER = 89, TYPO_DESCENDER = 90, 
    TYPO_LINE_GAP = 91, WIN_ASCENT = 92, WIN_DESCENT = 93, X_HEIGHT = 94, 
    CAP_HEIGHT = 95, SUBSCRIPT_X_SIZE = 96, SUBSCRIPT_X_OFFSET = 97, SUBSCRIPT_Y_SIZE = 98, 
    SUBSCRIPT_Y_OFFSET = 99, SUPERSCRIPT_X_SIZE = 100, SUPERSCRIPT_X_OFFSET = 101, 
    SUPERSCRIPT_Y_SIZE = 102, SUPERSCRIPT_Y_OFFSET = 103, STRIKEOUT_SIZE = 104, 
    STRIKEOUT_POSITION = 105, WEIGHT_CLASS = 106, WIDTH_CLASS = 107, VENDOR = 108, 
    UNICODE_RANGE = 109, CODE_PAGE_RANGE = 110, FAMILY_CLASS = 111, STAT = 112, 
    ELIDED_FALLBACK_NAME = 113, ELIDED_FALLBACK_NAME_ID = 114, DESIGN_AXIS = 115, 
    AXIS_VALUE = 116, FLAG = 117, LOCATION = 118, AXIS_EAVN = 119, AXIS_OSFA = 120, 
    VHEA = 121, VERT_TYPO_ASCENDER = 122, VERT_TYPO_DESCENDER = 123, VERT_TYPO_LINE_GAP = 124, 
    VMTX = 125, VERT_ORIGIN_Y = 126, VERT_ADVANCE_Y = 127, LCBRACE = 128, 
    RCBRACE = 129, LBRACKET = 130, RBRACKET = 131, LPAREN = 132, RPAREN = 133, 
    HYPHEN = 134, PLUS = 135, SEMI = 136, EQUALS = 137, MARKER = 138, COMMA = 139, 
    COLON = 140, STRVAL = 141, LNAME = 142, GCLASS = 143, AXISUNIT = 144, 
    CID = 145, ESCGNAME = 146, NAMELABEL = 147, EXTNAME = 148, POINTNUM = 149, 
    NUMEXT = 150, NUMOCT = 151, NUM = 152, A_WHITESPACE = 153, A_LABEL = 154, 
    A_LBRACE = 155, A_CLOSE = 156, A_LINE = 157, I_WHITESPACE = 158, I_RPAREN = 159, 
    IFILE = 160, I_LPAREN = 161
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

