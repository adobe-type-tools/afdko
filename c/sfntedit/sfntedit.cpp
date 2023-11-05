/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* sfnt table edit utility. */

#include <cstdarg>
#include <string>
#include <cstdio>
#include <filesystem>
#include <stdexcept>

#include <sstream>
#include <fstream>
#include <iostream>

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <map>

#include "slogger.h"

#define VERSION "1.4.3"

namespace fs = std::filesystem;

static const char *se_progname = "sfntedit";

/* Tag support */
typedef uint32_t Tag;
#define TAG(a, b, c, d) ((Tag)(a) << 24 | (Tag)(b) << 16 | (c) << 8 | (d))
#define TAG_ARG(t) (char)((t) >> 24 & 0xff), (char)((t) >> 16 & 0xff), \
                   (char)((t) >> 8 & 0xff), (char)((t)&0xff)

/* Recommended table data order for OTF fonts */
static std::unordered_map<Tag, uint16_t> otfOrder = {
    {TAG('h', 'e', 'a', 'd'), 0},
    {TAG('h', 'h', 'e', 'a'), 1},
    {TAG('m', 'a', 'x', 'p'), 2},
    {TAG('O', 'S', '/', '2'), 3},
    {TAG('n', 'a', 'm', 'e'), 4},
    {TAG('c', 'm', 'a', 'p'), 5},
    {TAG('p', 'o', 's', 't'), 6},
    {TAG('f', 'v', 'a', 'r'), 7},
    {TAG('M', 'M', 'S', 'D'), 8},
    {TAG('C', 'F', 'F', ' '), 9}
};

/* Recommended table data order for TTF fonts */
static std::unordered_map<Tag, uint16_t> ttfOrder = {
    {TAG('h', 'e', 'a', 'd'), 0},
    {TAG('h', 'h', 'e', 'a'), 1},
    {TAG('m', 'a', 'x', 'p'), 2},
    {TAG('O', 'S', '/', '2'), 3},
    {TAG('h', 'm', 't', 'x'), 4},
    {TAG('L', 'T', 'S', 'H'), 5},
    {TAG('V', 'D', 'M', 'X'), 6},
    {TAG('h', 'd', 'm', 'x'), 7},
    {TAG('c', 'm', 'a', 'p'), 8},
    {TAG('f', 'p', 'g', 'm'), 9},
    {TAG('p', 'r', 'e', 'p'), 10},
    {TAG('c', 'v', 't', ' '), 11},
    {TAG('l', 'o', 'c', 'a'), 12},
    {TAG('g', 'l', 'y', 'f'), 13},
    {TAG('k', 'e', 'r', 'n'), 14},
    {TAG('n', 'a', 'm', 'e'), 15},
    {TAG('p', 'o', 's', 't'), 16},
    {TAG('g', 'a', 's', 'p'), 17},
    {TAG('P', 'C', 'L', 'T'), 18},
    {TAG('D', 'S', 'I', 'G'), 19}
};

struct Table {
    explicit Table(Tag tag) {}
    Table() {}
    uint32_t checksum = 0;
    uint32_t offset = 0;
    uint32_t length = 0;
    uint16_t flags = 0;    // Option flags
#define TBL_SRC (1 << 10)  // Flags table in source sfnt
#define TBL_DST (1 << 11)  // Flags table in destination sfnt
    fs::path xfilename;    // Extract filename
    fs::path afilename;    // Add filename
};

struct cmpTagByOrder {
    explicit cmpTagByOrder(std::unordered_map<Tag, uint16_t> &tagorder) :
        tagorder(tagorder) {}
    bool operator ()(const Tag& a, const Tag& b) const {
        int16_t ao = 100, bo = 100;
        auto ai = tagorder.find(a);
        if (ai != tagorder.end())
            ao = ai->second;
        auto bi = tagorder.find(b);
        if (bi != tagorder.end())
            bo = bi->second;
        bool r;
        if (ao == 100 && bo == 100)
            return a < b;
        else
            return ao < bo;
    }
    const std::unordered_map<Tag, uint16_t> &tagorder;
};

#define ENTRY_SIZE (sizeof(uint32_t) * 4) /* Size of written fields */

struct sfnt_ { /* sfnt header */
    int32_t version = 0;
    // uint16_t numTables
    uint16_t searchRange = 0;
    uint16_t entrySelector = 0;
    uint16_t rangeShift = 0;
    std::map<Tag, Table> directory;
};
// Size of written fields
#define DIR_HDR_SIZE (sizeof(int32_t) + sizeof(uint16_t) * 4)

