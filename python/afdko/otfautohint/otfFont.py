# Copyright 2014 Adobe. All rights reserved.

"""
Utilities for converting between T2 charstrings and glyphData objects.
"""

import copy
import logging
import os
import subprocess
import tempfile
from fontTools.misc.psCharStrings import T2OutlineExtractor
from fontTools.ttLib import TTFont, newTable
from fontTools.misc.roundTools import noRound, otRound
from fontTools.varLib.varStore import VarStoreInstancer
from fontTools.varLib.cff import CFF2CharStringMergePen, MergeOutlineExtractor
# import subset.cff is needed to load the implementation for
# CFF.desubroutinize: the module adds this class method to the CFF and CFF2
# classes.
import fontTools.subset.cff
from typing import List, Dict

from . import fdTools, FontParseError
from .glyphData import glyphData

# keep linting tools quiet about unused import
assert fontTools.subset.cff is not None

log = logging.getLogger(__name__)

kStackLimit = 46
kStemLimit = 96


class SEACError(Exception):
    """Raised when a charString has an obsolete 'seac' operator"""
    pass


def _add_method(*clazzes):
    """Returns a decorator function that adds a new method to one or
    more classes."""
    def wrapper(method):
        done = []
        for clazz in clazzes:
            if clazz in done:
                continue  # Support multiple names of a clazz
            done.append(clazz)
            assert clazz.__name__ != 'DefaultTable', \
                'Oops, table class not found.'
            assert not hasattr(clazz, method.__name__), \
                "Oops, class '%s' has method '%s'." % (clazz.__name__,
                                                       method.__name__)
            setattr(clazz, method.__name__, method)
        return None
    return wrapper


def hintOn(i, hintMaskBytes):
    """Used to help convert T2 hintmask bytes to a boolean array"""
    byteIndex = int(i / 8)
    byteValue = hintMaskBytes[byteIndex]
    offset = 7 - (i % 8)
    return ((2**offset) & byteValue) > 0


def denormalizeValue(nv, a):
    l, d, u = a.minValue, a.defaultValue, a.maxValue
    assert l <= d <= u
    if nv == 0:
        v = d
    elif nv < 0:
        v = l + (d - l) * (1 + nv)
    else:
        v = d + (u - d) * nv
    return v


def denormalizeLocation(loc, axes):
    return {a.axisTag: denormalizeValue(loc.get(a.axisTag, a.defaultValue), a)
            for a in axes}


class T2ToGlyphDataExtractor(T2OutlineExtractor):
    """
    Inherits from the fontTools Outline Extractor and adapts some of the
    operator methods to match the "hint pen" interface of the glyphData class
    """
    def __init__(self, gd, localSubrs, globalSubrs, nominalWidthX,
                 defaultWidthX, readStems=True, readFlex=True):
        T2OutlineExtractor.__init__(self, gd, localSubrs, globalSubrs,
                                    nominalWidthX, defaultWidthX)
        self.glyph = gd
        self.readStems = readStems
        self.readFlex = readFlex
        self.hintMaskBytes = None
        self.vhintCount = 0
        self.hhintCount = 0

    def op_endchar(self, index):
        self.endPath()
        args = self.popallWidth()
        if args:  # It is a 'seac' composite character. Don't process
            raise SEACError

    def op_hflex(self, index):
        if self.readFlex:
            self.glyph.nextIsFlex()
        super().op_hflex(index)

    def op_flex(self, index):
        if self.readFlex:
            self.glyph.nextIsFlex()
        super().op_flex(index)

    def op_hflex1(self, index):
        if self.readFlex:
            self.glyph.nextIsFlex()
        super().op_hflex1(index)

    def op_flex1(self, index):
        if self.readFlex:
            self.glyph.nextIsFlex()
        super().op_flex1(index)

    def op_hstem(self, index):
        args = self.popallWidth()
        self.countHints(args, True)
        if not self.readStems:
            return
        self.glyph.hStem(args, False)

    def op_vstem(self, index):
        args = self.popallWidth()
        self.countHints(args, False)
        if not self.readStems:
            return
        self.glyph.vStem(args, False)

    def op_hstemhm(self, index):
        args = self.popallWidth()
        self.countHints(args, True)
        if not self.readStems:
            return
        self.glyph.hStem(args, True)

    def op_vstemhm(self, index):
        args = self.popallWidth()
        self.countHints(args, False)
        if not self.readStems:
            return
        self.glyph.vStem(args, True)

    def countHints(self, args, horiz):
        if horiz:
            self.hhintCount += len(args) // 2
        else:
            self.vhintCount += len(args) // 2

    def doMask(self, index, cntr=False):
        args = []
        if not self.hintMaskBytes:
            args = self.popallWidth()
            if args:
                # values after stems operators followed by a
                # hint/cntrmask is an implicit vstem(hm))
                self.countHints(args, False)
                self.glyph.vStem(args, None if cntr else True)

            self.hintMaskBytes = (self.vhintCount + self.hhintCount + 7) // 8

        hintMaskString, index = self.callingStack[-1].getBytes(
            index, self.hintMaskBytes)

        tc = self.hhintCount + self.vhintCount
        hhints = [hintOn(i, hintMaskString) for i in range(self.hhintCount)]
        vhints = [hintOn(i, hintMaskString)
                  for i in range(self.hhintCount, tc)]
        return hintMaskString, hhints, vhints, index

    def op_hintmask(self, index):
        hintString, hhints, vhints, index = self.doMask(index)
        if self.readStems:
            self.glyph.hintmask(hhints, vhints)
        return hintString, index

    def op_cntrmask(self, index):
        hintString, hhints, vhints, index = self.doMask(index, True)
        if self.readStems:
            self.glyph.cntrmask(hhints, vhints)
        return hintString, index


