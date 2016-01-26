__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__doc__ = """ This module provideds a set of functions for working with glyph name
dictionaries. In particular, to load the latest set of recommended Adobe Glyph
names and Unicode values, from one of the scripts in
FDK/Tools/SharedData/FDKScripts/, the set of commands is:
	import agd
	import os
	sharedData = os.path.dirname(__file__)
	sharedData = os.path.dirname(sharedData)
	kAGD_TXTPath = os.path.join(sharedData, "AGD.TXT")
	fp = open(kAGD_TXTPath, "rU")
	agdTextPath = fp.read()
	fp.close()
	gAGDDict = agd.dictionary(agdTextPath)
"""

from __future__ import print_function
import re, sys, os, time


#---------------------------------------
def agl():
	"Return a hash of AGL names to Unicode values."
	aglout = {}
	agldata = "A:0041 AE:00C6 AEacute:01FC Aacute:00C1 Abreve:0102 Acircumflex:00C2 Adieresis:00C4 Agrave:00C0 Alpha:0391 Alphatonos:0386 Amacron:0100 Aogonek:0104 Aring:00C5 Aringacute:01FA Atilde:00C3 B:0042 Beta:0392 C:0043 Cacute:0106 Ccaron:010C Ccedilla:00C7 Ccircumflex:0108 Cdotaccent:010A Chi:03A7 D:0044 Dcaron:010E Dcroat:0110 Delta:0394 E:0045 Eacute:00C9 Ebreve:0114 Ecaron:011A Ecircumflex:00CA Edieresis:00CB Edotaccent:0116 Egrave:00C8 Emacron:0112 Eng:014A Eogonek:0118 Epsilon:0395 Epsilontonos:0388 Eta:0397 Etatonos:0389 Eth:00D0 Euro:20AC F:0046 G:0047 Gamma:0393 Gbreve:011E Gcaron:01E6 Gcircumflex:011C Gcommaaccent:0122 Gdotaccent:0120 H:0048 H18533:25CF H18543:25AA H18551:25AB H22073:25A1 Hbar:0126 Hcircumflex:0124 I:0049 IJ:0132 Iacute:00CD Ibreve:012C Icircumflex:00CE Idieresis:00CF Idotaccent:0130 Ifraktur:2111 Igrave:00CC Imacron:012A Iogonek:012E Iota:0399 Iotadieresis:03AA Iotatonos:038A Itilde:0128 J:004A Jcircumflex:0134 K:004B Kappa:039A Kcommaaccent:0136 L:004C Lacute:0139 Lambda:039B Lcaron:013D Lcommaaccent:013B Ldot:013F Lslash:0141 M:004D Mu:039C N:004E Nacute:0143 Ncaron:0147 Ncommaaccent:0145 Ntilde:00D1 Nu:039D O:004F OE:0152 Oacute:00D3 Obreve:014E Ocircumflex:00D4 Odieresis:00D6 Ograve:00D2 Ohorn:01A0 Ohungarumlaut:0150 Omacron:014C Omega:03A9 Omegatonos:038F Omicron:039F Omicrontonos:038C Oslash:00D8 Oslashacute:01FE Otilde:00D5 P:0050 Phi:03A6 Pi:03A0 Psi:03A8 Q:0051 R:0052 Racute:0154 Rcaron:0158 Rcommaaccent:0156 Rfraktur:211C Rho:03A1 S:0053 SF010000:250C SF020000:2514 SF030000:2510 SF040000:2518 SF050000:253C SF060000:252C SF070000:2534 SF080000:251C SF090000:2524 SF100000:2500 SF110000:2502 SF190000:2561 SF200000:2562 SF210000:2556 SF220000:2555 SF230000:2563 SF240000:2551 SF250000:2557 SF260000:255D SF270000:255C SF280000:255B SF360000:255E SF370000:255F SF380000:255A SF390000:2554 SF400000:2569 SF410000:2566 SF420000:2560 SF430000:2550 SF440000:256C SF450000:2567 SF460000:2568 SF470000:2564 SF480000:2565 SF490000:2559 SF500000:2558 SF510000:2552 SF520000:2553 SF530000:256B SF540000:256A Sacute:015A Scaron:0160 Scedilla:015E Scircumflex:015C Scommaaccent:0218 Sigma:03A3 T:0054 Tau:03A4 Tbar:0166 Tcaron:0164 Tcommaaccent:0162 Theta:0398 Thorn:00DE U:0055 Uacute:00DA Ubreve:016C Ucircumflex:00DB Udieresis:00DC Ugrave:00D9 Uhorn:01AF Uhungarumlaut:0170 Umacron:016A Uogonek:0172 Upsilon:03A5 Upsilon1:03D2 Upsilondieresis:03AB Upsilontonos:038E Uring:016E Utilde:0168 V:0056 W:0057 Wacute:1E82 Wcircumflex:0174 Wdieresis:1E84 Wgrave:1E80 X:0058 Xi:039E Y:0059 Yacute:00DD Ycircumflex:0176 Ydieresis:0178 Ygrave:1EF2 Z:005A Zacute:0179 Zcaron:017D Zdotaccent:017B Zeta:0396 a:0061 aacute:00E1 abreve:0103 acircumflex:00E2 acute:00B4 acutecomb:0301 adieresis:00E4 ae:00E6 aeacute:01FD afii00208:2015 afii10017:0410 afii10018:0411 afii10019:0412 afii10020:0413 afii10021:0414 afii10022:0415 afii10023:0401 afii10024:0416 afii10025:0417 afii10026:0418 afii10027:0419 afii10028:041A afii10029:041B afii10030:041C afii10031:041D afii10032:041E afii10033:041F afii10034:0420 afii10035:0421 afii10036:0422 afii10037:0423 afii10038:0424 afii10039:0425 afii10040:0426 afii10041:0427 afii10042:0428 afii10043:0429 afii10044:042A afii10045:042B afii10046:042C afii10047:042D afii10048:042E afii10049:042F afii10050:0490 afii10051:0402 afii10052:0403 afii10053:0404 afii10054:0405 afii10055:0406 afii10056:0407 afii10057:0408 afii10058:0409 afii10059:040A afii10060:040B afii10061:040C afii10062:040E afii10065:0430 afii10066:0431 afii10067:0432 afii10068:0433 afii10069:0434 afii10070:0435 afii10071:0451 afii10072:0436 afii10073:0437 afii10074:0438 afii10075:0439 afii10076:043A afii10077:043B afii10078:043C afii10079:043D afii10080:043E afii10081:043F afii10082:0440 afii10083:0441 afii10084:0442 afii10085:0443 afii10086:0444 afii10087:0445 afii10088:0446 afii10089:0447 afii10090:0448 afii10091:0449 afii10092:044A afii10093:044B afii10094:044C afii10095:044D afii10096:044E afii10097:044F afii10098:0491 afii10099:0452 afii10100:0453 afii10101:0454 afii10102:0455 afii10103:0456 afii10104:0457 afii10105:0458 afii10106:0459 afii10107:045A afii10108:045B afii10109:045C afii10110:045E afii10145:040F afii10146:0462 afii10147:0472 afii10148:0474 afii10193:045F afii10194:0463 afii10195:0473 afii10196:0475 afii10846:04D9 afii299:200E afii300:200F afii301:200D afii57381:066A afii57388:060C afii57392:0660 afii57393:0661 afii57394:0662 afii57395:0663 afii57396:0664 afii57397:0665 afii57398:0666 afii57399:0667 afii57400:0668 afii57401:0669 afii57403:061B afii57407:061F afii57409:0621 afii57410:0622 afii57411:0623 afii57412:0624 afii57413:0625 afii57414:0626 afii57415:0627 afii57416:0628 afii57417:0629 afii57418:062A afii57419:062B afii57420:062C afii57421:062D afii57422:062E afii57423:062F afii57424:0630 afii57425:0631 afii57426:0632 afii57427:0633 afii57428:0634 afii57429:0635 afii57430:0636 afii57431:0637 afii57432:0638 afii57433:0639 afii57434:063A afii57440:0640 afii57441:0641 afii57442:0642 afii57443:0643 afii57444:0644 afii57445:0645 afii57446:0646 afii57448:0648 afii57449:0649 afii57450:064A afii57451:064B afii57452:064C afii57453:064D afii57454:064E afii57455:064F afii57456:0650 afii57457:0651 afii57458:0652 afii57470:0647 afii57505:06A4 afii57506:067E afii57507:0686 afii57508:0698 afii57509:06AF afii57511:0679 afii57512:0688 afii57513:0691 afii57514:06BA afii57519:06D2 afii57534:06D5 afii57636:20AA afii57645:05BE afii57658:05C3 afii57664:05D0 afii57665:05D1 afii57666:05D2 afii57667:05D3 afii57668:05D4 afii57669:05D5 afii57670:05D6 afii57671:05D7 afii57672:05D8 afii57673:05D9 afii57674:05DA afii57675:05DB afii57676:05DC afii57677:05DD afii57678:05DE afii57679:05DF afii57680:05E0 afii57681:05E1 afii57682:05E2 afii57683:05E3 afii57684:05E4 afii57685:05E5 afii57686:05E6 afii57687:05E7 afii57688:05E8 afii57689:05E9 afii57690:05EA afii57716:05F0 afii57717:05F1 afii57718:05F2 afii57793:05B4 afii57794:05B5 afii57795:05B6 afii57796:05BB afii57797:05B8 afii57798:05B7 afii57799:05B0 afii57800:05B2 afii57801:05B1 afii57802:05B3 afii57803:05C2 afii57804:05C1 afii57806:05B9 afii57807:05BC afii57839:05BD afii57841:05BF afii57842:05C0 afii57929:02BC afii61248:2105 afii61289:2113 afii61352:2116 afii61573:202C afii61574:202D afii61575:202E afii61664:200C afii63167:066D afii64937:02BD agrave:00E0 aleph:2135 alpha:03B1 alphatonos:03AC amacron:0101 ampersand:0026 angle:2220 angleleft:2329 angleright:232A anoteleia:0387 aogonek:0105 approxequal:2248 aring:00E5 aringacute:01FB arrowboth:2194 arrowdblboth:21D4 arrowdbldown:21D3 arrowdblleft:21D0 arrowdblright:21D2 arrowdblup:21D1 arrowdown:2193 arrowleft:2190 arrowright:2192 arrowup:2191 arrowupdn:2195 arrowupdnbse:21A8 asciicircum:005E asciitilde:007E asterisk:002A asteriskmath:2217 at:0040 atilde:00E3 b:0062 backslash:005C bar:007C beta:03B2 block:2588 braceleft:007B braceright:007D bracketleft:005B bracketright:005D breve:02D8 brokenbar:00A6 bullet:2022 c:0063 cacute:0107 caron:02C7 carriagereturn:21B5 ccaron:010D ccedilla:00E7 ccircumflex:0109 cdotaccent:010B cedilla:00B8 cent:00A2 chi:03C7 circle:25CB circlemultiply:2297 circleplus:2295 circumflex:02C6 club:2663 colon:003A colonmonetary:20A1 comma:002C congruent:2245 copyright:00A9 currency:00A4 d:0064 dagger:2020 daggerdbl:2021 dcaron:010F dcroat:0111 degree:00B0 delta:03B4 diamond:2666 dieresis:00A8 dieresistonos:0385 divide:00F7 dkshade:2593 dnblock:2584 dollar:0024 dong:20AB dotaccent:02D9 dotbelowcomb:0323 dotlessi:0131 dotmath:22C5 e:0065 eacute:00E9 ebreve:0115 ecaron:011B ecircumflex:00EA edieresis:00EB edotaccent:0117 egrave:00E8 eight:0038 element:2208 ellipsis:2026 emacron:0113 emdash:2014 emptyset:2205 endash:2013 eng:014B eogonek:0119 epsilon:03B5 epsilontonos:03AD equal:003D equivalence:2261 estimated:212E eta:03B7 etatonos:03AE eth:00F0 exclam:0021 exclamdbl:203C exclamdown:00A1 existential:2203 f:0066 female:2640 figuredash:2012 filledbox:25A0 filledrect:25AC five:0035 fiveeighths:215D florin:0192 four:0034 fraction:2044 franc:20A3 g:0067 gamma:03B3 gbreve:011F gcaron:01E7 gcircumflex:011D gcommaaccent:0123 gdotaccent:0121 germandbls:00DF gradient:2207 grave:0060 gravecomb:0300 greater:003E greaterequal:2265 guillemotleft:00AB guillemotright:00BB guilsinglleft:2039 guilsinglright:203A h:0068 hbar:0127 hcircumflex:0125 heart:2665 hookabovecomb:0309 house:2302 hungarumlaut:02DD hyphen:002D i:0069 iacute:00ED ibreve:012D icircumflex:00EE idieresis:00EF igrave:00EC ij:0133 imacron:012B infinity:221E integral:222B integralbt:2321 integraltp:2320 intersection:2229 invbullet:25D8 invcircle:25D9 invsmileface:263B iogonek:012F iota:03B9 iotadieresis:03CA iotadieresistonos:0390 iotatonos:03AF itilde:0129 j:006A jcircumflex:0135 k:006B kappa:03BA kcommaaccent:0137 kgreenlandic:0138 l:006C lacute:013A lambda:03BB lcaron:013E lcommaaccent:013C ldot:0140 less:003C lessequal:2264 lfblock:258C lira:20A4 logicaland:2227 logicalnot:00AC logicalor:2228 longs:017F lozenge:25CA lslash:0142 ltshade:2591 m:006D macron:00AF male:2642 minus:2212 minute:2032 mu:03BC multiply:00D7 musicalnote:266A musicalnotedbl:266B n:006E nacute:0144 napostrophe:0149 ncaron:0148 ncommaaccent:0146 nine:0039 notelement:2209 notequal:2260 notsubset:2284 ntilde:00F1 nu:03BD numbersign:0023 o:006F oacute:00F3 obreve:014F ocircumflex:00F4 odieresis:00F6 oe:0153 ogonek:02DB ograve:00F2 ohorn:01A1 ohungarumlaut:0151 omacron:014D omega:03C9 omega1:03D6 omegatonos:03CE omicron:03BF omicrontonos:03CC one:0031 onedotenleader:2024 oneeighth:215B onehalf:00BD onequarter:00BC onethird:2153 openbullet:25E6 ordfeminine:00AA ordmasculine:00BA orthogonal:221F oslash:00F8 oslashacute:01FF otilde:00F5 p:0070 paragraph:00B6 parenleft:0028 parenright:0029 partialdiff:2202 percent:0025 period:002E periodcentered:00B7 perpendicular:22A5 perthousand:2030 peseta:20A7 phi:03C6 phi1:03D5 pi:03C0 plus:002B plusminus:00B1 prescription:211E product:220F propersubset:2282 propersuperset:2283 proportional:221D psi:03C8 q:0071 question:003F questiondown:00BF quotedbl:0022 quotedblbase:201E quotedblleft:201C quotedblright:201D quoteleft:2018 quotereversed:201B quoteright:2019 quotesinglbase:201A quotesingle:0027 r:0072 racute:0155 radical:221A rcaron:0159 rcommaaccent:0157 reflexsubset:2286 reflexsuperset:2287 registered:00AE revlogicalnot:2310 rho:03C1 ring:02DA rtblock:2590 s:0073 sacute:015B scaron:0161 scedilla:015F scircumflex:015D scommaaccent:0219 second:2033 section:00A7 semicolon:003B seven:0037 seveneighths:215E shade:2592 sigma:03C3 sigma1:03C2 similar:223C six:0036 slash:002F smileface:263A space:0020 spade:2660 sterling:00A3 suchthat:220B summation:2211 sun:263C t:0074 tau:03C4 tbar:0167 tcaron:0165 tcommaaccent:0163 therefore:2234 theta:03B8 theta1:03D1 thorn:00FE three:0033 threeeighths:215C threequarters:00BE tilde:02DC tildecomb:0303 tonos:0384 trademark:2122 triagdn:25BC triaglf:25C4 triagrt:25BA triagup:25B2 two:0032 twodotenleader:2025 twothirds:2154 u:0075 uacute:00FA ubreve:016D ucircumflex:00FB udieresis:00FC ugrave:00F9 uhorn:01B0 uhungarumlaut:0171 umacron:016B underscore:005F underscoredbl:2017 union:222A universal:2200 uogonek:0173 upblock:2580 upsilon:03C5 upsilondieresis:03CB upsilondieresistonos:03B0 upsilontonos:03CD uring:016F utilde:0169 v:0076 w:0077 wacute:1E83 wcircumflex:0175 wdieresis:1E85 weierstrass:2118 wgrave:1E81 x:0078 xi:03BE y:0079 yacute:00FD ycircumflex:0177 ydieresis:00FF yen:00A5 ygrave:1EF3 z:007A zacute:017A zcaron:017E zdotaccent:017C zero:0030 zeta:03B6 "
	for n in agldata.split():
		nn = n.split(':')
		if len(nn) == 2: aglout[nn[0]] = nn[1]
	return aglout

