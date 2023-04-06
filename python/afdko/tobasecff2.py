import numbers

from fontTools.ttLib import TTFont, newTable
from fontTools.varLib.cff import convertCFFtoCFF2
from fontTools.fontBuilder import FontBuilder
from fontTools.misc.psCharStrings import T2WidthExtractor

topStrings = {"FullName": "fullName",
              "Copyright": "copyright",
              "FamilyName": "familyName"}


class CFFDefWidthDecompiler(T2WidthExtractor):
    """ This class is used to remove the initial width from the CFF
    charstring without trying to add the width to self.nominalWidthX,
    which is None. """
    def popallWidth(self, evenOdd=0):
        args = self.popall()
        if not self.gotWidth:
            if evenOdd ^ (len(args) % 2):
                assert self.defaultWidthX is not None, ("CFF2 CharStrings "
                                                        "must not have an "
                                                        "initial width value")
                self.width = self.nominalWidthX + args[0]
                args = args[1:]
                self.defWidth = False
            else:
                self.width = self.defaultWidthX
                self.defWidth = True
            self.gotWidth = 1
        return args


def toBaseCFF2(cffFile, outname):
    f = TTFont()
    cffTable = newTable('CFF ')
    f['CFF '] = cffTable
    cffTable.cff.decompile(cffFile, f)
    nameStrings = {}
    cffTable.cff.desubroutinize()
    fontName = cffTable.cff.fontNames[0]
    nameStrings["psName"] = fontName
    top = cffTable.cff[fontName]
    cs = top.CharStrings

    for a_top, k_name in topStrings.items():
        if hasattr(top, a_top):
            nameStrings[k_name] = getattr(top, a_top)

    unitsPerEm = 1000
    if hasattr(top, "FontMatrix"):
        fm = getattr(top, "FontMatrix")
        mfm = max([abs(v) for v in fm[:4]])
        unitsPerEm = int(1 / mfm)

    hMetrics = dict()
    for g in top.charset:
        c, _ = cs.getItemAndSelector(g)
        subrs = getattr(c.private, "Subrs", [])
        decompiler = CFFDefWidthDecompiler(subrs, c.globalSubrs,
                                           c.private.nominalWidthX,
                                           c.private.defaultWidthX)
        decompiler.execute(c)
        bb = c.calcBounds(None)
        if bb is not None:
            lsb = bb[0]
        else:
            lsb = 0
        hMetrics[g] = (decompiler.width, lsb)
        if not decompiler.defWidth:
            assert c.program and isinstance(c.program[0], numbers.Number)
            c.program.pop(0)
#        c.compile(True)

    glyphOrder = f.getGlyphOrder()
    top.charset = glyphOrder
    convertCFFtoCFF2(f)

    fb = FontBuilder(unitsPerEm, isTTF=False)
    fb.setupGlyphOrder(glyphOrder)
    fb.font.sfntVersion = 'OTTO'
    cff2Table = f['CFF2']
    cff2Table.cff.otFont = fb.font
    fb.font['CFF2'] = cff2Table
    fb.setupHorizontalMetrics(hMetrics)
    fb.setupHorizontalHeader()
    fb.setupNameTable(nameStrings)
    fb.setupPost(True)
    fb.font['post'].glyphOrder = glyphOrder
    fb.save(outname)
