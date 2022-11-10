/* Copyright 2022 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved. 
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* Code shared by tx, rotatefont, and mergefonts. */

#include "goadb.h"

#include <cctype>
#include <limits>
#include <filesystem>
#include <fstream>
#include <sstream>

std::string GOADB::scan(std::istream &s, const GOADBStateTable &action,
                        const GOADBStateTable &next, int nameType) {
    std::string r;
    int state = 0;
    char c;

    while (true) {
        s.get(c);
        int cls;
        /* Determine character class */
        if (!s.good()) {
            cls = 3;
        } else if (isalpha(c) || c == '_') {
            cls = 0;
        } else if (isdigit(c)) {
            cls = 1;
        } else if (c == '.') {
            cls = 2;
        } else if ((nameType == sourceName) && (c == '.' || c == '+' || c == '*' || c == ':' || c == '~' || c == '^' || c == '!' || c == '-')) {
            cls = 2;
        } else if ((nameType == uvName) && (c == ',')) {
            cls = 2;
        } else {
            cls = 3;
        }
        int actn = action[state][cls];
        state = next[state][cls];

        /* Performs actions */
        if (actn == 0) {
            r.push_back(c);
        } else if (actn & E_) {
            return "";
        } else if (actn & Q_) {
            if (s.good())
                s.unget();
            return r;
        }
    }
    return "";
}

void GOADB::parseError(const char *msg) {
    logger->log(sWARNING, "%s [%s:%ld] (record skipped)", msg, fname, lineno);
}

bool GOADB::read(const char *filename) {
    if (!std::filesystem::exists(std::filesystem::status(filename))) {
        logger->log(sERROR, "Glyph Name Alias Database file '%s' not found", filename);
        return false;
    }
    std::ifstream gnamefile(filename);
    std::string buf;
    int32_t order = -1;
    char c;

    lineno = 0;
    fname = filename;

    std::string msg = "Reading Glyph Name Alias Database file '";
    msg += filename;
    msg += "'";

    logger->set_context("ReadGOADB", sWARNING, msg.c_str());

    while (!std::getline(gnamefile, buf).fail()) {
        lineno++;
        std::istringstream bufs(buf);
        std::string fin;
        GOADBRec gr;

        const char *p = buf.c_str();

        /* Skip blanks */
        while (isspace(bufs.peek()))
            bufs.get(c);

        c = bufs.peek();
        if (c == '\0' || c == '#')
            continue; /* Skip blank or comment line */

        gr.order = ++order;
        /* Parse final name */
        fin = finalScan(bufs);
        if (fin.empty() || !isspace(bufs.peek())) {
            parseError("syntax error");
            continue;
        }

        /* Skip blanks */
        while (isspace(bufs.peek()))
            bufs.get(c);

        /* Parse alias name */
        gr.alias = devScan(bufs);
        if (gr.alias.empty() || (bufs.good() && !isspace(bufs.peek()))) {
            parseError("syntax error");
            continue;
        }

        if (bufs.good()) {
            /* Skip blanks */
            while (bufs.good() && isspace(bufs.peek()))
                bufs.get(c);

            /* Parse uv override name */

            c = bufs.peek();
            if (bufs.good() && c != '\0' && c != '#') {
                gr.uvName = uvScan(bufs);
                if (gr.uvName.empty() || (bufs.good() && !isspace(bufs.peek()))) {
                    parseError("syntax error");
                    continue;
                }
            }

            while (bufs.good() && isspace(bufs.peek()))
                bufs.get(c);

            c = bufs.peek();
            if (bufs.good() && c != '\0' && c != '#') {
                parseError("syntax error");
                continue;
            }
        }

        auto aer = aliasMap.emplace(gr.alias, fin);
        if (!aer.second) {
            parseError("duplicate name");
            continue;
        }

        auto fir = finalMap.insert(make_pair(fin, gr));
        if (!fir.second && fir.first->second.uvName != gr.uvName) {
            parseError("duplicate final name, with different uv ovveride");
            continue;
        }

        if (gnamefile.eof())
            break;
    }
    logger->clear_context("ReadGOADB");
/*    for (auto i : finalMap)
        std::cerr << i.first << " " << i.second.order << " " << i.second.alias << " " << i.second.uvName << std::endl; */
    return true;
}

const char *GOADB::getFinalGlyphName(const char *gname) {
    auto i = aliasMap.find(gname);
    if (i != aliasMap.end())
        return i->second.c_str();
    else
        return gname;
}

const char *GOADB::getSrcGlyphName(const char *gname) {
    auto i = finalMap.find(gname);
    if (i != finalMap.end())
        return i->second.alias.c_str();
    else
        return gname;
}

/* [hot callback] Get UV override in form of u<UV Code> glyph name. */
const char *GOADB::getUVOverrideName(const char *gname) {
    std::string name(gname);

    if (!useFinal) {
        auto i = aliasMap.find(name);
        if (i != aliasMap.end())
            name = i->second;
    }
    auto i = finalMap.find(name);
    if (i != finalMap.end() && !i->second.uvName.empty())
        return i->second.uvName.c_str();

    return NULL;
}

int32_t GOADB::getFinalAndOrder(const char *gname, const char **fin) {
    *fin = gname;
    // Put glyphs that aren't found at the end
    int32_t order = std::numeric_limits<int32_t>::max();

    auto i = aliasMap.find(*fin);
    if (i != aliasMap.end()) {
        *fin = i->second.c_str();
    }

    auto j = finalMap.find(*fin);
    if (j != finalMap.end()) {
        order = j->second.order;
    } else {
        *fin = NULL;
    }
    return order;
}