aglist = agl() # Because 'agl()' is slow, 'aglist' is defined here for use in this module



#===========================================================
# A glyph entry from a Glyph Dictionary

re_entry = re.compile(r'(?m)^[\t ]+(\S+?): +(\S.*?)$') # regexp: data entry line
re_name = re.compile(r'[A-Za-z0-9_\.]+') # regexp: glyph name
class glyph:
	def __init__(self, name):
		self.name = name
		self.uni = None # The Unicode
		self.fin = None # The final name
		self.ali = [] # list of name aliases
		self.sub = [] # list of substitutions
		self.set = [] # list of glyph set IDs
		self.min = None # minuscule
		self.maj = None # majuscule
		self.cmp = None # components used to create this composite glyph
		self.other = {} # Hash of any unknown tags

	# add entry data from input text:
	def parse(self, intext):
		ee = re_entry.findall(intext) # get all data entries
		if len(ee) < 1: return False # return if no data entries
		for e in ee:
			k, x = e # get key and its data
			if k == 'uni':
				self.uni = re.compile(r'[0-9A-F]{4}').search(x).group() # Unicode value
			elif k == 'fin': self.fin = re_name.search(x).group() # final name
			elif k == 'ali':
				for a in re_name.findall(x): self.ali.append(a) # name aliases
			elif k == 'sub': self.sub = x.split() # substitutions
			elif k == 'set': self.set.extend(x.split()) # glyph sets
			elif k == 'min': self.min = re_name.search(x).group() # minuscule
			elif k == 'maj': self.maj = re_name.search(x).group() # majuscule
			elif k == 'cmp': self.cmp = x # components
			else: self.other[k] = x # any unknown entry key
		self.check() # re-check glyph data
		return True

	# Check a glyph for internal conflicts, problems, etc.
	def check(self):
		m = [] # holds reports of what was fixed
		u = re.compile(r'uni([0-9A-F]{4})$') # Unicode value regexp
		n = {}
		# process aliases: remove duplicates, uninames, final names:
		for a in self.ali:
			# Remove alias if it matches the index name:
			if a == self.name:
				m.append("Removed alias '%s' from glyph '%s' (same as index name)." % (a, self.name))
				continue
			# Remove alias if it matches the final name:
			elif self.fin and a == self.fin:
				m.append("Removed alias '%s' from glyph '%s' (same as final name)." % (a, self.name))
				continue
			# Remove alias if it is a uni-name:
			elif u.match(a):
				m.append("Removed alias '%s' from glyph '%s' (uni-name for Unicode value)." % (a, self.name))
				continue
			else: n[a] = 1 # pass the alias
		self.ali = sorted(n.keys()) # sorted list of passed aliases
		# check final name:
		if self.fin and self.fin == self.name:
			m.append("Removed final name '%s' from glyph '%s' (same as index name)." % (self.fin, self.name))
			self.fin = None
		elif self.fin and self.uni and u.match(self.fin) and u.match(self.fin).group(1) != self.uni:
			m.append("Removed final name '%s' from glyph '%s'. Conflicts with Unicode %s." % (self.fin, self.name, self.uni))
			self.fin = None
		# Look for Unicode values in glyph names:
		if not self.uni:
			if self.fin and u.match(self.fin):
				m.append("Found Unicode value in final name '%s' of glyph '%s'." % (self.fin, self.name))
				self.uni = u.match(self.fin).group(1) # Get Unicode from final name
			else:
				for a in self.ali:
					if u.match(a):
						m.append("Found Unicode value in alias '%s' of glyph '%s'." % (a, self.name))
						self.uni = u.match(a).group(1)
						break
		return m

	# Return a list of characters mapped from a glyph name:
	def characters(self):
		c = self.name.split('.', 1)
		return c[0].split('_')

	# Return a list of all glyph names/aliases, including uni-names and final names
	def aliases(self, addglyphs=[]):
		self.check()
		n = {}
		n[self.name] = 1
		if self.fin: n[self.fin] = 1
		if self.uni: n[self.uniname()] = 1
		for i in self.ali: n[i] = 1 # add the glyph's own alias names
		for i in addglyphs: n[i] = 1 # add any additional alias names
		nn = sorted(n.keys())
		return nn

	# Return a text entry
	def entry(self):
		e = [] # Output lines
		e.append(self.name) # index name
		if self.uni: e.append("\tuni: %s" % self.uni) # Unicode value
		if self.fin: e.append("\tfin: %s" % self.fin) # final name
		if self.ali: e.append("\tali: %s" % ' '.join(self.ali)) # name aliases
		if self.sub: e.append("\tsub: %s" % ' '.join(self.sub)) # substitutions
		if self.set: e.append("\tset: %s" % ' '.join(self.set)) # glyph sets
		if self.min: e.append("\tmin: %s" % self.min) # minuscule
		if self.maj: e.append("\tmaj: %s" % self.maj) # majuscule
		if self.cmp: e.append("\tcmp: %s" % self.cmp) # components
		if self.other:
			otherkeys = sorted(self.other.keys())
			for k in otherkeys: e.append("\t%s: %s" % (k, self.other[k])) # other keys
		return '\n'.join(e) + '\n'

	# return a uni-name for the glyph's Unicode value:
	def uniname(self):
		if self.uni: return "uni%s"%self.uni
		else: return None

	# return the glyph's final name, which is a specified final name, or the work name:
	def finalname(self):
		return (self.fin or self.name or None)

	# return a glyph alias entry, with optionally-specified work name:
	def aliasline(self, workname=None):
		lineout = [] # output line
		lineout.append(self.fin or self.name)
		lineout.append(workname or self.name)
		if self.uni: lineout.append(self.uniname())
		return "\t".join(lineout)

	# report whether the glyph has a Unicode value in the PUA range (E000-F8FF):
	def haspua(self):
		if not self.uni: return False
		u = int(self.uni, 16)
		if 57343 < u < 63744: return True
		else: return False


