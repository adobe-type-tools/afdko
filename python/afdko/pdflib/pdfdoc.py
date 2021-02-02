# pdfdoc.py
"""
PDFgen is a library to generate PDF files containing text and graphics.  It is the
foundation for a complete reporting solution in Python.

The module pdfdoc.py handles the 'outer structure' of PDF documents, ensuring that
all objects are properly cross-referenced and indexed to the nearest byte.  The
'inner structure' - the page descriptions - are presumed to be generated before
each page is saved.
pdfgen.py calls this and provides a 'canvas' object to handle page marking operators.
piddlePDF calls pdfgen and offers a high-level interface.

(C) Copyright Andy Robinson 1998-1999

Modified 7/25/2006 read rooberts. Added supported for embedding fonts.
"""

import sys
import time
import zlib

from afdko.pdflib import pdfutils
from afdko.pdflib.pdfutils import LINEEND   # this constant needed in both

Log = sys.stderr  # reassign this if you don't want err output to console

##############################################################
#
#            Constants and declarations
#
##############################################################


StandardEnglishFonts = [
    'Courier', 'Courier-Bold', 'Courier-Oblique', 'Courier-BoldOblique',
    'Helvetica', 'Helvetica-Bold', 'Helvetica-Oblique',
    'Helvetica-BoldOblique',
    'Times-Roman', 'Times-Bold', 'Times-Italic', 'Times-BoldItalic',
    'Symbol','ZapfDingbats']

kDefaultEncoding = '/MacRoman'
kDefaultFontType = "/Type1"
AFMDIR = '.'

A4 = (595.27,841.89)   #default page size


class PDFError(KeyError):
    pass


