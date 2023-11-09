/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_GDEF_H_
#define ADDFEATURES_HOTCONV_GDEF_H_

#include <array>
#include <utility>
#include <vector>

#include "common.h"
#include "FeatCtx.h"
#include "otl.h"

#define GDEF_ TAG('G', 'D', 'E', 'F')
#define kMaxMarkAttachClasses 15

/* Standard functions */
void GDEFNew(hotCtx g);
int GDEFFill(hotCtx g);
void GDEFWrite(hotCtx g);
void GDEFReuse(hotCtx g);
void GDEFFree(hotCtx g);

class GDEF {
 public:
    struct GlyphClassTable {
        GlyphClassTable() = delete;
        explicit GlyphClassTable(hotCtx g) : cac(g) {}
        void set(GPat::ClassRec &simple, GPat::ClassRec &ligature,
                 GPat::ClassRec &mark, GPat::ClassRec &component, hotCtx g);
        static const char *names[4];
        static const uint16_t classIDmap[4];
        Offset fill(hotCtx g, Offset offset);
        void write() {
            if (!offset)
                return;
            cac.classWrite();
        }
        Offset offset {0};
        CoverageAndClass cac;
        std::array<GPat::ClassRec, 4> glyphClasses;
    };

    struct MarkAttachClassTable {
        MarkAttachClassTable() = delete;
        explicit MarkAttachClassTable(hotCtx g) : cac(g) {}
        uint16_t add(GPat::ClassRec &&markClassNode);
        void validate(hotCtx g);
        Offset fill(hotCtx g, Offset offset);
        void write() {
            if (!offset)
                return;
            cac.classWrite();
        }

        Offset offset {0};
        CoverageAndClass cac;
        std::vector<GPat::ClassRec> glyphClasses;
    };

    struct AttachEntry {
        explicit AttachEntry(GID gid) : gid(gid) {}
        static LOffset size(uint32_t numContours) {
            return sizeof(uint16_t) * (1 + numContours);
        }
        bool operator < (const AttachEntry &rhs) const {
            return gid < rhs.gid;
        }
        Offset offset {0};
        GID gid {GID_UNDEF};
        std::vector<uint16_t> contourIndices;
    };

    struct AttachTable {
        AttachTable() = delete;
        explicit AttachTable(hotCtx g) : cac(g) {}
        static LOffset size(uint32_t glyphCount) {
            return sizeof(uint16_t) * (2 + glyphCount);
        }
        bool add(GID gid, uint16_t contour);
        Offset fill(Offset offset);
        void write(GDEF *h);

        Offset offset {0};
        Offset Coverage {0};
        CoverageAndClass cac;
        std::vector<AttachEntry> attachEntries;
    };

    struct CaretTable {
        CaretTable(uint16_t cv, uint16_t format) : CaretValue(cv), format(format) {}
        bool operator < (const CaretTable &rhs) const {
            if (format != rhs.format)
                return format < rhs.format;
            if (format == 1)
                return (int16_t) CaretValue < (int16_t) rhs.CaretValue;
            else
                return (uint16_t) CaretValue < (uint16_t) rhs.CaretValue;
        }
        static LOffset size() { return sizeof(uint16_t) * 2; }
        Offset offset {0};
        uint16_t CaretValue {0};
        uint16_t format {0};
    };

    struct LigGlyphEntry {
        static LOffset size(uint32_t numCarets) {
            return sizeof(uint16_t) * (1 + numCarets);
        }
        bool operator < (const LigGlyphEntry &rhs) const {
            return gid < rhs.gid;
        }
        LigGlyphEntry(GID gid, uint16_t format) : gid(gid), format(format) {}
        GID gid {GID_UNDEF};
        uint16_t format {0};
        Offset offset {0};
        std::vector<CaretTable> caretTables;
    };

    struct LigCaretTable {
        LigCaretTable() = delete;
        explicit LigCaretTable(hotCtx g) : cac(g) {}
        static LOffset size(uint32_t glyphCount) {
            return sizeof(uint16_t) * (2 + glyphCount);
        }
        void add(GID gid, std::vector<uint16_t> &caretValues,
                 uint16_t format, hotCtx g);
        Offset fill(Offset offset);
        void write(GDEF *h);

        Offset offset {0};
        Offset Coverage {0};
        CoverageAndClass cac;
        std::vector<LigGlyphEntry> ligCaretEntries;
    };

    struct MarkSetEntry {
        MarkSetEntry() = delete;
        explicit MarkSetEntry(hotCtx g) : cac(g) {}
        LOffset MarkSetCoverage {0};
        CoverageAndClass cac;
    };

    struct MarkSetFilteringTable {
        static LOffset size(uint32_t markSetCount) {
            return sizeof(uint16_t) * 2 + sizeof(uint32_t) * markSetCount;
        }
        uint16_t add(GPat::ClassRec &&markClassNode);
        Offset fill(hotCtx G, Offset offset);
        void write(GDEF *h);

        Offset offset {0};
        uint16_t markSetTableFormat {0};
        std::vector<GPat::ClassRec> markSetClasses;
        std::vector<MarkSetEntry> markSetEntries;
    };


    GDEF() = delete;
    explicit GDEF(hotCtx g) : glyphClassTable(g), attachTable(g),
                              ligCaretTable(g), markAttachClassTable(g),
                              g(g) {}

    int Fill();
    void Write();

    static LOffset headerSize(uint16_t markSetClassCnt) {
        if (markSetClassCnt > 0)
            return sizeof(Fixed) + 5 * sizeof(Offset);
        else
            return sizeof(Fixed) + 4 * sizeof(Offset);
    }
    void setGlyphClass(GPat::ClassRec &simpl, GPat::ClassRec &ligature,
                       GPat::ClassRec &mark, GPat::ClassRec &component) {
        glyphClassTable.set(simpl, ligature, mark, component, g);
    }
    bool addAttachEntry(GID gid, uint16_t contour) {
        return attachTable.add(gid, contour);
    }
    void addLigCaretEntry(GID gid, std::vector<uint16_t> &caretValues,
                          uint16_t format) {
        ligCaretTable.add(gid, caretValues, format, g);
    }
    uint16_t addGlyphMarkClass(GPat::ClassRec &&markClass) {
        return markAttachClassTable.add(std::move(markClass));
    }
    uint16_t addMarkSetClass(GPat::ClassRec &&markClass) {
        return markSetClassTable.add(std::move(markClass));
    }

    void validateGlyphClasses(std::vector<GPat::ClassRec> &glyphClasses);

    uint32_t version {0};
    Offset offset {0};

    GlyphClassTable glyphClassTable;
    AttachTable attachTable;
    LigCaretTable ligCaretTable;
    MarkAttachClassTable markAttachClassTable;
    MarkSetFilteringTable markSetClassTable;

    hotCtx g;
};

#endif  // ADDFEATURES_HOTCONV_GDEF_H_