#---------------------------------------
# A Glyph Dictionary

class dictionary:
	def __init__(self, intext=None):
		self.list = [] # Ordered list of glyph names
		self.glyphs = {} # Dictionary of glyph names and parents
		self.index = {} # Hash of all glyph/alias names
		self.unicode = {} # Hash of Unicode values
		self.messages = [] # A list of error/warning messages
		if intext: self.parse(intext)

	# Read text, convert to glyph entries, and add to dictionary:
	def parse(self, intext, priority=1):
		intext += '\n' # ensure the file ends with a newline for grep purposes
		re_entry = re.compile(r'(?m)^([A-Za-z0-9\._]+)\n((?:[\t ]+\S.*?\n)*)') # regexp for glyph entry text
		ee = re_entry.findall(intext) # find all glyph entries
		if len(ee) < 1: return False # return if no entries found
		for e in ee:
			gname, ginfo = e
			g = glyph(gname) # create new glyph object
			g.parse(ginfo)
			self.add(g, priority)
		return True

	# Find a glyph via index lookup, or by Unicode value:
	def glyph(self, n):
		if not n: return None
		elif n in self.index: return self.glyphs[self.index[n]] # lookup by glyph name
		elif n in self.unicode: return self.glyphs[self.unicode[n]] # lookup by Unicode value
		else: return None

	# Remove an existing glyph entry
	def remove(self, n):
		if n in self.glyphs:
			g = self.glyphs[n]
			for a in g.aliases():
				if a in self.index: del self.index[a] # remove all index entries
			if g.uni and g.uni in self.unicode: del self.unicode[g.uni] # remove unicode entry
			del self.glyphs[n] # delete the glyph entry
			i = self.list.index(n) # get order position of glyph
			del self.list[i] # remove glyph name from order list
			return i
		else: return None

	# Find and remove an alias:
	def removealias(self, n):
		if n in self.glyphs:
			self.messages.append("Cannot remove alias '%s'. (Name exists as index name.)" % n)
			return False
		elif n in self.index:
			g = self.glyphs[self.index[n]] # the glyph entry containing the alias
			if g.uni and g.uniname() == n:
				self.messages.append("Cannot remove alias '%s' from glyph '%s'. (Alias is derived from Unicode value.)" % (n, g.name))
				return False
			if g.fin and g.fin == n:
				self.messages.append("Removed final name '%s' from glyph '%s'." % (n, g.name))
				g.fin = None
			else:
				self.messages.append("Removed alias '%s' from glyph '%s'." % (n, g.name))
				g.ali.remove(n)
			del self.index[n]
			g.check()
			return True
		else:
			self.messages.append("Cannot remove alias '%s'. (Name not found.)" % n)
			return False

	# Find and remove a Unicode value:
	def removeunicode(self, u):
		if u in self.unicode:
			if self.unicode[u] in self.glyphs:
				g = self.glyphs[self.unicode[u]]
				ua = g.uniname() # uni-name for Unicode value
				if ua in self.index: del self.index[ua]
				if ua in g.ali: g.ali.remove(ua) # remove uni-name from aliases
				g.uni = None # remove Unicode value
			del self.unicode[u]
			return True
		return False

	# Add a glyph object to the dictionary and perform cross-checking;
	# merge incoming data according to specified priority:
	#  priority=3: New entries completely replace existing entries
	#  priority=2: New data merged, override existing data
	#  priority=1: New data merged, defer to existing data
	#  priority=0: New entries defer entirely to existing entries
	def add(self, g, priority=1):
		m = self.messages
		u = re.compile(r'uni([0-9A-F]{4})$') # Unicode value regexp
		addplace = None # position of added glyph in glyph order
		g.check() # check glyph before adding

		# (t) The existing glyph, if any
		t = self.glyph(g.name) or self.glyph(g.uni) # the target glyph which matches the incoming glyph (g) (or None)
		for a in g.aliases():
			if t: break
			t = self.glyph(a)

		# if a target glyph (t) is found:
		if t:
			# Stop and return false if Priority is 0:
			if priority == 0:
				m.append("Cannot add glyph '%s'. Conflicts with existing glyph '%s' (priority 0)." % (g.name, t.name))
				return False

			# If Priority is 3, do not merge any data from (t) into (g)
			elif priority == 3:
				m.append("Replaced existing entry '%s' with new entry '%s' (priority 3)." % (t.name, g.name))

			# If Priority is 1 or 2, merge data from (t) into (g):
			else:
				m.append("Merged new glyph '%s' with existing glyph '%s' (priority %s)." % (g.name, t.name, priority))
				g.ali = t.aliases(g.aliases()) # append new aliases
				for i in t.set:
					if not i in g.set: g.set.append(i) # append new glyph sets
				for i in t.sub:
					if not i in g.sub: g.sub.append(i) # append new features

				if priority == 2:
					if t.uni and not g.uni: g.uni = t.uni # Unicode value
					if t.fin and not g.fin: g.fin = t.fin # final name
					if t.min and not g.min: g.min = t.min # minuscule
					if t.maj and not g.maj: g.maj = t.maj # majuscule
					if t.cmp and not g.cmp: g.cmp = t.cmp # components
					for i in t.other:
						if not i in g.other: g.other[i] = t.other[i]

				else:
					g.name = t.name
					if t.uni: g.uni = t.uni # Unicode value
					if g.fin and not g.fin in g.ali: g.ali.append(g.fin) # copy final name to aliases
					if t.fin: g.fin = t.fin # final name
					if t.min: g.min = t.min # minuscule
					if t.maj: g.maj = t.maj # majuscule
					if t.cmp: g.cmp = t.cmp # majuscule
					for i in t.other: g.other[i] = t.other[i]
			addplace = self.remove(t.name) # remove the conflicting glyph

		else: m.append("Adding new glyph: %s" % g.name)

		# check glyph data against dictionary:
		for n in g.ali:
			# check for existing alias names:
			if n in self.index:
				if priority > 1: self.removealias(n) # remove the alias from the dictionary
				else: g.ali.remove(n) # remove the new alias
		if g.uni and g.uni in self.unicode:
			# check for existing Unicode value
			if priority > 1: self.removeunicode(g.uni)
			else:
				if g.fin and u.match(g.fin) and u.match(g.fin).group(1) == g.uni: g.fin = None # remove incoming final uni-name
				g.uni = None # remove Unicode value
		if g.fin and g.fin in self.index:
			if priority > 1: self.glyphs[self.index[g.fin]].fin = None
			else: g.fin = None
