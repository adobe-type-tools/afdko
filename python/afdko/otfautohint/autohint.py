# Copyright 2016 Adobe. All rights reserved.

# Methods:

# Parse args. If glyphlist is from file, read in entire file as single string,
# and remove all white space, then parse out glyph-names and GID's.

import logging
import os
import time
from collections import namedtuple
from threading import Thread
from multiprocessing import Pool, Manager, current_process
from typing import Iterator, Optional, Union

from .otfFont import CFFFontData
from .ufoFont import UFOFontData
from .report import Report, GlyphReport
from .hinter import glyphHinter
from .logging import log_receiver
from .fdTools import FDDictManager

from . import (get_font_format, FontParseError)

log = logging.getLogger(__name__)


class ACOptions(object):
    def __init__(self):
        self.inputPaths = []
        self.outputPaths = []
        self.referenceFont = None
        self.referenceOutputPath = None
        self.glyphList = []
        # True when contents of glyphList were specified directly by the user
        self.explicitGlyphs = False
        self.nameAliases = {}
        self.excludeGlyphList = False
        self.overlapList = []
        self.overlapForcing = None
        self.hintAll = False
        self.readHints = True
        self.allowChanges = False
        self.noFlex = False
        self.noHintSub = False
        self.allowNoBlues = False
        self.fontinfoPath = None
        self.ignoreFontinfo = False
        self.logOnly = False
        self.removeConflicts = True
        # Copy of parse_args verbose for child processes
        self.verbose = 0
        self.printFDDictList = False
        self.printAllFDDict = False
        self.roundCoords = True
        self.writeToDefaultLayer = False
        # If this number of segments is exceeded in a dimension, don't hint
        # (Only applies when explicitGlyphs is False)
        self.maxSegments = 100
        self.font_format = None
        self.report_zones = False
        self.report_stems = False
        self.report_all_stems = False
        self.process_count = None
        # False in these dictionaries indicates that there should be no
        # warning if the glyph is missing
        self.hCounterGlyphs = {'element': False, 'equivalence': False,
                               'notelement': False, 'divide': False}
        self.vCounterGlyphs = {'m': False, 'M': False, 'T': False,
                               'ellipsis': False}
        self.upperSpecials = {'questiondown': False, 'exclamdown': False,
                              'semicolon': False}
        self.lowerSpecials = {'question': False, 'exclam': False,
                              'colon': False}
        self.noBlues = {'at': False, 'bullet': False, 'copyright': False,
                        'currency': False, 'registered': False}
#        self.newHintsOnMoveto = {'percent': False, 'perthousand': False}

    def __str__(self):
        # used only when debugging.
        import inspect
        data = []
        methodList = inspect.getmembers(self)
        for fname, fvalue in methodList:
            if fname[0] == "_":
                continue
            data.append(str((fname, fvalue)))
        data.append("")
        return os.linesep.join(data)

    def justReporting(self):
        return self.report_zones or self.report_stems


def getGlyphNames(glyphSpec, fontGlyphList, fDesc):
    # If the "range" is actually in the font, just ignore its apparent
    # range-ness
    if glyphSpec.isnumeric():
        GID = int(glyphSpec)
        if GID >= len(fontGlyphList):
            log.warning("glyph ID <%s> from glyph selection list option "
                        "exceeds number of glyphs in font <%s>.",
                        glyphSpec, fDesc)
            return None
        return [fontGlyphList[GID]]
    elif glyphSpec in fontGlyphList:
        return [glyphSpec]

    rangeList = glyphSpec.split("-")
    if len(rangeList) == 1:
        # Not numeric, not name, not range
        log.warning("glyph name <%s> from glyph selection list option is "
                    "not in font <%s>", glyphSpec, fDesc)
        return None
    elif len(rangeList) > 2:
        log.warning("Too many hyphens in glyph selection range <%s>",
                    glyphSpec)
        return None

    glyphName1, glyphName2 = rangeList
    gidList = []

    for r in rangeList:
        if r.isnumeric():
            GID = int(r)
            if GID >= len(fontGlyphList):
                log.warning("glyph name <%s> in range %s from glyph "
                            "selection list option is not in font <%s>.",
                            r, glyphSpec, fDesc)
                return None
        else:
            try:
                GID = fontGlyphList.index(r)
            except ValueError:
                log.warning("glyph name <%s> in range %s from glyph "
                            "selection list option is not in font <%s>.",
                            r, glyphSpec, fDesc)
                return None
        gidList.append(GID)

    glyphNameList = []
    for i in range(gidList[0], gidList[1] + 1):
        glyphNameList.append(fontGlyphList[i])

    return glyphNameList


