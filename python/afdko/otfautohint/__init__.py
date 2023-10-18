import os
from typing import Union


Number = Union[int, float]



class FontParseError(Exception):
    pass


def _font_is_ufo(path):
    from fontTools.ufoLib import UFOReader
    from fontTools.ufoLib.errors import UFOLibError
    try:
        UFOReader(path, validate=False)
        return True
    except (UFOLibError, KeyError, TypeError):
        return False


def get_font_format(font_file_path):
    if _font_is_ufo(font_file_path):
        return "UFO"
    elif os.path.isfile(font_file_path):
        with open(font_file_path, 'rb') as f:
            head = f.read(4)
            if head == b'OTTO':
                return 'OTF'
            elif head[0:2] == b'\x01\x00':
                return 'CFF'
            elif head[0:2] == b'\x80\x01':
                return 'PFB'
            elif head in (b'%!PS', b'%!Fo'):
                for fullhead in (b'%!PS-AdobeFont', b'%!FontType1',
                                 b'%!PS-Adobe-3.0 Resource-CIDFont'):
                    f.seek(0)
                    if f.read(len(fullhead)) == fullhead:
                        if b"CID" not in fullhead:
                            return 'PFA'
                        else:
                            return 'PFC'
        return None
    else:
        return None