#		m.extend(g.check()) # re-check the new glyph

		# Add the glyph to the dictionary:
		if addplace is None: self.list.append(g.name)
		else: self.list.insert(addplace, g.name) # replace the old glyph with the new glyph in the order list
		self.glyphs[g.name] = g
		for a in g.aliases(): self.index[a] = g.name # add aliases to index (including self, uni-name and final name)
		if g.uni: self.unicode[g.uni] = g.name  # add the Unicode value to the Unicode index
#		m.append("Added glyph '%s'." % g.name)

		return True

	# Create AGD text output of all entries:
	def entries(self):
		ee = []
		for g in self.list: ee.append(self.glyphs[g].entry())
		return ''.join(ee)

	# Create a list of Unicodes in the dictionary, sorted by Unicode value:
	def unicodes(self):
		x = []
		uu = sorted(self.unicode.keys())
		print("Unicodes: %s" % len(uu))
		for u in uu: x.append("%s\t%s" % (self.glyphs[self.unicode[u]].uni, self.unicode[u]))
		return "\n".join(x)

	# Return all logged messages:
	def report(self):
		x = [] # collect output lines
		x.append("\n== Glyph Dictionary messages ===================\n") # output header
		if len(self.messages) < 1: x.append("[No messages]") # if no messages
		else:
			for i in self.messages: x.append(i)
		x.append("\n")
		return "\n".join(x)

	# Return a GlyphOrderAndAliasDB file:
	# Subset and sort output with glyphs list nn
	def aliasfile(self, nn=None):
		textout = []
		if nn:
			outlist = []
			for n in nn:
				if n in self.index: outlist.append(n)
		else: outlist = self.list
		for workname in outlist:
			g = self.glyph(workname)
			textout.append(g.aliasline(workname))
		return '\n'.join(textout)

	# Change the order of the glyphs in the dictionary
	# Sort to match a list of glyph names (s)
	# If no order list s submitted, order is alphabetical
	def sort(self, s):
		nmatch = {} # the number of matches to the order list
		neworder = [] # the new sort order
		for n in s:
			if n in self.glyphs:
				neworder.append(n)
				nmatch[n] = 1
		for n in self.list:
			if not n in nmatch: neworder.append(n)
		self.list = neworder # set the new glyph order
		return len(nmatch) # return the number of name matches in the sort list







