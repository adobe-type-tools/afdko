/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_GDEF_H_
#define ADDFEATURES_HOTCONV_GDEF_H_

#include <array>
#include <memory>
#include <vector>
#include <utility>

#include "common.h"
#include "FeatCtx.h"
#include "otl.h"
#include "varsupport.h"

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
        explicit GlyphClassTable(GDEF &h) : cac(h.g), h(h) {}
        void set(GPat::ClassRec &simple, GPat::ClassRec &ligature,
                 GPat::ClassRec &mark, GPat::ClassRec &component);
        static const char *names[4];
        static const uint16_t classIDmap[4];
        Offset fill(Offset offset);
        void write() {
            if (!offset)
                return;
            cac.classWrite();
        }
        Offset offset {0};
        CoverageAndClass cac;
        std::array<GPat::ClassRec, 4> glyphClasses;
        GDEF &h;
    };

    struct MarkAttachClassTable {
        MarkAttachClassTable() = delete;
        explicit MarkAttachClassTable(GDEF &h) : cac(h.g), h(h) {}
        uint16_t add(GPat::ClassRec &&markClassNode);
        void validate();
        Offset fill(Offset offset);
        void write() {
            if (!offset)
                return;
            cac.classWrite();
        }

        Offset offset {0};
        CoverageAndClass cac;
        std::vector<GPat::ClassRec> glyphClasses;
        GDEF &h;
    };

    struct AttachTable {
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

        AttachTable() = delete;
        explicit AttachTable(GDEF &h) : cac(h.g) {}
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

    struct LigCaretTable {
        struct LigGlyphEntry {
            struct CaretTable {
                struct comparator {
                    comparator() = delete;
                    explicit comparator(const VarTrackVec &values) : values(values) {}
                    bool operator()(const std::unique_ptr<CaretTable> &a,
                                    const std::unique_ptr<CaretTable> &b) const {
                        return a->sortValue(values) < b->sortValue(values);
                    }
                    const VarTrackVec &values;
                };
                virtual LOffset size(const VarTrackVec &values) = 0;
                virtual uint16_t format(const VarTrackVec &values) = 0;
                virtual void write(GDEF *h, const VarTrackVec &values) = 0;
                virtual int16_t sortValue(const VarTrackVec &values) {
                    return 0;
                }
                Offset offset {0};
            };
            struct CoordCaretTable : public CaretTable {
                CoordCaretTable() = delete;
                explicit CoordCaretTable(uint32_t vi) : valueIndex(vi) {}
                LOffset size(const VarTrackVec &values) override {
                    return sizeof(uint16_t) + sizeof(int16_t);
                }
                int16_t sortValue(const VarTrackVec &values) override {
                    return values[valueIndex].getDefault();
                }
                uint16_t format(const VarTrackVec &values) override {
                    return 1;
                }
                void write(GDEF *h, const ValTrackVec &values) override {
                    OUT2(format());
                    OUT2(values[valueIndex].getDefault());
                }
                uint32_t valueIndex;
            };
            struct PointCaretTable : public CaretTable {
                // Don't sort point records
                PointCaretTable() = delete;
                explicit PointCaretTable(uint16_t point) : point(point) {}
                LOffset size(const VarTrackVec &values) override {
                    return 2 * sizeof(uint16_t);
                }
                uint16_t format(const VarTrackVec &values) override {
                    return 2;
                }
                void write(GDEF *h, const ValTrackVec &values) override {
                    OUT2(format());
                    OUT2(point);
                }
                uint16_t point;
            };
            static LOffset size(uint32_t numCarets) {
                return sizeof(uint16_t) * (1 + numCarets);
            }
            bool operator < (const LigGlyphEntry &rhs) const {
                return gid < rhs.gid;
            }
            LigGlyphEntry() = delete;
            explicit LigGlyphEntry(GID gid) : gid(gid) {}
            GID gid {GID_UNDEF};
            Offset offset {0};
            std::vector<std::unique_ptr<CaretTable>> caretTables;
        };

        LigCaretTable() = delete;
        explicit LigCaretTable(GDEF &h) : cac(h.g), h(h) {}
        static LOffset size(uint32_t glyphCount) {
            return sizeof(uint16_t) * (2 + glyphCount);
        }
        bool warnGid(GID gid);
        void addCoords(GID gid, std::vector<uint32_t> valIndexes, ValueVector &values);
        void addPoints(GID gid, std::vector<uint16_t> &points);
        Offset fill(Offset offset);
        void write(GDEF *h);

        Offset offset {0};
        Offset Coverage {0};
        CoverageAndClass cac;
        std::vector<LigGlyphEntry> ligCaretEntries;
        GDEF &h;
    };

    struct MarkSetFilteringTable {
        explicit MarkSetFilteringTable(GDEF &h) : h(h) {}
        struct MarkSetEntry {
            MarkSetEntry() = delete;
            explicit MarkSetEntry(GDEF &h) : cac(h.g) {}
            LOffset MarkSetCoverage {0};
            CoverageAndClass cac;
        };

        static LOffset size(uint32_t markSetCount) {
            return sizeof(uint16_t) * 2 + sizeof(uint32_t) * markSetCount;
        }
        uint16_t add(GPat::ClassRec &&markClassNode);
        Offset fill(Offset offset);
        void write(GDEF *h);

        Offset offset {0};
        uint16_t markSetTableFormat {0};
        std::vector<GPat::ClassRec> markSetClasses;
        std::vector<MarkSetEntry> markSetEntries;
        GDEF &h;
    };


    GDEF() = delete;
    explicit GDEF(hotCtx g) : g(g), glyphClassTable(*this), attachTable(*this),
                              ligCaretTable(*this), markAttachClassTable(*this),
                              markSetClassTable(*this) {}

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
        glyphClassTable.set(simpl, ligature, mark, component);
    }
    bool addAttachEntry(GID gid, uint16_t contour) {
        return attachTable.add(gid, contour);
    }
    void addLigCaretCoords(GID gid, ValueVector &coords) {
        std::vector<uint32_t> valIndexes;
        for (auto &vvr : coords)
            valIndexes.push_back(ivs.addValue(foo, vvr));
        ligCaretTable.addCoords(gid, valIndexes, values);
    }
    void addLigCaretPoints(GID gid, std::vector<uint16_t> &points) {
        ligCaretTable.addPoints(gid, points);
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

    hotCtx g;

    GlyphClassTable glyphClassTable;
    AttachTable attachTable;
    LigCaretTable ligCaretTable;
    MarkAttachClassTable markAttachClassTable;
    MarkSetFilteringTable markSetClassTable;

    itemVariationStore ivs;
};

#endif  // ADDFEATURES_HOTCONV_GDEF_H_