/* head.checkSumAdjustment offset within head table */
#define HEAD_ADJUST_OFFSET (2 * sizeof(int32_t))

static const char *se_backupname = "sfntedit.BAK";

#define OPT_EXTRACT (1 << 0)
#define OPT_DELETE  (1 << 1)
#define OPT_ADD     (1 << 2)
#define OPT_LIST    (1 << 3)
#define OPT_CHECK   (1 << 4)
#define OPT_FIX     (1 << 5)
#define OPT_USAGE   (1 << 6)
#define OPT_HELP    (1 << 7)

class SfntEdit {
 public:
    SfntEdit() : logger(slogger::getLogger(se_progname)) {}
    int run(int argc, char *argv[]);

 protected:
    uint32_t addTable(const Table &tbl, std::ostream &dstfile, uint32_t *length);
    int run_prot(int argc, char *argv[]);
    fs::path makeFullPath(const fs::path& source);
    void parseTagList(const std::string &arg, int option, int flag);
    template<typename T> void parseArgs(const T start, const T end);
    void readHdr();
    void dumpHdr();
    void checkChecksums();
    void extractTables();
    std::string makexFilename(Tag tag, const Table &tbl);
    bool sfntCopy();
    void fatal(const char *fmt, ...);
    void reset();

 private:
    const fs::path tmpname {"sfntedit.tmp"};
    fs::path sourcepath;
    fs::path scriptfilepath;
    fs::path srcfilepath, dstfilepath;

    sfnt_ sfnt;
    std::ifstream srcfile;
    std::fstream dstfile;
    long options {0};
    bool doingScripting {false};
    bool isTTF {false};

    std::shared_ptr<slogger> logger;
};

void SfntEdit::reset() {
    sourcepath.clear();
    scriptfilepath.clear();
    srcfilepath.clear();
    dstfilepath.clear();

    sfnt.directory.clear();
    options = 0;
    isTTF = false;
}

/* 1, 2, and 4 byte big-endian machine independent input */
template<class T>
static void readObject(std::istream &f, T& o) {
    static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4);
    uint32_t value;
    char b;

    switch (sizeof(T)) {
        case 1:
            f.get(b);
            *((uint8_t *)&o) = (uint8_t)b;
            break;
        case 2:
            f.get(b);
            value = (uint8_t)b;
            f.get(b);
            value = value << 8 | (uint8_t)b;
            *((uint16_t *)&o) = (uint16_t)value;
            break;
        case 4:
            f.get(b);
            value = (uint8_t)b;
            f.get(b);
            value = value << 8 | (uint8_t)b;
            f.get(b);
            value = value << 8 | (uint8_t)b;
            f.get(b);
            value = value << 8 | (uint8_t)b;
            *((uint32_t *)&o) = value;
            break;
    }
}

/* 1, 2, and 4 byte big-endian machine independent output */
template<class T>
static void writeObject(std::ostream &f, T& o) {
    static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4);
    char b;

    switch (sizeof(T)) {
        case 1:
            b = (uint8_t)(o & 0xFF);
            f.put(b);
            break;
        case 2:
            b = (uint8_t)(o >> 8 & 0xFF);
            f.put(b);
            b = (uint8_t)(o & 0xFF);
            f.put(b);
            break;
        case 4:
            b = (uint8_t)(o >> 24 & 0xFF);
            f.put(b);
            b = (uint8_t)(o >> 16 & 0xFF);
            f.put(b);
            b = (uint8_t)(o >> 8 & 0xFF);
            f.put(b);
            b = (uint8_t)(o & 0xFF);
            f.put(b);
            break;
    }
}

static void fileCopy(std::istream &src, std::ostream &dst, size_t count) {
    char buf[BUFSIZ];

    while (count > BUFSIZ) {
        src.read(buf, BUFSIZ);
        dst.write(buf, BUFSIZ);
        count -= BUFSIZ;
    }

    src.read(buf, count);
    dst.write(buf, count);
}

fs::path SfntEdit::makeFullPath(
        const fs::path &source) {
    if (source.empty())
        return sourcepath;
    else if (sourcepath.empty() || source.is_absolute())
        return source;
    else
        return sourcepath / source;
}

/* ----------------------------- Error Handling ---------------------------- */

#define INI_FATAL_BUFSIZ 128

/* Throw "fatal" exception */
void SfntEdit::fatal(const char *fmt, ...) {
    std::va_list ap;
    std::vector<char> buf(INI_FATAL_BUFSIZ);

    va_start(ap, fmt);
    int l = std::vsnprintf(buf.data(), buf.size(), fmt, ap) + 1;
    va_end(ap);

    if (l <= 0)
        throw std::runtime_error("Error during formatting");
    else if (l > INI_FATAL_BUFSIZ) {
        buf.resize(l);
        va_start(ap, fmt);
        std::vsnprintf(buf.data(), buf.size(), fmt, ap);
        va_end(ap);
    }
    throw std::runtime_error(buf.data());
}