class PDFDocument:
    """Responsible for linking and writing out the whole document.
    Builds up a list of objects using add(key, object).  Each of these
    must inherit from PDFObject and be able to write itself into the file.
    For cross-linking, it provides getPosition(key) which tells you where
    another object is, or raises a KeyError if not found.  The rule is that
    objects should only refer ones previously written to file.
    """
    def __init__(self):
        self.objects = []
        self.objectPositions = {}

        self.pages = []
        self.pagepositions = []

        # position 1
        cat = PDFCatalog()
        cat.RefPages = 3
        cat.RefOutlines = 2
        self.add('Catalog', cat)

        # position 2 - outlines
        outl = PDFOutline()
        self.add('Outline', outl)

        # position 3 - pages collection
        self.PageCol = PDFPageCollection()
        self.add('PagesTreeRoot',self.PageCol)

        # mapping of Postscript font names to internal ones;
        # add all the standard built-in fonts.
        self.fonts = StandardEnglishFonts # list of PS names. MakeType1Fonts()
        self.fontDescriptors = {} # Initally, we have no font descriptors, as these are required only
                                  # for fonts other than the built-in fonts.
        self.fontMapping = {}
        fontIndex = 0
        for i in range(len(self.fonts)):
            psName = self.fonts[i]
            fontIndex = i+1
            encoding = kDefaultEncoding
            clientCtx = None
            getFontDescriptor = None
            pdfFont = self.MakeType1Font(fontIndex, psName, encoding, clientCtx, getFontDescriptor)
            self.add('Font.'+pdfFont.keyname, pdfFont)
            objectNumber = len(self.objects)
            self.fontMapping[psName+repr(encoding)] = [fontIndex, pdfFont, objectNumber, psName, encoding]
        self.fontdict = MakeFontDictionary(self.fontMapping) # This needs to be called again whenever a font is added.


        # position 17 - Info
        self.info = PDFInfo()  #hang onto it!
        self.add('Info', self.info)
        self.infopos = len(self.objects)  #1-based, this gives its position


    def add(self, key, obj):
        self.objectPositions[key] = len(self.objects)  # its position
        self.objects.append(obj)
        obj.doc = self
        return len(self.objects) - 1  # give its position

    def getPosition(self, key):
        """Tell you where the given object is in the file - used for
        cross-linking; an object can call self.doc.getPosition("Page001")
        to find out where the object keyed under "Page001" is stored."""
        return self.objectPositions[key]

    def setTitle(self, title):
        "embeds in PDF file"
        self.info.title = title

    def setAuthor(self, author):
        "embedded in PDF file"
        self.info.author = author

    def setSubject(self, subject):
        "embeds in PDF file"
        self.info.subject = subject

    def printXref(self):
        self.startxref = sys.stdout.tell()
        Log.write('xref\n')
        Log.write("%s %s" % (0, len(self.objects) + 1))
        Log.write('0000000000 65535 f')
        for pos in self.xref:
            Log.write('%0.10d 00000 n\n' % pos)

    def writeXref(self, f):
        self.startxref = f.tell()
        f.write(('xref' + LINEEND).encode('utf-8'))
        f.write(('0 %d' % (len(self.objects) + 1) + LINEEND).encode('utf-8'))
        f.write(('0000000000 65535 f' + LINEEND).encode('utf-8'))
        for pos in self.xref:
            f.write(('%0.10d 00000 n' % pos + LINEEND).encode('utf-8'))

    def printTrailer(self):
        print('trailer')
        print('<< /Size %d /Root %d 0 R /Info %d 0 R>>' % (
            len(self.objects) + 1, 1, self.infopos))
        print('startxref')
        print(self.startxref)

    def writeTrailer(self, f):
        f.write(('trailer' + LINEEND).encode('utf-8'))
        f.write(('<< /Size %d /Root %d 0 R /Info %d 0 R>>' % (len(self.objects) + 1, 1, self.infopos)  + LINEEND).encode('utf-8'))
        f.write(('startxref' + LINEEND).encode('utf-8'))
        f.write((str(self.startxref)  + LINEEND).encode('utf-8'))

    def SaveToFile(self, filename):
        with open(filename, 'wb') as fileobj:
            self.SaveToFileObject(fileobj)

    def SaveToFileObject(self, fileobj):
        """Open a file, and ask each object in turn to write itself to
        the file.  Keep track of the file position at each point for
        use in the index at the end"""
        f = fileobj
        self.xref = []
        f.write(("%PDF-1.2" + LINEEND).encode('utf-8'))  # for CID support
        f.write(b"%\xed\xec\xb6\xbe\r\n")
        for i, obj in enumerate(self.objects, 1):
            pos = f.tell()
            self.xref.append(pos)
            f.write((str(i) + ' 0 obj' + LINEEND).encode('utf-8'))
            obj.save(f)
            f.write(('endobj' + LINEEND).encode('utf-8'))
        self.writeXref(f)
        self.writeTrailer(f)
        f.write(('%%EOF').encode('utf-8'))  # no lineend needed on this one!

    def printPDF(self):
        "prints it to standard output.  Logs positions for doing trailer"
        print("%PDF-1.0")
        print("%\xed\xec\xb6\xbe")
        i = 1
        self.xref = []
        for obj in self.objects:
            pos = sys.stdout.tell()
            self.xref.append(pos)
            print(i, '0 obj')
            obj.printPDF()
            print('endobj')
            i = i + 1
        self.printXref()
        self.printTrailer()
        print("%%EOF",)

    def addPage(self, page):
        """adds page and stream at end.  Maintains pages list"""
        #page.buildstream()
        pos = len(self.objects) # work out where added

        page.ParentPos = 3   #pages collection
        page.info = {
            'parentpos':3,
            'fontdict':self.fontdict,
            'contentspos':pos + 2,
            }

        self.PageCol.PageList.append(pos+1)
        self.add('Page%06d'% len(self.PageCol.PageList), page)
        #self.objects.append(page)
        self.add('PageStream%06d'% len(self.PageCol.PageList), page.stream)
        #self.objects.append(page.stream)

    def hasFont(self, psfontname, encoding=kDefaultEncoding):
        if encoding is None:
            encoding = kDefaultEncoding
        try:
           entry = self.fontMapping[psfontname+repr(encoding)]
           if entry[4] == encoding:
              return 1
           else:
              return 0
        except KeyError:
           return 0

    def addFont(self, psname, encoding=kDefaultEncoding, clientCtx=None, getFontDescriptorItems=None, getEncodingInfo=None):
        """ If you pass in just the psname, you can font dict that will work
        with only one of the built-in fonts. To emebed a font, you need to pass
        in all of the rest. "encoding" must be a string that is either one of
        the stanrd PDF encoding names, like '/MacRoman', or a string containing
        a well-formed PDF encoding dict.

        clientCtx is a client-provided Python object that cntains the data used
        be the client call back functions, getFontDescriptorItems,
        getEncodingInfo.

        getFontDescriptorItems is a client call back function that must return
        fdText, type, fontStream, fontStreamType

        fdText must be sttring containing a well-formed /FontDescriptor dict. In
        this string, the the reference to the embedded font stream must be as an
        indirect reference, with the object number being a string format , ie.
        /FontFile3 %s 0 R" This is is necessary ot allow the font stream object
        number ot be filled in later, by the line: fdText = fdText %
        objectNumber in MakeType1Font.

        type must tbe the PDF name for the font type, such as "/Type1"

        fontStream must be the actual font data to be embedded, not the entire
        PDF stream object. At the moment, it supports only CFF font data.

        fontStreamType is the value for the stream font SubType, e.g. /Type1C
        getEncodingInfo is a client call back function that must return
        firstChar, lastChar, widths

        firstChar is the index in the encoding array of the first glyph name
        which is not notdef lastChar is the index in the encoding array of the
        last glyph name whcih is not notdef widths is the width array of the
        encoded glyphs. It must contain a list of widths for the glyphs in the
        encoding array from firstChar to lastChar.

        The call to addFont will add the font descriptor object and the embedded
        stream object the first time it is called. If it is called again with
        the same PS name and encoding string, it will simply reference the font
        descriptor object and the embedded stream object that was already added,
        but will create a new font dict with a new widths and encoding dict.


        """
        if self.hasFont(psname, encoding):
           return
        numFonts = len(self.fontMapping)
        fontIndex = numFonts+1
        pdfFont = self.MakeType1Font(fontIndex, psname, encoding, clientCtx, getFontDescriptorItems, getEncodingInfo)
        self.add('Font.'+pdfFont.keyname, pdfFont)
        objIndex = len(self.objects)
        self.fontMapping[psname+repr(encoding)] = [fontIndex, pdfFont, objIndex, psname, encoding]
        self.fontdict = MakeFontDictionary(self.fontMapping)

    def getInternalFontName(self, psfontname, encoding=kDefaultEncoding):
        if encoding is None:
            encoding = kDefaultEncoding
        try:
            entry = self.fontMapping[psfontname+repr(encoding)]
            pdfFont = entry[1]
            return "/%s" % (pdfFont.keyname)
        except:
            raise PDFError("Font %s not available in document" % psfontname)

    def getAvailableFonts(self):
        # There may be more
        entries = self.fontMapping.values()
        fontEntries = map(lambda entry: [entry[3], entry[4]], entries) # [psName, encoding]
        fontEntries.sort()
        return fontEntries

    def MakeType1Font(self, fontIndex, psName, encoding=kDefaultEncoding, clientCtx=None, getFontDescriptorItems=None, getEncodingInfo=None):
        "returns a font object"
        objectNumber = None
        type = kDefaultFontType
        firstChar = lastChar = widths = None
         # If there is no getFontDescriptoItems function, we will assume that this font is a built-in font,
         # and hence does not need a font descriptor.
        if getFontDescriptorItems:
            if getEncodingInfo:
                firstChar, lastChar, widths = getEncodingInfo(clientCtx)
            try:
                # check if we have already seen this
                objectNumber,type = self.fontDescriptors[psName]
            except KeyError:
                # Need to create a font descriptor object.
                fdText, type, formatName, fontStream, fontStreamType = getFontDescriptorItems(clientCtx)
                fontStreamObj = PDFEmbddedFont(fontStreamType)
                fontStreamObj.setStream(fontStream)
                self.add(formatName, fontStreamObj)
                objectNumber = len(self.objects)
                fdText = fdText % objectNumber
                fontDescriptorObj = PDFontDescriptor(fdText)
                self.add('FontDescriptor', fontDescriptorObj)
                objectNumber = len(self.objects)
                self.fontDescriptors[psName] = (objectNumber, type)
        if type == "/Type0":
            font = PDFType0Font('F'+str(fontIndex), psName, encoding, objectNumber, type, firstChar, lastChar, widths)
        else:
            font = PDFType1Font('F'+str(fontIndex), psName, encoding, objectNumber, type, firstChar, lastChar, widths)
        return font