#---------------------------------------
# Feature file functions:
#---------------------------------------
# An OpenType glyph substitution object (s)
# s.input: a list of input glyphs to the substitution
# s.output: a list of output glyphs to the substitution
# s.type: the substitution's lookupType:
#   Lookup type 1: single substitution
#   Lookup type 3: alternate substitution
#   Lookup type 4: ligature substitution
#   Lookup type 6: chaining contextual substitution
class substitution:
	def __init__(self, a=[], b=[], intype=0):
		if isinstance(a, str): self.input = [a] # if a is string, not list
		else: self.input = list(a) # List of input glyphs
		if isinstance(b, str): self.output = [b] # if b is string, not list
		else: self.output = list(b) # List of output glyphs

		if len(self.input) == 1 and len(self.output) == 1: self.type = 1
		elif len(self.input) > 1 and len(self.output) == 1: self.type = 4
		elif len(self.input) == 1 and len(self.output) > 1: self.type = 3
		else: self.type = 0 # The GSUB lookup type

		if intype: self.type = intype # Set input override type

	# return a single glyph substitution as a tuple if the input and
	# output both have one glyphs, otherwise return None:
	def pair(self):
		if len(self.input) == 1 and len(self.output) == 1:
			return (self.input[0], self.output[0])
		else: return None

	# return a feature file substitution statement:
	def feature(self):
		len_a = len(self.input)
		len_b = len(self.output)
		subword = 'by'
		if len_a > 0:
			a = ' '.join(self.input)
			if len_a > 1 and self.type != 4: a = '[%s]' % a
		else: a = ''

		if len_b > 1: b = '[%s]' % ' '.join(self.output)
		elif len_b == 1: b = self.output[0]
		else: b = ''

		if len_a == 1 and len_b > 1: subword = 'from'

		if a and b: return '\tsub %s %s %s;' % (a, subword, b)
		elif b and not a: return '\t# %s' % b
		else: return ''

#-----------------------------------------------------------
# Looks through a list of substitution instances (ss) and
# consolidates one-for-one entries into many-for-one or one-from-many
def consolidate(ss):
	glyphs_a = {} # collects matching input glyphs
	glyphs_b = {} # collects matching output glyphs
	log_a = {} # used for logging input glyphs
	log_b = {} # used for logging output glyphs
	newlist = [] # holds consolidated substitution objects


	classidx = {} # dictionary of class glyphs and their class index
	classpairs = [] # holds substitution class pairs
	classpass = [] # holds substitutions passed by the class-checker
	glyphclasses = [
		"zero one two three four five six seven eight nine",
		"A B C D E F G H I J K L M N O P Q R S T U V W X Y Z",
		"a b c d e f g h i j k l m n o p q r s t u v w x y z"
	]

	# index all glyph names in glyph classes:
	for c in range(len(glyphclasses)):
		classpairs.append([])
		for n in glyphclasses[c].split(): classidx[n] = c

	# look through substitutions for glyph which match classes:
	for s in ss:
		if not s.pair():
			classpass.append(s) # pass subs which are not 1:1
			continue
		a, b = s.pair()
		if a in classidx:
			i = classidx[a] # the index of the matching class
			classpairs[i].append(s)
		elif b in classidx:
			i = classidx[b] # the index of the matching class
			classpairs[i].append(s)
		else: classpass.append(s) # pass subs which do not match

	# collate substitutions which match:
	for c in range(len(classpairs)):
		if not classpairs[c]: continue
		class_a = []
		class_b = []
		for s in classpairs[c]:
			a, b = s.pair()
			class_a.append(a)
			class_b.append(b)
		newlist.append(substitution(class_a, class_b, 1)) # add a new substitution object


	# consolidate matching input and output glyphs
	# only in one-to-one substitutions:
	for s in classpass:
		if not s.pair(): continue
		a, b = s.pair()
		if not a in glyphs_a: glyphs_a[a] = []
		glyphs_a[a].append(b)
		if not b in glyphs_b: glyphs_b[b] = []
		glyphs_b[b].append(a)

	# process consolidated glyphs, create new multi-glyph substitution objects:
	for s in classpass:
		if s.pair():
			a, b = s.pair()
			# if many input glyphs (many-to-one):
			if len(glyphs_b[b]) > 1:
				if not b in log_b:
					newlist.append(substitution(glyphs_b[b], b, 1)) # create a new substitution, lookup type 1
					log_b[b] = 1
			# if many output glyphs (one-from-many):
			elif len(glyphs_a[a]) > 1:
				if not a in log_a:
					newlist.append(substitution(a, glyphs_a[a], 3)) # create a new substitution, lookup type 3
					log_a[a] = 1
			else: newlist.append(s)
		else: newlist.append(s)
	return newlist

