/* Copyright 2016 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SHARED_INCLUDE_GOADB_H_
#define SHARED_INCLUDE_GOADB_H_

/* Glyph Name Aliasing Database File Format
   ========================================
   The glyph alias database uses a simple line-oriented ASCII format.

   Lines are terminated by either a carriage return (13), a line feed (10), or
   a carriage return followed by a line feed. Lines may not exceed 255
   characters in length (including the line terminating character(s)).

   Any of the tab (9), vertical tab (11), form feed (12), or space (32)
   characters are regarded as "blank" characters for the purposes of this
   description.

   Empty lines, or lines containing blank characters only, are ignored.

   Lines whose first non-blank character is hash (#) are treated as comment
   lines which extend to the line end and are ignored.

   All other lines must contain two glyph names separated by one or more blank
   characters. A working glyph name may be up to 63 characters in length,  a
   final character name may be up to 32 characters in length, and may only
   contain characters from the following set:

   A-Z  capital letters
   a-z  lowercase letters
   0-9  figures
    _   underscore
    .   period
    -   hyphen
    +   plus
    *   asterisk
    :   colon
    ~   tilde
    ^   asciicircum
    !   exclam

   A glyph name may not begin with a figure or consist of a period followed by
   a sequence of digits only.

   The first glyph name on a line is a final name that is stored within an
   OpenType font and second glyph name is an alias for the final name. The
   second name may be optionally followed a comment beginning with a hash (#)
   character and separated from the second name by one or more blank
   characters.

   If a final name has more that one alias it may be specified on a separate
   line beginning with the same final name.

   An optional third field may contain a glyph name in the form of 'u' + hexadecimal
   Unicode value. This will override Makeotf's default heuristics for assigning a
   UV, including any UV suggested by the form of the final name.

   All alias names must be
   unique. */

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "slogger.h"

/* ----------------------- Glyph Name Alias Database  ---------------------- */

#define Q_ (1 << 0) /* Quit scan on unrecognized character */
#define E_ (1 << 1) /* Report syntax error */

typedef uint8_t GOADBStateTable[3][4];

class GOADB {
 public:
    explicit GOADB(std::shared_ptr<slogger> l) : logger(l) {}
    bool read(const char *filename);
    void parseError(const char *msg);
    void setCancelled(bool c) { cancelled = c; }
    void useFinalNames() { useFinal = true; }
    const char *getFinalGlyphName(const char *gname);
    const char *getSrcGlyphName(const char *gname);
    const char *getUVOverrideName(const char *gname);
    int32_t getFinalAndOrder(const char *gname, const char **fin);

 protected:
    /* Next state table */
    static constexpr const GOADBStateTable nextFinal = {
        /*  A-Za-z_   0-9      .       *    index  */
        /* -------- ------- ------- ------- ------ */
        {     1,       0,      2,      0},  /* [0] */
        {     1,       1,      1,      0},  /* [1] */
        {     1,       2,      2,      0},  /* [2] */
    };

    /* Action table */

    static constexpr const GOADBStateTable actionFinal = {
        /*  A-Za-z_   0-9      .       *    index  */
        /* -------- ------- ------- ------- ------ */
        {     0,       E_,     0,      Q_}, /* [0] */
        {     0,       0,      0,      Q_}, /* [1] */
        {     0,       0,      0,      E_}, /* [2] */
    };

    /* Allow glyph names to start with numbers. */
    static constexpr const GOADBStateTable actionDev = {
        /*  A-Za-z_   0-9      .       *    index  */
        /* -------- ------- ------- ------- ------ */
        {     0,       0,      0,      Q_}, /* [0] */
        {     0,       0,      0,      Q_}, /* [1] */
        {     0,       0,      0,      E_}, /* [2] */
    };

    enum {
        finalName,
        sourceName,
        uvName
    };

    std::string scan(std::istream &s, const GOADBStateTable &action,
                     const GOADBStateTable &next, int nameType);
    std::string devScan(std::istream &s) {
        return scan(s, actionDev, nextFinal, sourceName);
    }
    std::string finalScan(std::istream &s) {
        return scan(s, actionFinal, nextFinal, finalName);
    }
    std::string uvScan(std::istream &s) {
        return scan(s, actionFinal, nextFinal, uvName);
    }

    void error(const char *msg) {}

    struct GOADBRec {
        int32_t order;
        std::string alias;
        std::string uvName;
    };

    bool useFinal = false, cancelled = false;
    std::unordered_map<std::string, std::string> aliasMap;
    std::unordered_map<std::string, GOADBRec> finalMap;
    std::shared_ptr<slogger> logger;
    uint32_t lineno;
    const char *fname;
};

#endif  // SHARED_INCLUDE_GOADB_H_