##############################################################
#
#            Utilities
#
##############################################################

class OutputGrabber:
    """At times we need to put something in the place of standard
    output.  This grabs stdout, keeps the data, and releases stdout
    when done.

    NOT working well enough!"""
    def __init__(self):
        self.oldoutput = sys.stdout
        sys.stdout = self
        self.closed = 0
        self.data = []

    def write(self, x):
        if not self.closed:
            self.data.append(x)

    def getData(self):
        return ''.join(self.data)

    def close(self):
        sys.stdout = self.oldoutput
        self.closed = 1

    def __del__(self):
        if not self.closed:
            self.close()


def testOutputGrabber():
    gr = OutputGrabber()
    for i in range(10):
        print('line', i)
    data = gr.getData()
    gr.close()
    print('Data...', data)


##############################################################
#
#            PDF Object Hierarchy
#
##############################################################



class PDFObject:
    """Base class for all PDF objects.  In PDF, precise measurement
    of file offsets is essential, so the usual trick of just printing
    and redirecting output has proved to give different behaviour on
    Mac and Windows.  While it might be soluble, I'm taking charge
    of line ends at the binary level and explicitly writing to a file.
    The LINEEND constant lets me try CR, LF and CRLF easily to help
    pin down the problem."""
    def save(self, file):
        "Save its content to an open file"
        file.write('% base PDF object' + LINEEND)
    def printPDF(self):
        self.save(sys.stdout)