def filterGlyphList(options, fontGlyphList, fDesc):
    """
    Returns the list of glyphs which are in the intersection of the argument
    list and the glyphs in the font.
    """
    if not options.glyphList:
        glyphList = fontGlyphList
    else:
        # expand ranges:
        glyphList = []
        for glyphSpec in options.glyphList:
            glyphNames = getGlyphNames(glyphSpec, fontGlyphList, fDesc)
            if glyphNames is not None:
                glyphList.extend(glyphNames)
        if options.excludeGlyphList:
            glyphList = [n for n in fontGlyphList if n not in glyphList]
    return glyphList


def get_glyph(options, font, name):
    try:
        gl = font.convertToGlyphData(name, options.readHints,  # stems
                                     options.readHints,  # flex
                                     options.roundCoords,
                                     options.hintAll)
        if gl is None or gl.isEmpty():
            log.info("Skipping glyph %s: no data from convertToGlyphData" %
                     name)
            return None
        return gl
    except KeyError:
        # Source fonts may be sparse, e.g. be a subset of the
        # reference font.
        return None


FontInstance = namedtuple("FontInstance", "font inpath outpath")


def setUniqueDescs(fontInstances):
    descs = [f.font.getPSName() for f in fontInstances]
    if None in descs or '' in descs or len(descs) != len(set(descs)):
        if len(fontInstances) == 1:
            # Don't add a substantive instance path when hinting one font
            fontInstances[0].font.desc = ''
            return
        descs = [f.inpath for f in fontInstances]
        try:
            prefix = os.path.commonpath(descs)
            descs = [os.path.relpath(p, prefix) for p in descs]
            while True:
                dirname = [os.path.dirname(p) for p in descs]
                if len(set(os.path.basename(p) for p in descs)) == 1:
                    descs = dirname
                else:
                    break
        except ValueError:
            pass
    for f, d in zip(fontInstances, descs):
        f.font.desc = d


class fontWrapper:
    """
    Stores references to one or more related instance font objects.
    Extracts glyphs from those objects by name, hints them, and
    stores the result back those objects. Optionally saves the
    modified glyphs in corresponding output font files.
    """
    def __init__(self, options, fil):
        self.options = options
        self.fontInstances = fil
        setUniqueDescs(fil)
        if len(fil) > 1:
            self.isVF = False
        else:
            self.isVF = (hasattr(fil[0].font, 'ttFont') and
                         'fvar' in fil[0].font.ttFont)
        self.reportOnly = options.justReporting()
        assert not self.reportOnly or (not self.isVF and len(fil) == 1)
        self.notFound = 0
        self.glyphNameList = filterGlyphList(options,
                                             fil[0].font.getGlyphList(),
                                             fil[0].font.desc)
        if not self.glyphNameList:
            raise FontParseError("Selected glyph list is empty for " +
                                 "font <%s>." % fil[0].font.desc)
        self.dictManager = FDDictManager(options, fil, self.glyphNameList,
                                         self.isVF)

    def numGlyphs(self):
        return len(self.glyphNameList)

    def hintStatus(self, name, hintedGlyphTuple):
        an = self.options.nameAliases.get(name, name)
        if hintedGlyphTuple is None:
            log.warning("%s: Could not hint!", an)
            return False
        hs = [g.hasHints(both=True) for g in hintedGlyphTuple if g is not None]
        if False in hs:
            if len(hintedGlyphTuple) == 1:
                log.info("%s: No hints added!", an)
                return False
            elif True in hs:
                log.info("%s: Hints only added to some instances!", an)
                return True
            else:
                log.info("%s: No hints added to any instances!", an)
                return False
        return True

    class glyphiter:
        def __init__(self, parent):
            self.fw = parent
            self.gnIter = parent.glyphNameList.__iter__()
            self.notFound = 0

        def __next__(self):
            # gnIter's StopIteration exception stops this iteration
            vsindex = 0
            stillLooking = True
            while stillLooking:
                stillLooking = False
                name = self.gnIter.__next__()
                if self.fw.reportOnly and name == '.notdef':
                    stillLooking = True
                    continue
                if self.fw.isVF:
                    t = self.fw.fontInstances[0].font.get_vf_glyphs(name)
                    gt, vsindex = t

                else:
                    gt = tuple((get_glyph(self.fw.options, f.font, name)
                                for f in self.fw.fontInstances))
                if True not in (g is not None for g in gt):
                    self.notFound += 1
                    stillLooking = True
            self.fw.notFound = self.notFound
            return name, gt, self.fw.dictManager.getRecKey(name, vsindex)

    def __iter__(self):
        return self.glyphiter(self)

    def hint(self):
        hintedAny = False

        report = Report() if self.reportOnly else None

        pcount = self.options.process_count
        if pcount is None:
            pcount = os.cpu_count()
        assert pcount is not None
        if pcount < 0:
            pcount = os.cpu_count() - pcount
            if pcount < 0:
                pcount = 1
        if pcount > self.numGlyphs():
            pcount = self.numGlyphs()

        if pcount > 1 and current_process().daemon:
            pcount = 1
            if self.options.process_count is not None:
                log.warning("Program was already spawned by a different " +
                            "python process: running as single process")

        pool = None
        logThread = None
        gmap: Optional[Iterator] = None
        try:
            dictRecord = self.dictManager.getDictRecord()
            if pcount == 1:
                glyphHinter.initialize(self.options, dictRecord)
                gmap = map(glyphHinter.hint, self)
            else:
                # set_start_method('spawn')
                manager = Manager()
                logQueue = manager.Queue(-1)
                logThread = Thread(target=log_receiver, args=(logQueue,))
                logThread.start()
                pool = Pool(pcount, initializer=glyphHinter.initialize,
                            initargs=(self.options, dictRecord,
                                      logQueue))
                if report is not None:
                    # Retain glyph ordering for reporting purposes
                    gmap = pool.imap(glyphHinter.hint, self)
                else:
                    gmap = pool.imap_unordered(glyphHinter.hint, self)

            for name, r in gmap:
                if isinstance(r, GlyphReport):
                    if report is not None:
                        report.glyphs[name] = r
                else:
                    hasHints = self.hintStatus(name, r)
                    if r is None:
                        r = [None] * len(self.fontInstances)
                    if not self.options.logOnly:
                        if hasHints:
                            hintedAny = True
                        font = self.fontInstances[0].font
                        for i, new_glyph in enumerate(r):
                            if i > 0 and not self.isVF:
                                font = self.fontInstances[i].font
                            font.updateFromGlyph(new_glyph, name)
                        if self.isVF:
                            font.merge_hinted_glyphs(name)

            if self.notFound:
                log.info("Skipped %s of %s glyphs.", self.notFound,
                         self.numGlyphs())

            if report is not None:
                report.save(self.fontInstances[0].outpath, self.options)
            elif not hintedAny:
                log.info("No glyphs were hinted.")

            if pool is not None:
                pool.close()
                pool.join()
                logQueue.put(None)
                assert logThread is not None
                logThread.join()
        finally:
            if pool is not None:
                pool.terminate()
                pool.join()
            if logThread is not None:
                logQueue.put(None)
                logThread.join()

        return hintedAny

    def save(self):
        for f in self.fontInstances:
            log.info("Saving font file %s with new hints..." % f.outpath)
            f.font.save(f.outpath)

    def close(self):
        for f in self.fontInstances:
            log.info("Closing font file %s without saving." % f.outpath)
            f.font.close()