#---------------------------------------
# Create feature file code from a dictionary (d) and
# optional list of glyph names (nn)
def makefeatures(d, nn=None):
	outlist = [] # List to collect output lines
	features = {} # Hash of feature names
	glyphnames = {} # Hash to hold real names matched to list names
	glyphpass = []

	nn = nn or d.list # Get list of glyphs from dictionary if not submitted

	# check list of glyph names:
	for n in nn:
		g = d.glyph(n)
		if g:
			glyphnames[g.name] = n
			glyphpass.append(g)

	for g in glyphpass:
		subpass = []
		if not g.sub: continue
		# Parse substitution data and create substitution objects:
		for subst in g.sub:
			ss = subst.split('+')
			f = ss.pop() # get the feature tag

			# filter substitution input glyphs which do not exist:
			if ss:
				for s in ss:
					if s in glyphnames: subpass.append(glyphnames[s])
				if not subpass: continue # skip this substitution if all input glyphs were not found

			if not (f in features): features[f] = [] # create a feature tag entry
			features[f].append(substitution(subpass, glyphnames[g.name])) # Append a substitution instance to the feature's list

	flist = sorted(features.keys()) # all feature tags

	# Create feature text output:
	for f in flist:
		outlist.append('feature %s {' % f)
		cs = consolidate(features[f]) # consolidate the feature's substitutions
		for s in cs: outlist.append(s.feature())
		outlist.append('} %s;\n' % f)

	return '\n'.join(outlist)







#-----------------------------------------------------------
# Make a derivedchars text entry from a glyph
def makederived(g):
	dvout = [] # output lines
	if not g.cmp: return "" # return if the glyph has no component entry
	dvout.append("/%s DerivedName" % g.name)
	dvbase = False
	for ca in g.cmp.split():
		dvadd = [] # the composite's components
		for cb in ca.split('+'):
			if len(dvadd) > 0: dvop = "AddCentered"
			elif not dvbase:
				dvop = "Base"
				dvbase = True
			else: dvop = "AddAdjacent"
			dvadd.append("(%s) %s" % (cb, dvop))
		dvout.extend(dvadd)
	dvout.append("EndDerived")
	return "\n".join(dvout)


#---------------------------------------
# Create derivedchars entries from composite information in
# dictionary (d). Produce entries for (glyphlist) or all glyphs
# in (d) if (glyphlist) is None.
# A composite entry will be skipped if all its components
# are not in (glyphlist).
def derivedchars(d, glyphlist=None):
	cmpsplit = re.compile(r' |\+') # regexp for splitting 'cmp' text
	dvout = [] # derivedchars output
	glyphlist = glyphlist or d.list
	for n in glyphlist:
		g = d.glyph(n)
		if not g: continue
		cflag = False # flag to check for missing components
		if g.cmp:
			nn = cmpsplit.split(g.cmp)
			for n in nn:
				if not n in glyphlist: cflag = True
			if cflag: continue
			dvout.append(makederived(g))
	return "\n\n".join(dvout)





#-----------------------------------------------------------
# Parse a GlyphOrderAndAliasDB
# return a list of glyph objects parsed from GlyphOrderAndAliasDB file (goadb)
def parsealiasfile(goadb):
	gg = [] # list of output glyphs
	ga_entry = re.compile(r'(?m)^([A-Za-z0-9\._]+)\t([A-Za-z0-9\._]+)(?:\t(?:uni([0-9A-F]{4}))?)?$')
	ee = ga_entry.findall(goadb) # find all glyph alias entries
	if len(ee) < 1: return gg # return if no entries found
	for e in ee:
		g = glyph(e[1]) # create new glyph object
		if e[0] != e[1]: g.fin = e[0]
		if e[2]: g.uni = e[2]
		gg.append(g)
	return gg



#---------------------------------------
# CFF glyph order
# return a list of glyph names from the CFF specification
# with a submitted list of glyphs (nn), returns that list sorted, with extra glyphs at the end
def cfforder(nn=None):
	c = ".notdef space exclam quotedbl numbersign dollar percent ampersand quoteright parenleft parenright asterisk plus comma hyphen period slash zero one two three four five six seven eight nine colon semicolon less equal greater question at A B C D E F G H I J K L M N O P Q R S T U V W X Y Z bracketleft backslash bracketright asciicircum underscore quoteleft a b c d e f g h i j k l m n o p q r s t u v w x y z braceleft bar braceright asciitilde exclamdown cent sterling fraction yen florin section currency quotesingle quotedblleft guillemotleft guilsinglleft guilsinglright fi fl endash dagger daggerdbl periodcentered paragraph bullet quotesinglbase quotedblbase quotedblright guillemotright ellipsis perthousand questiondown grave acute circumflex tilde macron breve dotaccent dieresis ring cedilla hungarumlaut ogonek caron emdash AE ordfeminine Lslash Oslash OE ordmasculine ae dotlessi lslash oslash oe germandbls onesuperior logicalnot mu trademark Eth onehalf plusminus Thorn onequarter divide brokenbar degree thorn threequarters twosuperior registered minus eth multiply threesuperior copyright Aacute Acircumflex Adieresis Agrave Aring Atilde Ccedilla Eacute Ecircumflex Edieresis Egrave Iacute Icircumflex Idieresis Igrave Ntilde Oacute Ocircumflex Odieresis Ograve Otilde Scaron Uacute Ucircumflex Udieresis Ugrave Yacute Ydieresis Zcaron aacute acircumflex adieresis agrave aring atilde ccedilla eacute ecircumflex edieresis egrave iacute icircumflex idieresis igrave ntilde oacute ocircumflex odieresis ograve otilde scaron uacute ucircumflex udieresis ugrave yacute ydieresis zcaron exclamsmall Hungarumlautsmall dollaroldstyle dollarsuperior ampersandsmall Acutesmall parenleftsuperior parenrightsuperior twodotenleader onedotenleader zerooldstyle oneoldstyle twooldstyle threeoldstyle fouroldstyle fiveoldstyle sixoldstyle sevenoldstyle eightoldstyle nineoldstyle commasuperior threequartersemdash periodsuperior questionsmall asuperior bsuperior centsuperior dsuperior esuperior isuperior lsuperior msuperior nsuperior osuperior rsuperior ssuperior tsuperior ff ffi ffl parenleftinferior parenrightinferior Circumflexsmall hyphensuperior Gravesmall Asmall Bsmall Csmall Dsmall Esmall Fsmall Gsmall Hsmall Ismall Jsmall Ksmall Lsmall Msmall Nsmall Osmall Psmall Qsmall Rsmall Ssmall Tsmall Usmall Vsmall Wsmall Xsmall Ysmall Zsmall colonmonetary onefitted rupiah Tildesmall exclamdownsmall centoldstyle Lslashsmall Scaronsmall Zcaronsmall Dieresissmall Brevesmall Caronsmall Dotaccentsmall Macronsmall figuredash hypheninferior Ogoneksmall Ringsmall Cedillasmall questiondownsmall oneeighth threeeighths fiveeighths seveneighths onethird twothirds zerosuperior foursuperior fivesuperior sixsuperior sevensuperior eightsuperior ninesuperior zeroinferior oneinferior twoinferior threeinferior fourinferior fiveinferior sixinferior seveninferior eightinferior nineinferior centinferior dollarinferior periodinferior commainferior Agravesmall Aacutesmall Acircumflexsmall Atildesmall Adieresissmall Aringsmall AEsmall Ccedillasmall Egravesmall Eacutesmall Ecircumflexsmall Edieresissmall Igravesmall Iacutesmall Icircumflexsmall Idieresissmall Ethsmall Ntildesmall Ogravesmall Oacutesmall Ocircumflexsmall Otildesmall Odieresissmall OEsmall Oslashsmall Ugravesmall Uacutesmall Ucircumflexsmall Udieresissmall Yacutesmall Thornsmall Ydieresissmall"
	cc = c.split()
	if nn:
		ndict = {}
		nout = []
		nend = []
		for n in nn:
			if n in cc: ndict[n] = 1 # glyph names in CFF order
			else: nend.append(n) # glyph names not in CFF order
		for c in cc:
			if c in ndict: nout.append(c)
		nout.extend(nend)
		return nout
	else: return cc