/* ----------------------------- Usage and Help ---------------------------- */

/* Print usage information */
static void printUsage(const char *progname) {
    std::printf(
            "Usage:\n"
            "    %s [options] <srcfile> [<dstfile>]\n"
            "OR: %s  -X <scriptfile>\n"
            "\n"
            "Options:\n"
            "    -x <tag>[=<file>][,<tag>[=<file>]]+ extract table to file\n"
            "    -d <tag>[,<tag>]+ delete table\n"
            "    -a <tag>=<file>[,<tag>=<file>]+ add (or replace) table\n"
            "    -l list sfnt directory (default)\n"
            "    -c check checksums\n"
            "    -f fix checksums (implies -c)\n"
            "    -u print usage\n"
            "    -h print help\n"
            "    -X execute command-lines from <scriptfile> [default: sfntedit.scr]\n"
            "\n"
            "Build:\n"
            "    Version: %s\n"
            "\n",
            progname,
            progname,
            VERSION);
}

/* Show usage and help information */
static void printHelp(const char *progname) {
    printUsage(progname);
    std::printf(
        "This program supports table-editing, listing, and checksumming options on\n"
        "sfnt-formatted files such as OpenType Format (OTF) or TrueType. The mandatory\n"
        "source file is specified as an argument to the program. An optional destination\n"
        "file may also be specified which receives the edited data otherwise the source\n"
        "data is edited in-place thus modifying the source file. In-place editing is\n"
        "achieved by the use of a temporary file called sfntedit.tmp that is created\n"
        "in the directory of execution (requiring you to have write permission to\n"
        "that directory).\n"
        "\n"
        "The target table of an editing option (-x, -d, and -a) is specified\n"
        "with a table tag argument that is nominally 4 characters long.\n"
        "If fewer than 4 characters are specified the tag is padded with spaces\n"
        "(more than 4 characters is a fatal error). Multiple tables may be specified\n"
        "as a single argument composed from a comma-separated list of tags.\n"
        "\n"
        "The extract option (-x) copies the table data into a file whose default name\n"
        "is the concatenation of the source filename (less its .otf or .ttf extension),\n"
        "a period character (.), and the table tag. If the tag contains non-alphanumeric\n"
        "characters they are replaced by underscore characters (_) and finally trailing\n"
        "underscores are removed. The default filename may be overridden by appending\n"
        "an equals character (=) followed by an alternate filename to the table tag\n"
        "argument. The delete option (-d) deletes a table. Unlike the -x option no files\n"
        "may be specified in the table tag list. The add option (-a) adds a table or\n"
        "replaces one if the table already exists. The source file containing the table\n"
        "data is specified by appending an equals character (=) followed by a filename\n"
        "to the table tag.\n"
        "\n"
        "The 3 editing options may be specified together as acting on the same table.\n"
        "In such cases the -x option is applied before the -d option which is applied\n"
        "before the -a option. (The -d option applied to the same table as a subsequent\n"
        "-a option is permitted but redundant.) The -d and -a options change the contents\n"
        "of the sfnt and cause the table checksums and the head table's checksum\n"
        "adjustment field to be recomputed.\n"
        "\n"
        "The list option (-l) simply lists the contents of the sfnt table directory.\n"
        "This is the default action if no other options are specified. The check\n"
        "checksum option (-c) performs a check of all the table checksums and the head\n"
        "table's checksum adjustment field and reports any errors. The fix checksum\n"
        "option (-f) fixes any checksum errors.\n"
        "\n"
        "The -d, -a, and -f options create a new sfnt file by copying tables from the\n"
        "source file to the destination file. The tables are copied in the order\n"
        "recommended in the OpenType specification. A side effect of copying is that all\n"
        "table information including checksums and sfnt search fields is recalculated.\n"
        "\n"
        "Examples:\n"
        "- Extract GPOS and GSUB tables to files minion.GPOS and minion.GSUB.\n"
        "    sfntedit -x GPOS,GSUB minion.otf\n"
        "    \n"
        "- Add tables extracted previously to different font.\n"
        "    sfntedit -a GPOS=minion.GPOS,GSUB=minion.GSUB minion.ttf\n"
        "    \n"
        "- Delete temporary tables from font.\n"
        "    sfntedit -d TR01,TR02,TR03 pala.ttf\n"
        "    \n"
        "- Copy font to new file fixing checksums and reordering tables.\n"
        "    sfntedit -f helv.ttf newhelv.ttf\n\n");
}