class PDFLiteral(PDFObject):
    " a ready-made one you wish to quote"
    def __init__(self, text):
        self.text = text
    def save(self, file):
        file.write(self.text + LINEEND)



class PDFCatalog(PDFObject):
    "requires RefPages and RefOutlines set"
    def __init__(self):
        self.template = LINEEND.join([
                        '<<',
                        '/Type /Catalog',
                        '/Pages %d 0 R',
                        '/Outlines %d 0 R',
                        '>>'
                        ])
    def save(self, file):
        file.write((self.template % (self.RefPages, self.RefOutlines) + LINEEND).encode('utf-8'))


class PDFInfo(PDFObject):
    """PDF documents can have basic information embedded, viewable from
    File | Document Info in Acrobat Reader.  If this is wrong, you get
    Postscript errors while printing, even though it does not print."""
    def __init__(self):
        self.title = "untitled"
        self.author = "anonymous"
        self.subject = "unspecified"

        now = time.localtime(time.time())
        self.datestr = '%04d%02d%02d%02d%02d%02d' % tuple(now[0:6])

    def save(self, file):
        file.write((LINEEND.join([
                "<</Title (%s)",
                "/Author (%s)",
                "/CreationDate (D:%s)",
                "/Producer (PDFgen)",
                "/Subject (%s)",
                ">>"
                ]) % (
    pdfutils._escape(self.title),
    pdfutils._escape(self.author),
    self.datestr,
    pdfutils._escape(self.subject)
    ) + LINEEND).encode('utf-8'))


class PDFOutline(PDFObject):
    "null outline, does nothing yet"
    def __init__(self):
        self.template = LINEEND.join([
                '<<',
                '/Type /Outlines',
                '/Count 0',
                '>>'])
    def save(self, file):
        file.write((self.template + LINEEND).encode('utf-8'))