#---------------------------------------
# Take a glyph list as text (textin),
# return a report of name changes, etc.,
# a feature file, and GOADB
# With namechange False, will retain submitted
# names instead of changing them to prescribed
# work names.
def looklist(textin, d, namechange=True):
	m = [] # output messages
	glyphlist = [] # list to store checked glyph names
	glyphnames = {}
	nn = textin.split()

	for n in nn:
		g = d.glyph(n)
		if g:
			glyphnames[g.name] = n
			# Look for names to change:
			if namechange is True and g.name != n and not g.name in nn:
				glyphlist.append(g.name)
				m.append("Glyph name '%s' changed to '%s'." % (n, g.name))
			else: glyphlist.append(n)
		else: m.append("Glyph %s not found. Skipping." % n)
	glyphlist = cfforder(glyphlist) # sort glyph list by CFF

	# Print GOADB file:
	aliasfile = d.aliasfile(glyphlist)
	print(aliasfile + "\n\n")

	# Print feature file
	featurefile = makefeatures(d, glyphlist)
	print(featurefile + "\n\n")

	# Print derivedchars:
	print(derivedchars(d, glyphlist) + "\n\n")

	# Check for minuscule/majuscule matches:
	for n in glyphlist:
		g = d.glyph(n)
		if g.min and not g.min in glyphnames:
			m.append("Minuscule match %s for glyph %s is missing." % (g.min, n))
		if g.maj and not g.maj in glyphlist:
			m.append("Majuscule match %s for glyph %s is missing." % (g.maj, n))

	return "\n".join(m)


#---------------------------------------
# Confirm a list of glyph names (nn) conforming to Adobe glyph name conventions
# return a list of names which DO NOT conform, unless 'match' is set to True
re_uall = re.compile(r'u(ni)?[0-9A-F]{4}') # Uni-name search pattern
def namecheck(nn, match=False):
	a = agl() # get AGL information
	nout = [] # list of glyph names to return

	if isinstance(nn, str): nn = [nn] # convert a single string to a list
	for n in nn:
		nomatch = 0 # flag for nonconforming names
		nsplit = n.split('.', 1) # separate name at first period
		nparts = nsplit[0].split('_') # separate name root components
		for p in nparts:
			if not (p in a or re_uall.match(p)): nomatch = 1
		if match is False and nomatch: nout.append(n)
		elif match is True and not nomatch: nout.append(n)

	return nout


#-----------------------------------------------------------
# Deconstruct the character mapping of a glyph name (n), and return
# (as a tuple) a list of Unicode values, and a suffix string
re_uni = re.compile(r'^uni((?:[0-9A-F]{4})+)$') # match uni-name
re_u = re.compile(r'^u([0-9A-F]{4}(?:[0-9A-F]{2})?)$') # match u-name
re_code = re.compile(r'[0-9A-F]{4}') # match Unicode value

def namemap(n):
	if not re.compile(r'^[A-Za-z0-9\._]+$').match(n): return None # If the glyph name does not match accepted characters
	uu = [] # collects Unicode values
	namesuffix = None # glyph name suffix

	nameparts = n.split(".", 1)
	if len(nameparts) > 1: namesuffix = nameparts[1] # set name suffix, if any
	nameparts = nameparts[0].split("_")
	for p in nameparts:
		#--- skipped first step to match Zapf Dingbats font names
		if p in aglist: uu.append(aglist[p]) # if name is in the Adobe Glyph List
		elif re_uni.match(p):
			m = re_uni.match(p).group(1)
			uu.extend(re_code.findall(m))
		elif re_u.match(p):
			uu.append(re_u.match(p).group(1))
		else:
			uu.append(None)
	return (uu, namesuffix)

kPunctuation = {
					0x0021 : "exclam",
					0x0022 : "quotation",
					0x0023 : "numbersign",
					0x0026 : "ampersand",
					0x0027 : "apostrophe",
					0x002C : "comma",
					0x002E : "period",
					0x003A : "colon",
					0x003B : "semicolon",
					0x003F : "questionmark",
					0x0040 : "at",
					0x0021 : "questiondown",
					"exclam" : 0x0021,
					"quotation" : 0x0022,
					"numbersign" : 0x0023,
					"ampersand" : 0x0026,
					"apostrophe" : 0x0027,
					"comma" : 0x002C,
					"period" : 0x002E,
					"colon" : 0x003A,
					"semicolon" : 0x003B,
					"questionmark" : 0x003F,
					"at" : 0x0040,
					"questiondown" : 0x0021,
					}