static void makeArgs(const fs::path &filename,
                     std::vector<std::vector<std::string>> &script) {
    int state = 0;
    std::string acc;
    char c;
    std::ifstream scriptfile(filename);

    script.emplace_back();

    while (scriptfile.get(c)) {
        switch (state) {
            case 0:
                switch ((int)c) {
                    case '\n':
                    case '\r':
                        script.emplace_back();
                        break;
                    case '\f':
                    case '\t':
                    case ' ':
                        break;
                    case '#':
                        state = 1;
                        break;
                    case '"':
                        state = 2;
                        break;
                    default:
                        acc.push_back(c);
                        state = 3;
                        break;
                }
                break;
            case 1: /* Comment */
                if (c == '\n' || c == '\r')
                    state = 0;
                break;
            case 2: /* Quoted string */
                if (c == '"') {
                    script.back().emplace_back().swap(acc);
                    state = 0;
                } else {
                    acc.push_back(c);
                }
                break;
            case 3: /* Space-delimited string */
                if (isspace((int)c)) {
                    script.back().emplace_back().swap(acc);
                    if ((c == '\n') || (c == '\r')) {
                        script.emplace_back();
                    }
                    state = 0;
                } else {
                    acc.push_back(c);
                }
                break;
        }
    }
    if (!acc.empty())
        script.back().emplace_back().swap(acc);
}

/* ---------------------------- Argument Parsing --------------------------- */

/* Parse table tag list */
void SfntEdit::parseTagList(const std::string &arg, int option, int flag) {
    std::istringstream ssarg(arg);
    std::string part;
    while (std::getline(ssarg, part, ',')) {
        size_t i = 0;
        Tag tag = 0;
        std::string filename, tagstr;

        /* Find filename separator and set to null if present */
        auto ep = part.find('=');
        if (ep != std::string::npos) {
            filename.assign(part, ep+1);
            if (filename.length() == 0)
                fatal("bad filename (-%c)", option);
            tagstr.assign(part, 0, ep);
        } else
            tagstr.assign(part);

        /* Validate tag length */
        if (tagstr.length() == 0 || tagstr.length() > 4)
            fatal("bad tag '%s' (-%c)", tagstr.c_str(), option);

        /* Make tag */
        for (auto c : tagstr) {
            tag = tag << 8 | c;
            i++;
        }

        /* Pad tag with space */
        for (; i < 4; i++)
            tag = tag << 8 | ' ';

        /* Add option */
        if (option == 'd' && !filename.empty()) {
            logger->log(sWARNING, "filename specified with -d [%s] ignored",
                        filename.c_str());
            filename.clear();
        }
        if (option == 'a' && filename.empty())
            fatal("filename missing (-a)");

        auto dre = sfnt.directory.emplace(tag, tag);
        Table &tbl = dre.first->second;

        /* Check tag not already in list */
        if (tbl.flags & flag)
            logger->log(sWARNING, "duplicate tag '%c%c%c%c' (-%c)",
                        TAG_ARG(tag), option);

        tbl.flags |= flag;

        if (option == 'x') {
            tbl.xfilename = makeFullPath(filename);
        } else if (option == 'a') {
            tbl.afilename = makeFullPath(filename);
        }
    }
}

/* Count bits in word */
static int countbits(long value) {
    int count;
    for (count = 0; value; count++)
        value &= value - 1;
    return count;
}