def convertT2ToGlyphData(t2CharString, readStems=True, readFlex=True,
                         roundCoords=True, name=None):
    """Wrapper method for T2ToGlyphDataExtractor execution"""
    gl = glyphData(roundCoords=roundCoords, name=name)
    subrs = getattr(t2CharString.private, "Subrs", [])
    extractor = T2ToGlyphDataExtractor(gl, subrs,
                                       t2CharString.globalSubrs,
                                       t2CharString.private.nominalWidthX,
                                       t2CharString.private.defaultWidthX,
                                       readStems, readFlex)
    extractor.execute(t2CharString)
    t2_width_arg = None
    if extractor.gotWidth and (extractor.width is not None):
        t2_width_arg = extractor.width - t2CharString.private.nominalWidthX
    gl.setWidth(t2_width_arg)
    return gl


def _run_tx(args):
    try:
        subprocess.check_call(["tx"] + args, stderr=subprocess.DEVNULL)
    except (subprocess.CalledProcessError, OSError) as e:
        raise FontParseError(e)


class CFFFontData:
    def __init__(self, path, font_format):
        self.inputPath = path
        self.font_format = font_format
        self.is_cff2 = False
        self.is_vf = False
        self.vs_data_models: List[VarDataModel] = []
        self.desc = None
        if font_format == "OTF":
            # It is an OTF font, we can process it directly.
            font = TTFont(path)
            if "CFF " in font:
                cff_format = "CFF "
            elif "CFF2" in font:
                cff_format = "CFF2"
                self.is_cff2 = True
            else:
                raise FontParseError("OTF font has no CFF table <%s>." % path)
        else:
            # Else, package it in an OTF font.
            cff_format = "CFF "
            if font_format == "CFF":
                with open(path, "rb") as fp:
                    data = fp.read()
            else:
                fd, temp_path = tempfile.mkstemp()
                os.close(fd)
                try:
                    _run_tx(["-cff", "+b", "-std", path, temp_path])
                    with open(temp_path, "rb") as fp:
                        data = fp.read()
                finally:
                    os.remove(temp_path)

            font = TTFont()
            font['CFF '] = newTable('CFF ')
            font['CFF '].decompile(data, font)

        self.ttFont = font
        self.cffTable = font[cff_format]

        # for identifier in glyph-list:
        # Get charstring.
        self.topDict = self.cffTable.cff.topDictIndex[0]
        self.charStrings = self.topDict.CharStrings
        if 'fvar' in self.ttFont:
            # have not yet collected VF global data.
            self.is_vf = True
            self.glyph_programs = []
            self.vsIndexMap = {}
            self.axes = self.ttFont['fvar'].axes
            CFF2 = self.cffTable
            CFF2.desubroutinize()
            topDict = CFF2.cff.topDictIndex[0]
            # We need a new charstring object into which we can save the
            # hinted CFF2 program data. Copying an existing charstring is a
            # little easier than creating a new one and making sure that all
            # properties are set correctly.
            self.temp_cs_extract = copy.deepcopy(self.charStrings['.notdef'])
            self.temp_cs_merge = copy.deepcopy(self.charStrings['.notdef'])
            self.vs_data_models = self.get_vs_data_models(topDict, self.axes)

    def getGlyphList(self):
        return self.ttFont.getGlyphOrder()

    def getPSName(self):
        if self.is_cff2 and 'name' in self.ttFont:
            psName = next((name_rec.string for name_rec in self.ttFont[
                'name'].names if (name_rec.nameID == 6) and (
                    name_rec.platformID == 3)))
            psName = psName.decode('utf-16be')
        else:
            psName = self.cffTable.cff.fontNames[0]
        return psName

    def get_min_max(self, upm):
        if self.is_cff2 and 'hhea' in self.ttFont:
            font_max = self.ttFont['hhea'].ascent
            font_min = self.ttFont['hhea'].descent
        elif hasattr(self.topDict, 'FontBBox'):
            font_max = self.topDict.FontBBox[3]
            font_min = self.topDict.FontBBox[1]
        else:
            font_max = upm * 1.25
            font_min = -upm * 0.25
        return font_min, font_max

    def convertToGlyphData(self, glyphName, readStems, readFlex,
                           roundCoords, doAll=False):
        t2CharString = self.charStrings[glyphName]
        try:
            gl = convertT2ToGlyphData(t2CharString, readStems, readFlex,
                                      roundCoords, name=glyphName)
        except SEACError:
            log.warning("Skipping %s: can't process SEAC composite glyphs.",
                        glyphName)
            gl = None
        return gl

    def updateFromGlyph(self, gl, glyphName):
        if gl is None:
            return
        t2Program = gl.T2()

        if not self.is_cff2:
            t2_width_arg = gl.getWidth()
            if t2_width_arg is not None:
                t2Program = [t2_width_arg] + t2Program
        if self.vs_data_models is not None:
            # It is a variable font. Accumulate the charstrings.
            self.glyph_programs.append(t2Program)
        else:
            # This is an MM source font. Update the font's charstring directly.
            t2CharString = self.charStrings[glyphName]
            # Not used but useful for debugging save failures
            t2CharString._name = glyphName
            t2CharString.program = t2Program

    def save(self, path):
        if path is None:
            path = self.inputPath

        if self.font_format == "OTF":
            self.ttFont.save(path)
            self.ttFont.close()
        else:
            data = self.ttFont["CFF "].compile(self.ttFont)
            if self.font_format == "CFF":
                with open(path, "wb") as fp:
                    fp.write(data)
            else:
                fd, temp_path = tempfile.mkstemp()
                os.write(fd, data)
                os.close(fd)

                try:
                    args = ["-t1", "-std"]
                    if self.font_format == "PFB":
                        args.append("-pfb")
                    _run_tx(args + [temp_path, path])
                finally:
                    os.remove(temp_path)

    def close(self):
        self.ttFont.close()

    def isCID(self):
        # XXX wrong for variable font
        return hasattr(self.topDict, "FDSelect")

    def getPrivateDictVal(self, pDict, attr, default, vsindex, vsi):
        val = getattr(pDict, attr, default)
        if attr in ('BlueValues', 'OtherBlues', 'FamilyBlues',
                    'FamilyOtherBlues', 'StemSnapH', 'StemSnapV'):
            last_def = 0
            last_calc = 0
            vlist = []
            for dl in val:
                if isinstance(dl, list):
                    assert vsi is not None
                    calc = (dl[0] - last_def +
                            vsi.interpolateFromDeltas(vsindex, dl[1:]))
                    vlist.append(last_calc + calc)
                    last_def = dl[0]
                    last_calc += calc
                else:
                    vlist.append(dl)
                    last_def = last_calc = dl
            val = vlist
        else:
            if isinstance(val, list):
                assert vsi is not None
                val = val[0] + vsi.interpolateFromDeltas(vsindex, val[1:])

        return val

    def getPrivateFDDict(self, allowNoBlues, noFlex, vCounterGlyphs,
                         hCounterGlyphs, desc, fdIndex=0, gl_vsindex=None):
        pTopDict = self.topDict
        if hasattr(pTopDict, "FDArray"):
            pDict = pTopDict.FDArray[fdIndex]
        else:
            assert fdIndex == 0
            pDict = pTopDict
        privateDict = pDict.Private

        if self.is_vf:
            dict_vsindex = getattr(privateDict, 'vsindex', 0)
            if gl_vsindex is None:
                gl_vsindex = dict_vsindex
            vs_data_model = self.vs_data_models[gl_vsindex]
            vsi_list = vs_data_model.master_vsi_list
        else:
            assert gl_vsindex is None
            dict_vsindex = None
            vsi_list = [None]
            fontName = desc

        fdDictArray = []

        for vn, vsi in enumerate(vsi_list):
            if self.is_vf:
                fontName = self.getInstanceName(vn, vsi, self.axes)
                if fontName is None:
                    fontName = "Model %d Instance %d" % (gl_vsindex, vn)
            fdDict = fdTools.FDDict(fdIndex, fontName=fontName)
            fdDict.setInfo('LanguageGroup',
                           self.getPrivateDictVal(privateDict,
                                                  'LanguageGroup', 0,
                                                  dict_vsindex, vsi))

            if not self.is_cff2 and hasattr(pDict, "FontMatrix"):
                fontMatrix = pDict.FontMatrix
                upm = int(1 / fontMatrix[0])
            elif hasattr(pTopDict, 'FontMatrix'):
                fontMatrix = pTopDict.FontMatrix
                upm = int(1 / fontMatrix[0])
            else:
                upm = 1000
            fdDict.setInfo('OrigEmSqUnits', upm)

            for bvattr, bvkeys in (('BlueValues',
                                    fdTools.kBlueValueKeys),
                                   ('OtherBlues',
                                    fdTools.kOtherBlueValueKeys)):
                bvs = self.getPrivateDictVal(privateDict, bvattr, [],
                                             dict_vsindex, vsi)
                numbvs = len(bvs)
                if bvattr == 'BlueValues' and numbvs < 4:
                    low, high = self.get_min_max(upm)
                    low = min(-upm * 0.25, low)
                    high = max(upm * 1.25, high)
                    # Make a set of inactive alignment zones: zones outside
                    # of the font BBox so as not to affect hinting. Used
                    # when source font has no BlueValues or has invalid
                    # BlueValues. Some fonts have bad BBox values, so I
                    # don't let this be smaller than -upm*0.25, upm*1.25.
                    inactiveAlignmentValues = [low, low, high, high]
                    if allowNoBlues or self.is_vf:
                        # XXX adjusted for vf
                        bvs = inactiveAlignmentValues
                        numbvs = len(bvs)
                    else:
                        raise FontParseError("Font must have at least four " +
                                             "values in its %s " % bvattr +
                                             "array for otfautohint to work!")
                    bvs.sort()

                # The first pair only is a bottom zone, where the first value
                # is the overshoot position. The rest are top zones, and second
                # value of the pair is the overshoot position.
                if bvattr == 'BlueValues':
                    bvs[0] = bvs[0] - bvs[1]
                    for i in range(3, numbvs, 2):
                        bvs[i] = bvs[i] - bvs[i - 1]
                else:
                    for i in range(0, numbvs, 2):
                        bvs[i] = bvs[i] - bvs[i + 1]

                numbvs = min(numbvs, len(bvkeys))
                for i in range(numbvs):
                    key = bvkeys[i]
                    value = bvs[i]
                    fdDict.setInfo(key, value)

            for ssnap, stdw, fdkey in (('StemSnapV', 'StdVW', 'DominantV'),
                                       ('StemSnapH', 'StdHW', 'DominantH')):
                if hasattr(privateDict, ssnap):
                    sstems = self.getPrivateDictVal(privateDict, ssnap,
                                                    [], dict_vsindex, vsi)
                elif hasattr(privateDict, stdw):
                    sstems = [self.getPrivateDictVal(privateDict, stdw,
                                                     -1, dict_vsindex, vsi)]
                else:
                    if allowNoBlues or self.is_vf:
                        # XXX adjusted for vf
                        # dummy value. Needs to be larger than any hint will
                        # likely be, as the autohint program strips out any
                        # hint wider than twice the largest global stem width.
                        sstems = [upm]
                    else:
                        raise FontParseError("Font has neither %s nor %s!" %
                                             (ssnap, stdw))
                sstems.sort()
                if (len(sstems) == 0) or ((len(sstems) == 1) and
                                          (sstems[0] < 1)):
                    sstems = [upm]  # dummy value that will allow PyAC to run
                    log.warning("There is no value or 0 value for %s." % fdkey)
                fdDict.setInfo(fdkey, sstems)

            if noFlex:
                fdDict.setInfo('FlexOK', False)
            else:
                fdDict.setInfo('FlexOK', True)

            # Add candidate lists for counter hints, if any.
            if vCounterGlyphs:
                fdDict.setInfo('VCounterChars', vCounterGlyphs)
            if hCounterGlyphs:
                fdDict.setInfo('HCounterChars', hCounterGlyphs)

            fdDict.setInfo('BlueFuzz',
                           self.getPrivateDictVal(privateDict, 'BlueFuzz',
                                                  1, dict_vsindex, vsi))
            fdDict.buildBlueLists()

            if not self.is_vf:
                return fdDict

            fdDictArray.append(fdDict)

        assert self.is_vf
        return fdDictArray, gl_vsindex

    def getfdIndex(self, name):
        try:
            gid = self.ttFont.getGlyphID(name)
        except KeyError:
            return None
        if gid is None:
            return None
        if hasattr(self.topDict, "FDSelect"):
            fdIndex = self.topDict.FDSelect[gid]
        else:
            fdIndex = 0
        return fdIndex

    def isVF(self):
        return self.is_vf

    def getInputPath(self):
        return self.inputPath

    def getPrivateDict(self, fdIndex):
        topDict = self.topDict
        if hasattr(topDict, "FDArray"):
            assert fdIndex < len(topDict.FDArray)
            private = topDict.FDArray[fdIndex].Private
        else:
            assert fdIndex == 0
            private = topDict.Private
        return private

    def mergePrivateMap(self, privateMap):
        privateDict = self.getPrivateDict(0)
        for k, v in privateMap.items():
            setattr(privateDict, k, v)

    @staticmethod
    def getInstanceName(vn, vsi, axes):
        if vn == 0:
            vals = (f'{v:g}' for v in (a.defaultValue for a in axes))
            return 'DEF{' + ','.join(vals) + '}'
        else:
            vals = (f'{v:g}'
                    for v in denormalizeLocation(vsi.location, axes).values())
        return '{' + ','.join(vals) + '}'

    def get_vf_glyphs(self, glyph_name):
        charstring = self.charStrings[glyph_name]

        if 'vsindex' in charstring.program:
            op_index = charstring.program.index('vsindex')
            vsindex = charstring.program[op_index - 1]
            self.vsIndexMap[glyph_name] = (vsindex, True)
        else:
            privateDict = self.getPrivateDict(self.getfdIndex(glyph_name))
            vsindex = getattr(privateDict, 'vsindex', 0)
            self.vsIndexMap[glyph_name] = (vsindex, False)
        vs_data_model = self.vs_data_models[vsindex]

        glyph_list = []
        for vsi in vs_data_model.master_vsi_list:
            iFD = vsi.interpolateFromDeltas
            t2_program = interpolate_cff2_charstring(charstring, glyph_name,
                                                     iFD, vsindex)
            self.temp_cs_extract.program = t2_program
            glyph = convertT2ToGlyphData(self.temp_cs_extract, True, False,
                                         True, name=glyph_name)
            glyph_list.append(glyph)

        return glyph_list, vsindex

    @staticmethod
    def get_vs_data_models(topDict, axes):
        otvs = topDict.VarStore.otVarStore
        region_list = otvs.VarRegionList.Region
        axis_tags = [axis_entry.axisTag for axis_entry in axes]
        vs_data_models = []
        for vsindex, var_data in enumerate(otvs.VarData):
            vsi = VarStoreInstancer(topDict.VarStore.otVarStore, axes, {})
            master_vsi_list = [vsi]
            for region_idx in var_data.VarRegionIndex:
                region = region_list[region_idx]
                loc = {}
                for i, axis in enumerate(region.VarRegionAxis):
                    loc[axis_tags[i]] = axis.PeakCoord
                vsi = VarStoreInstancer(topDict.VarStore.otVarStore, axes,
                                        loc)
                master_vsi_list.append(vsi)
            vdm = VarDataModel(var_data, vsindex, master_vsi_list)
            vs_data_models.append(vdm)
        return vs_data_models

    def merge_hinted_glyphs(self, name):
        vsindex, addIndex = self.vsIndexMap[name]
        new_t2cs = merge_hinted_programs(self.temp_cs_merge,
                                         self.glyph_programs, name,
                                         self.vs_data_models[vsindex])
        if addIndex:
            new_t2cs.program = [vsindex, 'vsindex'] + new_t2cs.program
        self.charStrings[name] = new_t2cs
        self.glyph_programs = []