kUnicodeScriptRanges = [
			# Note: CJK ranges omitted. Don;t expect to kern these!
			(0x0000, 0x007F, "Latin", "C0 Controls and Basic Latin (Basic Latin)"),
			(0x0080, 0x00FF, "Latin", "C1 Controls and Latin-1 Supplement (Latin-1 Supplement)"),
			(0x0100, 0x017F, "Latin", "Latin Extended-A"),
			(0x0180, 0x024F, "Latin", "Latin Extended-B"),
			(0x0250, 0x02AF, "IPA", "IPA Extensions"),
			(0x02B0, 0x02FF, "SML", "Spacing Modifier Letters"),
			(0x0300, 0x036F, "CDM", "Combining Diacritical Marks"),
			(0x0370, 0x03FF, "Greek", "Greek and Coptic"),
			(0x0400, 0x04FF, "Cyrillic", "Cyrillic"),
			(0x0500, 0x052F, "Cyrillic", "Cyrillic Supplement"),
			(0x0530, 0x058F, "Armenian", "Armenian"),
			(0x0590, 0x05FF, "Hebrew", "Hebrew"),
			(0x0600, 0x06FF, "Arabic", "Arabic"),
			(0x0700, 0x074F, "Syriac", "Syriac"),
			(0x0750, 0x077F, "Arabic", "Arabic Supplement"),
			(0x0780, 0x07BF, "Thaana", "Thaana"),
			(0x07C0, 0x07FF, "NKo", "NKo"),
			(0x0900, 0x097F, "Devanagari", "Devanagari"),
			(0x0980, 0x09FF, "Bengali", "Bengali"),
			(0x0A00, 0x0A7F, "Gurmukhi", "Gurmukhi"),
			(0x0A80, 0x0AFF, "Gujarati", "Gujarati"),
			(0x0B00, 0x0B7F, "Oriya", "Oriya"),
			(0x0B80, 0x0BFF, "Tamil", "Tamil"),
			(0x0C00, 0x0C7F, "Telugu", "Telugu"),
			(0x0C80, 0x0CFF, "Kannada", "Kannada"),
			(0x0D00, 0x0D7F, "Malayalam", "Malayalam"),
			(0x0D80, 0x0DFF, "Sinhala", "Sinhala"),
			(0x0E00, 0x0E7F, "Thai", "Thai"),
			(0x0E80, 0x0EFF, "Lao", "Lao"),
			(0x0F00, 0x0FFF, "Tibetan", "Tibetan"),
			(0x1000, 0x109F, "Myanmar", "Myanmar"),
			(0x10A0, 0x10FF, "Georgian", "Georgian"),
			(0x1100, 0x11FF, "CJK", "Hangul Jamo"),
			(0x1200, 0x137F, "Ethiopic", "Ethiopic"),
			(0x1380, 0x139F, "Ethiopic", "Ethiopic Supplement"),
			(0x13A0, 0x13FF, "Cherokee", "Cherokee"),
			(0x1400, 0x167F, "UCAS", "Unified Canadian Aboriginal Syllabics"),
			(0x1680, 0x169F, "Ogham", "Ogham"),
			(0x16A0, 0x16FF, "Runic", "Runic"),
			(0x1700, 0x171F, "Tagalog", "Tagalog"),
			(0x1720, 0x173F, "Hanunoo", "Hanunoo"),
			(0x1740, 0x175F, "Buhid", "Buhid"),
			(0x1760, 0x177F, "Tagbanwa", "Tagbanwa"),
			(0x1780, 0x17FF, "Khmer", "Khmer"),
			(0x1800, 0x18AF, "Mongolian", "Mongolian"),
			(0x1900, 0x194F, "Limbu", "Limbu"),
			(0x1950, 0x197F, "TaiLe", "Tai Le"),
			(0x1980, 0x19DF, "TaiLue", "New Tai Lue"),
			(0x19E0, 0x19FF, "Khmer", "Khmer Symbols"),
			(0x1A00, 0x1A1F, "Buginese", "Buginese"),
			(0x1B00, 0x1B7F, "Balinese", "Balinese"),
			(0x1D00, 0x1D7F, "PhoneticExtensions", "Phonetic Extensions"),
			(0x1D80, 0x1DBF, "PhoneticExtensions Supplement", "Phonetic Extensions Supplement"),
			(0x1DC0, 0x1DFF, "CDM", "Combining Diacritical Marks Supplement"),
			(0x1E00, 0x1EFF, "Latin", "Latin Extended Additional"),
			(0x1F00, 0x1FFF, "Greek", "Greek Extended"),
			(0x2000, 0x206F, "Punctuation", "General Punctuation"),
			(0x2070, 0x209F, "SuperscriptsSubscripts", "Superscripts and Subscripts"),
			(0x20A0, 0x20CF, "Currency", "Currency Symbols"),
			(0x20D0, 0x20FF, "CDMS", "Combining Diacritical Marks for Symbols"),
			(0x2100, 0x214F, "LetterlikeSymbols", "Letterlike Symbols"),
			(0x2150, 0x218F, "NumberForms", "Number Forms"),
			(0x2190, 0x21FF, "Arrows", "Arrows"),
			(0x2200, 0x22FF, "Mathematical", "Mathematical Operators"),
			(0x2300, 0x23FF, "MTechnical", "Miscellaneous Technical"),
			(0x2400, 0x243F, "ControlPictures", "Control Pictures"),
			(0x2440, 0x245F, "OCR", "Optical Character Recognition"),
			(0x2460, 0x24FF, "EnclosedAlphanumerics", "Enclosed Alphanumerics"),
			(0x2500, 0x257F, "BoxDrawing", "Box Drawing"),
			(0x2580, 0x259F, "BlockElements", "Block Elements"),
			(0x25A0, 0x25FF, "GeometricShapes", "Geometric Shapes"),
			(0x2600, 0x26FF, "Symbols", "Miscellaneous Symbols"),
			(0x2700, 0x27BF, "Dingbats", "Dingbats"),
			(0x27C0, 0x27EF, "MMathematical", "Miscellaneous Mathematical Symbols-A"),
			(0x27F0, 0x27FF, "Arrows-A", "Supplemental Arrows-A"),
			(0x2800, 0x28FF, "Braille", "Braille Patterns"),
			(0x2900, 0x297F, "Arrows-B", "Supplemental Arrows-B"),
			(0x2980, 0x29FF, "Mathematical", "Miscellaneous Mathematical Symbols-B"),
			(0x2A00, 0x2AFF, "Mathematical", "Supplemental Mathematical Operators"),
			(0x2B00, 0x2BFF, "Symbols", "Miscellaneous Symbols and Arrows"),
			(0x2C00, 0x2C5F, "Glagolitic", "Glagolitic"),
			(0x2C60, 0x2C7F, "Latin", "Latin Extended-C"),
			(0x2C80, 0x2CFF, "Coptic", "Coptic"),
			(0x2D00, 0x2D2F, "Georgian", "Georgian Supplement"),
			(0x2D30, 0x2D7F, "Tifinagh", "Tifinagh"),
			(0x2D80, 0x2DDF, "Ethiopic", "Ethiopic Extended"),
			(0x2E00, 0x2E7F, "Punctuation", "Supplemental Punctuation"),
			(0xA720, 0xA7FF, "Latin", "Latin Extended-D"),
			(0xA800, 0xA82F, "Sylot Nagri", "Syloti Nagri"),
			(0xA840, 0xA87F, "Phags-pa", "Phags-pa"),
			(0xFB00, 0xFB4F, "Alphabetic", "Alphabetic Presentation Forms"),
			(0xFB50, 0xFDFF, "Arabic", "Arabic Presentation Forms-A"),
			(0xFE70, 0xFEFF, "Arabic", "Arabic Presentation Forms-B"),
			(0xFF00, 0xFFEF, "Halfwidth and Fullwidth Forms", "Halfwidth and Fullwidth Forms"),
			(0xFFF0, 0xFFFF, "Specials", "Specials"),
			(0x10000, 0x1007F, "LinearB", "Linear B Syllabary"),
			(0x10080, 0x100FF, "LinearB", "Linear B Ideograms"),
			(0x10100, 0x1013F, "Aegean Numbers", "Aegean Numbers"),
			(0x10140, 0x1018F, "Ancient Greek Numbers", "Ancient Greek Numbers"),
			(0x10300, 0x1032F, "Old Italic", "Old Italic"),
			(0x10330, 0x1034F, "Gothic", "Gothic"),
			(0x10380, 0x1039F, "Ugaritic", "Ugaritic"),
			(0x103A0, 0x103DF, "Old Persian", "Old Persian"),
			(0x10400, 0x1044F, "Deseret", "Deseret"),
			(0x10450, 0x1047F, "Shavian", "Shavian"),
			(0x10480, 0x104AF, "Osmanya", "Osmanya"),
			(0x10800, 0x1083F, "Cypriot", "Cypriot Syllabary"),
			(0x10900, 0x1091F, "Phoenician", "Phoenician"),
			(0x10A00, 0x10A5F, "Kharoshthi", "Kharoshthi"),
			(0x12000, 0x123FF, "Cuneiform", "Cuneiform"),
			(0x12400, 0x1247F, "Cuneiform", "Cuneiform Numbers and Punctuation"),
			(0x1D000, 0x1D0FF, "ByzantineMusicalSymbols", "Byzantine Musical Symbols"),
			(0x1D100, 0x1D1FF, "Musical", "Musical Symbols"),
			(0x1D200, 0x1D24F, "Ancient Greek Musical Notation", "Ancient Greek Musical Notation"),
			(0x1D300, 0x1D35F, "TaiXuanJingSymbols", "Tai Xuan Jing Symbols"),
			(0x1D360, 0x1D37F, "CountingRodNumerals", "Counting Rod Numerals"),
			(0x1D400, 0x1D7FF, "Mathematical", "Mathematical Alphanumeric Symbols"),
						]
#-----------------------------------------------------------
# Get the script for a glyph based on Unicode blocks.
kUnknownScript = "Unknown"
def getscript(UV):
	for entry in kUnicodeScriptRanges:
		if (UV >=entry[0]) and (UV <= entry[1]):
			return entry[2]
	return kUnknownScript