/* Process options */
template<typename T>
void SfntEdit::parseArgs(const T start, const T end) {
    std::string arg, nextarg;

    reset();

    for (T i = start; i < end; i++) {
        arg.assign(*i);
        if (i + 1 < end)
            nextarg.assign(*(i+1));
        else
            nextarg.clear();
        switch (arg[0]) {
            case '-':
                switch (arg[1]) {
                    case 'X': /* script file to execute */
                        if (!doingScripting) {
                            doingScripting = true;
                            if (!nextarg.empty() && nextarg[0] != '-') {
                                scriptfilepath = nextarg;
                                i++;
                            }
                        }
                        break;
                    case 'x': /* Extract table */
                        if (arg[2] != '\0')
                            fatal("bad option (%s)", arg.c_str());
                        else if (nextarg.empty())
                            fatal("no arg for option: -%c", arg[1]);
                        i++;
                        parseTagList(nextarg, 'x', OPT_EXTRACT);
                        options |= OPT_EXTRACT;
                        break;
                    case 'd': /* Delete table */
                        if (arg[2] != '\0')
                            fatal("bad option (%s)", arg.c_str());
                        else if (nextarg.empty())
                            fatal("no arg for option: -%c", arg[1]);
                        i++;
                        parseTagList(nextarg, 'd', OPT_DELETE);
                        options |= OPT_DELETE;
                        break;
                    case 'a': /* Add table */
                        if (arg[2] != '\0')
                            fatal("bad option (%s)", arg.c_str());
                        else if (nextarg.empty())
                            fatal("no arg for option: -%c", arg[1]);
                        i++;
                        parseTagList(nextarg, 'a', OPT_ADD);
                        options |= OPT_ADD;
                        break;
                    case 'l': /* List sfnt table directory */
                        options |= OPT_LIST;
                        break;
                    case 'c': /* Check checksum */
                        options |= OPT_CHECK;
                        break;
                    case 'f': /* Fix checksum */
                        options |= OPT_FIX;
                        break;
                    case 'u':
                        options |= OPT_USAGE;
                        return;
                    case 'h':
                        options |= OPT_HELP;
                        return;
                    default:
                        fatal("unrecognized option (%s)", arg.c_str());
                }
                break;
            default: /* Non-option arg is taken to be filename */
                /* Validate options */
                if (options & (OPT_LIST | OPT_CHECK | OPT_FIX) &&
                    countbits(options) > 1)
                    fatal("option conflict");

                if (options == 0)
                    options |= OPT_LIST; /* No options set; apply default */

                /* Set up filenames */
                srcfilepath = makeFullPath(arg);

                if (i + 2 < end) {
                    fatal("too many files provided");
                } else if (!nextarg.empty()) {
                    if (options & (OPT_DELETE | OPT_ADD | OPT_FIX))
                        dstfilepath = makeFullPath(nextarg);
                    else
                        logger->log(sWARNING,
                                    "output file provided for non-edit option [%s] ignored",
                                     nextarg.c_str());
                }
                i = end-1;
        }
    }

    /* No files provided */
    if ((options & (OPT_DELETE | OPT_ADD | OPT_FIX)) &&
        dstfilepath.empty()) {
        dstfilepath = makeFullPath(tmpname);
    }
}

/* ------------------------- sfnt Table Processing ------------------------- */

void SfntEdit::readHdr() {
    int16_t numTables = 0;

    /* Read and validate version */

    readObject(srcfile, sfnt.version);
    switch (sfnt.version) {
        case 0x00010000: /* 1.0 */
        case TAG('t', 'r', 'u', 'e'):
        case TAG('t', 'y', 'p', '1'):
        case TAG('b', 'i', 't', 's'):
            isTTF = true;
            break;
        case TAG('O', 'T', 'T', 'O'):
            break;
        case TAG('t', 't', 'c', 'f'):
        default:
            srcfile.close();
            fatal("unrecognized/unsupported file type [%s]",
                  srcfilepath.c_str());
    }

    /* Read rest of header */
    readObject(srcfile, numTables);
    readObject(srcfile, sfnt.searchRange);
    readObject(srcfile, sfnt.entrySelector);
    readObject(srcfile, sfnt.rangeShift);
    for (int i = 0; i < numTables; i++) {
        Tag tag;
        readObject(srcfile, tag);
        auto dre = sfnt.directory.emplace(tag, tag);
        Table &tbl = dre.first->second;
        readObject(srcfile, tbl.checksum);
        readObject(srcfile, tbl.offset);
        readObject(srcfile, tbl.length);
        tbl.flags |= TBL_SRC;
    }

    /* Check options have corresponding tables */
    for (auto [tg, tbl] : sfnt.directory) {
        if (tbl.flags & (OPT_EXTRACT | OPT_DELETE) && !(tbl.flags & TBL_SRC)) {
            logger->log(sWARNING, "table missing (%c%c%c%c)", TAG_ARG(tg));
        }
    }
}

/* Dump sfnt header */
void SfntEdit::dumpHdr(void) {
    int i = 0;

    std::printf("--- sfnt header [%s]\n", srcfilepath.c_str());
    if (sfnt.version == 0x00010000)
        std::printf("version      =1.0 (00010000)\n");
    else
        std::printf("version      =%c%c%c%c (%08x)\n",
                TAG_ARG(sfnt.version), sfnt.version);
    std::printf("numTables    =%hu\n", (uint16_t)sfnt.directory.size());
    std::printf("searchRange  =%hu\n", sfnt.searchRange);
    std::printf("entrySelector=%hu\n", sfnt.entrySelector);
    std::printf("rangeShift   =%hu\n", sfnt.rangeShift);

    std::printf("--- table directory [index]={tag,checksum,offset,length}\n");
    for (auto &[tag, tbl] : sfnt.directory) {
        std::printf("[%2d]={%c%c%c%c,%08x,%08x,%08x}\n", i++,
                    TAG_ARG(tag), tbl.checksum, tbl.offset, tbl.length);
    }
}