class PDFPageCollection(PDFObject):
    "presumes PageList attribute set (list of integers)"
    def __init__(self):
        self.PageList = []

    def save(self, file):
        lines = [ '<<',
                '/Type /Pages',
                '/Count %d' % len(self.PageList),
                '/Kids ['
                ]
        for page in self.PageList:
            lines.append(str(page) + ' 0 R ')
        lines.append(']')
        lines.append('>>')
        text = LINEEND.join(lines)
        file.write((text + LINEEND).encode('utf-8'))


class PDFPage(PDFObject):
    """The Bastard.  Needs list of Resources etc. Use a standard one for now.
    It manages a PDFStream object which must be added to the document's list
    of objects as well."""
    def __init__(self):
        self.drawables = []
        self.pagewidth = 595  #these are overridden by piddlePDF
        self.pageheight = 842
        self.stream = PDFStream()
        self.hasImages = 0
        self.pageTransitionString = ''  # presentation effects
        # editors on different systems may put different things in the line end
        # without me noticing.  No triple-quoted strings allowed!
        self.template = LINEEND.join([
                '<<',
                '/Type /Page',
                '/Parent %(parentpos)d 0 R',
                '/Resources',
                '   <<',
                '   /Font %(fontdict)s',
                '   /ProcSet %(procsettext)s',
                '   >>',
                '/MediaBox [0 0 %(pagewidth)d %(pageheight)d]',  #A4 by default
                '/Contents %(contentspos)d 0 R',
                '%(transitionString)s',
                '>>'])

    def setCompression(self, onoff=0):
        "Turns page compression on or off"
        assert onoff in [0, 1], "Page compression options are 1=on, 2=off"
        self.stream.compression = onoff

    def save(self, file):
        self.info['pagewidth'] = self.pagewidth
        self.info['pageheight'] = self.pageheight
        # check for image support
        if self.hasImages:
            self.info['procsettext'] = '[/PDF /Text /ImageC]'
        else:
            self.info['procsettext'] = '[/PDF /Text]'
        self.info['transitionString'] = self.pageTransitionString

        file.write((self.template % self.info + LINEEND).encode('utf-8'))

    def clear(self):
        self.drawables = []

    def setStream(self, data):
        if isinstance(data, list):
            data = LINEEND.join(data)
        self.stream.setStream(data)

TestStream = "BT /F6 24 Tf 80 672 Td 24 TL (   ) Tj T* ET"


class PDFStream(PDFObject):
    "Used for the contents of a page"
    def __init__(self):
        self.data = None
        self.compression = 0

    def setStream(self, data):
        self.data = data

    def save(self, file):
        #avoid crashes if they wrote nothing in the page
        if self.data is None:
             self.data = TestStream

        if self.compression == 1:
            comp = zlib.compress((self.data).encode('utf-8'))   #this bit is very fast...
            base85 = pdfutils._AsciiBase85Encode(comp) #...sadly this isn't
            wrapped = pdfutils._wrap(base85)
            data_to_write = wrapped
        else:
            data_to_write = self.data
        # the PDF length key should contain the length including
        # any extra LF pairs added by Print on DOS.

        #lines = len(string.split(self.data,'\n'))
        #length = len(self.data) + lines   # one extra LF each
        length = len(data_to_write) + len(LINEEND)    #AR 19980202
        if self.compression:
            file.write(('<< /Length %d /Filter [/ASCII85Decode /FlateDecode]>>' % length + LINEEND).encode('utf-8'))
        else:
            file.write(('<< /Length %d >>' % length + LINEEND).encode('utf-8'))
        file.write(('stream' + LINEEND).encode('utf-8'))
        file.write((data_to_write + LINEEND).encode('latin-1'))  # Really?
        file.write(('endstream' + LINEEND).encode('utf-8'))