def interpolate_cff2_charstring(charstring, gname, interpolateFromDeltas,
                                vsindex):
    # Interpolate charstring
    # e.g replace blend op args with regular args,
    # and discard vsindex op.
    new_program = []
    last_i = 0
    seenVOP = False
    program = charstring.program
    for i, token in enumerate(program):
        if token == 'vsindex':
            if last_i != 0:
                new_program.extend(program[last_i:i - 1])
            last_i = i + 1
            seenVOP = True
        elif token == 'blend':
            num_regions = charstring.getNumRegions(vsindex)
            numInstances = 1 + num_regions
            num_args = program[i - 1]
            # The program list starting at program[i] is now:
            # ..args for following operations
            # num_args values  from the default font
            # num_args tuples, each with numInstances-1 delta values
            # num_blend_args
            # 'blend'
            argi = i - (num_args * numInstances + 1)
            if last_i != argi:
                new_program.extend(program[last_i:argi])
            end_args = tuplei = argi + num_args
            master_args = []
            while argi < end_args:
                next_ti = tuplei + num_regions
                deltas = program[tuplei:next_ti]
                val = interpolateFromDeltas(vsindex, deltas)
                master_val = program[argi]
                master_val += otRound(val)
                master_args.append(master_val)
                tuplei = next_ti
                argi += 1
            new_program.extend(master_args)
            last_i = i + 1
            seenVOP = True
    if last_i != 0 or not seenVOP:
        new_program.extend(program[last_i:])
    return new_program