/* Calculate values of binary search table parameters */
static void calcSearchParams(unsigned nUnits,
                             uint16_t *searchRange,
                             uint16_t *entrySelector,
                             uint16_t *rangeShift) {
    unsigned log2;
    unsigned pwr2;

    pwr2 = 2;
    for (log2 = 0; pwr2 <= nUnits; log2++)
        pwr2 *= 2;
    pwr2 /= 2;

    *searchRange = ENTRY_SIZE * pwr2;
    *entrySelector = log2;
    *rangeShift = ENTRY_SIZE * (nUnits - pwr2);
}

/* Check that the table checksums and the head adjustment checksums are
   calculated correctly. Also validate the sfnt search fields */
void SfntEdit::checkChecksums() {
    int i;
    long nLongs;
    int fail = 0;
    uint16_t searchRange;
    uint16_t entrySelector;
    uint16_t rangeShift;
    uint32_t checkSumAdjustment = 0;
    uint32_t totalsum = 0;

    /* Validate sfnt search fields */
    calcSearchParams(sfnt.directory.size(), &searchRange,
                     &entrySelector, &rangeShift);
    if (sfnt.searchRange != searchRange) {
        logger->log(sWARNING, "bad sfnt.searchRange: file=%hu, calc=%hu",
                    sfnt.searchRange, searchRange);
        fail = 1;
    }
    if (sfnt.entrySelector != entrySelector) {
        logger->log(sWARNING, "bad sfnt.entrySelector: file=%hu, calc=%hu",
                    sfnt.entrySelector, entrySelector);
        fail = 1;
    }
    if (sfnt.rangeShift != rangeShift) {
        logger->log(sWARNING, "bad sfnt.rangeShift: file=%hu, calc=%hu",
                    sfnt.rangeShift, rangeShift);
        fail = 1;
    }

    /* Read directory header */
    srcfile.seekg(0, std::ios::beg);
    nLongs = (DIR_HDR_SIZE + ENTRY_SIZE * sfnt.directory.size()) / 4;
    while (nLongs--) {
        uint32_t amt;
        readObject(srcfile, amt);
        totalsum += amt;
    }

    for (auto &[tag, entry] : sfnt.directory) {
        uint32_t checksum = 0;
        uint32_t amt;

        srcfile.seekg(entry.offset, std::ios::beg);

        nLongs = (entry.length + 3) / 4;
        while (nLongs--) {
            readObject(srcfile, amt);
            checksum += amt;
        }

        if (tag == TAG('h', 'e', 'a', 'd')) {
            /* Adjust sum to ignore head.checkSumAdjustment field */
            srcfile.seekg(entry.offset + HEAD_ADJUST_OFFSET,
                          std::ios::beg);
            readObject(srcfile, checkSumAdjustment);
            checksum -= checkSumAdjustment;
        }

        if (entry.checksum != checksum) {
            logger->log(sWARNING,
                        "'%c%c%c%c' bad checksum: file=%08lx, calc=%08lx",
                        TAG_ARG(tag), entry.checksum, checksum);
            fail = 1;
        }

        totalsum += checksum;
    }

    totalsum = 0xb1b0afba - totalsum;
    if (!fail && totalsum != checkSumAdjustment) {
        logger->log(sWARNING,
                    "bad head.checkSumAdjustment: file=%08lx, calc=%08lx",
                    checkSumAdjustment, totalsum);
        fail = 1;
    }
    logger->log(sINFO, fail ? "icheck failed [%s]" : "check passed [%s]",
                srcfilepath.c_str());
}

/* Make extract filename from option filename or src filename plus table tag */
std::string SfntEdit::makexFilename(Tag tag, const Table &tbl) {
    if (!tbl.xfilename.empty())
        return tbl.xfilename.string();
    else {
        std::string tagstr;

        for (int i = 24; i >= 0; i -= 8) {
            char c = (char)(tag >> i);
            // Replace potentially troublesome filename chars by underscores
            if (!isalnum(c))
                c = '_';
            tagstr.push_back(c);
        }

        // Trim trailing underscores
        while (tagstr.back() == '_')
            tagstr.pop_back();

        fs::path filename = srcfilepath;
        filename.replace_extension(tagstr);

        return filename.string();
    }
}