def openFont(path, options) -> Union[UFOFontData, CFFFontData]:
    font_format = get_font_format(path)
    if font_format is None:
        raise FontParseError(f"{path} is not a supported font format")

    if font_format == "UFO":
        return UFOFontData(path, options.logOnly, options.writeToDefaultLayer)
    else:
        return CFFFontData(path, font_format)


def get_outpath(options, font_path, i):
    if i == 'r':
        if options.referenceOutputPath is not None:
            outpath = options.referenceOutputPath
        else:
            outpath = font_path
    else:
        if options.outputPaths is not None and i < len(options.outputPaths):
            outpath = options.outputPaths[i]
        else:
            outpath = font_path
    return outpath


def hintFiles(options):
    fontInstances = []
    # If there is a reference font, prepend it to font paths.
    # It must be the first font in the list, code below assumes that.
    if options.referenceFont:
        font = openFont(options.referenceFont, options)
        if hasattr(font, 'ttFont'):
            assert 'fvar' not in font.ttFont, ("Can't use a CFF2 VF font as a "
                                               "default font in a set of MM "
                                               "fonts.")
        fontInstances.append(FontInstance(font,
                             os.path.abspath(options.referenceFont),
                             get_outpath(options, options.referenceFont, 'r')))

    # Open the rest of the fonts and handle output paths.
    for i, path in enumerate(options.inputPaths):
        fontInstances.append(FontInstance(openFont(path, options),
                                          os.path.abspath(path),
                                          get_outpath(options, path, i)))

    noFlex = options.noFlex
    if fontInstances and options.referenceFont:
        log.info("Hinting fonts with reference %s. Start time: %s.",
                 fontInstances[0].font.desc, time.asctime())
        if fontInstances[0].font.isCID():
            options.noFlex = True
        fw = fontWrapper(options, fontInstances)
        if fw.hint():
            fw.save()
        else:
            fw.close()
        log.info("Done hinting fonts with reference %s. End time: %s.",
                 fontInstances[0].font.desc, time.asctime())
    else:
        for fi in fontInstances:
            log.info("Hinting font %s. Start time: %s.", fi.font.desc,
                     time.asctime())
            if fi[0].isCID():
                options.noFlex = True  # Flex hinting in CJK fonts does
                # bad things. For CFF fonts, being a CID font is a good
                # indicator of being CJK.
            else:
                options.noFlex = noFlex
            fw = fontWrapper(options, [fi])
            if fw.hint():
                fw.save()
            else:
                fw.close()
            log.info("Done hinting font %s. End time: %s.", fi.font.desc,
                     time.asctime())