def merge_hinted_programs(charstring, t2_programs, gname, vs_data_model):
    num_masters = vs_data_model.num_masters
    var_pen = CFF2CharStringMergePen([], gname, num_masters, 0)
    charstring.outlineExtractor = MergeOutlineExtractor

    for i, t2_program in enumerate(t2_programs):
        var_pen.restart(i)
        charstring.program = t2_program
        charstring.draw(var_pen)

    new_charstring = var_pen.getCharString(
        private=charstring.private,
        globalSubrs=charstring.globalSubrs,
        var_model=vs_data_model, optimize=True)
    return new_charstring


@_add_method(VarStoreInstancer)
def get_scalars(self, vsindex, region_idx) -> Dict[int, float]:
    varData = self._varData
    # The index key needs to be the master value index, which includes
    # the default font value. VarRegionIndex provides the region indices.
    scalars = {0: 1.0}  # The default font always has a weight of 1.0
    region_index = varData[vsindex].VarRegionIndex
    for idx in range(region_idx):  # omit the scalar for the region.
        scalar = self._getScalar(region_index[idx])
        if scalar:
            scalars[idx + 1] = scalar
    return scalars


class VarDataModel(object):

    def __init__(self, var_data, vsindex, master_vsi_list):
        self.var_data = var_data
        self.master_vsi_list = master_vsi_list
        self._num_masters = len(master_vsi_list)

        # for default font value
        self.delta_weights: List[Dict[int, float]] = [{}]
        for region_idx, vsi in enumerate(master_vsi_list[1:]):
            scalars = vsi.get_scalars(vsindex, region_idx)
            self.delta_weights.append(scalars)

    @property
    def num_masters(self):
        return self._num_masters

    def getDeltas(self, master_values, *, round=noRound) -> List[float]:
        assert len(master_values) == len(self.delta_weights)
        out: List[float] = []
        for i, scalars in enumerate(self.delta_weights):
            delta = master_values[i]
            for j, scalar in scalars.items():
                if scalar:
                    delta -= out[j] * scalar
            out.append(round(delta))
        return out