class PDFImage(PDFObject):
    # sample one while developing.  Currently, images go in a literals
    def save(self, file):
        file.write(LINEEND.join([
                '<<',
                '/Type /XObject',
                '/Subtype /Image',
                '/Name /Im0',
                '/Width 24',
                '/Height 23',
                '/BitsPerComponent 1',
                '/ColorSpace /DeviceGray',
                '/Filter /ASCIIHexDecode',
                '/Length 174',
                '>>',
                'stream',
                '003B00 002700 002480 0E4940 114920 14B220 3CB650',
                '75FE88 17FF8C 175F14 1C07E2 3803C4 703182 F8EDFC',
                'B2BBC2 BB6F84 31BFC2 18EA3C 0E3E00 07FC00 03F800',
                '1E1800 1FF800>',
                'endstream',
                'endobj'
                ]) + LINEEND)

class PDFEmbddedFont(PDFStream):
    def __init__(self, fontStreamType):
        PDFStream.__init__(self)
        self.fontType = fontStreamType

    def save(self, file):
        #avoid crashes if they wrote nothing in the page
        if self.data is None:
             self.data = TestStream

        if self.compression == 1:
            comp = zlib.compress(self.data)   #this bit is very fast...
            base85 = pdfutils._AsciiBase85Encode(comp) #...sadly this isn't
            data_to_write = pdfutils._wrap(base85)
        else:
            data_to_write = self.data
        # the PDF length key should contain the length including
        # any extra LF pairs added by Print on DOS.

        #lines = len(string.split(self.data,'\n'))
        #length = len(self.data) + lines   # one extra LF each
        length = len(data_to_write) + len(LINEEND)    #AR 19980202
        if self.fontType is None:
            fontStreamEntry = ""
        else:
            fontStreamEntry = "/Subtype %s" % (self.fontType)

        if self.compression:
            file.write(('<<  %s /Length %d /Filter [/ASCII85Decode /FlateDecode] >>' % (fontStreamEntry, length) + LINEEND).encode('utf-8'))
        else:
            file.write(('<< /Length %d %s >>' % (length,  fontStreamEntry) + LINEEND).encode('utf-8'))
        file.write(('stream' + LINEEND).encode('utf-8'))
        file.write(data_to_write + b'\r\n')
        file.write(('endstream' + LINEEND).encode('utf-8'))

class PDFType1Font(PDFObject):
    def __init__(self, key, psName, encoding=kDefaultEncoding, fontDescriptObjectNumber = None, type = kDefaultFontType, firstChar = None, lastChar = None, widths = None):
        self.fontname = psName
        self.keyname = key
        if encoding is None:
           encoding = '/MacRomanEncoding'
        textList = [
                    '<<',
                    '/Type /Font',
                    '/Subtype %s' % type]
        if fontDescriptObjectNumber != None:
            textList.append("/FontDescriptor %s 0 R" % fontDescriptObjectNumber)
            textList.append("/FirstChar %s" % firstChar)
            textList.append("/LastChar %s" % lastChar)
            textList.append("/Widths %s" % widths)
        textList += ['/Name /%s', # filled in in 'save'
                    '/BaseFont /%s',# filled in in 'save'
                    "/Encoding %s" % (encoding),
                    '>>']
        self.template = LINEEND.join(textList)

    def save(self, file):
        file.write((self.template % (self.keyname, self.fontname) + LINEEND).encode('utf-8'))


class PDFType0Font(PDFType1Font):
    pass

class PDFontDescriptor(PDFObject):
    def __init__(self, text):
        self.text = text

    def save(self, file):
        file.write((self.text + LINEEND).encode('utf-8'))





##############################################################
#
#            some helpers
#
##############################################################


def MakeFontDictionary(fontMapping):
    # fontMapping[psName] = [fontIndex, pdfFont, objIndex]
    dict_ = "  <<" + LINEEND
    entries = sorted(fontMapping.values())
    for entry in entries:
        objectPos = entry[2]
        pdfFont = entry[1]
        dict_ = dict_ + '\t\t/%s %d 0 R ' % (pdfFont.keyname, objectPos) + LINEEND
    dict_ = dict_ + "\t\t>>" + LINEEND
    return dict_