/* Extract (-x) tables from source file */
void SfntEdit::extractTables() {
    for (auto &[tag, tbl] : sfnt.directory) {
        std::ofstream file;

        if (!(tbl.flags & OPT_EXTRACT))
            continue;

        if (!(tbl.flags & TBL_SRC))
            continue;
        std::string xfn = makexFilename(tag, tbl);
        file.open(xfn, std::ios::binary);
        if (file.fail())
            fatal("file error <could not open> [%s]", xfn.c_str());
        srcfile.seekg(tbl.offset, std::ios::beg);
        fileCopy(srcfile, file, tbl.length);
        file.close();
    }
}

/* Copy table and compute its checksum */
static uint32_t tableCopy(std::istream &src, std::ostream &dst,
                          long offset, long length) {
    int i;
    uint32_t value;
    uint32_t checksum = 0;

    src.seekg(offset, std::ios::beg);
    for (; length > 3; length -= 4) {
        readObject(src, value);
        writeObject(dst, value);
        checksum += value;
    }

    if (length == 0)
        return checksum;

    /* Read remaining bytes and pad to 4-byte boundary */
    value = 0;
    for (i = 0; i < length; i++) {
        char b;
        src.get(b);
        value = value << 8 | (uint8_t)b;
    }
    value <<= (4 - length) * 8;
    writeObject(dst, value);

    return checksum + value;
}

/* Add table from file */
uint32_t SfntEdit::addTable(const Table &tbl, std::ostream &dstfile,
                            uint32_t *length) {
    std::ifstream file;
    uint32_t checksum;

    file.open(tbl.afilename, std::ios::binary);
    if (file.fail())
        fatal("file error <could not open> [%s]", tbl.afilename.c_str());
    file.seekg(0, std::ios::end);
    *length = (uint32_t) file.tellg();
    checksum = tableCopy(file, dstfile, 0, *length);
    file.close();

    return checksum;
}

/* Copy tables from source file to destination file applying (-d, -a, and -f)
   options */
bool SfntEdit::sfntCopy() {
    int i;
    int nLongs;
    Tag *tags;
    uint16_t numDstTables;
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;
    uint32_t adjustOff;
    uint32_t totalsum;
    int headSeen = 0;
    FILE *f;
    bool changed = (options & OPT_FIX); /* write file only if we change it */

    /* Count destination tables */
    numDstTables = 0;
    for (auto &[tag, tbl] : sfnt.directory) {
        if (!(tbl.flags & OPT_DELETE))
            numDstTables++;
    }

    /* Skip directory */
    dstfile.seekp(DIR_HDR_SIZE + ENTRY_SIZE * numDstTables, std::ios::beg);

    std::vector<Tag> tagset;
    tagset.reserve(sfnt.directory.size());
    for (auto &[tag, tbl] : sfnt.directory)
        tagset.push_back(tag);

    std::sort(tagset.begin(), tagset.end(),
              cmpTagByOrder(isTTF ? ttfOrder : otfOrder));

    /* Write sfnt directory */
    totalsum = 0;
    for (auto tag : tagset) {
        auto result = sfnt.directory.find(tag);
        if (result == sfnt.directory.end())
            // Should never happen
            continue;
        auto &tbl = result->second;

        offset = dstfile.tellp();

        if (tbl.flags & OPT_ADD) {
            checksum = addTable(tbl, dstfile, &length);
            changed = true;
        } else {
            if (!(tbl.flags & TBL_SRC)) {
                continue; /* Skip table that is not in source font */
            } else if (tbl.flags & OPT_DELETE) {
                changed = true;
                continue; /* Skip deleted table */
            } else {
                length = tbl.length;
                checksum =
                    tableCopy(srcfile, dstfile, tbl.offset, length);
            }
        }

        if (tag == TAG('h', 'e', 'a', 'd')) {
            uint32_t b;
            /* Adjust sum to ignore head.checkSumAdjustment field */

            adjustOff = offset + HEAD_ADJUST_OFFSET;
            dstfile.seekg(adjustOff, std::ios::beg);
            readObject(dstfile, b);
            dstfile.seekg(0, std::ios::end);
            checksum -= b;
            headSeen = 1;
        }

        /* Update table entry */
        tbl.checksum = checksum;
        tbl.offset = offset;
        tbl.length = length;
        tbl.flags |= TBL_DST;

        totalsum += checksum;
    }

    if (!changed)
        return changed;

    /* Initialize sfnt header */
    calcSearchParams(numDstTables, &sfnt.searchRange,
                     &sfnt.entrySelector, &sfnt.rangeShift);

    /* Write sfnt header */
    dstfile.seekp(0, std::ios::beg);

    writeObject(dstfile, sfnt.version);
    writeObject(dstfile, numDstTables);
    writeObject(dstfile, sfnt.searchRange);
    writeObject(dstfile, sfnt.entrySelector);
    writeObject(dstfile, sfnt.rangeShift);

    /* Write sfnt directory */
    for (auto [tag, tbl] : sfnt.directory) {
        if (tbl.flags & TBL_DST) {
            writeObject(dstfile, tag);
            writeObject(dstfile, tbl.checksum);
            writeObject(dstfile, tbl.offset);
            writeObject(dstfile, tbl.length);
        }
    }

    /* Checksum sfnt header */

    dstfile.seekg(0, std::ios::beg);

    nLongs = (DIR_HDR_SIZE + ENTRY_SIZE * numDstTables) / 4;
    for (i = 0; i < nLongs; i++) {
        uint32_t b;
        readObject(dstfile, b);
        totalsum += b;
    }

    if (headSeen) {
        /* Write head.checkSumAdjustment */
        dstfile.seekp(adjustOff, std::ios::beg);
        uint32_t chksum = 0xb1b0afba - totalsum;
        writeObject(dstfile, chksum);
    }
    return changed;
}

int SfntEdit::run_prot(int argc, char *argv[]) {
    int i;
    bool changed = false;
    std::vector<std::vector<std::string>> script;

    doingScripting = false;

    parseArgs(argv+1, argv+argc);

    if (doingScripting) {
        if (scriptfilepath.empty())
            scriptfilepath = "sfntedit.scr";

        if (!fs::exists(scriptfilepath)) {
            fatal("Missing script file [%s]", scriptfilepath.c_str());
        }
        makeArgs(scriptfilepath, script);
        if (scriptfilepath.has_parent_path())
            sourcepath = scriptfilepath.parent_path();
    }

    do {
        if (!script.empty()) {
            std::vector<std::string> cmdl;
            cmdl.swap(script.back());
            script.pop_back();
            if (cmdl.size() < 2)
                continue;
            logger->log(sINDENT, "");
            logger->log(sINFO, "Executing command-line: ");
            std::string acc;
            for (auto &s : cmdl)
                acc += s + " ";
            logger->log(sINDENT, "%s %s", se_progname, acc.c_str());
            parseArgs(cmdl.begin(), cmdl.end());
        }

        if (!doingScripting && options & OPT_USAGE) {
            printUsage(se_progname);
            continue;
        }

        if (!doingScripting && options & OPT_HELP) {
            printHelp(se_progname);
            continue;
        }

        if (srcfilepath.empty()) {
            printUsage(se_progname);
            fatal("Missing/unrecognized font filename");
        }

        srcfile.open(srcfilepath, std::ios::binary);
        if (srcfile.fail())
            fatal("file error <could not open> [%s]", srcfilepath.c_str());

        if (!doingScripting && srcfilepath.has_parent_path())
            sourcepath = srcfilepath.parent_path();

        if (!dstfilepath.empty()) {  // Open destination file
            dstfile.open(dstfilepath, std::ios::in|std::ios::out|std::ios::binary|std::ios::trunc);
            if (dstfile.fail())
                fatal("file error <could not open> [%s]", dstfilepath.c_str());
        }

        /* Read sfnt header */
        readHdr();

        /* Process font */
        if (options & OPT_LIST)
            dumpHdr();
        else if (options & OPT_CHECK)
            checkChecksums();
        else {
            if (options & OPT_EXTRACT)
                extractTables();
            if (options & (OPT_DELETE | OPT_ADD | OPT_FIX))
                changed = sfntCopy();
        }

        /* Close files */
        srcfile.close();
        if (dstfile.is_open()) {
            dstfile.close();
            if (!changed) {
                fs::remove(dstfilepath);
            } else if (dstfilepath.filename() == tmpname) {  // XXX fs::equivalent not supported on OS X
                // Rename tmp file to source file
                fs::copy_file(srcfilepath, se_backupname,
                              fs::copy_options::overwrite_existing);
                fs::copy_file(dstfilepath, srcfilepath,
                              fs::copy_options::overwrite_existing);
                fs::remove(se_backupname);
                fs::remove(dstfilepath);
            }
        }
    } while (!script.empty());

    return 0;
}

int SfntEdit::run(int argc, char *argv[]) {
    try {
        return run_prot(argc, argv);
    } catch (const std::exception &ex) {
        if (fs::exists(tmpname))
            fs::remove(tmpname);
        logger->msg(sFATAL, ex.what());
        return 1;
    }
}

extern "C" int main__sfntedit(int argc, char *argv[]) {
    SfntEdit se;

    return se.run(argc, argv);
}
