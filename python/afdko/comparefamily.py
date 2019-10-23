#	comparefamily.py
#	This is not code to copy . The original developer did not know Python well,
#	and had odd ideas about design. However, it works.

__copyright__ = """Copyright 2015 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__usage__ = """
comparefamily 2.1.5 Jul 15 2019

comparefamily [u] -h] [-d <directory path>] [-tolerance <n>] -rm] [-rn] [-rp] [-nohints] [-l] [-rf] [-st n1,..] [-ft n1,..]
where 'n1' stands for the number of a test, such as "-st 26" to run Single Test 26.

Program for checking many font development problems, both within a
single font and between members of a family.

Some of the tests are specific to Adobe Type Dept practices; if you don't like them,
feel free to edit or disable them in the source script file at:
{afdko Root Directory}/python/afdko/comparefamily.py

See 'comparefamily -h' for details.
"""

__version__ = "2.1.5"

__help__ = """

comparefamily will look in the specified directory and examine and
   report on all the OpenType fonts it finds in that directory.

   -h:  write this help text

   -u  write usage

   -d <path>: Look in the specified directory for fonts. Default
      is the current directory

   -toleranance <integer>:  Set the tolerance for metrics differences
        in design space units. Default is 0. Set to a larger value to see
        only big problems. Affects Single Test 11 BASE table metrics,
        Single Test 22 ligature widths, and Single Test 23 Accent Widths.

   -rm:  Write the font menu name report. This prints out a table of the
       font menu names, and enables the font menu name checks.

   -rn:  Write the font metrics name report. This prints out a table of
       the global font metrics, so you can see what differs between
       faces in each family, Always look at this. Can't be automated
       because there are cases where the values should differ.

   -rp:  Write the Panose number report. This prints out a table of the
       Panose numbers as descriptive tags, so you can see if they differ
       appropriately between faces in each family.

	-nohints:  Supresses checking stem hint widths and positions. If not
		specified, then comparefamily will check that the stem hints
		are not absurdly wide, and that the hints are within the
		FontBBox.

   -l:  <path to log file>:  write output to log file instead of the
       terminal window.

   -rf:  Write only the feature lookup vs languge-system report.

   -st n1,n2,n3..:  Execute only the listed single face tests. For
        example: '-st 3,12,34". The max single face test number is 34.

   -ft n1,n2,n3..:  Execute only the listed family tests. For
        example: '-ft 1,18". The max family test number is 21.



comparefamily now depends on some configurable data for a few tests.

Single Face Test 6 requires that you have set an environment variable
named 'CF_DEFAULT_URL' in order to check if this string is in the name
table of your fonts. If this is not defined, it will skip the test.

It also requires that you have set an environment variable named
'CF_DEFAULT_FOUNDRY_CODE' in order to check if this 4 character code is
in the OS/2 table of your fonts. If this is not defined, it will skip
the test.

"""



import sys
import os
import struct
import re
import copy
import math

from fontTools import ttLib

from afdko import fdkutils


gDesignSpaceTolerance = 0 # don't complain about metrics differences greater than this amount.

kFixedPitchExceptionList = [
							"OCRB",
							"CaravanLH-Three",
							"CaravanLH-One"
							]


kLigFeatureList = ["clig", "dlig", "hlig", "rlig", "liga"] # List of ligature features; used to  load a dictionary of ligatures and their components, for width checks.

kKnownBaseTags = ["hang", # The hanging baseline. This is the horizontal line from which syllables seem to hang in Tibetan script.
			"icfb", # ideographic character face bottom edge baseline. (See section Ideographic Character Face below for usage.)
			"icft", # Ideographic character face top edge baseline. (See section Ideographic Character Face below for usage.)
			"ideo", # Ideographic em-box bottom edge baseline. (See section Ideographic Em-Box below for usage.)
			"idtp", # Ideographic em-box top edge baseline. (See section Ideographic Em-Box below for usage.)
			"math", # The baseline about which mathematical characters are centered.
			"romn", # The baseline used by simple alphabetic scripts such as Latin, Cyrillic and Greek.
			]


# Updated to OpenType specification version 1.8.3
# https://docs.microsoft.com/en-us/typography/opentype/spec/scripttags
kKnownScriptTags = [
	"adlm", # Adlam
	"ahom", # Ahom
	"hluw", # Anatolian Hieroglyphs
	"arab", # Arabic
	"armn", # Armenian
	"avst", # Avestan
	"bali", # Balinese
	"bamu", # Bamum
	"bass", # Bassa Vah
	"batk", # Batak
	"beng", # Bengali
	"bng2", # Bengali v.2
	"bhks", # Bhaiksuki
	"bopo", # Bopomofo
	"brah", # Brahmi
	"brai", # Braille
	"bugi", # Buginese
	"buhd", # Buhid
	"byzm", # Byzantine Music
	"cans", # Canadian Syllabics
	"cari", # Carian
	"aghb", # Caucasian Albanian
	"cakm", # Chakma
	"cham", # Cham
	"cher", # Cherokee
	"hani", # CJK Ideographic
	"copt", # Coptic
	"cprt", # Cypriot Syllabary
	"cyrl", # Cyrillic
	"DFLT", # Default
	"dsrt", # Deseret
	"deva", # Devanagari
	"dev2", # Devanagari v.2
	"dogr", # Dogra
	"dupl", # Duployan
	"egyp", # Egyptian Hieroglyphs
	"elba", # Elbasan
	"ethi", # Ethiopic
	"geor", # Georgian
	"glag", # Glagolitic
	"goth", # Gothic
	"gran", # Grantha
	"grek", # Greek
	"gujr", # Gujarati
	"gjr2", # Gujarati v.2
	"gong", # Gunjala Gondi
	"guru", # Gurmukhi
	"gur2", # Gurmukhi v.2
	"hang", # Hangul
	"jamo", # Hangul Jamo
	"rohg", # Hanifi Rohingya
	"hano", # Hanunoo
	"hatr", # Hatran
	"hebr", # Hebrew
	"kana", # Hiragana
	"armi", # Imperial Aramaic
	"phli", # Inscriptional Pahlavi
	"prti", # Inscriptional Parthian
	"java", # Javanese
	"kthi", # Kaithi
	"knda", # Kannada
	"knd2", # Kannada v.2
	"kana", # Katakana
	"kali", # Kayah Li
	"khar", # Kharosthi
	"khmr", # Khmer
	"khoj", # Khojki
	"sind", # Khudawadi
	"lao ", # Lao
	"latn", # Latin
	"lepc", # Lepcha
	"limb", # Limbu
	"lina", # Linear A
	"linb", # Linear B
	"lisu", # Lisu (Fraser)
	"lyci", # Lycian
	"lydi", # Lydian
	"mahj", # Mahajani
	"maka", # Makasar
	"mlym", # Malayalam
	"mlm2", # Malayalam v.2
	"mand", # Mandaic, Mandaean
	"mani", # Manichaean
	"marc", # Marchen
	"gonm", # Masaram Gondi
	"math", # Mathematical Alphanumeric Symbols
	"medf", # Medefaidrin (Oberi Okaime, Oberi Ɔkaimɛ)
	"mtei", # Meitei Mayek (Meithei, Meetei)
	"mend", # Mende Kikakui
	"merc", # Meroitic Cursive
	"mero", # Meroitic Hieroglyphs
	"plrd", # Miao
	"modi", # Modi
	"mong", # Mongolian
	"mroo", # Mro
	"mult", # Multani
	"musc", # Musical Symbols
	"mymr", # Myanmar
	"mym2", # Myanmar v.2
	"nbat", # Nabataean
	"newa", # Newa
	"talu", # New Tai Lue
	"nko ", # N'Ko
	"nshu", # Nüshu
	"orya", # Odia (formerly Oriya)
	"ory2", # Odia v.2 (formerly Oriya v.2)
	"ogam", # Ogham
	"olck", # Ol Chiki
	"ital", # Old Italic
	"hung", # Old Hungarian
	"narb", # Old North Arabian
	"perm", # Old Permic
	"xpeo", # Old Persian Cuneiform
	"sogo", # Old Sogdian
	"sarb", # Old South Arabian
	"orkh", # Old Turkic, Orkhon Runic
	"osge", # Osage
	"osma", # Osmanya
	"hmng", # Pahawh Hmong
	"palm", # Palmyrene
	"pauc", # Pau Cin Hau
	"phag", # Phags-pa
	"phnx", # Phoenician
	"phlp", # Psalter Pahlavi
	"rjng", # Rejang
	"runr", # Runic
	"samr", # Samaritan
	"saur", # Saurashtra
	"shrd", # Sharada
	"shaw", # Shavian
	"sidd", # Siddham
	"sgnw", # Sign Writing
	"sinh", # Sinhala
	"sogd", # Sogdian
	"sora", # Sora Sompeng
	"soyo", # Soyombo
	"xsux", # Sumero-Akkadian Cuneiform
	"sund", # Sundanese
	"sylo", # Syloti Nagri
	"syrc", # Syriac
	"tglg", # Tagalog
	"tagb", # Tagbanwa
	"tale", # Tai Le
	"lana", # Tai Tham (Lanna)
	"tavt", # Tai Viet
	"takr", # Takri
	"taml", # Tamil
	"tml2", # Tamil v.2
	"tang", # Tangut
	"telu", # Telugu
	"tel2", # Telugu v.2
	"thaa", # Thaana
	"thai", # Thai
	"tibt", # Tibetan
	"tfng", # Tifinagh
	"tirh", # Tirhuta
	"ugar", # Ugaritic Cuneiform
	"vai ", # Vai
	"wara", # Warang Citi
	"yi  ", # Yi
	"zanb", # Zanabazar Square (Zanabazarin Dörböljin Useg, Xewtee Dörböljin Bicig, Horizontal Square Script)
]

# Updated to OpenType specification version 1.8.3
# https://docs.microsoft.com/en-us/typography/opentype/spec/languagetags
# NOTE: The "dflt" language tag is not included in OT spec
kKnownLanguageTags = [
	"dflt", # Default
	"ABA ", # Abaza
	"ABK ", # Abkhazian
	"ACH ", # Acholi
	"ACR ", # Achi
	"ADY ", # Adyghe
	"AFK ", # Afrikaans
	"AFR ", # Afar
	"AGW ", # Agaw
	"AIO ", # Aiton
	"AKA ", # Akan
	"ALS ", # Alsatian
	"ALT ", # Altai
	"AMH ", # Amharic
	"ANG ", # Anglo-Saxon
	"APPH", # Phonetic transcription—Americanist conventions
	"ARA ", # Arabic
	"ARG ", # Aragonese
	"ARI ", # Aari
	"ARK ", # Rakhine
	"ASM ", # Assamese
	"AST ", # Asturian
	"ATH ", # Athapaskan
	"AVR ", # Avar
	"AWA ", # Awadhi
	"AYM ", # Aymara
	"AZB ", # Torki
	"AZE ", # Azerbaijani
	"BAD ", # Badaga
	"BAD0", # Banda
	"BAG ", # Baghelkhandi
	"BAL ", # Balkar
	"BAN ", # Balinese
	"BAR ", # Bavarian
	"BAU ", # Baulé
	"BBC ", # Batak Toba
	"BBR ", # Berber
	"BCH ", # Bench
	"BCR ", # Bible Cree
	"BDY ", # Bandjalang
	"BEL ", # Belarussian
	"BEM ", # Bemba
	"BEN ", # Bengali
	"BGC ", # Haryanvi
	"BGQ ", # Bagri
	"BGR ", # Bulgarian
	"BHI ", # Bhili
	"BHO ", # Bhojpuri
	"BIK ", # Bikol
	"BIL ", # Bilen
	"BIS ", # Bislama
	"BJJ ", # Kanauji
	"BKF ", # Blackfoot
	"BLI ", # Baluchi
	"BLK ", # Pa'o Karen
	"BLN ", # Balante
	"BLT ", # Balti
	"BMB ", # Bambara (Bamanankan)
	"BML ", # Bamileke
	"BOS ", # Bosnian
	"BPY ", # Bishnupriya Manipuri
	"BRE ", # Breton
	"BRH ", # Brahui
	"BRI ", # Braj Bhasha
	"BRM ", # Burmese
	"BRX ", # Bodo
	"BSH ", # Bashkir
	"BSK ", # Burushaski
	"BTI ", # Beti
	"BTS ", # Batak Simalungun
	"BUG ", # Bugis
	"BYV ", # Medumba
	"CAK ", # Kaqchikel
	"CAT ", # Catalan
	"CBK ", # Zamboanga Chavacano
	"CCHN", # Chinantec
	"CEB ", # Cebuano
	"CHE ", # Chechen
	"CHG ", # Chaha Gurage
	"CHH ", # Chattisgarhi
	"CHI ", # Chichewa (Chewa, Nyanja)
	"CHK ", # Chukchi
	"CHK0", # Chuukese
	"CHO ", # Choctaw
	"CHP ", # Chipewyan
	"CHR ", # Cherokee
	"CHA ", # Chamorro
	"CHU ", # Chuvash
	"CHY ", # Cheyenne
	"CGG ", # Chiga
	"CJA ", # Western Cham
	"CJM ", # Eastern Cham
	"CMR ", # Comorian
	"COP ", # Coptic
	"COR ", # Cornish
	"COS ", # Corsican
	"CPP ", # Creoles
	"CRE ", # Cree
	"CRR ", # Carrier
	"CRT ", # Crimean Tatar
	"CSB ", # Kashubian
	"CSL ", # Church Slavonic
	"CSY ", # Czech
	"CTG ", # Chittagonian
	"CUK ", # San Blas Kuna
	"DAN ", # Danish
	"DAR ", # Dargwa
	"DAX ", # Dayi
	"DCR ", # Woods Cree
	"DEU ", # German
	"DGO ", # Dogri
	"DGR ", # Dogri
	"DHG ", # Dhangu
	# "DHV ", # Divehi (Dhivehi, Maldivian) (deprecated)
	"DIQ ", # Dimli
	"DIV ", # Divehi (Dhivehi, Maldivian)
	"DJR ", # Zarma
	"DJR0", # Djambarrpuyngu
	"DNG ", # Dangme
	"DNJ ", # Dan
	"DNK ", # Dinka
	"DRI ", # Dari
	"DUJ ", # Dhuwal
	"DUN ", # Dungan
	"DZN ", # Dzongkha
	"EBI ", # Ebira
	"ECR ", # Eastern Cree
	"EDO ", # Edo
	"EFI ", # Efik
	"ELL ", # Greek
	"EMK ", # Eastern Maninkakan
	"ENG ", # English
	"ERZ ", # Erzya
	"ESP ", # Spanish
	"ESU ", # Central Yupik
	"ETI ", # Estonian
	"EUQ ", # Basque
	"EVK ", # Evenki
	"EVN ", # Even
	"EWE ", # Ewe
	"FAN ", # French Antillean
	"FAN0", # Fang
	"FAR ", # Persian
	"FAT ", # Fanti
	"FIN ", # Finnish
	"FJI ", # Fijian
	"FLE ", # Dutch (Flemish)
	"FMP ", # Fe'fe'
	"FNE ", # Forest Nenets
	"FON ", # Fon
	"FOS ", # Faroese
	"FRA ", # French
	"FRC ", # Cajun French
	"FRI ", # Frisian
	"FRL ", # Friulian
	"FRP ", # Arpitan
	"FTA ", # Futa
	"FUL ", # Fulah
	"FUV ", # Nigerian Fulfulde
	"GAD ", # Ga
	"GAE ", # Scottish Gaelic (Gaelic)
	"GAG ", # Gagauz
	"GAL ", # Galician
	"GAR ", # Garshuni
	"GAW ", # Garhwali
	"GEZ ", # Geez
	"GIH ", # Githabul
	"GIL ", # Gilyak
	"GIL0", # Kiribati (Gilbertese)
	"GKP ", # Kpelle (Guinea)
	"GLK ", # Gilaki
	"GMZ ", # Gumuz
	"GNN ", # Gumatj
	"GOG ", # Gogo
	"GON ", # Gondi
	"GRN ", # Greenlandic
	"GRO ", # Garo
	"GUA ", # Guarani
	"GUC ", # Wayuu
	"GUF ", # Gupapuyngu
	"GUJ ", # Gujarati
	"GUZ ", # Gusii
	"HAI ", # Haitian (Haitian Creole)
	"HAL ", # Halam (Falam Chin)
	"HAR ", # Harauti
	"HAU ", # Hausa
	"HAW ", # Hawaiian
	"HAY ", # Haya
	"HAZ ", # Hazaragi
	"HBN ", # Hammer-Banna
	"HER ", # Herero
	"HIL ", # Hiligaynon
	"HIN ", # Hindi
	"HMA ", # High Mari
	"HMN ", # Hmong
	"HMO ", # Hiri Motu
	"HND ", # Hindko
	"HO  ", # Ho
	"HRI ", # Harari
	"HRV ", # Croatian
	"HUN ", # Hungarian
	"HYE ", # Armenian
	"HYE0", # Armenian East
	"IBA ", # Iban
	"IBB ", # Ibibio
	"IBO ", # Igbo
	"IJO ", # Ijo languages
	"IDO ", # Ido
	"ILE ", # Interlingue
	"ILO ", # Ilokano
	"INA ", # Interlingua
	"IND ", # Indonesian
	"ING ", # Ingush
	"INU ", # Inuktitut
	"IPK ", # Inupiat
	"IPPH", # Phonetic transcription—IPA conventions
	"IRI ", # Irish
	"IRT ", # Irish Traditional
	"ISL ", # Icelandic
	"ISM ", # Inari Sami
	"ITA ", # Italian
	"IWR ", # Hebrew
	"JAM ", # Jamaican Creole
	"JAN ", # Japanese
	"JAV ", # Javanese
	"JBO ", # Lojban
	"JCT ", # Krymchak
	"JII ", # Yiddish
	"JUD ", # Ladino
	"JUL ", # Jula
	"KAB ", # Kabardian
	"KAB0", # Kabyle
	"KAC ", # Kachchi
	"KAL ", # Kalenjin
	"KAN ", # Kannada
	"KAR ", # Karachay
	"KAT ", # Georgian
	"KAZ ", # Kazakh
	"KDE ", # Makonde
	"KEA ", # Kabuverdianu (Crioulo)
	"KEB ", # Kebena
	"KEK ", # Kekchi
	"KGE ", # Khutsuri Georgian
	"KHA ", # Khakass
	"KHK ", # Khanty-Kazim
	"KHM ", # Khmer
	"KHS ", # Khanty-Shurishkar
	"KHT ", # Khamti Shan
	"KHV ", # Khanty-Vakhi
	"KHW ", # Khowar
	"KIK ", # Kikuyu (Gikuyu)
	"KIR ", # Kirghiz (Kyrgyz)
	"KIS ", # Kisii
	"KIU ", # Kirmanjki
	"KJD ", # Southern Kiwai
	"KJP ", # Eastern Pwo Karen
	"KJZ ", # Bumthangkha
	"KKN ", # Kokni
	"KLM ", # Kalmyk
	"KMB ", # Kamba
	"KMN ", # Kumaoni
	"KMO ", # Komo
	"KMS ", # Komso
	"KMZ ", # Khorasani Turkic
	"KNR ", # Kanuri
	"KOD ", # Kodagu
	"KOH ", # Korean Old Hangul
	"KOK ", # Konkani
	"KON ", # Kikongo
	"KOM ", # Komi
	"KON0", # Kongo
	"KOP ", # Komi-Permyak
	"KOR ", # Korean
	"KOS ", # Kosraean
	"KOZ ", # Komi-Zyrian
	"KPL ", # Kpelle
	"KRI ", # Krio
	"KRK ", # Karakalpak
	"KRL ", # Karelian
	"KRM ", # Karaim
	"KRN ", # Karen
	"KRT ", # Koorete
	"KSH ", # Kashmiri
	"KSH0", # Ripuarian
	"KSI ", # Khasi
	"KSM ", # Kildin Sami
	"KSW ", # S’gaw Karen
	"KUA ", # Kuanyama
	"KUI ", # Kui
	"KUL ", # Kulvi
	"KUM ", # Kumyk
	"KUR ", # Kurdish
	"KUU ", # Kurukh
	"KUY ", # Kuy
	"KYK ", # Koryak
	"KYU ", # Western Kayah
	"LAD ", # Ladin
	"LAH ", # Lahuli
	"LAK ", # Lak
	"LAM ", # Lambani
	"LAO ", # Lao
	"LAT ", # Latin
	"LAZ ", # Laz
	"LCR ", # L-Cree
	"LDK ", # Ladakhi
	"LEZ ", # Lezgi
	"LIJ ", # Ligurian
	"LIM ", # Limburgish
	"LIN ", # Lingala
	"LIS ", # Lisu
	"LJP ", # Lampung
	"LKI ", # Laki
	"LMA ", # Low Mari
	"LMB ", # Limbu
	"LMO ", # Lombard
	"LMW ", # Lomwe
	"LOM ", # Loma
	"LRC ", # Luri
	"LSB ", # Lower Sorbian
	"LSM ", # Lule Sami
	"LTH ", # Lithuanian
	"LTZ ", # Luxembourgish
	"LUA ", # Luba-Lulua
	"LUB ", # Luba-Katanga
	"LUG ", # Ganda
	"LUH ", # Luyia
	"LUO ", # Luo
	"LVI ", # Latvian
	"MAD ", # Madura
	"MAG ", # Magahi
	"MAH ", # Marshallese
	"MAJ ", # Majang
	"MAK ", # Makhuwa
	"MAL ", # Malayalam
	"MAM ", # Mam
	"MAN ", # Mansi
	"MAP ", # Mapudungun
	"MAR ", # Marathi
	"MAW ", # Marwari
	"MBN ", # Mbundu
	"MBO ", # Mbo
	"MCH ", # Manchu
	"MCR ", # Moose Cree
	"MDE ", # Mende
	"MDR ", # Mandar
	"MEN ", # Me'en
	"MER ", # Meru
	"MFA ", # Pattani Malay
	"MFE ", # Morisyen
	"MIN ", # Minangkabau
	"MIZ ", # Mizo
	"MKD ", # Macedonian
	"MKR ", # Makasar
	"MKW ", # Kituba
	"MLE ", # Male
	"MLG ", # Malagasy
	"MLN ", # Malinke
	"MLR ", # Malayalam Reformed
	"MLY ", # Malay
	"MND ", # Mandinka
	"MNG ", # Mongolian
	"MNI ", # Manipuri
	"MNK ", # Maninka
	"MNX ", # Manx
	"MOH ", # Mohawk
	"MOK ", # Moksha
	"MOL ", # Moldavian
	"MON ", # Mon
	"MOR ", # Moroccan
	"MOS ", # Mossi
	"MRI ", # Maori
	"MTH ", # Maithili
	"MTS ", # Maltese
	"MUN ", # Mundari
	"MUS ", # Muscogee
	"MWL ", # Mirandese
	"MWW ", # Hmong Daw
	"MYN ", # Mayan
	"MZN ", # Mazanderani
	"NAG ", # Naga-Assamese
	"NAH ", # Nahuatl
	"NAN ", # Nanai
	"NAP ", # Neapolitan
	"NAS ", # Naskapi
	"NAU ", # Nauruan
	"NAV ", # Navajo
	"NCR ", # N-Cree
	"NDB ", # Ndebele
	"NDC ", # Ndau
	"NDG ", # Ndonga
	"NDS ", # Low Saxon
	"NEP ", # Nepali
	"NEW ", # Newari
	"NGA ", # Ngbaka
	"NGR ", # Nagari
	"NHC ", # Norway House Cree
	"NIS ", # Nisi
	"NIU ", # Niuean
	"NKL ", # Nyankole
	"NKO ", # N'Ko
	"NLD ", # Dutch
	"NOE ", # Nimadi
	"NOG ", # Nogai
	"NOR ", # Norwegian
	"NOV ", # Novial
	"NSM ", # Northern Sami
	"NSO ", # Sotho, Northern
	"NTA ", # Northern Tai
	"NTO ", # Esperanto
	"NYM ", # Nyamwezi
	"NYN ", # Norwegian Nynorsk (Nynorsk, Norwegian)
	"NZA ", # Mbembe Tigon
	"OCI ", # Occitan
	"OCR ", # Oji-Cree
	"OJB ", # Ojibway
	"ORI ", # Odia (formerly Oriya)
	"ORO ", # Oromo
	"OSS ", # Ossetian
	"PAA ", # Palestinian Aramaic
	"PAG ", # Pangasinan
	"PAL ", # Pali
	"PAM ", # Pampangan
	"PAN ", # Punjabi
	"PAP ", # Palpa
	"PAP0", # Papiamentu
	"PAS ", # Pashto
	"PAU ", # Palauan
	"PCC ", # Bouyei
	"PCD ", # Picard
	"PDC ", # Pennsylvania German
	"PGR ", # Polytonic Greek
	"PHK ", # Phake
	"PIH ", # Norfolk
	"PIL ", # Filipino
	"PLG ", # Palaung
	"PLK ", # Polish
	"PMS ", # Piemontese
	"PNB ", # Western Panjabi
	"POH ", # Pocomchi
	"PON ", # Pohnpeian
	"PRO ", # Provençal / Old Provençal
	"PTG ", # Portuguese
	"PWO ", # Western Pwo Karen
	"QIN ", # Chin
	"QUC ", # K’iche’
	"QUH ", # Quechua (Bolivia)
	"QUZ ", # Quechua
	"QVI ", # Quechua (Ecuador)
	"QWH ", # Quechua (Peru)
	"RAJ ", # Rajasthani
	"RAR ", # Rarotongan
	"RBU ", # Russian Buriat
	"RCR ", # R-Cree
	"REJ ", # Rejang
	"RIA ", # Riang
	"RIF ", # Tarifit
	"RIT ", # Ritarungo
	"RKW ", # Arakwal
	"RMS ", # Romansh
	"RMY ", # Vlax Romani
	"ROM ", # Romanian
	"ROY ", # Romany
	"RSY ", # Rusyn
	"RTM ", # Rotuman
	"RUA ", # Kinyarwanda
	"RUN ", # Rundi
	"RUP ", # Aromanian
	"RUS ", # Russian
	"SAD ", # Sadri
	"SAN ", # Sanskrit
	"SAS ", # Sasak
	"SAT ", # Santali
	"SAY ", # Sayisi
	"SCN ", # Sicilian
	"SCO ", # Scots
	"SEK ", # Sekota
	"SEL ", # Selkup
	"SGA ", # Old Irish
	"SGO ", # Sango
	"SGS ", # Samogitian
	"SHI ", # Tachelhit
	"SHN ", # Shan
	"SIB ", # Sibe
	"SID ", # Sidamo
	"SIG ", # Silte Gurage
	"SKS ", # Skolt Sami
	"SKY ", # Slovak
	"SCS ", # North Slavey
	"SLA ", # Slavey
	"SLV ", # Slovenian
	"SML ", # Somali
	"SMO ", # Samoan
	"SNA ", # Sena
	"SNA0", # Shona
	"SND ", # Sindhi
	"SNH ", # Sinhala (Sinhalese)
	"SNK ", # Soninke
	"SOG ", # Sodo Gurage
	"SOP ", # Songe
	"SOT ", # Sotho, Southern
	"SQI ", # Albanian
	"SRB ", # Serbian
	"SRD ", # Sardinian
	"SRK ", # Saraiki
	"SRR ", # Serer
	"SSL ", # South Slavey
	"SSM ", # Southern Sami
	"STQ ", # Saterland Frisian
	"SUK ", # Sukuma
	"SUN ", # Sundanese
	"SUR ", # Suri
	"SVA ", # Svan
	"SVE ", # Swedish
	"SWA ", # Swadaya Aramaic
	"SWK ", # Swahili
	"SWZ ", # Swati
	"SXT ", # Sutu
	"SXU ", # Upper Saxon
	"SYL ", # Sylheti
	"SYR ", # Syriac
	"SYRE", # Syriac, Estrangela
	"SYRJ", # Syriac, Western
	"SYRN", # Syriac, Eastern
	"SZL ", # Silesian
	"TAB ", # Tabasaran
	"TAJ ", # Tajiki
	"TAM ", # Tamil
	"TAT ", # Tatar
	"TCR ", # TH-Cree
	"TDD ", # Dehong Dai
	"TEL ", # Telugu
	"TET ", # Tetum
	"TGL ", # Tagalog
	"TGN ", # Tongan
	"TGR ", # Tigre
	"TGY ", # Tigrinya
	"THA ", # Thai
	"THT ", # Tahitian
	"TIB ", # Tibetan
	"TIV ", # Tiv
	"TKM ", # Turkmen
	"TMH ", # Tamashek
	"TMN ", # Temne
	"TNA ", # Tswana
	"TNE ", # Tundra Nenets
	"TNG ", # Tonga
	"TOD ", # Todo
	"TOD0", # Toma
	"TPI ", # Tok Pisin
	"TRK ", # Turkish
	"TSG ", # Tsonga
	"TSJ ", # Tshangla
	"TUA ", # Turoyo Aramaic
	"TUM ", # Tulu
	"TUL ", # Tumbuka
	"TUV ", # Tuvin
	"TVL ", # Tuvalu
	"TWI ", # Twi
	"TYZ ", # Tày
	"TZM ", # Tamazight
	"TZO ", # Tzotzil
	"UDM ", # Udmurt
	"UKR ", # Ukrainian
	"UMB ", # Umbundu
	"URD ", # Urdu
	"USB ", # Upper Sorbian
	"UYG ", # Uyghur
	"UZB ", # Uzbek
	"VEC ", # Venetian
	"VEN ", # Venda
	"VIT ", # Vietnamese
	"VOL ", # Volapük
	"VRO ", # Võro
	"WA  ", # Wa
	"WAG ", # Wagdi
	"WAR ", # Waray-Waray
	"WCR ", # West-Cree
	"WEL ", # Welsh
	"WLN ", # Walloon
	"WLF ", # Wolof
	"WTM ", # Mewati
	"XBD ", # Lü
	"XKF ", # Khengkha
	"XHS ", # Xhosa
	"XJB ", # Minjangbal
	"XOG ", # Soga
	"XPE ", # Kpelle (Liberia)
	"YAK ", # Sakha
	"YAO ", # Yao
	"YAP ", # Yapese
	"YBA ", # Yoruba
	"YCR ", # Y-Cree
	"YIC ", # Yi Classic
	"YIM ", # Yi Modern
	"ZEA ", # Zealandic
	"ZGH ", # Standard Moroccan Tamazight
	"ZHA ", # Zhuang
	"ZHH ", # Chinese, Hong Kong SAR
	"ZHP ", # Chinese Phonetic
	"ZHS ", # Chinese Simplified
	"ZHT ", # Chinese Traditional
	"ZND ", # Zande
	"ZUL ", # Zulu
	"ZZA ", # Zazaki
]


gAGDDict = {}

class CompareFamilyFont:
		def __init__(self, ttFont, path):
			self.ttFont = ttFont
			self.path = path


def readGlyphInfo(cmpfFont):

	# Get glyph name list.
	glyphnames = cmpfFont.ttFont.getGlyphOrder()
	cmpfFont.glyphnames = glyphnames

	# get/ print the horizontal advance and side-bearing for each glyph.
	# note that any table can be accessed by using the table tag as a key
	# into the font dict. Ifthe table has not been parsed, it is parsed at this point.

	tables = cmpfFont.ttFont.keys() # get list of sfnt tables from ttFont
	if 'hmtx' in tables:
		htmx_table = cmpfFont.ttFont['hmtx']

		# I looked at the file _h_m_t_x.py, in:
		# :RoboFog 163 (pro): Fog: RoboFogMarkII: Lib: fontTool:ttLib:tables
		# to see fields would be available in this class.
		widthsDict = {}
		myrange = range(len(htmx_table.metrics))
		glyph_name_list1 = []
		glyph_width_list1 = []
		glyph_name_list2 = []
		glyph_width_list2 = []
		glyph_name_list3 = []
		glyph_width_list3 = []
		glyph_widths = []
		tab_glyph_list1 = ["zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "zero.slash"]
		tab_glyph_list2 = ["zero.taboldstyle", "one.taboldstyle", "two.taboldstyle", "three.taboldstyle", "four.taboldstyle", "five.taboldstyle", "six.taboldstyle", "seven.taboldstyle", "eight.taboldstyle", "nine.taboldstyle"]
		tab_glyph_list3 = ["zero.numerator", "one.numerator", "two.numerator", "three.numerator", "four.numerator", "five.numerator", "six.numerator", "seven.numerator", "eight.numerator", "nine.numerator"]
		for index in myrange:
			glyph_name = glyphnames[index]
			gnameList = widthsDict.get(htmx_table.metrics[glyph_name][0], [])
			gnameList.append(glyph_name)
			widthsDict[htmx_table.metrics[glyph_name][0]] = gnameList
			if glyph_name in tab_glyph_list1:
				glyph_name_list1.append(glyph_name)
				glyph_width_list1.append(htmx_table.metrics[glyph_name][0])
			if glyph_name in tab_glyph_list2:
				glyph_name_list2.append(glyph_name)
				glyph_width_list2.append(htmx_table.metrics[glyph_name][0])
			if glyph_name in tab_glyph_list3:
				glyph_name_list3.append(glyph_name)
				glyph_width_list3.append(htmx_table.metrics[glyph_name][0])
			glyph_widths.append(htmx_table.metrics[glyph_name][0])

		cmpfFont.glyph_name_list1 =	glyph_name_list1
		cmpfFont.glyph_width_list1 = glyph_width_list1
		cmpfFont.glyph_name_list2 =	glyph_name_list2
		cmpfFont.glyph_width_list2 = glyph_width_list2
		cmpfFont.glyph_name_list3 =	glyph_name_list3
		cmpfFont.glyph_width_list3 = glyph_width_list3
		cmpfFont.glyph_widths = glyph_widths
		cmpfFont.isFixedPitchfromHMTX = len(widthsDict) == 1


	# Use tx to get RSB
	### glyph[tag] {gname,enc,width,{left,bottom,right,top}}
	# glyph[1] {space,0x0020,250,{0,0,0,0}}
	command = "tx -mtx \"%s\" 2>&1" % (cmpfFont.path)
	report = fdkutils.runShellCmd(command)
	metrics = re.findall(r"glyph\S+\s+{([^,]+),[^,]+,([^,]+),{([-0-9]+),([-0-9]+),([-0-9]+),([-0-9]+)}}", report)
	if not metrics:
		print("Error: Quitting. Could not run 'tx' against the font %s to get font metrics." % cmpfFont.path)
		print("\t tx log output <" + report + ">.")
		sys.exit(1)
	cmpfFont.metricsDict = {}
	for entry in metrics:
		valList = [eval(val) for val in entry[1:]]
		cmpfFont.metricsDict[entry[0]] = valList
	# use spot to get ligature defintions.
	command = "spot -t GSUB=7 \"%s\" 2>&1" % (cmpfFont.path)
	report = fdkutils.runShellCmd(command)
	if cmpfFont.isTTF:
		report = re.sub(r"(\S+)@\d+", r"\1", report)
	if cmpfFont.isCID:
		report = re.sub(r"\\(\d+)", r"\1", report)
	cmpfFont.ligDict = {}
	for ligfeatureTag in kLigFeatureList:
		pat = re.compile(r"# Printing lookup \d+ in feature '%s'[^\r\n]+[\r\n](.+?)(?=[\r\n][\r\n])" % ligfeatureTag, re.DOTALL)
		subTableList = re.findall(pat,  report)
		if subTableList:
			for subtable in subTableList:
				ligList = re.findall(r"sub(?:stitute)*\s+(.+?)\s+by\s+([^;]+);",subtable)
				for ligEntry in ligList:
					if "[" in ligEntry[0]:
						continue # can't be a ligature if the input contains a class entry
					lig = ligEntry[1].strip()

					if " " in lig:
						continue # it is a ligature subs only if it has one output.

					ligCompInput = ligEntry[0].strip()

					# clean up contextual outptu for spot.
					if "lookup" in ligCompInput:
						ligCompInput = re.sub(r"lookup\s\d+", "", ligCompInput)
						ligCompInput = re.sub(r";", "", ligCompInput)

					ligCompList = ligCompInput.split()
					if "'" in  ligCompInput: # If contextual , filter out all but the input string.
						ligCompList = filter(lambda name: name[-1] == "'", ligCompList)
						ligCompList = map(lambda name: name[:-1], ligCompList)
					if len(ligCompList) < 2:
						continue # it is a ligature subs only if input has nore than one entry
					compList = cmpfFont.ligDict.get(lig, [])
					compList.append(ligCompList)
					cmpfFont.ligDict[lig] = compList

def readCFFTable(cmpfFont):
	# get 'CFF ' table private dict
	cff_table = cmpfFont.ttFont['CFF ']

	first_font_name = cff_table.cff.fontNames[0]
	first_font_topDict = cff_table.cff[first_font_name]
	cmpfFont.topDict = first_font_topDict
	try:
		cmpfFont.FDArray = first_font_topDict.FDArray
		cmpfFont.isCID = True
	except:
		cmpfFont.isCID = False
		cmpfFont.FDArray = [first_font_topDict]


# sorting routines for reports

def sort_font(fontlist):
	"""
	Returns a new list that is first sorted by the font's compatibleFamilyName3
	(Windows compatible name), and secondly by the font's macStyle (style name).
	"""
	return sorted(fontlist, key=lambda font: (font.compatibleFamilyName3, font.macStyle))

def sort_by_numGlyphs_and_PostScriptName1(fontlist):
	"""
	Returns a new list that is first sorted by the font's numGlyphs,
	and secondly by the font's PostScriptName1.
	"""
	return sorted(fontlist, key=lambda font: (font.numGlyphs, font.PostScriptName1))

def sort_angle(fontlist):
	newlist = [fontlist[0]]
	for font in fontlist[1:]:
		if font.italicAngle >=newlist[-1].italicAngle:
			newlist.append(font)
		else:
			for i in range(len(newlist)):
				if	font.italicAngle < newlist[i].italicAngle:
					newlist.insert(i,font)
					break
	return newlist

def sort_version(fontlist):
	newlist = [fontlist[0]]
	for font in fontlist[1:]:
		if font.OTFVersion <=newlist[-1].OTFVersion:
			newlist.append(font)
		else:
			for i in range(len(newlist)):
				if	font.OTFVersion > newlist[i].OTFVersion:
					newlist.insert(i,font)
					break
	return newlist

def sort_weight(fontlist):
	newlist = [fontlist[0]]
	for font in fontlist[1:]:
		if font.usWeightClass <=newlist[-1].usWeightClass:
			newlist.append(font)
		else:
			for i in range(len(newlist)):
				if	font.usWeightClass > newlist[i].usWeightClass:
					newlist.insert(i,font)
					break
	return newlist

def sort_width(fontlist):
	newlist = [fontlist[0]]
	for font in fontlist[1:]:
		if font.usWidthClass <=newlist[-1].usWidthClass:
			newlist.append(font)
		else:
			for i in range(len(newlist)):
				if	font.usWidthClass > newlist[i].usWidthClass:
					newlist.insert(i,font)
					break
	return newlist

def sort_underline(fontlist):
	newlist = [fontlist[0]]
	for font in fontlist[1:]:
		if font.underlinePos <=newlist[-1].underlinePos:
			newlist.append(font)
		else:
			for i in range(len(newlist)):
				if	font.underlinePos > newlist[i].underlinePos:
					newlist.insert(i,font)
					break
	return newlist



# Read entries from the naming table
def readNameTable(cmpfFont):
	name_table = cmpfFont.ttFont['name']

# A Python dictionary structure is used to hold all the nameID entries from the naming table of a font.
# The attribute of the dictionary is composed of (platform, encoding, language, ID), and the value
# is the content of the nameID.
# Any nameID can then be accessed by making a key (platform, encoding, language, ID) to the dictionary.
	cmpfFont.nameIDDict = {}
	cmpfFont.hasMacNames = 0
	noMacNames  = 1
	for namerecord in name_table.names:
		cmpfFont.nameIDDict[(namerecord.platformID, namerecord.platEncID, namerecord.langID, namerecord.nameID)] = namerecord.string
		if noMacNames and namerecord.platformID == 1:
			cmpfFont.hasMacNames = 1
			noMacNames = 0

	if cmpfFont.hasMacNames:
		# Fill in font info with Mac Names from Mac Platform, Roman Encoding, English Language
		try:
			cmpfFont.PostScriptName1 = cmpfFont.nameIDDict[(1, 0, 0, 6)].decode('mac_roman')
		except KeyError:
			print("	Error: Missing Mac Postcriptname from name table! Should be in record", (1, 0, 0, 6))
			cmpfFont.PostScriptName1 = ""

		try:
			cmpfFont.compatibleFamilyName1 = cmpfFont.nameIDDict[(1, 0, 0, 1)].decode('mac_roman')
		except KeyError:
			print("	Error: Missing Mac Compatible Family Name from name table! Should be in record", (1, 0, 0, 1) , cmpfFont.PostScriptName1)
			cmpfFont.compatibleFamilyName1 = ""

		try:
			cmpfFont.compatibleSubFamilyName1 = cmpfFont.nameIDDict[(1, 0, 0, 2)].decode('mac_roman')
		except KeyError:
			print("	Error: Missing Mac Compatible SubFamily Name from name table! Should be in record", (1, 0, 0, 2), cmpfFont.PostScriptName1)
			cmpfFont.compatibleSubFamilyName1 = ""

		if (1, 0, 0, 16) in cmpfFont.nameIDDict:
			cmpfFont.preferredFamilyName1 = cmpfFont.nameIDDict[(1, 0, 0, 16)].decode('mac_roman')
		else:
			cmpfFont.preferredFamilyName1 = cmpfFont.compatibleFamilyName1

		if (1, 0, 0, 17) in cmpfFont.nameIDDict:
			cmpfFont.preferredSubFamilyName1 = cmpfFont.nameIDDict[(1, 0, 0, 17)].decode('mac_roman')
		else:
			cmpfFont.preferredSubFamilyName1 = cmpfFont.compatibleSubFamilyName1


		try:
			cmpfFont.FullFontName1 = cmpfFont.nameIDDict[(1, 0, 0, 4)].decode('mac_roman')
		except KeyError:
			print("	Error: Missing Mac Full Font Name from name table! Should be in record", (1, 0, 0, 4), cmpfFont.PostScriptName1)
			cmpfFont.FullFontName1 = ""

		try:
			cmpfFont.VersionStr1 = cmpfFont.nameIDDict[(1, 0, 0, 5)].decode('mac_roman')
		except KeyError:
			print("	Warning: Missing Mac Version String from name table! Should be in record", (1, 0, 0, 5), cmpfFont.PostScriptName1)
			cmpfFont.VersionStr1 = ""

		try:
			cmpfFont.copyrightStr1 = cmpfFont.nameIDDict[(1, 0, 0, 0)].decode('mac_roman')
		except KeyError:
			print("	Warning: Missing Mac Copyright String from name table! Should be in record", (1, 0, 0, 0), cmpfFont.PostScriptName1)
			cmpfFont.copyrightStr1 = ""

		try:
			cmpfFont.trademarkStr1 = cmpfFont.nameIDDict[(1, 0, 0, 7)].decode('mac_roman')
		except KeyError:
			print("	Warning: Missing Mac Trademark String from name table! Should be in record", (1, 0, 0, 7), cmpfFont.PostScriptName1)
			cmpfFont.trademarkStr1 = ""


		try:
			cmpfFont.nameIDDict[(1, 0, 0, 14)].decode('mac_roman')
		except KeyError:
			print("	Warning: Missing Mac platform License Notice URL  from name table! Should be in record", (1, 0, 0, 14))

		if (1, 0, 0, 18) in cmpfFont.nameIDDict:
			cmpfFont.MacCompatibleFullName1 = cmpfFont.nameIDDict[(1, 0, 0, 18)].decode('mac_roman')
		else:
			cmpfFont.MacCompatibleFullName1 = cmpfFont.FullFontName1

# Fill in font info with Win Names from Win Platform, Unicode Encoding, English Language
	try:
		cmpfFont.PostScriptName3 = cmpfFont.nameIDDict[(3, 1, 1033, 6)].decode('utf_16_be')
	except KeyError:
		print("	Error: Missing Windows PostScript Name/Unicode encoding from name table! Should be in record", (3, 1, 1033, 6), cmpfFont.PostScriptName3)
		cmpfFont.PostScriptName3 = ""

	try:
		cmpfFont.compatibleFamilyName3 = cmpfFont.nameIDDict[(3, 1, 1033, 1)].decode('utf_16_be')
	except KeyError:
		print("	Error: Missing Windows Compatible Family Name/Unicode encoding String from name table! Should be in record", (3, 1, 1033, 1), cmpfFont.PostScriptName3)
		cmpfFont.compatibleFamilyName3 = ""

	if (3, 1, 1033, 16) in cmpfFont.nameIDDict:
		cmpfFont.preferredFamilyName3 = cmpfFont.nameIDDict[(3, 1, 1033, 16)].decode('utf_16_be')
	else:
		cmpfFont.preferredFamilyName3 = cmpfFont.compatibleFamilyName3


	try:
		cmpfFont.compatibleSubFamilyName3 = cmpfFont.nameIDDict[(3, 1, 1033, 2)].decode('utf_16_be')
	except KeyError:
		print("	Error: Missing Windows Compatible SubFamily Name/Unicode encoding String from name table! Should be in record", (3, 1, 1033, 2), cmpfFont.PostScriptName3)
		cmpfFont.compatibleSubFamilyName3 = ""

	if (3, 1, 1033, 17) in cmpfFont.nameIDDict:
		cmpfFont.preferredSubFamilyName3 = cmpfFont.nameIDDict[(3, 1, 1033, 17)].decode('utf_16_be')
	else:
		cmpfFont.preferredSubFamilyName3 = cmpfFont.compatibleSubFamilyName3

	try:
		cmpfFont.FullFontName3 = cmpfFont.nameIDDict[(3, 1, 1033, 4)].decode('utf_16_be')
	except KeyError:
		print("	Error: Missing Windows Full Family Name/Unicode encoding String from name table! Should be in record", (3, 1, 1033, 4), cmpfFont.PostScriptName3)
		cmpfFont.FullFontName3 = ""

	try:
		cmpfFont.VersionStr3 = cmpfFont.nameIDDict[(3, 1, 1033, 5)].decode('utf_16_be')
	except KeyError:
		print("	Error: Missing Windows Version String/Unicode encoding String from name table! Should be in record", (3, 1, 1033, 5), cmpfFont.PostScriptName3)
		cmpfFont.VersionStr3 = ""

	try:
		cmpfFont.copyrightStr3 = cmpfFont.nameIDDict[(3, 1, 1033, 0)].decode('utf_16_be')
	except KeyError:
		print("	Warning: Missing Windows Copyright String/Unicode encoding String from name table! Should be in record", (3, 1, 1033, 0), cmpfFont.PostScriptName3)
		cmpfFont.copyrightStr3 = ""


	try:
		cmpfFont.trademarkStr3 = cmpfFont.nameIDDict[(3, 1, 1033, 7)].decode('utf_16_be')
	except KeyError:
		print("	Warning: Missing Windows Trademark String/Unicode encoding String from name table! Should be in record", (3, 1, 1033, 7), cmpfFont.PostScriptName3)
		cmpfFont.trademarkStr3 = ""

	if not cmpfFont.hasMacNames:
		cmpfFont.PostScriptName1 = cmpfFont.PostScriptName3
		cmpfFont.compatibleFamilyName1 = cmpfFont.compatibleFamilyName3
		cmpfFont.compatibleSubFamilyName1 = cmpfFont.compatibleSubFamilyName3
		cmpfFont.preferredFamilyName1 = cmpfFont.preferredFamilyName3
		cmpfFont.preferredSubFamilyName1 = cmpfFont.preferredSubFamilyName3
		cmpfFont.FullFontName1 = cmpfFont.FullFontName3
		cmpfFont.VersionStr1 = cmpfFont.VersionStr3
		cmpfFont.copyrightStr1 = cmpfFont.copyrightStr3
		cmpfFont.trademarkStr1 = cmpfFont.trademarkStr3
		cmpfFont.MacCompatibleFullName1 = cmpfFont.FullFontName1

	# Complain if preferred font name has preferred subfamily name, or std styles in name.
	styleList = [cmpfFont.preferredSubFamilyName1] + [ "Regular", "Bold", "Italic", "Bold Italic"]
	for style in styleList:
		stylePat = " %s " % format(style)
		if stylePat in cmpfFont.preferredFamilyName1:
			print("	Error: Preferred family name '%s' contains the style name '%s'. This is usually confusing to users. %s" % (cmpfFont.preferredFamilyName1, style, cmpfFont.PostScriptName1))

	if (3, 1, 1033, 14) not in cmpfFont.nameIDDict:
		print("	Warning: Missing Windows License Notice URL from name table! Should be in record", (3, 1, 1033, 14))

	if (3, 1, 1033, 8) not in cmpfFont.nameIDDict:
		print("	Warning: Missing Windows Manufacturer from name table! Should be in record", (3, 1, 1033, 8))


# Fill in font info with values from OS/2 table
def readOS2Table(cmpfFont):
	try:
		os2table = cmpfFont.ttFont['OS/2']

		cmpfFont.OS2version	=  os2table.version
		cmpfFont.avgCharWidth	=  os2table.xAvgCharWidth

		cmpfFont.ySubscriptXSize	=  os2table.ySubscriptXSize
		cmpfFont.ySubscriptYSize	=  os2table.ySubscriptYSize
		cmpfFont.ySubscriptXOffset	=  os2table.ySubscriptXOffset
		cmpfFont.ySubscriptYOffset	=  os2table.ySubscriptYOffset

		cmpfFont.ySuperscriptXSize	=  os2table.ySuperscriptXSize
		cmpfFont.ySuperscriptYSize	=  os2table.ySuperscriptYSize
		cmpfFont.ySuperscriptXOffset	=  os2table.ySuperscriptXOffset
		cmpfFont.ySuperscriptYOffset	=  os2table.ySuperscriptYOffset

		cmpfFont.yStrikeoutPosition	=  os2table.yStrikeoutPosition
		cmpfFont.yStrikeoutSize	=  os2table.yStrikeoutSize

		cmpfFont.usWeightClass	=  os2table.usWeightClass
		cmpfFont.usWidthClass	=  os2table.usWidthClass
		cmpfFont.fsType			=  os2table.fsType
		cmpfFont.achVendID		=  os2table.achVendID
		fsSelection	=  os2table.fsSelection
		cmpfFont.isItalic2	   = fsSelection & 0x0001
		cmpfFont.isBold2		   = (fsSelection >> 5) & 0x0001
		cmpfFont.sTypoAscender	=  os2table.sTypoAscender
		cmpfFont.sTypoDescender	=  os2table.sTypoDescender
		cmpfFont.sTypoLineGap	=  os2table.sTypoLineGap
		cmpfFont.fsSelection =  os2table.fsSelection
		try:
			cmpfFont.usDefaultChar =  os2table.usDefaultChar
		except AttributeError:
			cmpfFont.usDefaultChar  = None

		try:
			cmpfFont.sxHeight		=  os2table.sxHeight
		except AttributeError:
			cmpfFont.sxHeight		=  -1
		try:
			cmpfFont.sCapHeight		=  os2table.sCapHeight
		except AttributeError:
			cmpfFont.sCapHeight		=  -1
		cmpfFont.panose			=  [
			os2table.panose.bFamilyType,
			os2table.panose.bSerifStyle,
			os2table.panose.bWeight,
			os2table.panose.bProportion,
			os2table.panose.bContrast,
			os2table.panose.bStrokeVariation,
			os2table.panose.bArmStyle,
			os2table.panose.bLetterForm,
			os2table.panose.bMidline,
			os2table.panose.bXHeight ]
	except KeyError:
		print("	Error! No OS/2 table!", cmpfFont.PostScriptName1)
		headTable = cmpfFont.ttFont['head']
		cmpfFont.avgCharWidth	=  -1
		cmpfFont.usWeightClass	=  -1
		cmpfFont.usWidthClass	=  -1
		cmpfFont.fsType			=  -1
		cmpfFont.achVendID		=  'UNKN'
		cmpfFont.isItalic2	     = -1
		cmpfFont.isBold2		=  -1
		cmpfFont.sTypoAscender	=  -1
		cmpfFont.sTypoDescender	=  -1
		cmpfFont.sTypoLineGap	=  -1
		cmpfFont.sxHeight		=  -1
		cmpfFont.sCapHeight		=  -1
		cmpfFont.panose			=  []
		cmpFont.fsSelection = 0
		cmpfFont.usDefaultChar  = None



# Fill in font info with values from post table
def readpostTable(cmpfFont):
	try:
		postTable = cmpfFont.ttFont['post']

		cmpfFont.italicAngle	= postTable.italicAngle
		cmpfFont.underlinePos	= postTable.underlinePosition
		cmpfFont.underlineThickness	= postTable.underlineThickness
		cmpfFont.isFixedPitch	= postTable.isFixedPitch
	except KeyError:
		print("	Error! No post table!", cmpfFont.PostScriptName1)
		cmpfFont.italicAngle	= -1
		cmpfFont.underlinePos	= -1
		cmpfFont.underlineThickness	= -1
		cmpfFont.isFixedPitch	= -1



# Fill in font info with values from head table
def readheadTable(cmpfFont):
	headTable = cmpfFont.ttFont['head']

	cmpfFont.OTFVersion = int( (headTable.fontRevision + 0.0005)*1000)/1000.0
	cmpfFont.fontBBox   = (headTable.xMin, headTable.yMin, headTable.xMax, headTable.yMax)
	macStyle = headTable.macStyle
	cmpfFont.isBold	 = macStyle & 0x0001
	cmpfFont.isItalic = (macStyle >> 1) & 0x0001
	cmpfFont.macStyle = cmpfFont.isItalic | cmpfFont.isBold << 1	# for sorting purpose
	cmpfFont.unitsPerEm = headTable.unitsPerEm
	cmpfFont.fontDirectionHint = headTable.fontDirectionHint
	cmpfFont.isRTL = headTable.flags  & 1

# Fill in font info with values from hhea table
def readhheaTable(cmpfFont):
	hheaTable = cmpfFont.ttFont['hhea']
	maxpTable = cmpfFont.ttFont['maxp']

	cmpfFont.numGlyphs = maxpTable.numGlyphs
	cmpfFont.sTypoAscender2	= hheaTable.ascent
	cmpfFont.sTypoDescender2 = hheaTable.descent
	cmpfFont.sTypoLineGap2	= hheaTable.lineGap



def handle_datafork_file(path):
	return ttLib.TTFont(path)



def handle_resource_file(path):
	tt = None
	filename = os.path.basename(path)
	fss = macfs.FSSpec(path)
	try:
		resref = Carbon.Res.FSpOpenResFile(fss, 1)  # read-only
	except:
		return "unknown"
	Carbon.Res.UseResFile(resref)
	try:
		n = Carbon.Res.Count1Resources("sfnt")
		for i in range(1, n+1):
			res = Carbon.Res.Get1IndResource('sfnt', i)
			resid, restype, resname = res.GetResInfo()
			if not resname:
				resname = filename + i
			tt = ttLib.TTFont(path, i)
	finally:
		Carbon.Res.CloseResFile(resref)
	return tt


def guessfiletype(path):
	try:
		with open(path, "rb") as f:
			data = f.read(512)
	except (IOError, OSError):
		print("Unable to check file type of file %s. Skipping." % (path))
		return "unknown"

	if not data or len(data) < 512:
		return "unknown"

	if data[:4] in (b"\000\001\000\000", b"OTTO", b"true", 0x00010000):
		return "datafork"
	elif os.name=='mac':
		# assume res fork font
		fss = macfs.FSSpec(path)
		try:
			resref = Carbon.Res.FSpOpenResFile(fss, 1)  # read-only
		except:
			return "unknown"
		Carbon.Res.UseResFile(resref)
		i = Carbon.Res.Count1Resources("sfnt")
		Carbon.Res.CloseResFile(resref)
		if i > 0:
			return "resource"
	return "unknown"



# Fill in font info by reading the required tables
def get_fontinfo(pathname, font_type):
	if font_type == "resource":
		ttFont = handle_resource_file(pathname)
	elif font_type == "datafork":
		ttFont = handle_datafork_file(pathname)
	else:
		ttFont = None

	return ttFont

# Get all the .otf file from the directory, read in the font info and build a font list
def build_fontlist_from_dir(directory):
	global fontlist
	try:
		# get all files in the directory
		dirlist = sorted(os.listdir(directory))
	except OSError:
		print("No files found in the directory", directory)
		return []

	for filename in dirlist:
		path = os.path.join(directory, filename )
		if os.path.isfile(path):
			font_type = guessfiletype(path)
			if font_type == "unknown":
				continue
		else:
			continue

		ttFont = get_fontinfo(path, font_type)
		if ttFont is not None:
			cmpfFont = CompareFamilyFont(ttFont, path)
			if 'CFF ' in cmpfFont.ttFont:
				cmpfFont.isTTF = False
				readCFFTable(cmpfFont)
			else:
				cmpfFont.isTTF = True
				cmpfFont.isCID = False
				cmpfFont.topDict = None
			readNameTable(cmpfFont)
			readheadTable(cmpfFont)
			readhheaTable(cmpfFont)
			readOS2Table(cmpfFont)
			readpostTable(cmpfFont)
			readGlyphInfo(cmpfFont)
			fontlist.append(cmpfFont)

	if not fontlist:
		print("No TrueType or OpenType files found in the directory. ", directory)

	return None

def doSingleTest1():
	global fontlist

	print("\nSingle Face Test 1: Length overrun check for name ID 18. Max 63 characters, must be unique within 31 chars.")
	reporter1_fontname = []
	reporter1_failedvalue = []
	nameDict = {}
	for font in fontlist:
		if font.MacCompatibleFullName1:
			key = font.MacCompatibleFullName1[:32]
			if key in nameDict:
				nameDict[key].append(font.PostScriptName1)
				print("	Error: The first 32 chars of the Mac platform name ID 18 Compatible Full Name must be unique within Preferred Family Name group. name: '%s'. Conflicting fonts: %s." % (font.MacCompatibleFullName1, nameDict[key]))
			else:
				nameDict[key] = [font.PostScriptName1]
			if len(font.MacCompatibleFullName1) > 63:
				reporter1_fontname.append(font.PostScriptName1)
				reporter1_failedvalue.append(len(font.MacCompatibleFullName1))
	for i in range(len(reporter1_fontname)):
		print("	Error: Name ID 18, Mac-compatible full name, is", end=' ')
		print(reporter1_failedvalue[i], "characters for Font", reporter1_fontname[i])



def doSingleTest2():
	global fontlist

	print("\nSingle Face Test 2: Length overrun check for name ID's 1,2, 4, 16, 17. Max 63 characters.")
	reporter1_fontname = []
	reporter1_failedvalue = []
	reporter2_fontname = []
	reporter2_failedvalue = []
	reporter3_fontname = []
	reporter3_failedvalue = []
	reporter4_fontname = []
	reporter4_failedvalue = []
	reporter5_fontname = []
	reporter5_failedvalue = []
	nameDict = {}
	for font in fontlist:
		if (len(font.compatibleFamilyName3) > 63):
			reporter1_fontname.append(font.PostScriptName1)
			reporter1_failedvalue.append(len(font.compatibleFamilyName3))
		if (len(font.compatibleSubFamilyName3) > 63):
			reporter2_fontname.append(font.PostScriptName1)
			reporter2_failedvalue.append(len(font.compatibleSubFamilyName3))
		if font.FullFontName1 in nameDict:
			nameDict[font.FullFontName1].append(font.PostScriptName1)
			print("	Error: The Mac platform name ID 4 Preferred Full Name must be unique within Preferred Family Name group. name: '%s'. Conflicting fonts: %s." % (nameDict[font.FullFontName1][0], nameDict[font.FullFontName1][-1]))
		else:
			nameDict[font.FullFontName1] = [font.FullFontName1]
		if (len(font.FullFontName1) > 63):
			reporter3_fontname.append(font.PostScriptName1)
			reporter3_failedvalue.append(len(font.FullFontName1))
		if font.preferredFamilyName3 and (len(font.preferredFamilyName3) > 63):
			reporter4_fontname.append(font.PostScriptName1)
			reporter4_failedvalue.append(len(font.preferredFamilyName3))
		if font.preferredSubFamilyName3 and (len(font.preferredSubFamilyName3) > 63):
			reporter5_fontname.append(font.PostScriptName1)
			reporter5_failedvalue.append(len(font.preferredSubFamilyName3))
	for i in range(len(reporter1_fontname)):
		print("	Error: Name ID 1, Font compatible family name, is", end='')
		print(reporter1_failedvalue[i], "characters for Font", reporter1_fontname[i])
	for i in range(len(reporter2_fontname)):
		print("	Error: Name ID 2, Font compatible subfamily name, is", end='')
		print(reporter2_failedvalue[i], "characters for Font", reporter2_fontname[i])
	for i in range(len(reporter3_fontname)):
		print("	Error: Name ID 4, full font name, is", end='')
		print(reporter3_failedvalue[i], "characters for Font", reporter3_fontname[i])
	for i in range(len(reporter4_fontname)):
		print("	Error: Name ID 16, preferred family name, is", end='')
		print(reporter4_failedvalue[i], "characters for Font", reporter4_fontname[i])
	for i in range(len(reporter5_fontname)):
		print("	Error: Name ID 17, preferred subfamily name, is", end='')
		print(reporter5_failedvalue[i], "characters for Font", reporter5_fontname[i])


def doSingleTest3():
	print("\nSingle Face Test 3: Check that name ID 4 (Full Name) starts with same string as Preferred Family Name, and is the same as the CFF font Full Name.")
	for font in fontlist:
		if not  font.FullFontName1.startswith(font.preferredFamilyName1):
			print("	Error: Mac platform Full Name  name id 4) '%s' does not begin with the string used for font Preferred Family Name, '%s', for Font %s." %  (font.FullFontName1,  font.preferredFamilyName1,  font.PostScriptName1))

		if 'CFF ' in font.ttFont:
			try:
				cffFullName = font.topDict.FullName
				if (font.FullFontName1 != cffFullName):
					print("	Warning: Mac platform Full Name  name id 4) '%s' is not the same as the font CFF table Full Name, '%s', for Font %s." %  (font.FullFontName1,  font.topDict.FullName,  font.PostScriptName1))
					print("This has no functional effect, as the CFF Full Name in an OpenType CFF table is never used. However, having different values for different copies of the same field and cause confusion when using font development tools.")
			except AttributeError:
				print("Note: font CFF table has no Full Name entry, for Font %s." %  (font.PostScriptName1))
				print("This has no functional effect, as the CFF Full Name in an OpenType CFF table is never used.")


def doSingleTest4():
	global fontlist

	print("\nSingle Face Test 4: Version name string matches release font criteria and head table value")
	for font in fontlist:
		if font.VersionStr1.find("Development") != -1:
			print("	Warning: Version string contains the word 'Development' for Font", font.PostScriptName1)
	for font in fontlist:
		versionMatch = re.search(r"(Version|OTF)\s+(\d+\.(\d+))", font.VersionStr1)
		if not versionMatch:
			print("	Error: could not find a valid decimal font version with the prefix 'Version ' or 'OTF ' in the name table name ID 5 'Version'  string: %s, for Font %s." %  (font.VersionStr1,  font.PostScriptName1))
			continue
		if len(versionMatch.group(3)) != 3:
			print("	Error: Version string does not have 3 decimal places in the name table name ID 5 'Version'  string: %s, for Font %s." %  (font.VersionStr1,  font.PostScriptName1))
			continue

		ver = eval(versionMatch.group(2))
		if  ver <= 1.0:
			print("	Warning: Version string is not greater than 1.0000 for Font", font.PostScriptName1)
		if ver != font.OTFVersion:
			print("Error: font version %.3f in name table name ID 5 'Version' string does not match the head table fontRevision value %.3f, for font %s." % (ver, font.OTFVersion,  font.PostScriptName1))

def doSingleTest5():
	global fontlist

	print("\nSingle Face Test 5: Check that CFF PostScript name is same as name table name ID 6.")
	for font in fontlist:
		if 'CFF ' in font.ttFont:
			cffPSName = font.ttFont['CFF '].cff.fontNames[0]
			if cffPSName != font.PostScriptName1:
				print("	Error: Postscript name in CFF table '%s' is not the same as name table Macintosh name ID 6 '%s'." % (cffPSName, font.PostScriptName1))
		if font.PostScriptName3 != font.PostScriptName1:
			print("	Error: Windows name table name id 6 '%s' is not the same as Macintosh name ID 6 '%s'." % (font.PostScriptName3, font.PostScriptName1))

def doSingleTest6():
	global fontlist

	print("\nSingle Face Test 6: Check that Copyright, Trademark, Designer note, and foundry values are present, and match default values.")
	try:
		defaultURL = os.environ["CF_DEFAULT_URL"]
	except KeyError:
		print("	Error: Environment variable CF_DEFAULT_URL is not set, so I can't compare it to  Mac Foundry URL name id 11.")
		defaultURL = None
	try:
		defaultFoundryCode= os.environ["CF_DEFAULT_FOUNDRY_CODE"]
	except KeyError:
		print("	Error: Environment variable CF_DEFAULT_FOUNDRY_CODE is not set, so I can't compare it to  the OS/2 table foundry code.")
		defaultFoundryCode = None

	for font in fontlist:
		# verify that there is no patent number in the copyright.
		if font.copyrightStr1 and re.search("[ \t,]\d\d\d,*\d\d\d[ \t.,]", font.copyrightStr1):
			print(" Error: There is a patent number in the copyright string. Please check.", font.PostScriptName1)

		# Missing Copyright and Trademark strings are actually already reported in the function Read Nmae Table
		if (1, 0, 0, 9) not in font.nameIDDict:
			print("	Error: Missing Mac Designer Note name id 9 from name table! Should be in record", (1, 0, 0, 9), font.PostScriptName1)
		if (3, 1, 1033, 9) not in font.nameIDDict:
			print("	Error: Missing Windows Designer Note name id 9 from name table! Should be in record", (3, 1, 1033, 9), font.PostScriptName1)
		if (1, 0, 0, 11) not in font.nameIDDict:
			print("	Error: Missing Mac Foundry URL name id 11 from name table! Should be in record", (1, 0, 0, 11), font.PostScriptName1)
		else:
			foundryURL = font.nameIDDict[(1, 0, 0, 11)]
			if defaultURL  and  (defaultURL != foundryURL):
				print("	Error: Mac Foundry URL name id 11 '%s' is not the same as the default URL set by the environment variable CF_DEFAULT_URL '%s' for font %s." % (defaultURL, foundryURL, font.PostScriptName1))

		if (3, 1, 1033, 11) not in font.nameIDDict:
			print("	Error: Missing Windows Foundry URL name id 11 from name table! Should be in record", (3, 1, 1033, 11), font.PostScriptName1)

		if defaultFoundryCode  and  (defaultFoundryCode != font.achVendID):
			print("	Error: OS/2 table foundry code '%s' is not the same as the default code set by the environment variable CF_DEFAULT_FOUNDRY_CODE '%s' for font %s." % (font.achVendID, defaultFoundryCode, font.PostScriptName1))

def doSingleTest7():
	global fontlist
	print("\nSingle Face Test 7: Checking for deprecated CFF operators.")
	for font in fontlist:
		if 'CFF ' not in font.ttFont:
			continue
		command = "tx -dump -5 -n \"%s\" 2>&1" % (font.path)
		report = fdkutils.runShellCmd(command)
		glyphList = re.findall(r"glyph[^{]+?\{([^,]+),[^[\]]+\sdotsection\s", report)
		if glyphList:
			glyphList = ", ".join(glyphList)
			print("Error: font contains deprecated dotsection perator. These should be removed. %s. %s" % (font.PostScriptName1, glyphList))
		glyphList = re.findall(r"glyph[^{]+?\{([^,]+),[^[\]]+\sseac\s", report)
		if glyphList:
			glyphList = ", ".join(glyphList)
			print("Error: font contains deprecated seac operator. These composite glyphs must be decomposed. %s. %s" % (font.PostScriptName1, glyphList))

def doSingleTest8():
	global fontlist

	print("\nSingle Face Test 8: Check SubFamily Name (name ID 2) for  Regular Style, Bold Style, Italic Style, and BoldItalic Style")
	if not fontlist[0].compatibleSubFamilyName3:
		print("Skipping test - Windows Unicode SubFamily name does not exist.", fontlist[0].PostScriptName1)
		return
	for font in fontlist:
		if font.isBold == 0 and font.isItalic == 0:
			if font.compatibleSubFamilyName3 != "Regular":	#Unicode String
				print("	Warning: Style bit not set, Compatible SubFamily Name with Win Platform is not 'Regular' for Font", font.PostScriptName3)
		elif font.isBold and font.isItalic == 0:
			if font.compatibleSubFamilyName3 != "Bold":	 #Unicode String
				print("	Warning: Only Bold style bit set, Compatible SubFamily Name with Win Platform is not 'Bold' for Font", font.PostScriptName3)
		elif font.isBold == 0 and font.isItalic:
			if font.compatibleSubFamilyName3 != "Italic":  #Unicode String
				print("	Warning: Only Italic style bit set, Compatible SubFamily Name with Win Platform is not 'Italic' for Font", font.PostScriptName3)
		elif font.isBold and font.isItalic:
			if font.compatibleSubFamilyName3 != "Bold Italic" and font.compatibleSubFamilyName3 != "Bold Italic":
				print("	Warning: Both Bold and Italic style bits set, Compatible SubFamily Name with Win Platform is not 'Bold Italic' for Font", font.PostScriptName3)
				print("It is", font.compatibleSubFamilyName3)

def doSingleTest9():
	global fontlist

	print("\nSingle Face Test 9: Check that no OS/2.usWeightClass is less than 250")
	if fontlist[0].usWeightClass == -1:
		print("Skipping test - OS/2 table does not exist.", fontlist[0].PostScriptName1)
		return

	for font in fontlist:
		if font.usWeightClass < 250:
			print("	Error: OS/2.usWeightClass is", font.usWeightClass, "for Font", font.PostScriptName1, "\nThis may cause the font glyphs to be smear-bolded under Windows 2000.")

def doSingleTest10():
	global fontlist

	print("\nSingle Face Test 10: Check that no Bold Style face has OS/2.usWeightClass of less than 500")
	if fontlist[0].usWeightClass == -1:
		print("Skipping test - OS/2 table does not exist.", fontlist[0].PostScriptName1)
		return
	baseFont = None
	boldFont = None
	for font in fontlist:
		if font.isBold:
			boldFont = font
			if font.usWeightClass < 500:
				print("	Error: OS/2.usWeightClass is", font.usWeightClass, "and Bold bit is set for Font", font.PostScriptName1)

		# warn if bold font has ForceBold set.
		if not font.isTTF: #is CFF
			for fontDict in font.FDArray:
				try:
					if fontDict.Private.ForceBold:
						print("Warning: Font has ForceBold field. This is harmless, but is deprecated.", font.PostScriptName1)
				except KeyError:
					pass
def doSingleTest11():
	global fontlist

	print("\nSingle Face Test 11: Check that BASE table exists, and has reasonable values")
	for font in fontlist:
		try:
			baseTable = font.ttFont['BASE']
		except KeyError:
			print("	Error: font %s does not have a BASE table. This is necessary for users who are editing in a different script than the font is designed for." % (font.PostScriptName1))
			return
		try:
			baseDict = {}
			for axisName in ['HorizAxis', 'VertAxis']:
				axisRecord = eval("baseTable.table.%s" % (axisName))
				if not hasattr(axisRecord, 'BaseTagList'):
					continue
				if not axisRecord.BaseScriptList.BaseScriptCount == len(axisRecord.BaseScriptList.BaseScriptRecord):
					print("	Error: bad BASE table: %s.BaseScriptList.BaseScriptCount is not equal to the number of records in the list %s.BaseScriptList.BaseScriptRecord. %s." % (axisName, axisName, font.PostScriptName1))
					return
				if not axisRecord.BaseTagList.BaseTagCount == len(axisRecord.BaseTagList.BaselineTag):
					print("	Error: bad BASE table: %s.BaseTagList.BaseTagCount is not equal to the number of tags in the list %s.BaseTagList.BaselineTag. %s." % (axisName, axisName, font.PostScriptName1))
					return
				for tag in axisRecord.BaseTagList.BaselineTag:
					if not tag in kKnownBaseTags:
						print("	Error: Base table tag '%s' not in list of registered BASE table tags. %s." % (tag, font.PostScriptName1))
				baseDict[axisName] = {}
				for baseRecord in axisRecord.BaseScriptList.BaseScriptRecord:
					baseValues = baseRecord.BaseScript.BaseValues
					if not baseValues.BaseCoordCount == axisRecord.BaseTagList.BaseTagCount:
						print("	Error: bad BASE table: %s.BaseTagList.BaseTagCount is not equal to %s.BaseScriptList.BaseScriptRecord.BaseScript.BaseValues.BaseCoordCount. %s." % (axisName, axisName, font.PostScriptName1))
						return
					if not (axisRecord.BaseTagList.BaseTagCount == len(axisRecord.BaseTagList.BaselineTag)):
						print("	Error: bad BASE table: number of records in list %s.BaseScriptList.BaseScriptRecord.BaseScript.BaseValues.BaseCoord is not equal to  %s.BaseScriptList.BaseScriptRecord.BaseScript.BaseValues.BaseCoordCount. %s." % (axisName, axisName, font.PostScriptName1))
						return
					defaultIndex = baseValues.DefaultIndex
					coords = {}
					scriptTag = baseRecord.BaseScriptTag
					if not  scriptTag in kKnownScriptTags:
						print("	Error: Base table script tag '%s' not in list of registered script  tags. %s." % (scriptTag, font.PostScriptName1))
					baseDict[axisName][scriptTag] = coords
					for i in range(axisRecord.BaseTagList.BaseTagCount ):
						tag = axisRecord.BaseTagList.BaselineTag[i]
						coord = baseValues.BaseCoord[i].Coordinate
						coords[tag] = coord
						if (defaultIndex == i) and (coord != 0):
							print("\tWarning: default BASE table offset for script '%s', tag '%s' is non-zero (%s) for the default font BASE tag. %s." % (scriptTag, tag, coord, font.PostScriptName1))
						elif tag == 'ideo':
							calculatedValue = -(font.unitsPerEm -  font.sCapHeight)/2
							diff = abs(coord - calculatedValue)
							if diff > gDesignSpaceTolerance:
								print("\tWarning:  In order to center OS/2 Capheight in the ideo em-square, the BASE table offset for script '%s', tag '%s' should be '%s' rather than '%s'. %s." % (scriptTag, tag, calculatedValue, coord, font.PostScriptName1))
							#else:
							#	print("\tGood values calculatedValue %s , coord %s,font.sCapHeight %s " % (calculatedValue, coord, font.sCapHeight))
		except AttributeError as e:
			print("	Error: I don't understand this BASE table: Could be bad BASE table, probably this programis out of date or incomplete. %s." % (font.PostScriptName1))
	return


def doSingleTest12():
	global fontlist

	print("\nSingle Face Test 12: Check that Italic style is set when post table italic angle is non-zero, and that italic angle is reasonable.")
	if fontlist[0].italicAngle == -1:
		print("Skipping test - post table does not exist.", fontlist[0].PostScriptName1)
		return

	for font in fontlist:
		if font.isItalic and (abs(font.italicAngle) <= 4):
			print("\tWarning: Italic style bit is set, but Italic Angle '%s' is a smaller italic angle than -4 for Font %s." % (font.italicAngle, font.PostScriptName1))
		if (font.italicAngle > 0.0):
			print("\tWarning: Italic Angle '%s' is greater than zero. Is it really supposed to be leaning to the left, rather than the right?  %s." % (font.italicAngle, font.PostScriptName1))
	for font in fontlist:
		if (not font.isItalic) and  (abs(font.italicAngle) > 4):
			print("\tWarning: Italic style bit is clear, but Italic Angle '%s' is a larger italic angle than -4 for Font %s." % (font.italicAngle, font.PostScriptName1))

def doSingleTest13():
	global fontlist

	print("\nSingle Face Test 13: Warn if post.isFixedPitch is set when font is not monospaced.")

	if fontlist[0].isFixedPitch == -1:
		print("Skipping test - post table is missing", fontlist[0].PostScriptName1)
		return

	if fontlist[0].isCID :
		print("Skipping test - not useful for CID fonts", fontlist[0].PostScriptName1)
		return

	for font in fontlist:
		if font.PostScriptName1 in kFixedPitchExceptionList:
			continue
		glyphnames = font.glyphnames
		glyphwidths = font.glyph_widths

		widthTable = MakeWidthTable(glyphnames, glyphwidths)
		numWidthEntries = len(glyphwidths)
		numMostPopularWidth = widthTable[-1][0]

		if 'CFF ' in font.ttFont:
			if font.topDict.isFixedPitch != font.isFixedPitch:
				if font.isFixedPitch:
					print("	Error: post table isFixedPitch indicates that the font is monospaced, but the CFF top dict IsFixedPitch is off.")
				else:
					print("	Error: post table isFixedPitch indicates that the font is not monospaced, but the CFF top dict IsFixedPitch is on.")

		if font.isFixedPitch:
			if font.isFixedPitch == -1:
				print("Skipping test for this face- post table is missing", font.PostScriptName1)
				continue

			if len(widthTable) > 1:
				print("	Error: Font is marked as being FIxedPitch in the post table, but not all glyphs are the same width. font", font.PostScriptName1)
				for entry in widthTable[:-1]:
					print("non-std width " + str(entry[1]) + " Number of glyphs " + str(entry[0]))
					for name in entry[2]:
						print("\t\t" + name)

		else:
			if (numWidthEntries * 0.80 < numMostPopularWidth):
				print("	Warning: More than 80% of the glyphs in this font are the same width. Maybe it should be marked as FixedPitch. font", font.PostScriptName1)
				for entry in widthTable[:-1]:
					print("non-std width " + str(entry[1]) + " Number of glyphs " + str(entry[0]))
					for name in entry[2]:
						print("\t\t" + name)

def doSingleTest14():
	global fontlist

	print("\nSingle Face Test 14: Warn if Bold/Italic style bits do not match between the head Table and OS/2 Table")
	if fontlist[0].isItalic2 == -1:
		print("Skipping test - OS/2 table does not exist.", fontlist[0].PostScriptName1)
		return

	for font in fontlist:
		if font.isItalic != font.isItalic2:
			print("	Error: Italic style bit in head.macStyle does not match with OS/2.fsSelection for Font", font.PostScriptName1)
			print("\thead.macStyle:", font.isItalic, "OS/2.fsSelection:",font.isItalic2)
	for font in fontlist:
		if font.isBold != font.isBold2:
			print("	Error: Bold style bit in head.macStyle does not match with OS/2.fsSelection for Font", font.PostScriptName1)
			print("\thead.macStyle:", font.isBold, "OS/2.fsSelection:",font.isBold2)


def doSingleTest15():
	global fontlist

	print("\nSingle Face Test 15: Warn if Font BBox x/y coordinates are improbable, or differ between head table and CFF.")
	for font in fontlist:
		limit1 = 2* font.unitsPerEm
		limit2 = 1.5* font.unitsPerEm
		if font.fontBBox[0] < -limit1 or font.fontBBox[0] > limit1:
			print("	Warning: BBox X-Min '%s' is out of usual range for Font %s." % (font.fontBBox[0], font.PostScriptName1))
		if font.fontBBox[1] < -limit2 or font.fontBBox[1] > limit2:
			print("	Warning: BBox Y-Min '%s' is out of usual range for Font %s." % (font.fontBBox[1], font.PostScriptName1))
		if font.fontBBox[2] < -limit1 or font.fontBBox[2] > limit1:
			print("	Warning: BBox X-Max '%s' is out of usual range for Font %s." % (font.fontBBox[2], font.PostScriptName1))
		if font.fontBBox[3] < -limit2 or font.fontBBox[3] > limit2:
			print("	Warning: BBox Y-Max '%s' is out of usual range for Font %s." % (font.fontBBox[3], font.PostScriptName1))

		if 'CFF ' in font.ttFont:
			headBox =  font.fontBBox
			cffBox = font.topDict.FontBBox
			diff = 0
			for i in [0,1,2,3]:
				if abs(headBox[i] - cffBox[i]) > 1:
					diff = 1
					break
			if diff:
				print("The head table xMin, yMin, xMax, yMax values '%s' differ from the CFF table FontBBox values '%s'. %s." % (headBox, cffBox, font.PostScriptName1))

def doSingleTest16():
	global fontlist

	print("\nSingle Face Test 16: Check values of Ascender and Descender vs em-square.")
	if fontlist[0].sTypoAscender == -1:
		print("Skipping test - OS/2 table does not exist", fontlist[0].PostScriptName1)
		return

	emSquareeDict = {}
	for font in fontlist:
		emList = emSquareeDict.get(font.unitsPerEm, [])
		emList.append(font.PostScriptName1)
		emSquareeDict[font.unitsPerEm] = emList
		os2table = font.ttFont['OS/2']

		diff = os2table.sTypoAscender - os2table.sTypoDescender
		if diff != font.unitsPerEm:
			print("	Warning: Difference of OS/2 sTypoAscender '%s' and sTypoDescender '%s' is not the same as the em-square'%s' for Font %s." % (os2table.sTypoAscender, os2table.sTypoDescender, font.unitsPerEm, font.PostScriptName1))

		# usWinDescent is a positive value when below the baseline; sTypoDescender is a negative value.
		if font.isCID:
			if  os2table.sTypoLineGap != font.unitsPerEm:
				print("	Warning: OS/2 table sTypoLineGap '%s' is not equal to the font em-square '%s' for this CID font %s." %  (os2table.sTypoLineGap, font.unitsPerEm, font.PostScriptName1))

		# compare with hhea values.
		if font.sTypoAscender2 != os2table.sTypoAscender:
			print("	Warning: hhea table ascent field '%s' is not the same as OS/2 table sTypoeAscender '%s'. %s" % (font.sTypoAscender2 , os2table.sTypoAscender, font.PostScriptName1))
		if font.sTypoDescender2 != os2table.sTypoDescender:
			print("	Warning: hhea table descent field '%s' is not the same as OS/2 table sTypoDescender '%s'. %s" % (font.sTypoDescender2 , os2table.sTypoDescender, font.PostScriptName1))
		if font.sTypoLineGap2 != os2table.sTypoLineGap:
			print("	Warning: hhea table lineGap field '%s' is not the same as OS/2 table sTypoLineGap '%s'. %s" % (font.sTypoLineGap2 , os2table.sTypoLineGap, font.PostScriptName1))

		# check the sWin values are same as font Bbox.
		if font.fontBBox[1] != -os2table.usWinDescent:
			print("	Warning: OS/2 table usWinDescent field '%s' is not the same as the font bounding box y min '%s'. %s" % (-os2table.usWinDescent, font.fontBBox[1], font.PostScriptName1))
		if font.fontBBox[3] != os2table.usWinAscent:
			print("	Warning: OS/2 table usWinAscent field '%s' is not the same as the font bounding box y max '%s'. %s" % (os2table.usWinAscent, font.fontBBox[3], font.PostScriptName1))
		# make sure that USE_TYPO_METRICS is on
		if not (font.fsSelection & (1<<7)):
			if os2table.version >= 4:
				print("	Warning. The OS/2 table version 4 fsSelection field bit 7 'USE_TYPO_METRICS' is not turned on. Windows applications will (eventually) use the OS/2 sTypo fields only if this bit is on. %s." % (font.PostScriptName1))
		else:
			if os2table.version < 4:
				print("	Error. The OS/2 table version 4 fsSelection field bit 7 'USE_TYPO_METRICS' is set on, but the OS/2 version '%s' is not 4 or greater.. %s." % (os2table.version, font.PostScriptName1))
	if len(emSquareeDict.keys()) > 1:
		print("	Error: fonts in family have different em-squares! %s." % (font.preferredFamilyName1))
		for item in emSquareeDict.items():
			print("\tEm square: %s. Fonts %s." % (item[0], item[1]))
	if font.unitsPerEm not in [1000, 2048]:
		print("	Warning: Em square '%s' is not either 1000 or 2048. Some programs will behave oddly with this family. %s." % (font.unitsPerEm, font.preferredFamilyName1))

def MakeWidthTable(glyphnames, glyphwidths ):
	widthTableDict = {}
	widthTableList = []

	numWidths = len(glyphwidths)
	for i in range(numWidths):
		width = glyphwidths[i]

		if width in widthTableDict:
			tableRecord = widthTableDict[width]
			#print("tableRecord", tableRecord)
			#print("entry", entry)
			tableRecord[0] = tableRecord[0] + 1
			tableRecord[1].append(glyphnames[i])
		else:
			tableRecord = [ 1 ,[ glyphnames[i]]]
			widthTableDict[width] = tableRecord

	widthTableKeys = widthTableDict.keys()
	for key in widthTableKeys:
		tableRecord = widthTableDict[key]
		widthTableList.append([tableRecord[0], key, tableRecord[1]])

	return sorted(widthTableList)

def doSingleTest17():
	print("\nSingle Face Test 17: Verify that all tabular glyphs have the same width.")
	for font in fontlist:
		if font.PostScriptName1 in kFixedPitchExceptionList:
			continue
		glyphnames = font.glyph_name_list1
		glyphwidths = font.glyph_width_list1

		widthTable = MakeWidthTable(glyphnames, glyphwidths)
		numWidthEntries = len(glyphwidths)
		if numWidthEntries > 9 :
			numMostPopularWidth = widthTable[-1][0]
			if (numWidthEntries * 0.5 < numMostPopularWidth) and (len(widthTable) > 1):
				print("Tabular Width Check: tablining group, font", font.PostScriptName1)
				print("	Error: tablining More than 50% the glyphs are the same width - this set of numbers is meant to be tabular. ",font.PostScriptName1)
				for entry in widthTable[:-1]:
					print("non-std width " + str(entry[1]) + " Number of glyphs " + str(entry[0]))
					for name in entry[2]:
						print("\t\t" + name)

		glyphnames = font.glyph_name_list2
		glyphwidths = font.glyph_width_list2

		widthTable = MakeWidthTable(glyphnames, glyphwidths)
		numWidthEntries = len(glyphwidths)
		if numWidthEntries > 9 :
			numMostPopularWidth = widthTable[-1][0]
			if (numWidthEntries * 0.5 < numMostPopularWidth) and (len(widthTable) > 1):
				print("Tabular Width Check: taboldstyle group, font", font.PostScriptName1)
				print("	Error: taboldstyle More than 50% the glyphs are the same width - this set of numbers is meant to be tabular. ",font.PostScriptName1)
				for entry in widthTable[:-1]:
					print("non-std width " + str(entry[1]) + " Number of glyphs " + str(entry[0]))
					for name in entry[2]:
						print("\t\t" + name)

		glyphnames = font.glyph_name_list3
		glyphwidths = font.glyph_width_list3

		widthTable = MakeWidthTable(glyphnames, glyphwidths)
		numWidthEntries = len(glyphwidths)
		if numWidthEntries > 9 :
			numMostPopularWidth = widthTable[-1][0]
			if (numWidthEntries * 0.5 < numMostPopularWidth) and (len(widthTable) > 1):
				print("Tabular Width Check: tabnumerator group, font", font.PostScriptName1)
				print("	Error: tabnumerator More than 50% the glyphs are the same width - this set of numbers is meant to be tabular. ",font.PostScriptName1)
				for entry in widthTable[:-1]:
					print("non-std width " + str(entry[1]) + " Number of glyphs " + str(entry[0]))
					for name in entry[2]:
						print("\t\t" + name)


hintPat = re.compile(r"(?:(\s-*[0-9.]+)(\s-*[0-9.]+)*)\s([a-z]+)")
def checkHintEntry(entry, missingHintsGlyphs, glyphBox, fontPSName):
	errorCount = 0
	errMsgs = []
	name = entry[0]
	if entry[-1] == "endchar": # non-marking char
		return errorCount

	hintList = hintPat.findall(entry[2])
	if not hintList:
		missingHintsGlyphs.append(name)
		return errorCount

	for lo, high, op in hintList:
		if op[0] in ['h', 't', 'b']:
			min = glyphBox[1]
			max = glyphBox[3]
		else:
			min = glyphBox[0]
			max = glyphBox[2]

		glyphBBoxSize = max-min

		lo = eval(lo)
		if high:
			high = eval(high)
			stemWidth = high-lo
			if stemWidth < 0:
				errMsgs +=  ["Error: hint %s %s %s in glyph %s has a negative stem width. %s." % (lo, high, op, name, fontPSName)]
			if abs(stemWidth) == 1978:
				errMsgs += ["Error: hint %s %s %s in glyph %s makevrt2 bug. %s." % (lo, high, op, name, fontPSName)]
			elif abs(stemWidth) > glyphBBoxSize:
				errMsgs += ["Error: hint %s %s %s in glyph %s has a bogus stem width, as it is greater than the glyph bbox size %s. %s." % (lo, high, op, name, glyphBBoxSize, fontPSName)]
			if high > max:
				errMsgs += ["Error: hint %s %s %s in glyph %s has a high value greater than the high value (%s) of the glyph bbox. %s." % (lo, high, op, name, max, fontPSName)]
			if lo < min:
				errMsgs += ["Error: hint %s %s %s in glyph %s has a low value less than the low value (%s) of the glyph bbox. %s." % (lo, high, op, name, min, fontPSName)]
		else:
			# is a ghost hint - there is only one value.
			if lo > max:
				errMsgs += ["Error: edge hint %s %s in glyph %s has a value greater than the high value of the glyph bbox (%s). %s." % (lo, op, name, max, fontPSName)]
			elif lo < min:
				errMsgs += ["Error: edge hint %s %s in glyph %s has a value less than the low value of the glyph bbox (%s). %s." % (lo, op, name, min, fontPSName)]
	if errMsgs:
		for msg in errMsgs:
			print(msg)
	errorCount = len(errMsgs)
	return errorCount


def doSingleTest18():
	print("\nSingle Face Test 18: Hint Check.  Verify that there is at least one hint for each charstring in each font, and that no charstring is > 32K limit for Mac OSX 10.3.x and earlier.")
	recursionLevel = 0
	txDumpMathPat = re.compile(r"glyph\[\d+\] \{([^,]+),[^\s]+\s+(\S+)\s+width(.+?)(move|endchar)", re.DOTALL)
	for cmpfFont in fontlist:

		# Get glyph name list.
		if 'CFF ' not in cmpfFont.ttFont:
			print("Skipping test - can't check hints in TrueType font.", cmpfFont.PostScriptName1)
			break

		glyphnames = cmpfFont.glyphnames
		charStrings = cmpfFont.topDict.CharStrings
		char_list = charStrings.keys()
		missingHintsGlyphs = []
		count = 0
		debugKeys = [] # the list of keys for which to print out all the parsing data.
		for key in char_list:
			count = count + 1

			if debugKeys and (key > debugKeys[-1]):
				sys.exit(1)

			if debugKeys and (key not in debugKeys):
				continue

			recursionLevel = 0
			t2CharString = charStrings[key]
			if not t2CharString.bytecode:
				t2CharString,compile()
			if len(t2CharString.bytecode) > 32767:
				print("Error: charstring for glyph %s is greater than 32k bytes. this can cause problems in Mac OSX 1.3.x and earlier." % (key))

		# Now check the hints.
		if not doStemWidthChecks:
			return

		command = "tx -dump -6 \"%s\" 2>&1" % (cmpfFont.path)
		report = fdkutils.runShellCmd(command)
		if not re.search(r"## glyph", report[:1000]):
			print("Error: could not check hinting for %s. 'tx' command found some error. See following log." % (cmpfFont.PostScriptName1))
			print(report)
			return
		hintList = txDumpMathPat.findall(report)
		errorCount = 0
		for entry in hintList:
			glyphBox = cmpfFont.metricsDict[entry[0]][-4:]
			errorCount += checkHintEntry(entry, missingHintsGlyphs, glyphBox, cmpfFont.PostScriptName1)
			if errorCount > 40:
				print("Ending processing of hint errors - too many to enumerate.")
				break

		if missingHintsGlyphs:
			print("\nError: No hint found in the following glyphs for font:", cmpfFont.PostScriptName1)
			print("Missing hints:", missingHintsGlyphs)

def doSingleTest19():
	global fontlist

	print("\nSingle Face Test 19: Warn if the Unicode cmap table does not exist, or there are double mapped glyphs in the Unicode cmap table")
	for cmpfFont in fontlist:
		unicodeCmapList = []
		if 'cmap' in cmpfFont.ttFont:
			cmap_table = cmpfFont.ttFont['cmap']
			platformID = 0
			platEncID = 3
			cmapUnicodeTable = cmap_table.getcmap(platformID, platEncID)
			if	not cmapUnicodeTable:
				platformID = 3
				platEncID = 1
				cmapUnicodeTable = cmap_table.getcmap(platformID, platEncID)

			if cmapUnicodeTable:
				unicodeCmapList = cmapUnicodeTable.cmap.items()

		else:
			print("	Error: font has no cmap table: ", cmpfFont.PostScriptName1)
			continue

		if not unicodeCmapList:
			print("	Error: font has no Unicode cmap table  (platform, encoding) (0,3) or (3,1) : ", cmpfFont.PostScriptName1)
			continue

		if cmpfFont.isCID:
			# " double-mapped test - CID fonts use  a lot of double-mapping."
			continue

		from fontTools.unicode import Unicode
		nameDict = {}
		duplicateList = []
		for code, glyphName in unicodeCmapList:
			if glyphName in nameDict:
				code2 = nameDict[glyphName]
				unicodeComment1 = Unicode[code]
				unicodeComment2 = Unicode[code2]
				message = "\tGlyph " +str(glyphName) + " is double-mapped to both unicode " + str(hex(code)) + " " + unicodeComment1 + " and " + str(hex(code2))  + " " + unicodeComment2 + "."
				duplicateList.append(message)
			else:
				nameDict[glyphName] = code
		cmpfFont.uniNameDict = nameDict
		if duplicateList:
			print("	Warning: font %s has glyphs mapped to more than one Unicode value. The OTF spec permits this, but it makes it hard for Acrobat to reverse map from Unicode encodings to char codes for searching." % (cmpfFont.PostScriptName1))
			for message in duplicateList:
				print(message)


def doSingleTest20():
	global fontlist

	print("\nSingle Face Test 20: Warn if there are double spaces in the name table font menu names.")

	reporter1_fontname = []
	reporter1_failedvalue = []
	reporter2_fontname = []
	reporter2_failedvalue = []
	reporter3_fontname = []
	reporter3_failedvalue = []
	reporter4_fontname = []
	reporter4_failedvalue = []
	reporter5_fontname = []
	reporter5_failedvalue = []
	reporter6_fontname = []
	reporter6_failedvalue = []

	for font in fontlist:
		if (font.compatibleFamilyName3.find("  ") != -1):
			reporter1_fontname.append(font.PostScriptName1)
			reporter1_failedvalue.append(font.compatibleFamilyName3)
		if (font.compatibleSubFamilyName3.find("  ") != -1):
			reporter2_fontname.append(font.PostScriptName3)
			reporter2_failedvalue.append(font.compatibleSubFamilyName3)
		if (font.FullFontName1.find("  ") != -1):
			reporter3_fontname.append(font.PostScriptName1)
			reporter3_failedvalue.append(font.FullFontName1)
		if	font.preferredFamilyName3 and (font.preferredFamilyName3.find("  ") != -1):
				if (font.preferredFamilyName3.find("  ") != -1):
					reporter4_fontname.append(font.PostScriptName1)
					reporter4_failedvalue.append(font.preferredFamilyName3)
		if font.compatibleSubFamilyName3 and (font.compatibleSubFamilyName3.find("  ") != -1):
			reporter5_fontname.append(font.PostScriptName1)
			reporter5_failedvalue.append(font.compatibleSubFamilyName3)
		if font.MacCompatibleFullName1 and (font.MacCompatibleFullName1.find("  ") != -1):
			reporter6_fontname.append(font.PostScriptName1)
			reporter6_failedvalue.append(font.MacCompatibleFullName1)

	for i in range(len(reporter1_fontname)):
		print("	Error: Name ID 1, Font compatible family name <" +\
		str(reporter1_failedvalue[i]) + "> has double spaces in name. ", reporter1_fontname[i])
	for i in range(len(reporter2_fontname)):
		print("	Error: Name ID 2, Font compatible subfamily name <" +\
		str(reporter2_failedvalue[i]) + "> has double spaces in name. ", reporter2_fontname[i])
	for i in range(len(reporter3_fontname)):
		print("	Error: Name ID 4, full font name <" +\
		str(reporter3_failedvalue[i]) + "> has double spaces in name. ", reporter3_fontname[i])
	for i in range(len(reporter4_fontname)):
		print("	Error: Name ID 16, preferred family name <" +\
		str(reporter4_failedvalue[i]) + "> has double spaces in name. ", reporter4_fontname[i])
	for i in range(len(reporter5_fontname)):
		print("	Error: Name ID 17, preferred subfamily name <" +\
		str(reporter5_failedvalue[i]) + "> has double spaces in name. ", reporter5_fontname[i])
	for i in range(len(reporter6_fontname)):
		print("	Error: Name ID 18, Mac-compatible full name <" +\
		str(reporter6_failedvalue[i]) + "> has double spaces in name. ", reporter6_fontname[i])



def doSingleTest21():
	global fontlist

	print("\nSingle Face Test 21: Warn if there trailing or leading spaces in the name table font menu names.")

	reporter1_fontname = []
	reporter1_failedvalue = []
	reporter2_fontname = []
	reporter2_failedvalue = []
	reporter3_fontname = []
	reporter3_failedvalue = []
	reporter4_fontname = []
	reporter4_failedvalue = []
	reporter5_fontname = []
	reporter5_failedvalue = []
	reporter6_fontname = []
	reporter6_failedvalue = []

	for font in fontlist:
		if (len(font.compatibleFamilyName3.strip()) != len(font.compatibleFamilyName3)):
			reporter1_fontname.append(font.PostScriptName1)
			reporter1_failedvalue.append(font.compatibleFamilyName3)
		if (len(font.compatibleSubFamilyName3.strip()) != len(font.compatibleSubFamilyName3)):
			reporter2_fontname.append(font.PostScriptName1)
			reporter2_failedvalue.append(font.compatibleSubFamilyName3)
		if (len(font.FullFontName1.strip()) != len(font.FullFontName1)):
			reporter3_fontname.append(font.PostScriptName1)
			reporter3_failedvalue.append(font.FullFontName1)
		if font.preferredFamilyName3 and (len(font.preferredFamilyName3.strip()) != len(font.preferredFamilyName3)):
			reporter4_fontname.append(font.PostScriptName1)
			reporter4_failedvalue.append(font.preferredFamilyName3)
		if font.preferredSubFamilyName3 and (len(font.preferredSubFamilyName3.strip()) != len(font.preferredSubFamilyName3)):
			reporter5_fontname.append(font.PostScriptName1)
			reporter5_failedvalue.append(font.preferredSubFamilyName3)
		if font.MacCompatibleFullName1 and (len(font.MacCompatibleFullName1.strip()) != len(font.MacCompatibleFullName1)):
			reporter6_fontname.append(font.PostScriptName1)
			reporter6_failedvalue.append(font.MacCompatibleFullName1)

	for i in range(len(reporter1_fontname)):
		print("	Error: Name ID 1, Font compatible family name <" +\
		str(reporter1_failedvalue[i]) +	 "> has trailing or leading spaces in name. ", reporter1_fontname[i])
	for i in range(len(reporter2_fontname)):
		print("	Error: Name ID 2, Font compatible subfamily name <"+\
		str(reporter2_failedvalue[i]) +	 "> has trailing or leading spaces in name. ", reporter2_fontname[i])
	for i in range(len(reporter3_fontname)):
		print("	Error: Name ID 4, full font name <"+\
		str(reporter3_failedvalue[i]) +	 "> has trailing or leading spaces in name. ", reporter3_fontname[i])
	for i in range(len(reporter4_fontname)):
		print("	Error: Name ID 16, preferred family name <"+\
		str(reporter4_failedvalue[i]) +	 "> has trailing or leading spaces in name. ", reporter4_fontname[i])
	for i in range(len(reporter5_fontname)):
		print("	Error: Name ID 17, preferred subfamily name <"+\
		str(reporter5_failedvalue[i]) +	 "> has trailing or leading spaces in name. ", reporter5_fontname[i])
	for i in range(len(reporter6_fontname)):
		print("	Error: Name ID 18, Mac-compatible full name <"+\
		str(reporter6_failedvalue[i]) +	 "> has trailing or leading spaces in name. ", reporter6_fontname[i])


#ae oe ct ff fi ffi fl ffl ij AE OE IJ CT ST TH TT Th  UE ue fk ft gg sl  NJ DZ Lj Nj Dz lj nj dz sp st tt zy
oldLigatureNames = {
		"ae" : "a",
		"AE" : "A",
		"ct" : "c",
		"CT" : "C",
		"dz" : "d",
		"Dz" : "D",
		"DZ" : "D",
		"ff" : "f",
		"ffi" : "f",
		"ffl" : "f",
		"fi" : "f",
		"fk" : "f",
		"fl" : "f",
		"ft" : "f",
		"gg" : "g",
		"ij" : "i",
		"IJ" : "I",
		"lj" : "l",
		"Lj" : "L",
		"nj" : "n",
		"Nj" : "N",
		"NJ" : "N",
		"oe" : "o",
		"OE" : "O",
		"sl" : "s",
		"sp" : "s",
		"st" : "s",
		"ST" : "S",
		"Th" : "T",
		"TH" : "T",
		"tt" : "t",
		"TT" : "T",
		"ue" : "u",
		"UE" : "U",
		"zy" : "z",
		}

NAMES_OF_ACCENTS = set([
			"accent",
			"acute",
			"acutecomb",
			"bar",
			"breve",
			"caron",
			"cedilla",
			"comb",
			"circumflex",
			"croat",
			"dieresis",
			"dot",
			"dotaccent",
			"grave",
			"gravecomb",
			"hook",
			"hookabove",
			"hookabovecomb",
			"hungarumlaut",
			"macron",
			"ogonek",
			"ring",
			"ringacute",
			"slash",
			"slashacute",
			"tilde",
			"tildecomb",
			"uni0302",
			"uni0313",
			"umlaut",
			])

uniLigPattern = re.compile(r"^uni([0-8A-Fa-f][0-8A-Fa-f][0-8A-Fa-f][0-8A-Fa-f])([0-8A-Fa-f][0-8A-Fa-f][0-8A-Fa-f][0-8A-Fa-f])")

def getLigatureName(font, name):
	entry = None
	altBaseName = None # alternate form to try if first is not in font.
	ligCompName = None # last component of ligature; used only to check if it is an accent.
	try:
		entry = [name, oldLigatureNames[name], altBaseName, None]
	except KeyError:
		pass

	if entry is None:
		baseName = None

		# get extension
		try:
			simpleName, ext = name.split(".", 1)
		except ValueError:
			ext = None
			simpleName = name

		if "_" in simpleName:
			nameList = simpleName.split("_")
			baseName = nameList[0]
			ligCompName = nameList[-1] # just keep the first ligature component.
		elif name.startswith("uni"):
			ligmatch = uniLigPattern.search(simpleName)
			if ligmatch:
				baseName = ligmatch.group(1)
				ligCompName = ligmatch.groups()[-1]
		if baseName:
			if baseName in ["space"]:
				entry = None
				return

			if ext:
				try:
					suffixClass = suffixDict[ext]
				except KeyError:
					suffixClass = None

				if suffixClass == kAddSuffixToBoth:
					altBaseName = baseName
					baseName = baseName +  "." + ext
					altligCompName = ligCompName+  "." + ext
					if altligCompName in font.metricsDict:
						ligCompName = altligCompName
				elif suffixClass == kAddSuffixToEnd:
					altligCompName = ligCompName+  "." + ext
					if altligCompName in font.metricsDict:
						ligCompName = altligCompName
					else:
						altBaseName = baseName
						baseName = baseName +  "." + ext
				elif suffixClass == kAddSuffixToFirst:
					altBaseName = baseName
					baseName = baseName +  "." + ext
				else:
					altBaseName = baseName
					baseName = baseName +  "." + ext
					if baseName not in font.metricsDict:
						baseName = altBaseName
						altBaseName = None
						altligCompName = ligCompName+  "." + ext
						if altligCompName in font.metricsDict:
							ligCompName = altligCompName

			entry = [name, baseName, altBaseName, ligCompName]
	return entry


def getLigatureNames(font):
	# Find lligatures. Look for old names, fi, fl ,ffi,ffl, and any names with underscores.
	nameList = font.ttFont.getGlyphOrder()
	ligatureList = []
	for name in nameList:
		ligatureList.append(getLigatureName(font, name))
	ligatureList = filter(None, ligatureList) # get rid of all empty entries.
	return ligatureList

def isAccent(glyphName):
	if glyphName in accentNames:
		return 1
	if glyphName.startswith("uni0"):
		return (glyphName >= "uni0300") and (glyphName <= "uni034F")
	if glyphName.startswith("u0"):
		return (glyphName >= "u0300") and (glyphName <= "u034F")

def doSingleTest22():
	global fontlist

	print("\nSingle Face Test 22: Warn if any ligatures have a width which not larger than the width of the first glyph, or, if first glyph is not in font, if the RSB is negative.")
	for font in fontlist:
		# Ligature entry = [ligature name, base name, alternate base name, last ligature component].  Base glyph name may be None, if I couldn't find it
		ligatureNames = font.ligDict.keys()
		if not ligatureNames:
			return

		for ligName  in ligatureNames:
			ligComponentsList =  font.ligDict[ligName]
			for ligComponents in ligComponentsList:
				try:
					widthLig, lsbLig, bottomLig, rightLig, topLig  = font.metricsDict[ligName]
				except KeyError:
					print("Skipping ligature glyph '%s'; not found in metrics dict." % (ligName))
					continue
				rsbLig = widthLig - rightLig

				leftComp = ligComponents[0]
				try:
					widthLeftComp, lsbLeftComp, bottomLeftComp, rightLeftComp, topLeftCompp  = font.metricsDict[leftComp]
				except KeyError:
					print("Skipping ligature left component glyph '%s'; not found in metrics dict." % (leftComp))
					continue

				rightComp = ligComponents[-1]
				try:
					widthRightComp, lsbRightComp, bottomRightComp, rightRightComp, topRightComp  = font.metricsDict[rightComp]
				except KeyError:
					print("Skipping ligature right component glyph  '%s'; not found in metrics dict." % (rightComp))
					continue

				if widthLig <=  (widthLeftComp - gDesignSpaceTolerance):
					print("	Warning: Width of ligature %s: %s is less than or the same as that of the left component glyph %s: %s for font %s." % (ligName, widthLig, leftComp, widthLeftComp, font.PostScriptName1))

				diff = abs(lsbLig - lsbLeftComp)
				if diff > gDesignSpaceTolerance:
					print("	Warning: left side bearing %s of ligature %s is not equal to the lsb %s of the left side component %s for font %s." % (lsbLig, ligName, lsbLeftComp, leftComp, font.PostScriptName1))
				rsbRightComp = widthRightComp - rightRightComp
				diff = abs(rsbRightComp - rsbLig)
				if diff > gDesignSpaceTolerance:
					print("	Warning: right side bearing %s of ligature %s is not equal to the rsb %s of the last component %s for font %s." % (rsbLig, ligName, rsbRightComp, rightComp, font.PostScriptName1))

def  getAcentEntries(font):
	# Simply delete all the sub-strings in the accentedNames list from the glyph name. If the result differs from the original, store it.
	# We don't know yet if the baseName glyph exists.
	accentNames = []
	nameList = font.ttFont.getGlyphOrder()
	for name in nameList:
		baseName = name
		for subName in sorted(NAMES_OF_ACCENTS):
			if baseName == subName:
				break
			baseName = re.sub(subName, "", baseName)
		if baseName == name:
			continue
		if baseName[-1] == ".": # happens in cases like one.inferior
			baseName = baseName[:-1]
		accentNames.append([name, baseName])
	return accentNames

def doSingleTest23():
	global fontlist

	print("\nSingle Face Test 23: Warn if any accented glyphs have a width different than the base glyph.")
	for font in fontlist:
		accentEntries = getAcentEntries(font) # get a list of entries [ accented glyph-name, base-glyph name]. base glyph name may be None, if I couldn't find it
		hmtxTable = font.ttFont['hmtx']

		for entry  in accentEntries:
			try:
				baseName = entry[1]
				accentName = entry[0]
				widthbase, lsbBase = hmtxTable[baseName] # I do this first so as to not waste time if the base glyph does not exist.
				widthAccent, lsbLig = hmtxTable[accentName]
				diff = abs(widthAccent - widthbase)
				if diff > gDesignSpaceTolerance:
					print("\tWarning: Width of glyph %s: %s differs from that of the base glyph %s: %s for font %s." % (accentName, widthAccent, baseName, widthbase, font.PostScriptName1))
			except KeyError:
				pass

def doSingleTest24():
	global fontlist
	print("\nSingle Face Test 24: Warn if font has 'size' feature, and design size is not in specified range.")
	for font in fontlist:
		font.sizeDesignSize = 0
		font.sizeSubfFamilyID = 0
		font.sizeNameID = 0
		font.sizeRangeStart = 0
		font.sizeRangeEnd = 0
		font.sizeMenuName = "NoSizefeature"

		command = "spot -t GPOS \"%s\" 2>&1" % (font.path)
		report = fdkutils.runShellCmd(command)
		if "size" in report:
			match = re.search(r"Design Size:\s*(\d+).+?Subfamily Identifier:\s*(\d+).+name table [^:]+:\s*(\d+).+Range Start:\s*(\d+).+?Range End:\s*(\d+)", report, re.DOTALL)
			if match:
				font.sizeDesignSize = designSize = eval(match.group(1))
				font.sizeSubfFamilyID = subfFamilyID = eval(match.group(2))
				font.sizeNameID = sizeNameID = eval(match.group(3))
				font.sizeRangeStart = rangeStart = eval(match.group(4))
				font.sizeRangeEnd = rangeEnd = eval(match.group(5))
				if font.sizeNameID:
					# only check range if there is one.
					if (designSize < rangeStart) or (designSize > rangeEnd):
						print("\tError: 'size' feature design size %s is not within design range %s - %s for font %s." % (designSize, rangeStart, rangeEnd, font.PostScriptName1))
				font.sizeMenuName = ""
				for keyTuple in font.nameIDDict.keys():
					if keyTuple[3] == font.sizeNameID:
						font.sizeMenuName = font.nameIDDict[keyTuple]
						break
				if not font.sizeMenuName:
					print("	Error: the 'size' feature refers to name table name ID '%s', but I can't find it. %s." % (font.sizeNameID, font.PostScriptName1))
			else:
				print("Program error: font has 'size' feature, but could not parse the values for font %s." % ( font.PostScriptName1))


def doSingleTest25():
	global fontlist

	print("\nSingle Face Test 25: Check that fonts do not have UniqueID, UID, or XUID in CFF table.")

	if 'CFF ' not in fontlist[0].ttFont:
		print("Skipping test - applies only to CFF fonts.")
		return
	if fontlist[0].isCID:
		print("Skipping test - applies only to name-keyed CFF fonts.")
		return

	hasError = 0
	for font in fontlist:
		if hasattr(font.topDict, "UniqueID"):
			print("	Error: Font has UniqueID in CFF table.", font.PostScriptName1)
			hasError = 1
		if hasattr(font.topDict, "XUID"):
			print("	Error: Font has XUID in CFF table.", font.PostScriptName1)
			hasError = 1
		if hasattr(font.topDict, "UIDBase"):
			print("	Error: Font has UIDBase in CFF table.", font.PostScriptName1)
			hasError = 1
	if hasError:
		print(""" Use of UniqueID/XUID is still syntactically correct. However, end=''
		the advantage in caching the font in printers for fonts smaller than  1
		Mbyte is small, and and the risk of conflicting with another font that
		has an arbitrarily assigned UniqueID value is significant. The Adobe
		Type Dept has stopped using them for non-CJK fonts.""")

def doSingleTest26():
	global fontlist

	print("\nSingle Face Test 26: Glyph name checks.")

	matchPats = [ re.compile(r"^uni([0-9A-F][0-9A-F][0-9A-F][0-9A-F])+$"), re.compile(r"^u[0-9A-F][0-9A-F][0-9A-F][0-9A-F]([0-9A-F]*[0-9A-F]*)$")]

	for font in fontlist:
		if font.usDefaultChar == None:
			break
		if font.usDefaultChar!= 0:
			print("	Warning: the OS/2 default char is set to char code '%s'; it is most usefully set to 0, aka .notdef. %s" % (font.usDefaultChar, font.PostScriptName1))


	for font in fontlist:
		if 'CFF ' not in font.ttFont:
			glyphnames = font.glyphnames
			notdefGlyphName = glyphnames[0]
			nullGlyphName = glyphnames[1]
			crGlyphName = glyphnames[2]
			spaceGlyphName = glyphnames[3]
			glyphnames = font.glyphnames[4:]

			if  notdefGlyphName !=  ".notdef":
				print("	Error: In TrueType fonts, glyphID 0 should be .notdef rather than '%s'. %s." % (notdefGlyphName, font.PostScriptName1))
				notdefGlyphName = None

			if  nullGlyphName !=  "NULL":
				if nullGlyphName ==  ".null":
					print("	Warning: glyph ID 1 is named '.null'; should be 'NULL'", font.PostScriptName1)
				else:
					print("	Error: In TrueType fonts, glyphID 1 should be NULL rather than '%s'. %s." % (nullGlyphName, font.PostScriptName1))
					nullGlyphName = None

			if  crGlyphName !=  "CR":
				if crGlyphName ==  "nonmarkingreturn":
					print("	Warning: glyph ID 2 is named 'nonmarkingreturn'; should be 'CR'", font.PostScriptName1)
				else:
					print("	Error: In TrueType fonts, glyphID 2 should be 'CR' rather than '%s'. %s." % (crGlyphName, font.PostScriptName1))
					crGlyphName = None

			if font.isCID:
				continue

			if  spaceGlyphName !=  "space":
				print("	Error: In TrueType fonts, glyphID 3 should be 'space' rather than '%s'. %s." % (spaceGlyphName, font.PostScriptName1))
				spaceGlyphName = None

			if hasattr(font, "uniNameDict"):
				if nullGlyphName:
					try:
						code = font.uniNameDict[nullGlyphName]
						if code != 0:
							print("	Error: TrueType font should have NULL glyph '%s' encoded at Unicode value 0 rather than %s. %s" % (nullGlyphName, hex(code), font.PostScriptName1))
					except KeyError:
						print("	Error: TrueType font should have NULL glyph '%s' encoded at Unicode value 0. %s" % (nullGlyphName, font.PostScriptName1))
				if crGlyphName:
					try:
						code = font.uniNameDict[crGlyphName]
						if code != 0xD:
							print("	Error: TrueType font should have CR glyph '%s' encoded at Unicode value 0xD rather than %s. %s" % (crGlyphName, hex(code), font.PostScriptName1))
					except KeyError:
						print("	Error: TrueType font should have CR glyph '%s' encoded at Unicode value 0xD. %s" % (crGlyphName, font.PostScriptName1))
				if spaceGlyphName:
					try:
						code = font.uniNameDict[spaceGlyphName]
						if code != 0x20:
							print("	Error: TrueType font should have CR glyph '%s' encoded at Unicode value 0x20 rather than %s. %s" % (spaceGlyphName, hex(code), font.PostScriptName1))
					except KeyError:
						print("	Error: TrueType font should have CR glyph '%s' encoded at Unicode value 0x20. %s" % (spaceGlyphName, font.PostScriptName1))

	# Now to check if all names or AGL compliant.
	for font in fontlist:
		if font.isCID:
				continue

		if 'CFF ' in font.ttFont:
			# Check that the total size of glyph names is less than the limit or Mac OSX 10.4.x and earlier.
			gblock = "  ".join(font.glyphnames) + "  "
			glyphNameBufferSize = len(gblock)
			if glyphNameBufferSize > 32767:
				print("\tError: The total size of the glyph names is greater than the safe limit of 32767 for Mac OSX 10.4.x and earlier by %s characters, and will cause crashes. %s." % (glyphNameBufferSize - 32767, font.PostScriptName1))
			elif ((2*len(font.glyphnames)) + glyphNameBufferSize) > 32767:
				print("\tWarning: The total size of the glyph names is getting close to  than the safe limit of 32767 for Mac OSX 10.4.x..%s\tThe safety margin is  %s bytes. If this margin goes below zero, the font will cause crashes. %s" % (os.linesep, 32767 - glyphNameBufferSize, font.PostScriptName1))

		glyphnames = font.glyphnames
		htmx_table = font.ttFont['hmtx']

		for gname in glyphnames:
			enWidth = font.unitsPerEm/2
			if gname in ["endash ", "onedotenleader", "twodotenleader", "threedotenleader"]:
				gwidth = htmx_table.metrics[gname][0]
				if gwidth != enWidth:
					print("\tError: glyph %s has a width of %s rather than 1/2 the em-square size. %s" % (gname, gwidth, font.PostScriptName1))
				continue
			if gname == "emdash":
				gwidth = htmx_table.metrics[gname][0]
				if gwidth != font.unitsPerEm:
					print("\tError: glyph %s has a width of %s rather than the em-square size. %s" % (gname, gwidth, font.PostScriptName1))
				continue
			if gname == "figurespace":
				gwidth = htmx_table.metrics[gname][0]
				for tname in ["one.tab", "one.taboldstyle"]:
					try:
						twidth = htmx_table.metrics[tname][0]
						if twidth !=  gwidth:
							print("\tError: Width (%s) of %s is not the same as the width (%s) of the tabular glyph %s. %s" % (gwidth, gname, twidth, tname, font.PostScriptName1))
						break
					except KeyError:
						pass
			if gname[0] != ".":
				baseName = gname.split(".")[0]
				ligatureParts = baseName.split("_")
				isLig = len(ligatureParts) > 1
				for partName in ligatureParts:
					dictGlyph = gAGDDict.glyph(partName)
					if  dictGlyph:
						if hasattr(dictGlyph, "fin") and dictGlyph.fin and (dictGlyph.fin != partName):
							# complain if glyph is an AGD source name
							if isLig:
								print("	Warning. Font uses working name '%s' rather than Adobe final name '%s' in ligature name '%s'. %s" % (partName, dictGlyph.fin, gname, font.PostScriptName1))
							elif (not dictGlyph.fin in glyphnames):
								# but only if there is not another glyph with the same final name in the font. Some glypsh are duplicated under "old" and "new" names for legacy issues.
								# This is becuase underMacmac OSX, glyphs are still encoded based on their glyph name in some cases, so changing Delta to uni0394 is a bad idea.
								print("	Warning. Font uses working name '%s' rather than Adobe final name '%s'. %s" % (partName, dictGlyph.fin, font.PostScriptName1))
						continue

					# Not in AGD. Check if is an alternate name, and the alternate is in AGD.
					nameParts = partName.split(".", 1)
					baseName = nameParts[0]
					if len(nameParts) > 1:
						dictGlyph = gAGDDict.glyph(baseName)
						if  dictGlyph:
							if hasattr(dictGlyph, "fin") and dictGlyph.fin and (dictGlyph.fin != baseName):
								if isLig:
									print("	Warning. Font uses working name '%s' rather than Adobe final name '%s' in ligature name '%s'. %s" % (partName, dictGlyph.fin + "." + nameParts[1], gname, font.PostScriptName1))
								else:
									print("	Warning. Font uses working name '%s' rather than Adobe final name '%s'. %s" % (partName, dictGlyph.fin + "." + nameParts[1], font.PostScriptName1))
							continue
					# baseName is not in AGD either. Check if it is a valid uni name.
					isUniName = 0
					for pat in matchPats:
						if pat.match(baseName):
							isUniName = 1
							break
					if not isUniName:
						if isLig:
							print("	Warning. Glyph name '%s' in ligature name '%s' is neither in the AGD, nor a 'uni' name, and can't be mapped to a Unicode value. %s" % (partName, gname, font.PostScriptName1))
						else:
							print("	Warning. Glyph name '%s' is neither in the AGD, nor a 'uni' name, and can't be mapped to a Unicode value. %s" % (partName, font.PostScriptName1))


def doSingleTest27():
	global fontlist

	print("\nSingle Face Test 27: Check strikeout/subscript/superscript positions.")
	for font in fontlist:
		# Heurisitcs are from makeotf hotconv/source/hot.c::prepWinData()
		yStrikeoutSize = font.underlineThickness
		if font.sxHeight > 0:
			yStrikeoutPosition = int(0.5 + font.sxHeight*0.6)
		else:
			yStrikeoutPosition = int(0.5 + font.unitsPerEm * 0.22)

		ySubscriptXSize = int(0.5 + font.unitsPerEm*0.65)
		ySubscriptYSize = int(0.5 + font.unitsPerEm*0.60)
		ySubscriptYOffset = int(0.5 + font.unitsPerEm*0.075)

		ySuperscriptXSize = ySubscriptXSize
		ySuperscriptYSize = ySubscriptYSize
		ySuperscriptYOffset = int(0.5 + font.unitsPerEm*0.35)

		if font.italicAngle == 0:
			ySubscriptXOffset = 0
			ySuperscriptXOffset = 0
		else:
			tanv =  math.tan(math.radians(-font.italicAngle))
			ySubscriptXOffset = -int(0.5 + ySubscriptYOffset *tanv)
			ySuperscriptXOffset = int(0.5 + ySuperscriptYOffset *tanv)
		for fieldName in ["yStrikeoutSize","yStrikeoutPosition",
						"ySubscriptXSize","ySubscriptYSize","ySubscriptXOffset","ySubscriptYOffset",
						"ySuperscriptXSize","ySuperscriptYSize","ySuperscriptXOffset","ySuperscriptYOffset"]:
			diff = abs(eval("font.%s - %s" % (fieldName, fieldName)))
			if diff > 5:
				print("\tWarning: OS/2 table field %s of %s differs from default calculated value %s. Please check. %s." % (fieldName, eval("font.%s" % (fieldName)),eval(fieldName), font.PostScriptName1))


def doSingleTest28():
	global fontlist

	# Copied from makeotf.
	codePageTable = [{  "bitNum": 0, "uv1": 0x00EA, "gname1": "ecircumflex",     "uv2": None, "gname2": None,       "isSupported": -1, "script": "latn_"}, #1252 Latin 1 (ANSI)     Roman
		{ "bitNum": 1, "uv1": 0x0148, "gname1": "ncaron", "uv2": 0x010C, "gname2": "Ccaron", "isSupported": -1, "script": "latn_" }, #  1250 Latin 2 (CE)       CE
		{ "bitNum": 2, "uv1": 0x0451, "gname1": "afii10071", "uv2": 0x042F, "gname2": "afii10049", "isSupported": -1, "script": "cyrl_" }, #  1251 Cyrillic (Slavic)  9.0 Cyrillic
		{ "bitNum": 3, "uv1": 0x03CB, "gname1": "upsilondieresis", "uv2": None, "gname2": None, "isSupported": -1, "script": "grek_" }, #  1253 Greek              Greek
		{ "bitNum": 4, "uv1": 0x0130, "gname1": "Idotaccent", "uv2": None, "gname2": None, "isSupported": -1, "script": "latn_" }, #  1254 Latin 5 (Turkish)  Turkish
		{ "bitNum": 5, "uv1": 0x05D0, "gname1": "afii57664", "uv2": None, "gname2": None, "isSupported": -1, "script": "hebr_" }, #  1255 Hebrew             Hebrew
		{ "bitNum": 6, "uv1": 0x0622, "gname1": "afii57410", "uv2": None, "gname2": None, "isSupported": -1, "script": "arab_" }, #  1256 Arabic             Arabic
		{ "bitNum": 7, "uv1": 0x0173, "gname1": "uogonek", "uv2": None, "gname2": None, "isSupported": -1, "script": "latn_" }, #  1257 Baltic Rim         -
		{ "bitNum": 8, "uv1": 0x20AB, "gname1": "dong", "uv2": 0x01B0, "gname2": None, "isSupported": -1, "script": "latn_" }, #  1258 Vietnamese         -
		{ "bitNum": 16, "uv1": 0x0E01, "gname1": None, "uv2": None, "gname2": None, "isSupported": -1, "script": "thai_" }, #  874 Thai                Thai
		{ "bitNum": 29, "uv1": 0xFB02, "gname1": "fl", "uv2": 0x0131, "gname2": "dotlessi", "isSupported": -1, "script": "latn_" }, #  1252 Latin 1 (ANSI)     Roman
		{ "bitNum": 30, "uv1": 0x2592, "gname1": "shade", "uv2": None, "gname2": None, "isSupported": -1, "script": "latn_" }, #  OEM character set       -
			]

	"""The current definition of numRequired ia a really crude fudge. The numRequired is currently set for
	each block in uniblock.h, as the number of entries in the range from min UV to max UV
	of the block. This is already not right, as in most blocks, not all UV's in the range are valid.
	However, most of these ranges include a lot of archaic and  unusual glyphs not really needed for
	useful support of the script. I am going to just say that if at least third of the entry range
	is included in the font, the designer was making a reasonable effort to support this block
	"""

	unicodeTable = [{ "firstUV": 0x0020, "secondUV": 0x007E, "numRequired": 95, "numFound": 0, "bitNum": 0, "isSupported": 0, "blockName": "Basic Latin" },
	{ "firstUV": 0x0080, "secondUV": 0x00FF, "numRequired": 128, "numFound": 0, "bitNum": 1, "isSupported": 0, "blockName": "Latin-1 Supplement" },
	{ "firstUV": 0x0100, "secondUV": 0x017F, "numRequired": 128, "numFound": 0, "bitNum": 2, "isSupported": 0, "blockName": "Latin Extended-A" },
	{ "firstUV": 0x0180, "secondUV": 0x024F, "numRequired": 208, "numFound": 0, "bitNum": 3, "isSupported": 0, "blockName": "Latin Extended-B" },
	{ "firstUV": 0x0250, "secondUV": 0x02AF, "numRequired": 96, "numFound": 0, "bitNum": 4, "isSupported": 0, "blockName": "IPA Extensions" },
	{ "firstUV": 0x02B0, "secondUV": 0x02FF, "numRequired": 80, "numFound": 0, "bitNum": 5, "isSupported": 0, "blockName": "Spacing Modifier Letters" },
	{ "firstUV": 0x0300, "secondUV": 0x036F, "numRequired": 112, "numFound": 0, "bitNum": 6, "isSupported": 0, "blockName": "Combining Diacritical Marks" },
	{ "firstUV": 0x0370, "secondUV": 0x03FF, "numRequired": 144, "numFound": 0, "bitNum": 7, "isSupported": 0, "blockName": "Greek" },
	{ "firstUV": 0x0400, "secondUV": 0x04FF, "numRequired": 256, "numFound": 0, "bitNum": 9, "isSupported": 0, "blockName": "Cyrillic" },
	{ "firstUV": 0x0530, "secondUV": 0x058F, "numRequired": 96, "numFound": 0, "bitNum": 10, "isSupported": 0, "blockName": "Armenian" },
	{ "firstUV": 0x0590, "secondUV": 0x05FF, "numRequired": 112, "numFound": 0, "bitNum": 11, "isSupported": 0, "blockName": "Hebrew" },
	{ "firstUV": 0x0600, "secondUV": 0x06FF, "numRequired": 256, "numFound": 0, "bitNum": 13, "isSupported": 0, "blockName": "Arabic" },
	{ "firstUV": 0x0900, "secondUV": 0x097F, "numRequired": 128, "numFound": 0, "bitNum": 15, "isSupported": 0, "blockName": "Devanagari" },
	{ "firstUV": 0x0980, "secondUV": 0x09FF, "numRequired": 128, "numFound": 0, "bitNum": 16, "isSupported": 0, "blockName": "Bengali" },
	{ "firstUV": 0x0A00, "secondUV": 0x0A7F, "numRequired": 128, "numFound": 0, "bitNum": 17, "isSupported": 0, "blockName": "Gurmukhi" },
	{ "firstUV": 0x0A80, "secondUV": 0x0AFF, "numRequired": 128, "numFound": 0, "bitNum": 18, "isSupported": 0, "blockName": "Gujarati" },
	{ "firstUV": 0x0B00, "secondUV": 0x0B7F, "numRequired": 128, "numFound": 0, "bitNum": 19, "isSupported": 0, "blockName": "Oriya" },
	{ "firstUV": 0x0B80, "secondUV": 0x0BFF, "numRequired": 128, "numFound": 0, "bitNum": 20, "isSupported": 0, "blockName": "Tamil" },
	{ "firstUV": 0x0C00, "secondUV": 0x0C7F, "numRequired": 128, "numFound": 0, "bitNum": 21, "isSupported": 0, "blockName": "Telugu" },
	{ "firstUV": 0x0C80, "secondUV": 0x0CFF, "numRequired": 128, "numFound": 0, "bitNum": 22, "isSupported": 0, "blockName": "Kannada" },
	{ "firstUV": 0x0D00, "secondUV": 0x0D7F, "numRequired": 128, "numFound": 0, "bitNum": 23, "isSupported": 0, "blockName": "Malayalam" },
	{ "firstUV": 0x0E00, "secondUV": 0x0E7F, "numRequired": 128, "numFound": 0, "bitNum": 24, "isSupported": 0, "blockName": "Thai" },
	{ "firstUV": 0x0E80, "secondUV": 0x0EFF, "numRequired": 128, "numFound": 0, "bitNum": 25, "isSupported": 0, "blockName": "Lao" },
	{ "firstUV": 0x10A0, "secondUV": 0x10FF, "numRequired": 96, "numFound": 0, "bitNum": 26, "isSupported": 0, "blockName": "Georgian" },
	{ "firstUV": 0x1100, "secondUV": 0x11FF, "numRequired": 256, "numFound": 0, "bitNum": 28, "isSupported": 0, "blockName": "Hangul Jamo" },
	{ "firstUV": 0x1E00, "secondUV": 0x1EFF, "numRequired": 256, "numFound": 0, "bitNum": 29, "isSupported": 0, "blockName": "Latin Extended Additional" },
	{ "firstUV": 0x1F00, "secondUV": 0x1FFF, "numRequired": 256, "numFound": 0, "bitNum": 30, "isSupported": 0, "blockName": "Greek Extended" },
	{ "firstUV": 0x2000, "secondUV": 0x206F, "numRequired": 112, "numFound": 0, "bitNum": 31, "isSupported": 0, "blockName": "General Punctuation" },
	{ "firstUV": 0x2070, "secondUV": 0x209F, "numRequired": 48, "numFound": 0, "bitNum": 32, "isSupported": 0, "blockName": "Superscripts and Subscripts" },
	{ "firstUV": 0x20A0, "secondUV": 0x20CF, "numRequired": 48, "numFound": 0, "bitNum": 33, "isSupported": 0, "blockName": "Currency Symbols" },
	{ "firstUV": 0x20D0, "secondUV": 0x20FF, "numRequired": 48, "numFound": 0, "bitNum": 34, "isSupported": 0, "blockName": "Combining Marks for Symbols" },
	{ "firstUV": 0x2100, "secondUV": 0x214F, "numRequired": 80, "numFound": 0, "bitNum": 35, "isSupported": 0, "blockName": "Letterlike Symbols" },
	{ "firstUV": 0x2150, "secondUV": 0x218F, "numRequired": 64, "numFound": 0, "bitNum": 36, "isSupported": 0, "blockName": "Number Forms" },
	{ "firstUV": 0x2190, "secondUV": 0x21FF, "numRequired": 112, "numFound": 0, "bitNum": 37, "isSupported": 0, "blockName": "Arrows" },
	{ "firstUV": 0x2200, "secondUV": 0x22FF, "numRequired": 256, "numFound": 0, "bitNum": 38, "isSupported": 0, "blockName": "Mathematical Operators" },
	{ "firstUV": 0x2300, "secondUV": 0x23FF, "numRequired": 256, "numFound": 0, "bitNum": 39, "isSupported": 0, "blockName": "Miscellaneous Technical" },
	{ "firstUV": 0x2400, "secondUV": 0x243F, "numRequired": 64, "numFound": 0, "bitNum": 40, "isSupported": 0, "blockName": "Control Pictures" },
	{ "firstUV": 0x2440, "secondUV": 0x245F, "numRequired": 32, "numFound": 0, "bitNum": 41, "isSupported": 0, "blockName": "Optical Character Recognition" },
	{ "firstUV": 0x2460, "secondUV": 0x24FF, "numRequired": 160, "numFound": 0, "bitNum": 42, "isSupported": 0, "blockName": "Enclosed Alphanumerics" },
	{ "firstUV": 0x2500, "secondUV": 0x257F, "numRequired": 128, "numFound": 0, "bitNum": 43, "isSupported": 0, "blockName": "Box Drawing" },
	{ "firstUV": 0x2580, "secondUV": 0x259F, "numRequired": 32, "numFound": 0, "bitNum": 44, "isSupported": 0, "blockName": "Block Elements" },
	{ "firstUV": 0x25A0, "secondUV": 0x25FF, "numRequired": 96, "numFound": 0, "bitNum": 45, "isSupported": 0, "blockName": "Geometric Shapes" },
	{ "firstUV": 0x2600, "secondUV": 0x26FF, "numRequired": 256, "numFound": 0, "bitNum": 46, "isSupported": 0, "blockName": "Miscellaneous Symbols" },
	{ "firstUV": 0x2700, "secondUV": 0x27BF, "numRequired": 192, "numFound": 0, "bitNum": 47, "isSupported": 0, "blockName": "Dingbats" },
	{ "firstUV": 0x3000, "secondUV": 0x303F, "numRequired": 64, "numFound": 0, "bitNum": 48, "isSupported": 0, "blockName": "CJK Symbols and Punctuation" },
	{ "firstUV": 0x3040, "secondUV": 0x309F, "numRequired": 96, "numFound": 0, "bitNum": 49, "isSupported": 0, "blockName": "Hiragana" },
	{ "firstUV": 0x30A0, "secondUV": 0x30FF, "numRequired": 96, "numFound": 0, "bitNum": 50, "isSupported": 0, "blockName": "Katakana" },
	{ "firstUV": 0x3100, "secondUV": 0x312F, "numRequired": 48, "numFound": 0, "bitNum": 51, "isSupported": 0, "blockName": "Bopomofo" },
	{ "firstUV": 0x3130, "secondUV": 0x318F, "numRequired": 96, "numFound": 0, "bitNum": 52, "isSupported": 0, "blockName": "Hangul Compatibility Jamo" },
	{ "firstUV": 0x3200, "secondUV": 0x32FF, "numRequired": 150, "numFound": 0, "bitNum": 54, "isSupported": 0, "blockName": "Enclosed CJK Letters and Months" },
	{ "firstUV": 0x3300, "secondUV": 0x33FF, "numRequired": 150, "numFound": 0, "bitNum": 55, "isSupported": 0, "blockName": "CJK Compatibility" },
	{ "firstUV": 0x4E00, "secondUV": 0x9FFF, "numRequired": 16000, "numFound": 0, "bitNum": 59, "isSupported": 0, "blockName": "CJK Unified Ideographs" },
	{ "firstUV": 0xAC00, "secondUV": 0xD7A3, "numRequired": 5000, "numFound": 0, "bitNum": 56, "isSupported": 0, "blockName": "Hangul Syllables" },
	{ "firstUV": 0xE000, "secondUV": 0xF8FF, "numRequired": 6400, "numFound": 0, "bitNum": 60, "isSupported": 0, "blockName": "Private Use" },
	{ "firstUV": 0xF900, "secondUV": 0xFAFF, "numRequired": 150, "numFound": 0, "bitNum": 61, "isSupported": 0, "blockName": "CJK Compatibility Ideographs" },
	{ "firstUV": 0xFB00, "secondUV": 0xFB4F, "numRequired": 40, "numFound": 0, "bitNum": 62, "isSupported": 0, "blockName": "Alphabetic Presentation Forms" },
	{ "firstUV": 0xFB50, "secondUV": 0xFDFF, "numRequired": 688, "numFound": 0, "bitNum": 63, "isSupported": 0, "blockName": "Arabic Presentation Forms-A" },
	{ "firstUV": 0xFE20, "secondUV": 0xFE2F, "numRequired": 16, "numFound": 0, "bitNum": 64, "isSupported": 0, "blockName": "Combining Half Marks" },
	{ "firstUV": 0xFE30, "secondUV": 0xFE4F, "numRequired": 32, "numFound": 0, "bitNum": 65, "isSupported": 0, "blockName": "CJK Compatibility Forms" },
	{ "firstUV": 0xFE50, "secondUV": 0xFE6F, "numRequired": 32, "numFound": 0, "bitNum": 66, "isSupported": 0, "blockName": "Small Form Variants" },
	{ "firstUV": 0xFE70, "secondUV": 0xFEFF, "numRequired": 144, "numFound": 0, "bitNum": 67, "isSupported": 0, "blockName": "Arabic Presentation Forms-B" },
	{ "firstUV": 0xFF00, "secondUV": 0xFFEF, "numRequired": 150, "numFound": 0, "bitNum": 68, "isSupported": 0, "blockName": "Halfwidth and Fullwidth Forms" },
	{ "firstUV": 0xFFF0, "secondUV": 0xFFFF, "numRequired": 16, "numFound": 0, "bitNum": 69, "isSupported": 0, "blockName": "Specials" },
	]

	print("\nSingle Face Test 28: Check font OS/2 code pages for the a common set of code page bits.")
	for font in fontlist:
		cmapUnicodeTables = [None, None]
		hmtxTable = font.ttFont['hmtx'].metrics
		haveUVCmap =0
		haveUVCmap32 =0
		if 'cmap' in font.ttFont:
			cmap_table = font.ttFont['cmap']
			platformID = 3
			platEncID = 1
			cmapUnicodeTables[0] = cmap_table.getcmap(platformID, platEncID)
			platformID = 3
			platEncID = 10
			cmapUnicodeTables[1] = cmap_table.getcmap(platformID, platEncID)
		if cmapUnicodeTables[0]:
			cmapUnicodeTables[0] = cmapUnicodeTables[0].cmap
			haveUVCmap = 1
		if cmapUnicodeTables[1]:
			cmapUnicodeTables[1] = cmapUnicodeTables[1].cmap
			haveUVCmap = 1
			haveUVCmap32 = 1

		os2table = font.ttFont['OS/2']
		# 'ulCodePageRange1', 'ulCodePageRange2', 'ulUnicodeRange1', 'ulUnicodeRange2', 'ulUnicodeRange3', 'ulUnicodeRange4

		# Check ulCodePageRange. If either of the test glyphs are present for a code page, mark it as supported.
		codePageDict = {}
		for codePage in codePageTable:
			codePage["isSupported"] = 0
			for cmapUnicodeTable in cmapUnicodeTables:
				if cmapUnicodeTable:
					if (codePage["uv1"] in cmapUnicodeTable or (codePage["uv2"] and codePage["uv2"] in cmapUnicodeTable)):
						codePage["isSupported"] = 1
						break
			if not haveUVCmap:
				if codePage["gname1"] in hmtxTable or (codePage["gname2"] and codePage["gname2"] in hmtxTable):
						codePage["isSupported"] = 1

			codePageDict[codePage["bitNum"]] = codePage["isSupported"]

		try:
			for codepageIndex in range(2):
				codeRange = eval("os2table.ulCodePageRange%s" % (codepageIndex+1))
				for bitNum in range(32):
					dictBit = bitNum + ( 32*(codepageIndex))
					if dictBit in codePageDict:
						if codePageDict[dictBit] and not (codeRange & (1<<bitNum)):
							print("\tError: font has OS/2.ulCodePageRange%s bit %s not set, but the test heuristics say it should  be set. %s." % (codepageIndex+1, bitNum, font.PostScriptName1))
						elif  (not codePageDict[dictBit]) and (codeRange & (1<<bitNum)):
							print("\tError: font has OS/2.ulCodePageRange%s bit %s set, but the test heuristics say it should not be set. %s." % (codepageIndex+1, bitNum, font.PostScriptName1))
					else:
						if (codeRange & (1<<bitNum)):
							print("\tNote: Font has  OS/2.ulCodePageRange%s bit %s set, which is not covered by this programs rules. %s." % (codepageIndex+1, bitNum, font.PostScriptName1))
		except AttributeError:
			pass

		# Check unicodeTable. Say block is supported if it has at least 1/3 of the necessary glyphs.
		unitTableDict = {}
		# build dict of which blocks are/are nto supported.
		for unicodeBlock in unicodeTable:
			cnt = 0
			blockNum = unicodeBlock["bitNum"]
			for uv in range(unicodeBlock["firstUV"], 1 + unicodeBlock["secondUV"]):
				for cmapUnicodeTable in cmapUnicodeTables:
					if not cmapUnicodeTable:
						continue
					if uv in cmapUnicodeTable:
						cnt += 1
						break

			if cnt*3 >= unicodeBlock["numRequired"]: # At the moment, 'numRequired" is et ot the full range. Requiring one third of this is a crude hack.
				unitTableDict[blockNum]= [1, unicodeBlock["blockName"]]
			else:
				unitTableDict[blockNum] = [0, unicodeBlock["blockName"]]
			unicodeBlock["cnt"] = cnt

		for codepageIndex in range(4):
			codeRange = eval("os2table.ulUnicodeRange%s" % (codepageIndex+1))
			for bitNum in range(32):
				blockIndex = bitNum + ( 32*(codepageIndex))
				if blockIndex in unitTableDict:
					if unitTableDict[blockIndex][0] and not (codeRange & (1<<bitNum)):
						print("\tError: font has OS/2.ulUnicodeRange%s bit %s not set for Unicode block %s, but the test heuristics say it should  be set. %s." % (codepageIndex+1, blockIndex, unitTableDict[blockIndex][1], font.PostScriptName1))
					elif  (not unitTableDict[blockIndex][0]) and (codeRange & (1<<bitNum)):
						print("\tError: font has OS/2.ulUnicodeRange%s bit %s set for Unicode block %s, but the test heuristics say it should not be set. %s." % (codepageIndex+1, blockIndex, unitTableDict[blockIndex][1], font.PostScriptName1))
				else:
					if codeRange & (1<<bitNum):
						if (blockIndex == 57):
							if not haveUVCmap32:
								print("\tError: Font has  OS/2.ulUnicodeRange%s blockIndex %s set, but does not have a 32 bit Unicode cmap table. %s." % (codepageIndex+1, blockIndex, font.PostScriptName1))
						else:
							print("\tNote: Font has  OS/2.ulUnicodeRange%s blockIndex %s set, which is not covered by this programs rules. %s." % (codepageIndex+1, blockIndex, font.PostScriptName1))

def doSingleTest29(): # Place-holder
	pass

def doSingleTest30():
	global preferredFamilyList1

	print("\nSingle Face Test 30: Check that there are no more than 7 pairs of BlueValues and FamilyBlues in a font, and there is an even number of values.")

	for name in preferredFamilyList1.keys():
		fontgroup = preferredFamilyList1[name]
		for font in fontgroup:
			if not font.topDict: # not CFF
				print("\t\tSkipping non-PostScript Font:", font.PostScriptName1)
				break
			for fontDict in font.FDArray:
				try: # not all fonts have BlueValues, but they should.
					if len(fontDict.Private.BlueValues) > 14:
						print("\tError: The font has more than 7 pairs of BlueValues")
						print("\t\tFont:", font.PostScriptName1)
						print("\t\tBlueValues:", fontDict.Private.BlueValues)
					if len(fontDict.Private.BlueValues) % 2 != 0:
						print("\tError: The font does not have an even number of BlueValues")
						print("\t\tFont:", font.PostScriptName1)
						print("\t\tBlueValues:", fontDict.Private.BlueValues)
				except AttributeError:
					print("\tError: Skipping test: the font has no BlueValues for", font.PostScriptName1)
					continue

				try: # not all fonts have FamilyBlues.
					if len(fontDict.Private.FamilyBlues) > 14:
						print("\nError: The font has more than 7 pairs of FamilyBlues")
						print("\tFont:", font.PostScriptName1)
						print("\tFamilyBlues:", fontDict.Private.FamilyBlues)

					if len(fontDict.Private.FamilyBlues) % 2 != 0:
						print("\nError: The font does not have an even number of FamilyBlues")
						print("\tFont:", font.PostScriptName1)
						print("\tFamilyBlues:", fontDict.Private.FamilyBlues)
				except AttributeError:
					pass

def doSingleTest31():
	global preferredFamilyList1

	print("\nSingle Face Test 31: Check that there are no more than 5 pairs of OtherBlues and FamilyOtherBlues in a font, and there is an even number of values.")

	for name in preferredFamilyList1.keys():
		fontgroup = preferredFamilyList1[name]
		for font in fontgroup:
			if not font.topDict: # not CFF
				print("\t\tSkipping non-PostScript Font:", font.PostScriptName1)
				break
			for fontDict in font.FDArray:
				try: # not all fonts have FamilyOtherBlues.
					if len(fontDict.Private.OtherBlues) > 10:
						print("\tError: The font has more than 5 pairs of OtherBlues")
						print("\t\tFont:", font.PostScriptName1)
						print("\t\tOtherBlues:", fontDict.Private.OtherBlues)
					if len(fontDict.Private.OtherBlues) % 2 != 0:
						print("\tError: The font does not have an even number of OtherBlues")
						print("\t\tFont:", font.PostScriptName1)
						print("\t\tOtherBlues:", fontDict.Private.OtherBlues)
				except AttributeError:
					print("Skipping test - font dict does not contain OtherBlues", font.PostScriptName1)
					continue

				try: # not all fonts have FamilyOtherBlues.
					if len(fontDict.Private.FamilyOtherBlues) > 10:
						print("\tError: The font has more than 5 pairs of FamilyOtherBlues")
						print("\t\tFont:", font.PostScriptName1)
						print("\t\tFamilyOtherBlues:", fontDict.Private.FamilyOtherBlues)
					if len(fontDict.Private.FamilyOtherBlues) % 2 != 0:
						print("\tError: The font does not have an even number of FamilyOtherBlues")
						print("\t\tFont:", font.PostScriptName1)
						print("\t\tFamilyOtherBlues:", fontDict.Private.FamilyOtherBlues)
				except AttributeError:
					pass

def doSingleTest32():
	global preferredFamilyList1

	print("\nSingle Face Test 32: Check that all fonts have blue value pairs with first integer is less than or equal to the second integer in pairs.")

	for name in preferredFamilyList1.keys():
		fontgroup = preferredFamilyList1[name]
		for font in fontgroup:
			if not font.topDict: # not CFF
				print("\t\tSkipping non-PostScript Font:", font.PostScriptName1)
				break
			for fontDict in font.FDArray:
				try: # not all fonts have BlueValues, but they should.
					for i in range(0, len(fontDict.Private.BlueValues), 2):
						if fontDict.Private.BlueValues[i] > fontDict.Private.BlueValues[i+1]:
							print("\nError: The font has BlueValues pairs with the first value is greater than the second value in pairs")
							print("\tFont:", font.PostScriptName1)
							print("\tPairs:", fontDict.Private.BlueValues[i], fontDict.Private.BlueValues[i+1])
							print("\tBlueValues:", fontDict.Private.BlueValues)
				except AttributeError:
					print("Skipping test - missing BlueValues - for ", font.PostScriptName1)
					continue

				try: # not all fonts have OtherBlues.
					for i in range(0, len(fontDict.Private.OtherBlues), 2):
						if fontDict.Private.OtherBlues[i] > fontDict.Private.OtherBlues[i+1]:
							print("\nError: The font has OtherBlues pairs with the first value is greater than the second value in pairs")
							print("\tFont:", font.PostScriptName1)
							print("\tPairs:", fontDict.Private.OtherBlues[i], fontDict.Private.OtherBlues[i+1])
							print("\tOtherBlues:", fontDict.Private.OtherBlues)
				except AttributeError:
					pass

				try: # not all fonts have FamilyBlues.
					for i in range(0, len(fontDict.Private.FamilyBlues), 2):
						if fontDict.Private.FamilyBlues[i] > fontDict.Private.FamilyBlues[i+1]:
							print("\nError: The font has FamilyBlues pairs with the first value is greater than the second value in pairs")
							print("\tFont:", font.PostScriptName1)
							print("\tPairs:", fontDict.Private.FamilyBlues[i], fontDict.Private.FamilyBlues[i+1])
							print("\tFamilyBlues:", fontDict.Private.FamilyBlues)
				except AttributeError:
					pass

				try: # not all fonts have FamilyOtherBlues.
					for i in range(0, len(fontDict.Private.FamilyOtherBlues), 2):
						if fontDict.Private.FamilyOtherBlues[i] > fontDict.Private.FamilyOtherBlues[i+1]:
							print("\nError: The font has FamilyOtherBlues pairs with the first value is greater than the second value in pairs")
							print("\tFont:", font.PostScriptName1)
							print("\tPairs:", fontDict.Private.FamilyOtherBlues[i], fontDict.Private.FamilyOtherBlues[i+1])
							print("\tFamilyOtherBlues:", fontDict.Private.FamilyOtherBlues)
				except AttributeError:
					pass

def doSingleTest33():
	global preferredFamilyList1

	print("\nSingle Face Test 33: Check that Bottom Zone blue value pairs and Top Zone blue value pairs are at least (2*BlueFuzz+1) unit apart in a font.")
	for name in preferredFamilyList1.keys():
		fontgroup = preferredFamilyList1[name]
		for font in fontgroup:
			if not font.topDict: # not CFF
				print("\t\tSkipping non-PostScript Font:", font.PostScriptName1)
				break
			for fontDict in font.FDArray:
				try: # not all fonts have OtherBlues.
					bottomZone = fontDict.Private.BlueValues[0:2] + fontDict.Private.OtherBlues[:]
				except AttributeError:
					try: # not all fonts have BlueValues.
						bottomZone = fontDict.Private.BlueValues[0:2]
					except AttributeError:
						print("Skipping test - missing BlueValues - for ", font.PostScriptName1)
						continue

				for i in range(0,len(bottomZone)-1,2):	# compare a pair to the rest
					for j in range(i+2, len(bottomZone)):
						if abs(bottomZone[i] - bottomZone[j]) < (2*fontDict.Private.BlueFuzz + 1):	# first integer in pair
							print("\nError: The BottomZone have pairs less than %d units apart." % (2*fontDict.Private.BlueFuzz + 1))
							print("\tFont:", font.PostScriptName1)
							print("\tValues:", bottomZone[i], bottomZone[j])
							print("\tBottomZone:", bottomZone)
						if abs(bottomZone[i+1] - bottomZone[j]) < (2*fontDict.Private.BlueFuzz + 1):	# second integer in pair
							print("\nError: The BottomZone have pairs less than %d units apart." % (2*fontDict.Private.BlueFuzz + 1))
							print("\tFont:", font.PostScriptName1)
							print("\tValues:", bottomZone[i+1], bottomZone[j])
							print("\tBottomZone:", bottomZone)

				topZone = fontDict.Private.BlueValues[2:]
				for i in range(0,len(topZone)-1,2): # compare a pair to the rest
					for j in range(i+2, len(topZone)):
						if abs(topZone[i] - topZone[j]) < (2*fontDict.Private.BlueFuzz + 1):
							print("\nError: The TopZone have pairs less than %d units apart." % (2*fontDict.Private.BlueFuzz + 1))
							print("\tFont:", font.PostScriptName1)
							print("\tValues:", topZone[i], topZone[j])
							print("\tTopZone:", topZone)
						if abs(topZone[i+1] - topZone[j]) < (2*fontDict.Private.BlueFuzz + 1):
							print("\nError: The TopZone have pairs less than %d units apart." % (2*fontDict.Private.BlueFuzz + 1))
							print("\tFont:", font.PostScriptName1)
							print("\tValues:", topZone[i+1], topZone[j])
							print("\tTopZone:", topZone)

def doSingleTest34():
	global preferredFamilyList1

	print("\nSingle Face Test 34: Check that the difference between numbers in blue value pairs meet the requirement.")
	for name in preferredFamilyList1.keys():
		fontgroup = preferredFamilyList1[name]
		for font in fontgroup:
			if not font.topDict: # not CFF
				print("\t\tSkipping non-PostScript Font:", font.PostScriptName1)
				break
			for fontDict in font.FDArray:
				try:
					for i in range(0, len(fontDict.Private.BlueValues), 2):
						diff = abs(fontDict.Private.BlueValues[i] - fontDict.Private.BlueValues[i+1])
						if (fontDict.Private.BlueScale*240*diff) >= 240.0:
							print("\nError: The BlueValues pair have distance greater than the maximum limit.")
							print("\tFont:", font.PostScriptName1)
							print("\tPairs:", fontDict.Private.BlueValues[i], fontDict.Private.BlueValues[i+1])
							print("\tDistance (%d) is greater than 1/BlueScale (%5.2f) " % (diff, 1.0 / fontDict.Private.BlueScale))
							print("\tBlueValues:", fontDict.Private.BlueValues)
				except AttributeError:
					print("Skipping test - font dict does not contain OtherBlues", font.PostScriptName1)
					continue
		try:
			for font in fontgroup:
				for fontDict in font.FDArray:
					for i in range(0, len(fontDict.Private.OtherBlues), 2):
						diff = abs(fontDict.Private.OtherBlues[i] - fontDict.Private.OtherBlues[i+1])
						if (fontDict.Private.BlueScale*240*diff) >= 240.0:
							print("\nError: The OtherBlues pair have distance greater than the maximum limit.")
							print("\tFont:", font.PostScriptName1)
							print("\tPairs:", fontDict.Private.OtherBlues[i], fontDict.Private.OtherBlues[i+1])
							print("\tDistance (%d) is greater than 1/BlueScale (%5.2f) " % (diff, 1.0 / fontDict.Private.BlueScale))
							print("\tOtherBlues:", fontDict.Private.OtherBlues)
		except AttributeError:
			pass

		try:
			for font in fontgroup:
				for fontDict in font.FDArray:
					for i in range(0, len(fontDict.Private.FamilyBlues), 2):
						diff = abs(fontDict.Private.FamilyBlues[i] - fontDict.Private.FamilyBlues[i+1])
						if (fontDict.Private.BlueScale*240*diff) >= 240.0:
							print("\nError: The FamilyBlues pair have distance greater than the maximum limit.")
							print("\tFont:", font.PostScriptName1)
							print("\tPairs:", fontDict.Private.FamilyBlues[i], fontDict.Private.FamilyBlues[i+1])
							print("\tDistance (%d) is greater than 1/BlueScale (%5.2f) " % (diff, 1.0 / fontDict.Private.BlueScale))
							print("\tFamilyBlues:", fontDict.Private.FamilyBlues)
		except AttributeError:
			pass

		try:
			for font in fontgroup:
				for fontDict in font.FDArray:
					for i in range(0, len(fontDict.Private.FamilyOtherBlues), 2):
						diff = abs(fontDict.Private.FamilyOtherBlues[i] - fontDict.Private.FamilyOtherBlues[i+1])
						if (fontDict.Private.BlueScale*240*diff) >= 240.0:
							print("\nError: The FamilyOtherBlues pair have distance greater than the maximum limit.")
							print("\tFont:", font.PostScriptName1)
							print("\tPairs:", fontDict.Private.FamilyOtherBlues[i], fontDict.Private.FamilyOtherBlues[i+1])
							print("\tDistance (%d) is greater than 1/BlueScale (%5.2f) " % (diff, 1.0 / fontDict.Private.BlueScale))
							print("\tFamilyOtherBlues:", fontDict.Private.FamilyOtherBlues)
		except AttributeError:
			pass

def doSingleFaceTests(singleTestList, familyTestList):
	global doStemWidthChecks

	if (singleTestList or familyTestList):
		if singleTestList:
			g = globals()
			for val in singleTestList:
				funcName = "doSingleTest%s" % (val)
				if funcName not in g:
					print("Error: cannot find function '%s' to execute." % (funcName))
					continue
				exec("%s()" % (funcName))
	else:
		doSingleTest1()
		doSingleTest2()
		doSingleTest3()
		doSingleTest4()
		doSingleTest5()
		doSingleTest6()
		doSingleTest7()
		doSingleTest8()
		doSingleTest9()
		doSingleTest10()
		doSingleTest11()
		doSingleTest12()
		doSingleTest13()
		doSingleTest14()
		doSingleTest15()
		doSingleTest16()
		doSingleTest17()
		if ('CFF ' in fontlist[0].ttFont.keys()):
			doSingleTest18()
		doSingleTest19()
		doSingleTest20()
		doSingleTest21()
		doSingleTest22()
		doSingleTest23()
		doSingleTest24()
		doSingleTest25()
		doSingleTest26()
		doSingleTest27()
		doSingleTest28()
		doSingleTest29()
		if ('CFF ' in fontlist[0].ttFont.keys()):
			doSingleTest30()
			doSingleTest31()
			doSingleTest32()
			doSingleTest33()
			doSingleTest34()

def doFamilyTest1():
	global compatibleFamilyList3

	print("\nFamily Test 1: Verify that each group of fonts with the same nameID 1 has maximum of 4 fonts.")
	for name in compatibleFamilyList3.keys():
		if len(compatibleFamilyList3[name]) > 4:
			print("\nError: More than 4 fonts with the same nameID 1, compatible family name")
			print("\t Compatible Family Name:", name)
			for font in compatibleFamilyList3[name]:
				print("\t PostScript Name:", font.PostScriptName3)


def doFamilyTest2():
	global compatibleFamilyList3
	kUnicodeLanguageID = 1033

	print("\nFamily Test 2: Check that the Compatible Family group has same name ID's in all languages except for the compatible names 16,17,18, 20, 21 and 22.")
	for name in compatibleFamilyList3.keys():
		fontgroup = compatibleFamilyList3[name]
		LanguageDict = {}
		fontList = ""
		for font in fontgroup:
			fontList += " %s" % font.PostScriptName1
			for nameID in font.nameIDDict.keys():
				if (nameID[-1] in [16,17,18,20,21,22]) or (nameID[-1] > 255):
					continue
				count = LanguageDict.get(nameID, 0)
				LanguageDict[nameID] = count +1
		countList = LanguageDict.values()
		minCount = min(countList)
		maxCount = max(countList)
		if minCount != maxCount:
			print("\tError: There are name table {platform, script, language, name ID} combinations that exist")
			print("\tin some fonts but not all fonts within the Compatible Family '%s'. Please check fonts '%s'." % (name, fontList))
			print("\t{platform, script, language, name ID} combination : Number of fonts in the family with this combination")
			itemList = sorted(LanguageDict.items())
			for entry in itemList:
				print("\t%s\t%s" % (entry[0], entry[1]))
			print("All fonts should have the same set of {platform, script, language, name ID} combinations, so the number of fonts for each combination should be the same, excepting name ID's 16,17,18, and 20.")

def doFamilyTest3():
	global compatibleFamilyList3

	print("\nFamily Test 3: Check that the Compatible Family group has same Preferred Family name (name ID 16)in all languagesID 16 in all other languages.")
	for name in compatibleFamilyList3.keys():
		fontgroup = compatibleFamilyList3[name]
		LanguageList = []
		# Collect the list of name ID 16's for the Microsoft platform.
		for font in fontgroup:
			for nameIDKey in font.nameIDDict.keys():
				Platform, Encoding, Language, ID = nameIDKey
				Language,font.PostScriptName1
				if Platform == 3 and Encoding == 1 and ID == 16 and LanguageList.count(Language) == 0:
					LanguageList.append(Language)
		if len(LanguageList) == 0:
			continue
		for Language in LanguageList:
			try:
				font1 = fontgroup[0]
				nameID1 = font1.nameIDDict[(3, 1, Language, 16)].decode('utf_16_be')
			except KeyError:
				nameID1 = None
			for font in fontgroup[1:]:
				try:
					nameID = font.nameIDDict[(3, 1, Language, 16)].decode('utf_16_be')
				except KeyError:
					nameID = None
				if nameID != nameID1:
					print("\tError: NameID 16 '%s' for language ID %s in font %s is not the same as '%s' in font %s." % (nameID, Language, font.PostScriptName1, nameID1, font1.PostScriptName1))

def doFamilyTest4():
	print("\nFamily Test 4: Family-wide 'size' feature checks.")
	# make sure that all fonts with the same subgroup ID have the same size menu name
	# make sure that within a size subgroup, there is no overlap
	# make sure that within a size subgroup, coverage is continuous
	# warn if subgrousp have different size ranges.
	# requie that all fonts in the same Preferred Family with the same Bold and Italic
	# style values and OS/2 weight and width values should share the same subfamily name,
	# but this should be a warning

	sizeGroupDict = {}
	seenSize  = 0

	global fontlist
	for font in fontlist:
		if hasattr(font,'sizeNameID'):
			continue
		font.sizeDesignSize = 0
		font.sizeSubfFamilyID = 0
		font.sizeNameID = 0
		font.sizeRangeStart = 0
		font.sizeRangeEnd = 0
		font.sizeMenuName = "NoSizefeature"

		command = "spot -t GPOS \"%s\" 2>&1" % (font.path)
		report = fdkutils.runShellCmd(command)
		if "size" in report:
			match = re.search(r"Design Size:\s*(\d+).+?Subfamily Identifier:\s*(\d+).+name table [^:]+:\s*(\d+).+Range Start:\s*(\d+).+?Range End:\s*(\d+)", report, re.DOTALL)
			if match:
				font.sizeDesignSize = eval(match.group(1))
				font.sizeSubfFamilyID = eval(match.group(2))
				font.sizeNameID = eval(match.group(3))
				font.sizeRangeStart = eval(match.group(4))
				font.sizeRangeEnd = eval(match.group(5))
				font.sizeMenuName = ""
				for keyTuple in font.nameIDDict.keys():
					if keyTuple[3] == font.sizeNameID:
						font.sizeMenuName = font.nameIDDict[keyTuple]
						break
			else:
				print("Program error: font has 'size' feature, but could not parse the values for font %s." % ( font.PostScriptName1))

	for name in preferredFamilyList1.keys():
		fontgroup = preferredFamilyList1[name]
		sizeGroupbyID = {}
		sizeGroupbyMenu = {}
		for font in fontgroup:
			if font.sizeNameID:
				seenSize = 1
			gList = sizeGroupbyMenu.get(font.sizeMenuName, [])
			gList.append(font)
			sizeGroupbyMenu[font.sizeMenuName] = gList
			gList = sizeGroupbyID.get(font.sizeSubfFamilyID, [])
			gList.append(font)
			sizeGroupbyID[font.sizeSubfFamilyID] = gList
		if not seenSize:
			continue
		fByID = sorted(sizeGroupbyID.values())
		fByName = sorted(sizeGroupbyMenu.values())
		if fByName != fByID:
			print("	Error: in family %s, the fonts with the same size subgroup ID's do not all have the same menu names" % (name))

		# now check the ranges.
		lastList = None
		for sid in sizeGroupbyID.keys():
			fList = sizeGroupbyID[sid]
			pList = map(lambda font: font.PostScriptName1, fList)
			menuName = fList[0].sizeMenuName
			sid = fList[0].sizeSubfFamilyID
			if lastList and len(pList) != len(lastList):
				print("	Warning: in family %s, the 'size' group (id %s, menu name %s) has %s members, while the group (id %s, menu name %s) has %s members." % (name, sid, menuName, len(pList), lastSID, lastMenuName, len(lastList) ))
				print("\t (sub group id %s, menu name %s, fonts: %s" % ( sid, menuName, pList))
				print("\t (sub group id %s, menu name %s, fonts: %s" % (lastSID, lastMenuName, lastList))
			lastList = pList
			lastMenuName = menuName
			lastSID = sid

			# For current list, check the design sizes are contiguous
			if len(fList) < 2:
				continue
			sizeCoverage = []
			for font in fList:
				sizeCoverage.append([font.sizeRangeStart, font.sizeRangeEnd, font.PostScriptName1, font.sizeMenuName, font.sizeSubfFamilyID])
			sizeCoverage.sort()
			error = 0
			lastEntry = sizeCoverage[0]
			errorList = []
			for entry in sizeCoverage[1:]:
				if entry[0] != round(lastEntry[1] + 0.1):
					error = 1
					errorList.append([lastEntry, entry])
				lastEntry = entry
			if error:
				print("	Error: design ranges for size subgroup ID %s, size menu %s in family %s are not contiguous." % (sid, font.sizeMenuName, name))
				for item in errorList:
					print("previous range: %s - %s for %s, next range: %s - %s for %s. sub group ID %s." % (item[0][0], item[0][1], item[0][2], item[1][0], item[1][1], item[1][2], item[0][3]))



def doFamilyTest5():
	global compatibleFamilyList3


	print("\nFamily Test 5: Check that style settings for each face is unique within Compatible Family group, in all languages.")
	for name in compatibleFamilyList3.keys():
		fontgroup = compatibleFamilyList3[name]
		for i in range(len(fontgroup)):
			font = fontgroup[i]
			for font1 in fontgroup[i+1:]:
				if font.isItalic == font1.isItalic and font.isBold == font1.isBold:
					print("\nError: These two fonts have the same italic and bold style setting")
					print("\tFont 1:", font.PostScriptName3)
					print("\t\tItalic Bit:", font.isItalic, "Bold Bit:", font.isBold)
					print("\tFont 2:", font1.PostScriptName3)
					print("\t\tItalic Bit:", font1.isItalic, "Bold Bit:", font1.isBold)

def doFamilyTest6():
	global compatibleFamilyList3

	print("\nFamily Test 6: Check that the Compatible Family group has a base font and at least two faces, and check if weight class is valid.")
	baseBoldRangeDict = {250:(600,1000), 300:(600,950), 350:(600,900), 400:(600,850), 450:(600,800), 500:(650,750), 550:(600,800)}
	for name in compatibleFamilyList3.keys():
		basefont = ""
		boldfont = ""
		for font in compatibleFamilyList3[name]:
			if not font.isBold and not font.isItalic:
				basefont = font
			if font.isBold and not font.isItalic:
				boldfont = font
		if basefont == "":
			basefont = boldfont
		if basefont == "":
			print("\tError: No base font in the group. Compatible family name:", name)
			continue

		if basefont.usWeightClass == -1:
			print("Skipping test - no OS/2 table.", basefont.PostScriptName1)
			return

		if basefont == boldfont and len(compatibleFamilyList3[name]) > 2:
			print("\tError: Base font is not Regular face and the group has more than two faces for the group. Compatible family name:", name)
			continue
		if basefont == boldfont or boldfont == "":
			continue
		try:
			wgtValue = 50*int(basefont.usWeightClass/50)
			boldRange = baseBoldRangeDict[wgtValue]
		except KeyError:
			print("\tError: usWeightClass %s of regular font %s is outside of range that allows style linking to a Bold face: this will result is fake bolding of the Regular face when the Bold style is chsoen." % (basefont.usWeightClass, font.PostScriptName1))
			print("\t\tPermissible range for base font %s in order for it to link to a Bold font: %s - %s" % (basefont.PostScriptName1, 250, 550))
			continue

		if boldfont.usWeightClass < boldRange[0] or boldfont.usWeightClass > boldRange[1]:
			print("\tError: usWeightClass '%s'of the Bold font is outside of BOLD RANGE [%s - %s] for the group. Compatible family name: %s" % (boldfont.usWeightClass, boldRange[0], boldRange[1], name))
			print("\t\tBase Font:", basefont.PostScriptName3)
			print("\t\tusWeightClass:", basefont.usWeightClass)
			print("\t\tBold Font:", boldfont.PostScriptName3)
			print("\tu\tsWeightClass:", boldfont.usWeightClass)
			print("\t\tBOLD RANGE:", boldRange[0], "-", boldRange[1])
			continue
		if (boldfont.usWeightClass % 50) != 0:
			print("\tWarning: usWeightClass of the Bold font is within range, but not in increments of 50 for the group. Compatible family name:", name)
			print("\t\tBase Font:", basefont.PostScriptName3)
			print("\t\tusWeightClass:", basefont.usWeightClass)
			print("\t\tBold Font:", boldfont.PostScriptName3)
			print("\t\tusWeightClass:", boldfont.usWeightClass)

def doFamilyTest7():
	global preferredFamilyList1

	print("\nFamily Test 7: Check that all faces in the Preferred Family group have the same Copyright and Trademark string.")
	for name in preferredFamilyList1.keys():
		fontgroup = preferredFamilyList1[name]
		font = fontgroup[0]
		for font1 in fontgroup[1:]:
			if font.copyrightStr1 != font1.copyrightStr1:
				temp1 = re.sub(" *[21][980]\d\d,* *", "", font.copyrightStr1)
				temp2 =	 re.sub(" *[21][980]\d\d,* *", "", font1.copyrightStr1)
				temp1 = re.sub("\s+", " ", temp1)
				temp2 =	 re.sub("\s+", " ", temp2)
				if temp1 != temp2:
					print("\nWarning: These two fonts do not have the same Copyright String. Preferred family:", name)
				else:
					print("\nNote: These two fonts differ only in the years in the	Copyright String. Preferred family:", name)
				print("\tFont 1:", font.PostScriptName1)
				print("\tCopyright:", font.copyrightStr1)
				print("\tFont 2:", font1.PostScriptName1)
				print("\tCopyright:", font1.copyrightStr1)
		for font1 in fontgroup[1:]:
			if font.trademarkStr1 != font1.trademarkStr1:
				temp1 = re.sub(" *[21][980]\d\d,* *", "", font.trademarkStr1)
				temp2 =	 re.sub(" *[21][980]\d\d,* *", "", font1.trademarkStr1)
				temp1 = re.sub("\s+", " ", temp1)
				temp2 =	 re.sub("\s+", " ", temp2)
				if temp1 != temp2:
					print("\nWarning: These two fonts do not have the same Trademark String. Preferred family:", name)
				else:
					print("\nNote: These two fonts differ only in the years in the	Trademark String. Preferred family:", name)
				print("\tFont 1:", font.PostScriptName1)
				print("\tTrademark:", font.trademarkStr1)
				print("\tFont 2:", font1.PostScriptName1)
				print("\tTrademark:", font1.trademarkStr1)

def doFamilyTest8():
	global compatibleFamilyList3, fontlist

	print("\nFamily Test 8: Check the Compatible Family group style vs OS/2.usWeightClass settings. Max 2 usWeightClass allowed.")
	for name in compatibleFamilyList3.keys():
		fontgroup = compatibleFamilyList3[name]
		weightClassList = []

		if fontgroup[0].usWeightClass == -1:
			print("Skipping test - no OS/2 table.", fontgroup[0].PostScriptName1)
			return

		boldFont = None
		baseFont = None
		for font in fontgroup:
			if weightClassList.count(font.usWeightClass) == 0:
				weightClassList.append(font.usWeightClass)
			if font.isBold:
				boldFont = font
			elif not font.isItalic:
				baseFont = font
		if len(weightClassList) > 2:
			print("\nError: More than two usWeightClass values in the Group:", name)
			print("\tusWeightClass values:", weightClassList)
			continue
		if len(weightClassList) < 2:
			if boldFont and baseFont:
				print("\nError: Bold style font set with same usWeightClass value as the Regular face, for font", boldFont.PostScriptName3)
			continue
		if weightClassList[0] > weightClassList[1]:
			usWeightClass1, usWeightClass2 = weightClassList[1], weightClassList[0]
		else:
			usWeightClass1, usWeightClass2 = weightClassList[0], weightClassList[1]
		for font in fontgroup:
			if font.usWeightClass == usWeightClass1 and font.isBold:
				print("\nError: Bold style set with lower usWeightClass value for the Font", font.PostScriptName3)
				print("\tusWeightClass:", font.usWeightClass)
			if font.usWeightClass == usWeightClass2 and not font.isBold:
				print("\nError: Bold style not set with higher usWeightClass value for the Font", font.PostScriptName3)
				print("\tusWeightClass:", font.usWeightClass)

def doFamilyTest9():
	global compatibleFamilyList3

	print("\nFamily Test 9: Check that all faces in the Compatible Family group have the same OS/2.usWidthClass value.")
	for name in compatibleFamilyList3.keys():
		fontgroup = compatibleFamilyList3[name]
		font = fontgroup[0]
		if font.usWidthClass == -1:
			print("Skipping test - no OS/2 table.", font.PostScriptName1)
			return

		for font1 in fontgroup[1:]:
			if font.usWidthClass != font1.usWidthClass:
				print("\nWarning: These two fonts do not have the same usWidthClass in Group", name)
				print("\tFont 1:", font.PostScriptName3)
				print("\tusWidthClass:", font.usWidthClass)
				print("\tFont 2:", font1.PostScriptName3)
				print("\tusWidthClass:", font1.usWidthClass)

def doFamilyTest10():

	def getPanoseStatus(panoseList):
		# makeotf sets only the first four values. If any of the rest are non-zero, it is a real value.
		panoseStatus = 0
		for value in panoseList[4:]:
			if value != 0:
				panoseStatus = 2
				break
		if not panoseStatus:
			for value in panoseList[:4]:
				if value != 0:
					panoseStatus = 1
					break

		return panoseStatus # 0 means all the values are 0; 1 means they were probably derived by makeotf; 2 means they exist, and were not derived by makeotf.
	firstFontName = ""
	print("\nFamily Test 10: Check that if all faces in family have a Panose number and that CFF ISFixedPtch matches the Panose monospace setting.")
	for name in preferredFamilyList1.keys():
		fontgroup = preferredFamilyList1[name]
		if not fontgroup[0].panose:
			print("Skipping test: font is missing the OS.2 table", fontgroup[0].PostScriptName1)
			return

		for i in range(len(fontgroup)):
			font = fontgroup[i]
			fontPanoseStatus= getPanoseStatus(font.panose)
			if not fontPanoseStatus == 2:
				if fontPanoseStatus == 1:
					print("	Warning: ", font.PostScriptName1, " has only the first 4 Panose values set; these are may be default values derived by makeotf.")
				elif fontPanoseStatus == 0:
					print("	Error: ", font.PostScriptName1, " has only  0 Panose values.")
			else:
				# if has real Panose value, check Monospace fields.
				isPanoseMonospaced = 0
				valueIndex = 0
				if ((font.panose[0] == 2) and (font.panose[3] == 9)) or \
					((font.panose[0] == 3) and (font.panose[3] == 3)) or \
					((font.panose[0] == 4) and (font.panose[3] == 9)) or \
					((font.panose[0] == 5) and (font.panose[3] == 3)):
					isPanoseMonospaced = 1
				else:
					isPanoseMonospaced = 0
				if 'CFF ' in font.ttFont and (isPanoseMonospaced != font.topDict.isFixedPitch):
					if isPanoseMonospaced:
						print("	Error: OS/2 Panose digit 4 indicates that the font is monospaced, but the CFF top dict IsFixedPitch value is set false.")
					else:
						print("	Error: OS/2 Panose digit 4 indicates that the font is not monospaced, but the CFF top dict IsFixedPitch value is set true.")
				if  (isPanoseMonospaced != font.isFixedPitchfromHMTX):
					if isPanoseMonospaced:
						print("	Error: OS/2 Panose digit 4 indicates that the font is monospaced, but there is more than one width value in the htmx table.")
					else:
						print("	Error: OS/2 Panose digit 4 indicates that the font is not monospaced, but there is only one width value in the hmtx table.")
				if 'CFF ' in font.ttFont and (font.topDict.isFixedPitch != font.isFixedPitchfromHMTX):
					if font.topDict.isFixedPitch:
						print("	Error: the CFF top dict IsFixedPitch value is set true, but there is more than one width value in the htmx table.")
					else:
						print("	Error: the CFF top dict IsFixedPitch value is set false, but there is only one width value in the hmtx table.")


def doFamilyTest11():
	global compatibleFamilyList3

	# crude heuristics: only compare lower ascii values
	def CompareWinToMacMenuName(winMenuName, macMenuName):
		isEqual = 0

		lenMac = len(macMenuName)
		if (lenMac == len(winMenuName)):
			# might be the same!
			isEqual = 1
			for i in range(lenMac):
				if ord(macMenuName[i]) > 127:
					continue # ignore upper ascii
				if (winMenuName[i] == macMenuName[i]):
					continue
				isEqual = 0
				break

		return isEqual


	print("\nFamily Test 11: Check that Mac and Windows menu names differ for all but base font, and are the same for the base font.")
	for name in compatibleFamilyList3.keys():
		basefont = ""

		for font in compatibleFamilyList3[name]:
			macMenuName = ""
			winMenuName = ""
			winCompMenuName = ""
			for nameIDKey in font.nameIDDict.keys():
				Platform, Encoding, Language, ID = nameIDKey

				if (Platform == 1) and (Encoding == 0)  and (ID == 18) and Language == 0:
					macMenuName = font.nameIDDict[nameIDKey].decode('macroman')

				if (Platform == 1) and (Encoding == 0) and (ID == 4) and Language == 0 and not macMenuName:
					macMenuName = font.nameIDDict[nameIDKey].decode('macroman')

				if Platform == 3 and Encoding == 1 and ID == 1 and Language == 1033:
					winMenuName = font.nameIDDict[nameIDKey].decode('utf_16_be')
			if font.isBold or font.isItalic:
				if not font.isCID:
					if CompareWinToMacMenuName(winMenuName, macMenuName):
						print("\nError: The Mac and Windows Compatible Names should differ in a style-linked BOLD or Italic font, but do not. Font", font.PostScriptName1)
						print("\tMac Compatible Name:", macMenuName)
						print("\tWindows Compatible Name:", winMenuName)
			else:
				if not font.isCID:
					if not CompareWinToMacMenuName(winMenuName, macMenuName):
						print("\nError: The  Mac and Windows Compatible Names for the regular face of a style-linked group should be the same. Font", font.PostScriptName1)
						print("\tMac Compatible Name:", macMenuName)
						print("\tWindows Compatible Name:", winMenuName)


def doFamilyTest12():
	"""
	For each font, step through the script/language/feature tree, and collect a fontDict[script][language][feature] == lookupSetIndex,
	where lookupSetIndex is a number that uniquely identifies a set of lookups belonging to a feature. This mapping is kept
	in lookupDefDict[lookupSetIndex] = <real lookup index list>
	"""
	print("\nFamily Test 12: Check that GSUB/GPOS script and language feature lists are the same in all faces, and that DFLT/dflt and latn/dflt are present.")
	dictList = []
	for name in preferredFamilyList1.keys():
		fontgroup = preferredFamilyList1[name]
		for i in range(len(fontgroup)):
			font = fontgroup[i]
			fontDict  = {}
			for tableTag in ["GPOS", "GSUB"]:
				lookupListDict = {}
				featDict = {}
				scriptDict ={}
				langDict = {}
				lookupDefDict = {}
				try:
					layoutTable = font.ttFont[tableTag]
					featList = layoutTable.table.FeatureList.FeatureRecord
					for scriptRecord in layoutTable.table.ScriptList.ScriptRecord:
						stag = scriptRecord.ScriptTag
						scriptDict[stag] = {}
						langSysRecords = list(map(lambda rec: [rec.LangSysTag, rec.LangSys],  scriptRecord.Script.LangSysRecord))
						if scriptRecord.Script.DefaultLangSys:
							langSysRecords.append( ['dflt', scriptRecord.Script.DefaultLangSys])
						for langSysRecord in langSysRecords:
							ltag = langSysRecord[0]
							langDict[ltag] = 1
							scriptDict[stag][ltag] = {}
							for featIndex in langSysRecord[1].FeatureIndex:
								featRecord = featList[featIndex]
								lstr = str(featRecord.Feature.LookupListIndex)
								if lstr not in lookupListDict:
									lookupDef = 1 + len(lookupListDict.keys())
									lookupListDict[lstr] = lookupDef
									lookupDefDict[lookupDef] = lstr
								lookupDef = lookupListDict[lstr]
								if featRecord.FeatureTag not in featDict:
									featDict[featRecord.FeatureTag] = []
								featDict[featRecord.FeatureTag].append( [stag, ltag, lookupDef])
								scriptDict[stag][ltag][featRecord.FeatureTag] = lookupDef
							if type(langSysRecord[1].ReqFeatureIndex) == type([]):
								for featIndex in langSysRecord[1].ReqFeatureIndex:
									featRecord = featList[featIndex]
									lstr = str(featRecord.Feature.LookupListIndex)
									if lstr not in lookupListDict:
										lookupListDict[lstr] = 1 + len(lookupListDict.keys())
									lookupDef = lookupListDict[lstr]
									if featRecord.FeatureTag not in featDict:
										featDict[featRecord.FeatureTag] = []
									featDict[featRecord.FeatureTag].append( [stag, ltag, lookupDef])
									scriptDict[stag][ltag][featRecord.FeatureTag] = lookupDef

				except KeyError:
					pass

				# check script and language tags.
				for tag in scriptDict.keys():
					if tag not in kKnownScriptTags:
						print("	Error: font uses unknown script tag %s. %s." % (tag, font.PostScriptName1))
				for tag in langDict.keys():
					if tag not in kKnownLanguageTags:
						print("	Error: font uses unknown language tag %s. %s." % (tag, font.PostScriptName1))

				fontDict[tableTag] = [scriptDict, featDict]
				if featDict:
						if "TUR" in langDict:
							print("	Error: font uses incorrect language tag TUR rather the TRK in table %s. %s." % (tableTag, font.PostScriptName1))
						try:
							rec = scriptDict['DFLT']['dflt']
						except:
							print("	Warning: font does not have script 'DFLT' language 'dflt' in table %s. %s." % (tableTag, font.PostScriptName1))
						try:
							rec = scriptDict['latn']['dflt']
						except:
							print("	Warning: font does not have script 'latn' language 'dflt' in table %s. %s." % (tableTag, font.PostScriptName1))

			dictList.append( [font.PostScriptName1,fontDict, lookupDefDict])

	# Group fonts with the same script-language-lookups.
	langSysDict = {}
	for entry in dictList:
		key = str(entry[1]) + str(entry[2])

		if key in langSysDict:
			langSysDict[key].append(entry)
		else:
			langSysDict[key] = [entry]

	if len(langSysDict.keys()) > 1:
		print("	Error: In GPOS/GUSB tables, the sets of lookups used by features in the script-language systems  differ between fonts.")
		print("\tThis may be intended if the faces have different charsets.")

	for value in sorted(langSysDict.values()):
		print()
		print("Lang/Sys Table for font(s): ", end=' ')
		for entry in  value:
			print(entry[0], end=' ')
		print()
		fontDict = entry[1]
		lookupDefDict = entry[2]
		for layoutTag in sorted(fontDict.keys()):
			print()
			print("\t%s Table - script:tag list." % layoutTag)
			scriptDict = fontDict[layoutTag][0]
			if not scriptDict:
				print("\t\tNo features present for table %s" % (layoutTag))
				break
			featDict = fontDict[layoutTag][1]
			scriptList = sorted(scriptDict.keys())

			# print script/language lists
			for stag in  scriptList:
				print("\t", end='')
				print(("%s:" % stag).ljust(6), end=' ')
				langList = sorted(scriptDict[stag].keys())
				for ltag in langList[:-1]:
					print(ltag.ljust(6), end=' ')
				print(langList[-1])
			print()

			# print print lookup def mappings
			print("\tlookup group ID to lookup index list:")
			for key in lookupDefDict.keys():
				print("\t", end='')
				print(("ID %s:" % key).ljust(6), end=' ')
				print(" maps to lookups %s." % lookupDefDict[key])
			print()
			# print header for feature table
			print("\t%s Table - feature lookup groups by script:tag column headers."  % layoutTag)
			print("\t(The lookup group ID assigned to each set of lookups is  an arbitrary - see list above for map to actual lookup indices.)")
			print("\t", end='')
			for stag in  scriptList:
				print(("%s:" % stag).rjust(5), end=' ')
				langList = sorted(scriptDict[stag].keys())
				for ltag in langList:
					print(ltag.ljust(5), end=' ')
			print()


			fList = sorted(featDict.keys())
			for ftag in fList:
				summary = ""
				setsDiffer = 0
				firstSet = None
				print("\t" + ftag.ljust(5), end=' ')
				lastScript = scriptList[0]
				for stag in  scriptList:
					if lastScript != stag:
						summary = summary +  " "*6
					langList = sorted(scriptDict[stag].keys())
					for ltag in langList:
						try:
							lookupSet = str(scriptDict[stag][ltag][ftag])
						except KeyError:
							lookupSet = "-"
						setText = lookupSet.ljust(6)
						if not firstSet:
							firstSet = setText
						elif (setText != firstSet):
							setsDiffer = 1
						summary  = summary + setText
				if setsDiffer:
					print(summary)
				else:
					print("<same>")

def doFamilyTest13():
	kTestBit = 1 << 8
	print("\nFamily Test 13: Check that no two faces in a preferred group have the same weight/width/Italic-style values when the OS/2 table fsSelection bit 8 (WEIGHT_WIDTH_SLOPE_ONLY) is set.")
	dictList = []
	for name in preferredFamilyList1.keys():
		fontgroup = preferredFamilyList1[name]
		groupDict = {}

		# first check that they all have the same setting for bit cmpFont.fsSelection
		firstFontBit = fontgroup[0].fsSelection & kTestBit
		numFonts = len(fontgroup)
		haveError = 0
		for i in range(numFonts)[1:]:
			font = fontgroup[i]
			if (font.fsSelection & kTestBit) != firstFontBit:
				haveError = 1
				break
		if haveError:
			print("\tError: fonts in family of %s do not all have the same setting for the WEIGHT_WIDTH_SLOPE_ONLY bit in the OS/2 selection table." % (name))
			for i in range(numFonts):
				font = fontgroup[i]
				print("\tError: for font %s, WEIGHT_WIDTH_SLOPE_ONLY bit value %s." % (font.FullFontName1, (font.fsSelection & kTestBit) != 0))
			break

		# then check that OS/2 version is 4 if bit is on.
		if firstFontBit:
			for i in range(numFonts):
				font = fontgroup[i]
				if font.OS2version < 4:
					print("\tError: font %s has WEIGHT_WIDTH_SLOPE_ONLY bit in the OS/2 selection table set, but the OS/2 table version is less than 4." % (font.FullFontName1))
					haveError = 1
		if haveError:
			break

		if not firstFontBit:
			break

		for i in range(len(fontgroup)):
			font = fontgroup[i]
			hashStr = "%s %s %s" % (font.usWidthClass, font.usWeightClass, font.isItalic)
			try:
				prevName = groupDict[hashStr]
				print("\tError: font %s has the same weight/width/Italic values as font %s in the same family group when WEIGHT_WIDTH_SLOPE_ONLY is set." % ( font.FullFontName1, prevName))
			except KeyError:
				groupDict[hashStr] = font.FullFontName1


def doFamilyTest14():
	kTestBit = 1 << 8
	print("\nFamily Test 14: Check that all faces in a preferred group have the same fsType embedding values.")
	dictList = []
	for name in preferredFamilyList1.keys():
		preferredSubFamilyList1 = preferredFamilyList1[name]
		font0 = preferredSubFamilyList1[0]
		if font0.fsType == -1:
			print("\tError: Skipping Embedding permissions report - OS/2 table does not exist in first font", font0.PostScriptName1)
			continue
		if (font0.fsType != 8) and ((not font0.trademarkStr1) or ("Adobe Systems" in font0.trademarkStr1)):
			print("\tError: fsType embedding permission '%s' for '%s' should be 8 for Adobe fonts." % (font0.fsType, font0.PostScriptName1))

		for font in preferredSubFamilyList1[1:]:
			if font.fsType == -1:
				print("	Error: Skipping font - OS/2 table does not exist in base font", font.PostScriptName1)
				continue

			if font0.fsType != font.fsType:
				print("\tError: fsType embedding permission '%s' for '%s' differs from value '%s' in '%s'." % (font.fsType, font.PostScriptName1, font0.fsType, font0.PostScriptName1 ))
			if (font.fsType != 8) and ((not font.trademarkStr1) or ("Adobe Systems" in font.trademarkStr1)):
				print("\tError: fsType embedding permission '%s' for '%s' should be 8 for Adobe fonts, aka OS/2.achVendID == Adobe." % (font.fsType, font.PostScriptName1))


def doFamilyTest15():
	print("\nFamily Test 15: Check that all faces in a preferred group have the same underline position and width.")
	for name in preferredFamilyList1.keys():
		preferredSubFamilyList1 = preferredFamilyList1[name]
		font0 = preferredSubFamilyList1[0]
		if font0.underlinePos == -1:
			print("\tError: Skipping underline report - post table does not exist in first font", font0.PostScriptName1)
			continue

		underlinePos = font0.underlinePos
		underlineThickness = font0.underlineThickness

		for font in preferredSubFamilyList1[1:]:
			if font.underlinePos == -1:
				print("\tError: Skipping font - post table does not exist in base font", font.PostScriptName1)
				continue

			if underlinePos != font.underlinePos:
				print("\tError: 'post' table underline position '%s' for '%s' differs from value '%s' in '%s'." % (font.underlinePos, font.PostScriptName1, underlinePos, font0.PostScriptName1 ))
			if underlineThickness != font.underlineThickness:
				print("\tError: 'post' table underline thickness '%s' for '%s' differs from value '%s' in '%s'." % (font.underlineThickness, font.PostScriptName1, underlineThickness, font0.PostScriptName1 ))


def doFamilyTest16():
	kWidthMultiplier = 3

	print("\nFamily Test 16: Check that for all faces in a preferred family group, that the width of any glyph is not more than %s times the width of the same glyph in any other face." % (kWidthMultiplier))
	for name in preferredFamilyList1.keys():
		# First, collect charsets.
		fontgroup = preferredFamilyList1[name]
		if len(fontgroup) == 1:
			print("\tSkipping Family Test 16: can't usefully compare glyph widths between fonts when there is only one font in a preferred family. %s." % (name))
			continue
		charsetDict = {}
		for font in fontgroup:
			charset = sorted(copy.copy(font.glyphnames))
			charset = tuple(charset)
			fontList = charsetDict.get(charset, [])
			fontList.append(font)
			charsetDict[charset] = fontList
		charsetList = sorted(charsetDict.keys())

		# Can't usefully compare between fonts when there is only 1. Add these to the end of the longest charset.
		singletonFonts = [] # list of fonts which had a unique charset.

		lastCharSet = None
		for charset in charsetList:
			fontList = charsetDict[charset]
			if len(fontList) == 1:
				singletonFonts.append(fontList[0])
				del charsetDict[charset]
				lastCharSet = charset
		if charsetDict:
			charsetList = sorted(list(charsetDict.keys()), key=len)
			longestCharset = charsetList[-1]
			charsetDict[longestCharset].extend(singletonFonts)
		else:
			charsetDict[lastCharSet] = singletonFonts
			charsetList = charsetDict.keys()

		for charset in charsetList:
			fontList = charsetDict[charset]
			for gname in charset:
				widthList = []
				for font in fontList:
					htmx_table = font.ttFont['hmtx']
					try:
						widthList.append([htmx_table.metrics[gname][0], font.PostScriptName1])
					except KeyError:
						print("\tError: Glyph '%s' is in font, but is not in the width list. This can happen when the number of glyphs in the maxp table is less than the actual number of glyphs." % (gname))
				if not widthList:
					continue
				widthList.sort()
				if (widthList[0][0]*kWidthMultiplier < widthList[-1][0]):
					print("\tWarning: width (%s) of glyph %s in %s is more than %s times the width (%s) in %s." % (widthList[-1][0], gname,  widthList[-1][1], kWidthMultiplier,widthList[0][0], widthList[0][1]))

def doFamilyTest17():
	print("\nFamily Test 17: Check that fonts have OS/2 table version 4.")
	for name in preferredFamilyList1.keys():
		fontgroup = preferredFamilyList1[name]
		for font in fontgroup:
			if font.OS2version < 4:
				print("\tWarning: The font %s should have an OS/2 table version of at least 4 instead of %s." % (font.PostScriptName1, font.OS2version))

def doFamilyTest18():
	global compatibleFamilyList3

	print("\nFamily Test 18: Check that all faces in a Compatible Family group have the same array size of BlueValues and OtherBlues within a Compatible Family Name Italic or Regular sub-group of the family.")
	# start with Compatible Family Name groups
	# divide these up into subgroups by charset
	# further divide these groups by Italic vs Roman.
	for name in compatibleFamilyList3.keys():
		fontgroup = compatibleFamilyList3[name]
		groups = []
		for font in fontgroup:
			found = 0
			for g in groups:
				if g[0].glyphnames == font.glyphnames:
					g.append(font)
					found = 1
			if found == 0:
				groups.append([font])
		for subgroup in groups:
			font = subgroup[0]
			if not font.topDict: # not CFF
				print("\t\tSkipping non-PostScript Font:", font.PostScriptName1)
				break
			# check vs bbox, and check order
			try:
				font.BlueValues = font.FDArray[0].Private.BlueValues
			except AttributeError:
				font.BlueValues = []
			try:
				font.OtherBlues = font.FDArray[0].Private.OtherBlues
			except AttributeError:
				font.OtherBlues = []

			tempBlues = copy.copy(font.BlueValues)
			if len(tempBlues) < 2:
				print("	Error: font BlueValues field has no blue values! %s." % (font.PostScriptName1))
			if (len(tempBlues) == 2) and (tempBlues == [0,0]):
				print("	Error: font BlueValues field has no blue values! %s." % (font.PostScriptName1))

			if len(tempBlues) > 14:
				print("	Error: font BlueValues has '%s' zones, but is allowed to hold at most 7. %s" % (len(tempBlues)/2, font.PostScriptName1))
			if (len(tempBlues) % 2) != 0:
				print("	Error: font BlueValues has '%s' integer values, List of values must be even as it takes a pair to describe an alignment zone. %s" % (len(tempBlues), font.PostScriptName1))
			tempBlues.sort()
			#if tempBlues != font.BlueValues: # this turns out to be OK.
			#	print("	Error: font BlueValues '%s' are not sorted in ascending order. %s." % (font.BlueValues, font.PostScriptName1))
			if tempBlues[1] < font.fontBBox[1]:
				print("	Error: font BlueValues has lowest zone (%s,%s) outside of minimum y of font bounding box '%s'. %s" % (tempBlues[0], tempBlues[1], font.fontBBox[1], font.PostScriptName1))
			if tempBlues[-2] > font.fontBBox[3]:
				print("	Error: font BlueValues has highest zone (%s,%s) outside of maximum y of font bounding box '%s'. %s" % (tempBlues[-2], tempBlues[-1], font.fontBBox[3], font.PostScriptName1))

			if font.OtherBlues and (len(tempBlues) < 2):
				tempBlues = copy.copy(font.OtherBlues)
				if len(tempBlues) > 10:
					print("	Error: font OtherBlues has '%s' zones, but is allowed to hold at most 5. %s" % (len(tempBlues)/2, font.PostScriptName1))
				if (len(tempBlues) % 2) != 0:
					print("	Error: font OtherBlues has '%s' integer values, List of values must be even as it takes a pair to describe an alignment zone. %s" % (len(tempBlues), font.PostScriptName1))
				tempBlues.sort()
				#if tempBlues != font.OtherBlues: # this turns out to be OK.
				#	print("	Error: font OtherBlues '%s' are not sorted in ascending order. %s." % (font.OtherBlues, font.PostScriptName1))
				if tempBlues[1] < font.fontBBox[1]:
					print("	Error: font OtherBlues has lowest zone (%s,%s) outside of minimum y of font bounding box '%s'. %s" % (tempBlues[0], tempBlues[1], font.fontBBox[1], font.PostScriptName1))
				if tempBlues[-2] > font.fontBBox[3]:
					print("	Error: font OtherBlues has highest zone (%s,%s) outside of maximum y of font bounding box '%s'. %s" % (tempBlues[-2], tempBlues[-1], font.fontBBox[3], font.PostScriptName1))

			for font1 in subgroup[1:]:
				try:
					font1.BlueValues = font1.FDArray[0].Private.BlueValues
				except AttributeError:
					font1.BlueValues = []
				try:
					font1.OtherBlues = font1.FDArray[0].Private.OtherBlues
				except AttributeError:
					font1.OtherBlues = []

				if len(font.BlueValues) != len(font1.BlueValues):
					print("\nError: These two fonts do not have the same array size of BlueValues for", name)
					print("\tFont 1:", font.PostScriptName1)
					print("\tBlueValues:", font.BlueValues)
					print("\tFont 2:", font1.PostScriptName1)
					print("\tBlueValues:", font1.BlueValues)

				# check vs bbox, and check order
				tempBlues = copy.copy(font1.BlueValues )
				if len(tempBlues) > 14:
					print("	Error: font BlueValues has '%s' zones, but is allowed to hold at most 7. %s" % (len(tempBlues)/2, font1.PostScriptName1))
				if (len(tempBlues) % 2) != 0:
					print("	Error: font BlueValues has '%s' integer values, List of values must be even as it takes a pair to describe an alignment zone. %s" % (len(tempBlues), font1.PostScriptName1))
				tempBlues.sort()
				#if tempBlues != font1.BlueValues: # this turns out to be OK.
				#	print("	Error: font BlueValues '%s' are not sorted in ascending order. %s." % (font1.BlueValues, font1.PostScriptName1))
				if tempBlues[1] < font1.fontBBox[1]:
					print("	Error: font BlueValues has lowest zone (%s,%s) outside of minimum y of font bounding box '%s'. %s" % (tempBlues[0], tempBlues[1], font1.fontBBox[1], font1.PostScriptName1))
				if tempBlues[-2] > font1.fontBBox[3]:
					print("	Error: font BlueValues has highest zone (%s,%s) outside of maximum y of font bounding box '%s'. %s" % (tempBlues[-2], tempBlues[-1], font1.fontBBox[3], font1.PostScriptName1))

				if len(font.OtherBlues) != len(font1.OtherBlues):
					print("\nError: These two fonts do not have the same array size of OtherBlues for", name)
					print("\tFont 1:", font.PostScriptName1)
					print("\tOtherBlues:", font.OtherBlues)
					print("\tFont 2:", font1.PostScriptName1)
					print("\tOtherBlues:", font1.OtherBlues)

				if font1.OtherBlues:
					tempBlues = copy.copy(font1.OtherBlues)
					if len(tempBlues) > 10:
						print("	Error: font OtherBlues has '%s' zones, but is allowed to hold at most 5. %s" % (len(tempBlues)/2, font1.PostScriptName1))
					if (len(tempBlues) % 2) != 0:
						print("	Error: font OtherBlues has '%s' integer values, List of values must be even as it takes a pair to describe an alignment zone. %s" % (len(tempBlues), font1.PostScriptName1))
					tempBlues.sort()
					#if tempBlues != font1.OtherBlues: # this turns out to be OK.
					#	print("	Error: font OtherBlues '%s' are not sorted in ascending order. %s." % (font1.OtherBlues, font1.PostScriptName1))
					if tempBlues[1] < font1.fontBBox[1]:
						print("	Error: font OtherBlues has lowest zone (%s,%s) outside of minimum y of font bounding box '%s'. %s" % (tempBlues[0], tempBlues[1], font1.fontBBox[1], font1.PostScriptName1))
					if tempBlues[-2] > font1.fontBBox[3]:
						print("	Error: font OtherBlues has highest zone (%s,%s) outside of maximum y of font bounding box '%s'. %s" % (tempBlues[-2],tempBlues[-1], font1.fontBBox[3], font1.PostScriptName1))

def doFamilyTest19():
	global compatibleFamilyList3

	print("\nFamily Test 19: Check that all faces in the Preferred Family group have the same values of FamilyBlues and FamilyOtherBlues, and are valid.")
	# A preferred family can contain groups of fonts that have different Family Blues; different optical sizes, for example.
	# What I will do is collect a dict of unique FamilyBlue values, and report on any that are unique, or do not contain a "regular" member".

	FBList = [ ["FamilyBlues", "BlueValues", {}, 0], ["FamilyOtherBlues", "OtherBlues",{}, 0]]

	# Set blue values
	for name in preferredFamilyList1.keys():
		fontgroup = preferredFamilyList1[name]
		for font in fontgroup:
			if not font.topDict: # not CFF
				print("\t\tSkipping non-PostScript Font:", font.PostScriptName1)
				break
			if not hasattr(font, 'BlueValues'):
				try:
					font.BlueValues = font.FDArray[0].Private.BlueValues
				except AttributeError:
					font.BlueValues = []
				try:
					font.OtherBlues = font.FDArray[0].Private.OtherBlues
				except AttributeError:
					font.OtherBlues = []

	for name in preferredFamilyList1.keys():
		seenFamilyBlues = 0
		fontgroup = preferredFamilyList1[name]
		if len(fontgroup) < 2:
			continue
		seenFamilyBlues = 0
		minYBBox = 1000
		maxYBBox = -1000
		for font in fontgroup:
			if not font.topDict: # not CFF
				print("\t\tSkipping non-PostScript Font:", font.PostScriptName1)
				break
			if font.fontBBox[1] < minYBBox:
				minYBBox = font.fontBBox[1]
			if font.fontBBox[3] > maxYBBox:
				maxYBBox = font.fontBBox[3]

			for FBEntry in FBList:
				FBKey = FBEntry[0]
				BKey = FBEntry[1]
				FBDict = FBEntry[2]
				try: # not all fonts have FamilyBlues
					testFamilyBlues = eval("font.FDArray[0].Private.%s" % (FBKey))
					testFamilyBlues = tuple(testFamilyBlues)
					seenFamilyBlues = 1
					FBEntry[3] = 1
				except AttributeError:
					try:
						testFamilyBlues = eval("font.FDArray[0].Private.%s" % (BKey)) # If a font does not have Private.FamilyBlues, it's Private.FamilyBlues are considered to be the same as it's BlueValues.
						testFamilyBlues = tuple(testFamilyBlues)
					except AttributeError:
						testFamilyBlues = ()

				bList = FBDict.get(testFamilyBlues, [])
				bList.append(font)
				FBDict[testFamilyBlues] = bList

		if (not seenFamilyBlues) and hasattr(fontgroup[0], "BlueValues"):
			blueValuesDiffer = 0
			stdBlues = fontgroup[0].BlueValues
			for font in fontgroup[1:]:
				if not font.topDict: # not CFF
					continue
				if font.BlueValues != stdBlues:
					blueValuesDiffer = 1
					break
			# if all the blue values are the same, then FamilyBlues are not needed.
			if blueValuesDiffer:
				print("	Warning. The fonts in the family group %s do not have  FamilyBlue values. This font feature helps with alignment of different styles of the same family on a text line." % (name))
			continue

		for FBEntry in FBList:
			FBKey = FBEntry[0]
			BKey = FBEntry[1]
			FBDict = FBEntry[2]
			seenFBKey = FBEntry[3]
			if not seenFBKey:
				continue

			for tempBlues in FBDict.keys():
				tempBlues = tuple(tempBlues)
				fontList = FBDict[tempBlues]
				regularFont = None
				for font in fontList:
					if tuple(eval("font.%s" % (BKey))) == tempBlues:
						regularFont = font
				if len(fontList) > 1:
					if not regularFont:
						print("	Warning: In family %s, a group of fonts have %s, but there is no base font with the same value in its %s array. This affects" % (name, FBKey, BKey))
						for font in fontList:
							print("\t\t", font.PostScriptName1)
				else:
					if tempBlues:
						print("	Error: In family %s, a font has a unique %s array. This is certainly unintended. %s." % (name, FBKey, font.PostScriptName1))
				#print FBKey, tempBlues
				#for font in fontList:
				#	print font.PostScriptName1,
				#print
				if not tempBlues:
					continue
				if len(tempBlues) > 14:
					print("	Error: In family %s, %s has '%s' zones, but is allowed to hold at most 7. This affects:" % (name, FBKey, len(tempBlues)/2))
					for font in fontList:
						print("\t\t", font.PostScriptName1)
				if (len(tempBlues) % 2) != 0:
					print("	Error: In family %s, %s has '%s' integer values. List of values must be even as it takes a pair to describe an alignment zone. This affects:" % (name, FBKey, len(tempBlues)))
					for font in fontList:
						print("\t\t", font.PostScriptName1)

				# Note: It is not at error to have Family zones outside of font bbox.


def doFamilyTest20():
	global compatibleFamilyList3

	print("\nFamily Test 20: Check that all faces in the Compatible Family group have the same BlueScale value.")
	for name in compatibleFamilyList3.keys():
		fontgroup = compatibleFamilyList3[name]
		font = fontgroup[0]
		if not font.topDict: # not CFF
			print("\t\tSkipping non-PostScript Font:", font.PostScriptName1)
			continue
		baseBlueScale = font.FDArray[0].Private.BlueScale
		for font1 in fontgroup[1:]:
			if not font1.topDict: # not CFF
				print("\t\tSkipping non-PostScript Font:", font.PostScriptName1)
				break
			testBlueScale = font1.FDArray[0].Private.BlueScale
			if baseBlueScale != testBlueScale:
				print("\nNote: These two fonts do not have the same values of BlueScale for", name)
				print("\tFont 1:", font.PostScriptName1)
				print("\tBlueScale:", baseBlueScale)
				print("\tFont 2:", font1.PostScriptName1)
				print("\tBlueScale:", testBlueScale)

def doFamilyTest21():
	global compatibleFamilyList3

	print("\nFamilyTest 21: Check that all faces in the Compatible Family group have the same BlueShift value.")
	for name in compatibleFamilyList3.keys():
		fontgroup = compatibleFamilyList3[name]
		font = fontgroup[0]
		if not font.topDict: # not CFF
			print("\t\tSkipping non-PostScript Font:", font.PostScriptName1)
			continue
		baseBlueShift = font.FDArray[0].Private.BlueShift
		for font1 in fontgroup[1:]:
			if not font1.topDict: # not CFF
				print("\t\tSkipping non-PostScript Font:", font.PostScriptName1)
				break
			testBlueShift = font1.FDArray[0].Private.BlueShift
			if baseBlueShift != testBlueShift:
				print("\nNote: These two fonts do not have the same values of BlueShift for", name)
				print("\tFont 1:", font.PostScriptName1)
				print("\tBlueShift:", baseBlueShift)
				print("\tFont 2:", font1.PostScriptName1)
				print("\tBlueShift:", testBlueShift)

def doFamilyTests(singleTestList, familyTestList):
	global compatibleFamilyList3
	# build a group of fonts with the same nameID 1(Win, Unicode, English) for later use in individual Family Test
	for font in fontlist:
		if font.compatibleFamilyName3 in compatibleFamilyList3:
			compatibleSubFamilyList3 = compatibleFamilyList3[font.compatibleFamilyName3]
		else:
			compatibleSubFamilyList3 = []
			compatibleFamilyList3[font.compatibleFamilyName3] = compatibleSubFamilyList3
		compatibleSubFamilyList3.append(font)
	if (singleTestList or familyTestList):
		if familyTestList:
			g = globals()
			for val in familyTestList:
				funcName = "doFamilyTest%s" % (val)
				if funcName not in g:
					print("Error: cannot find function '%s' to execute." % (funcName))
					continue
				exec("%s()" % (funcName))
	else:
		doFamilyTest1()
		doFamilyTest2()
		doFamilyTest3()
		doFamilyTest4()
		doFamilyTest5()
		doFamilyTest6()
		doFamilyTest7()
		doFamilyTest8()
		doFamilyTest9()
		doFamilyTest10()
		doFamilyTest11()
		doFamilyTest12()
		doFamilyTest13()
		doFamilyTest14()
		doFamilyTest15()
		doFamilyTest16()
		doFamilyTest17()
		if ('CFF ' in fontlist[0].ttFont.keys()) and (not fontlist[0].isCID):
			doFamilyTest18()
			doFamilyTest19()
			doFamilyTest20()
			doFamilyTest21()

def build_preferredFamilyList(fontlist):
	global preferredFamilyList1

	preferredFamilyList1 = {}
	for font in fontlist:
		if font.preferredFamilyName1 in preferredFamilyList1:
			preferredSubFamilyList1 = preferredFamilyList1[font.preferredFamilyName1]
		else:
			preferredSubFamilyList1 = []
			preferredFamilyList1[font.preferredFamilyName1] = preferredSubFamilyList1
		preferredSubFamilyList1.append(font)


def print_menu_name_report():
	global preferredFamilyList1

	print("\nMenu Name Report:")
	for name in preferredFamilyList1.keys():
		preferredSubFamilyList1 = preferredFamilyList1[name]
		print("\nPreferred Menu							  Mac Compatible Menu				 Windows Compatible Menu")
		for font in preferredSubFamilyList1:
			print((font.preferredFamilyName3+"/"+font.preferredSubFamilyName3).ljust(40),
				font.MacCompatibleFullName1.ljust(34),
				font.compatibleFamilyName3+"/"+font.compatibleSubFamilyName3)



def print_font_metric_report():
	global preferredFamilyList1

	print("\nFONT METRICS REPORT")
	print("Report 1:", end=' ')
	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
		print("Num glyphs from maxp.numGlyphs:")
		font0 = preferredSubFamilyList1[0]
		diff = 0
		for font in preferredSubFamilyList1[1:]:
			if font0.numGlyphs != font.numGlyphs:
				diff = 1
		if diff == 0:
			print("All fonts have the same value:", font0.numGlyphs)
			continue
		for font in sort_by_numGlyphs_and_PostScriptName1(preferredSubFamilyList1):
			print(("%s:" % font.PostScriptName1).ljust(40), font.numGlyphs)

	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
		font0 = preferredSubFamilyList1[0]
		diff = 0

		if font0.sTypoAscender == -1:
			print("Skipping ascender/descender/linegap report - OS/2 table does not exist", font0.PostScriptName1)
		else:
			for font in preferredSubFamilyList1[1:]:
				if font.sTypoAscender == -1:
					print("Skipping font - OS/2 table does not exist", font.PostScriptName1)
					diff = 1
					continue

				if (font0.sCapHeight != -1) and ((font0.sxHeight!= -1) ):
					if font0.sTypoAscender != font.sTypoAscender or font0.sTypoDescender != font.sTypoDescender or font0.sTypoLineGap != font.sTypoLineGap or font0.sCapHeight != font.sCapHeight or font0.sxHeight != font.sxHeight:
						diff = 1

			if diff == 0:
				print("OS/2 sTypoAscender/sTypoDescender/sTypoLineGap/sCapHeight/sxHeight")
				print("All fonts have the same value:", end=' ')
				print("%5d%5d%5d%5d%5d" % (font0.sTypoAscender, font0.sTypoDescender, font0.sTypoLineGap, font0.sCapHeight, font0.sxHeight))
				continue
			print("Font									TypoAscent/TypoDescent/LineGap/CapHeight/sxHeight")
			for font in preferredSubFamilyList1:
				print("%40s%5d%8d%8d%8d%8d" % (("%s:" % font.PostScriptName1).ljust(40), font.sTypoAscender, font.sTypoDescender, font.sTypoLineGap, font.sCapHeight, font.sxHeight))


	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
		print("Italic Angle from post.italicAngle:")
		font0 = preferredSubFamilyList1[0]
		diff = 0
		for font in preferredSubFamilyList1[1:]:
			if font0.italicAngle != font.italicAngle:
				diff = 1
		if diff == 0:
			print("All fonts have the same value:", "%7.3f" % font0.italicAngle)
			continue
		for font in sort_angle(preferredSubFamilyList1):
			print(("%s:" % font.PostScriptName1).ljust(40), "%7.3f" % font.italicAngle)


	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
		print("isBold from hhea.mscStyle:")
		table = ["Not Bold", "	  Bold"]
		font0 = preferredSubFamilyList1[0]
		diff = 0
		for font in preferredSubFamilyList1[1:]:
			if font0.isBold != font.isBold:
				diff = 1
		if diff == 0:
			print("All fonts have the same value:", table[font0.isBold])
			continue
		boldlist = []
		notboldlist = []
		for font in preferredSubFamilyList1:
			if font.isBold:
				boldlist.append(font)
			else:
				notboldlist.append(font)
		for font in boldlist:
			print(("%s:" % font.PostScriptName1).ljust(40), table[font.isBold])
		for font in notboldlist:
			print(("%s:" % font.PostScriptName1).ljust(40), table[font.isBold])

	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
		print("isItalic from hhea.mscStyle:")
		table = ["Not Italic", "	Italic"]
		font0 = preferredSubFamilyList1[0]
		diff = 0
		for font in preferredSubFamilyList1[1:]:
			if font0.isItalic != font.isItalic:
				diff = 1
		if diff == 0:
			print("All fonts have the same value:", table[font0.isItalic])
			continue
		italiclist = []
		notitaliclist = []
		for font in preferredSubFamilyList1:
			if font.isItalic:
				italiclist.append(font)
			else:
				notitaliclist.append(font)
		for font in italiclist:
			print(("%s:" % font.PostScriptName1).ljust(40), table[font.isItalic])
		for font in notitaliclist:
			print(("%s:" % font.PostScriptName1).ljust(40), table[font.isItalic])

	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
		print("OTF version from head.fontRevision:")
		font0 = preferredSubFamilyList1[0]
		diff = 0
		for font in preferredSubFamilyList1[1:]:
			if font0.OTFVersion != font.OTFVersion:
				diff = 1
		if diff == 0:
			print("All fonts have the same value:", "%5.3f" % font0.OTFVersion)
			continue
		for font in sort_version(preferredSubFamilyList1):
			print(("%s:" % font.PostScriptName1).ljust(40), "%5.3f" % font.OTFVersion)

	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
		print("Embedding permissions setting from OS/2.fsType:")
		font0 = preferredSubFamilyList1[0]
		if font0.fsType == -1:
			print("Skipping Embedding permissions report - OS/2 table does not exist in base font", font0.PostScriptName1)
		else:

			diff = 0
			for font in preferredSubFamilyList1[1:]:
				if font.fsType == -1:
					print("Skipping font - OS/2 table does not exist in base font", font.PostScriptName1)
					diff = 1
					continue

				if font0.fsType != font.fsType:
					diff = 1
			if diff == 0:
				print("All fonts have the same value:", "0x%04x" % font0.fsType, end=' ')
				if font0.fsType == 0:
					print("'Installable embedding'", end=' ')
				if font0.fsType & 2:
					print("'restricted license embedding'", end=' ')
				if font0.fsType & 4:
					print("'print and preview'", end=' ')
				if font0.fsType & 8:
					print("'editable embedding'", end=' ')
				if font0.fsType & 0x0100:
					print("'no subsetting'", end=' ')
				if font0.fsType & 0x0200:
					print("'bitmapping embedding only'", end=' ')
				print("")
				continue
			for font in preferredSubFamilyList1:
				print(("%s:" % font.PostScriptName1).ljust(40), "0x%04x" % font.fsType, end=' ')
				if font.fsType == 0:
					print("'Installable embedding'", end=' ')
				if font.fsType & 2:
					print("'restricted license embedding'", end=' ')
				if font.fsType & 4:
					print("'print and preview'", end=' ')
				if font.fsType & 8:
					print("'editable embedding'", end=' ')
				if font.fsType & 0x0100:
					print("'no subsetting'", end=' ')
				if font.fsType & 0x0200:
					print("'bitmapping embedding only'", end=' ')
				print("")

	print("\nReport 2:")
	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
		print("FontBBox from head.XY min/max:\t\tX Min, YMin, XMax, YMax")
		for font in preferredSubFamilyList1:
			print(("%s:" % font.PostScriptName1).ljust(40), font.fontBBox)

	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
		print("usWeightClass from OS/2.usWeightClass:")
		font0 = preferredSubFamilyList1[0]
		diff = 0
		if font0.usWeightClass == -1:
			print("Skipping weightClass - OS/2 table does not exist in base font", font0.PostScriptName1)
		else:
			for font in preferredSubFamilyList1[1:]:
				if font.usWeightClass == -1:
					print("Skipping font - OS/2 table does not exist in base font", font.PostScriptName1)
					diff = 1
					continue

				if font0.usWeightClass != font.usWeightClass:
					diff = 1
			if diff == 0:
				print("All fonts have the same value:", font0.usWeightClass, end=' ')
				if font0.usWeightClass == 100:
					print("Thin")
				elif font0.usWeightClass == 200:
					print("Extra-Light (Ultra-light)")
				elif font0.usWeightClass == 300:
					print("Light")
				elif font0.usWeightClass == 400:
					print("Normal (Regular)")
				elif font0.usWeightClass == 500:
					print("Medium")
				elif font0.usWeightClass == 600:
					print("Semi-bold (Demi-bold)")
				elif font0.usWeightClass == 700:
					print("Bold")
				elif font0.usWeightClass == 800:
					print("Extra-Bold (Ultra-bold)")
				elif font0.usWeightClass == 900:
					print("Black (Heavy)")
				elif (font0.usWeightClass > 100) and (font0.usWeightClass < 200):
					print("Thin to Extra-Light (Ultra-light)")
				elif (font0.usWeightClass > 200) and (font0.usWeightClass < 300):
					print("Extra-Light (Ultra-light) to Light")
				elif (font0.usWeightClass > 300) and (font0.usWeightClass < 400):
					print("Light to Normal (Regular)")
				elif (font0.usWeightClass > 400) and (font0.usWeightClass < 500):
					print("Normal (Regular) to Medium")
				elif (font0.usWeightClass > 500) and (font0.usWeightClass < 600):
					print("Medium to Semi-bold (Demi-bold)")
				elif (font0.usWeightClass > 600) and (font0.usWeightClass < 700):
					print("Semi-bold (Demi-bold) to Bold")
				elif (font0.usWeightClass > 700) and (font0.usWeightClass < 800):
					print("Bold to Extra-bold (Ultra-bold)")
				elif (font0.usWeightClass > 800) and (font0.usWeightClass < 900):
					print("Extra-bold (Ultra-bold) to Black (Heavy)")
				else:
					print("value out of range")
				continue
			for font in sort_weight(preferredSubFamilyList1):
				print(("%s:" % font.PostScriptName1).ljust(40), "%3d" % font.usWeightClass, end=' ')
				if font.usWeightClass == 100:
					print("Thin")
				elif font.usWeightClass == 200:
					print("Extra-Light (Ultra-light)")
				elif font.usWeightClass == 300:
					print("Light")
				elif font.usWeightClass == 400:
					print("Normal (Regular)")
				elif font.usWeightClass == 500:
					print("Medium")
				elif font.usWeightClass == 600:
					print("Semi-bold (Demi-bold)")
				elif font.usWeightClass == 700:
					print("Bold")
				elif font.usWeightClass == 800:
					print("Extra-Bold (Ultra-bold)")
				elif font.usWeightClass == 900:
					print("Black (Heavy)")
				elif (font.usWeightClass > 100) and (font.usWeightClass < 200):
					print("Thin to Extra-Light (Ultra-light)")
				elif (font.usWeightClass > 200) and (font.usWeightClass < 300):
					print("Extra-Light (Ultra-light) to Light")
				elif (font.usWeightClass > 300) and (font.usWeightClass < 400):
					print("Light to Normal (Regular)")
				elif (font.usWeightClass > 400) and (font.usWeightClass < 500):
					print("Normal (Regular) to Medium")
				elif (font.usWeightClass > 500) and (font.usWeightClass < 600):
					print("Medium to Semi-bold (Demi-bold)")
				elif (font.usWeightClass > 600) and (font.usWeightClass < 700):
					print("Semi-bold (Demi-bold) to Bold")
				elif (font.usWeightClass > 700) and (font.usWeightClass < 800):
					print("Bold to Extra-bold (Ultra-bold)")
				elif (font.usWeightClass > 800) and (font.usWeightClass < 900):
					print("Extra-bold (Ultra-bold) to Black (Heavy)")
				else:
					print("value out of range")

	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
		widthClassTable = ['Ultra-condensed', 'Extra-condensed', 'Condensed', 'Semi-condensed', 'Medium (normal)', 'Semi-expanded', 'Expanded', 'Extra-expanded', 'Ultra-expanded']
		print("usWidthClass from OS/2.usWidthClass:")
		font0 = preferredSubFamilyList1[0]
		diff = 0
		if font0.usWidthClass == -1:
			print("Skipping WidthClass report - OS/2 table does not exist in base font", font0.PostScriptName1)
		else:
			for font in preferredSubFamilyList1[1:]:
				if font.usWidthClass == -1:
					print("Skipping font - OS/2 table does not exist in base font", font.PostScriptName1)
					diff = 1
					continue
				if font0.usWidthClass != font.usWidthClass:
					diff = 1
			if diff == 0:
				print("All fonts have the same value:", font0.usWidthClass, widthClassTable[font0.usWidthClass-1])
				continue
			for font in sort_width(preferredSubFamilyList1):
				print(("%s:" % font.PostScriptName1).ljust(40), font.usWidthClass, widthClassTable[font.usWidthClass-1])

	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
		print("underlinePosition from post.underlinePosition:")
		font0 = preferredSubFamilyList1[0]
		diff = 0
		for font in preferredSubFamilyList1[1:]:
			if font0.underlinePos != font.underlinePos:
				diff = 1
		if diff == 0:
			print("All fonts have the same value:", font0.underlinePos)
			continue
		for font in sort_underline(preferredSubFamilyList1):
			print(("%s:" % font.PostScriptName1).ljust(40), font.underlinePos)

	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
		print("Vendor ID from OS/2.achVendID:")
		font0 = preferredSubFamilyList1[0]
		diff = 0
		if font0.usWidthClass == -1:
			print("Skipping vendor ID report - OS/2 table does not exist in base font", font0.PostScriptName1)
		else:
			for font in preferredSubFamilyList1[1:]:
				if font.usWidthClass == -1:
					print("Skipping font - OS/2 table does not exist in base font", font.PostScriptName1)
					diff = 1
					continue
				if font0.achVendID != font.achVendID:
					diff = 1
			if diff == 0:
				print("All fonts have the same value:", font0.achVendID)
				continue
			for font in preferredSubFamilyList1:
				print(("%s:" % font.PostScriptName1).ljust(40), font.achVendID)

	print("\nReport 3:Copyright and Trademark strings for the first face in the group")
	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
		font = preferredSubFamilyList1[0]

		cprgt = font.copyrightStr1
		console_encoding = sys.stdout.encoding
		if not isinstance(cprgt, str):
			# font.copyrightStr1 is bytes
			cprgt = font.copyrightStr1.encode('utf-8')

		elif console_encoding and console_encoding != 'UTF-8':
			# The console's encoding is set and is different from 'UTF-8'.
			# Convert the string to bytes, then decode using the console's encoding
			cprgt = cprgt.encode('utf-8').decode(console_encoding)

		print("First Face:", font.PostScriptName1)
		print("Copyright: ", cprgt)
		print("Trademark: ", font.trademarkStr1)

def print_panose_report():
	global preferredFamilyList1


	print("\nPanose Report:")

	familykind = ["Any","No Fit","Latin Text","Latin Hand Written","Latin Decorative","Latin Symbol"]
	kind2 = [familykind]
	kind2.append(["Any","No Fit","Cove","Obtuse Cove","Square Cove","Obtuse Square Cove","Square","Thin","Oval","Exaggerated","Triangle","Normal Sans","Obtuse Sans","Perpendicular Sans","Flared","Rounded"])
	kind2.append(["Any","No Fit","Very Light","Light","Thin","Book","Medium","Demi","Bold","Heavy","Black","Extra Black"])
	kind2.append(["Any","No Fit","Old Style","Modern","Even Width","Extended","Condensed","Very Extended","Very Condensed","Monospaced"])
	kind2.append(["Any","No Fit","None","Very Low","Low","Medium Low","Medium","Medium High","High","Very High"])
	kind2.append(["Any","No Fit","No Variation","Gradual/Diagonal","Gradual/Transitional","Gradual/Vertical","Gradual/Horizontal","Rapid/Vertical","Rapid/Horizontal","Instant/Vertical","Instant/Horizontal"])
	kind2.append(["Any","No Fit","Straight Arms/Horizontal","Straight Arms/Wedge","Straight Arms/Vertical","Straight Arms/Single Serif","Straight Arms/Double Serif","Non-Straight/Horizontal","Non-Straight/Wedge","Non-Straight/Vertical","Non-Straight/Single Serif","Non-Straight/Double Serif"])
	kind2.append(["Any","No Fit","Normal/Contact","Normal/Weighted","Normal/Boxed","Normal/Flattened","Normal/Rounded","Normal/Off Center","Normal/Square","Oblique/Contact","Oblique/Weighted","Oblique/Boxed","Oblique/Flattened","Oblique/Rounded","Oblique/Off Center","Oblique/Square"])
	kind2.append(["Any","No Fit","Standard/Trimmed","Standard/Pointed","Standard/Serifed","High/Trimmed","High/Pointed","High/Serifed","Constant/Trimmed","Constant/Pointed","Constant/Serifed","Low/Trimmed","Low/Pointed","Low/Serifed"])
	kind2.append(["Any","No Fit","Constant/Small","Constant/Standard","Constant/Large","Ducking/Small","Ducking/Standard","Ducking/Large"])
	field2 = ["Family Kind","Serif Style","Weight","Proportion","Contrast","Stroke Variation","Arm Style","Letterform","Midline","X-height"]
	kind3 = [familykind]
	kind3.append(["Any","No Fit","Flat Nib","Pressure Point","Engraved","Ball (Round Cap)","Brush","Rough","Felt Pen/Brush Tip","Wild Brush - Drips a lot"])
	kind3.append(["Any","No Fit","Very Light","Light","Thin","Book","Medium","Demi","Bold","Heavy","Black","Extra Black (Nord)"])
	kind3.append(["Any","No Fit","Proportional Spaced","Monospaced"])
	kind3.append(["Any","No Fit","Very Condensed","Condensed","Normal","Expanded","Very Expanded"])
	kind3.append(["Any","No Fit","None","Very Low","Low","Medium Low","Medium","Medium High","High","Very High"])
	kind3.append(["Any","No Fit","Roman Disconnected","Roman Trailing","Roman Connected","Cursive Disconnected","Cursive Trailing","Cursive Connected","Blackletter Disconnected","Blackletter Trailing","Blackletter Connected"])
	kind3.append(["Any","No Fit","Upright / No Wrapping","Upright / Some Wrapping","Upright / More Wrapping","Upright / Extreme Wrapping","Oblique / No Wrapping","Oblique / Some Wrapping","Oblique / More Wrapping","Oblique / Extreme Wrapping","Exaggerated / No Wrapping","Exaggerated / Some Wrapping","Exaggerated / More Wrapping","Exaggerated / Extreme Wrapping"])
	kind3.append(["Any","No Fit","None / No loops","None / Closed loops","None / Open loops","Sharp / No loops","Sharp / Closed loops","Sharp / Open loops","Tapered / No loops","Tapered / Closed loops","Tapered / Open loops","Round / No loops","Round / Closed loops","Round / Open loops"])
	kind3.append(["Any","No Fit","Very Low","Low","Medium","High","Very High"])
	field3 = ["Family Kind","Tool kind","Weight","Spacing","Aspect Ratio","Contrast","Topology","Form","Finials","X-Ascent"]
	kind4 = [familykind]
	kind4.append(["Any","No Fit","Derivative","Non-standard Topology","Non-standard Elements","Non-standard Aspect","Initials","Cartoon","Picture Stems","Ornamented","Text and Background","Collage","Montage"])
	kind4.append(["Any","No Fit","Very Light","Light","Thin","Book","Medium","Demi","Bold","Heavy","Black","Extra Black"])
	kind4.append(["Any","No fit","Super Condensed","Very Condensed","Condensed","Normal","Extended","Very Extended","Super Extended","Monospaced"])
	kind4.append(["Any","No Fit","None","Very Low","Low","Medium Low","Medium","Medium High","High","Very High","Horizontal Low","Horizontal Medium","Horizontal High","Broken"])
	kind4.append(["Any","No Fit","Cove","Obtuse Cove","Square Cove","Obtuse Square Cove","Square","Thin","Oval","Exaggerated","Triangle","Normal Sans","Obtuse Sans","Perpendicular Sans","Flared","Rounded","Script"])
	kind4.append(["Any","No Fit","None - Standard Solid Fill","White / No Fill","Patterned Fill","Complex Fill","Shaped Fill","Drawn / Distressed"])
	kind4.append(["Any","No Fit","None","Inline","Outline","Engraved (Multiple Lines)","Shadow","Relief","Backdrop"])
	kind4.append(["Any","No Fit","Standard","Square","Multiple Segment","Deco (E,M,S) Waco midlines","Uneven Weighting","Diverse Arms","Diverse Forms","Lombardic Forms","Upper Case in Lower Case","Implied Topology","Horseshoe E and A","Cursive","Blackletter","Swash Variance"])
	kind4.append(["Any","No Fit","Extended Collection","Litterals","No Lower Case","Small Caps"])
	field4 = ["Family Kind","Class","Weight","Aspect","Contrast","Serif Variant","Treatment","Lining","Topology","Range of Characters"]
	kind5 = [familykind]
	kind5.append(["Any","No Fit","Montages","Pictures","Shapes","Scientific","Music","Expert","Patterns","Boarders","Icons","Logos","Industry specific"])
	kind5.append(["Any","No Fit"])
	kind5.append(["Any","No Fit","Proportional Spaced","Monospaced"])
	kind5.append(["Any","No Fit"])
	kind5.append(["Any","No Fit","No Width","Exceptionally Wide","Super Wide","Very Wide","Wide","Normal","Narrow","Very Narrow"])
	kind5.append(["Any","No Fit","No Width","Exceptionally Wide","Super Wide","Very Wide","Wide","Normal","Narrow","Very Narrow"])
	kind5.append(["Any","No Fit","No Width","Exceptionally Wide","Super Wide","Very Wide","Wide","Normal","Narrow","Very Narrow"])
	kind5.append(["Any","No Fit","No Width","Exceptionally Wide","Super Wide","Very Wide","Wide","Normal","Narrow","Very Narrow"])
	kind5.append(["Any","No Fit","No Width","Exceptionally Wide","Super Wide","Very Wide","Wide","Normal","Narrow","Very Narrow"])
	field5 = ["Family Kind","Kind","Weight","Spacing","Aspect ratio & contrast","Aspect ratio of character 94","Aspect ratio of character 119","Aspect ratio of character 157","Aspect ratio of character 163","Aspect ratio of character 211"]
	fieldlist = ["Any", "No Fit", field2, field3, field4, field5]

	kindlist = ["Any", "No Fit", kind2, kind3, kind4, kind5]
	for name in preferredFamilyList1.keys():
		print("\nPreferred Family:", name)
		preferredSubFamilyList1 = preferredFamilyList1[name]
	# find base font which has usWeightClass 400 (regular), usWidthClass 5 (regular) and not italic.
		baselist = []
		for font in preferredSubFamilyList1:
			if font.usWeightClass == 400 and font.usWidthClass == 5 and not font.isItalic:
				baselist.append(font)
		if baselist != []:
	# set base font to the one with "Regular" Preferred Family Name. If "Regular" is not found, set base font to the first one in list.
			font0 = baselist[0]
			for font in baselist:
				if font.preferredSubFamilyName1 == "Regular":
					font0 = font
		else:
			font0 = preferredSubFamilyList1[0]
		print(font0.PostScriptName1.ljust(36), end=' ')
		panose0 = font0.panose
		if not panose0:
			print("Skipping Panose report - OS/2 table does not exist in base font.", font0.PostScriptName1)
			continue

		for p in panose0:
			print("%3d" % p, end=' ')
		print("")
		kind = kindlist[panose0[0]]
		field = fieldlist[panose0[0]]
		if panose0[0] < 2 or panose0[0] > 5:
			print("%5d" % panose0[0], kind)
			continue
		for i in range(10):
			subkind = kind[i]
			fieldValue = panose0[i]
			try:
				subkindText = subkind[fieldValue]
			except:
				subkindText = "Error: Panose field value out of range"
			print("\t" + field[i].ljust(20) + "%5d" % fieldValue, subkindText)
		for font in preferredSubFamilyList1:
			if font == font0:
				continue
			print(font.PostScriptName1.ljust(36), end=' ')
			panose = font.panose
			for p in panose:
				print("%3d" % p, end=' ')
			print("")
			kind = kindlist[panose[0]]
			field = fieldlist[panose[0]]
			if panose[0] < 2 or panose[0] > 5:
				print("\t" + "Any".ljust(20) + "%5d" % panose[0], kind)
				continue
			for i in range(10):
				subkind = kind[i]
				fieldValue = panose[i]
				try:
					subkindText = subkind[fieldValue]
				except:
					subkindText = "Error: Panose field value out of range"
				if panose[i] != panose0[i]:
					print("\t" + field[i].ljust(20) + "%5d" % fieldValue, subkindText)



def print_std_charset_report(charSetName):
	charsetList = sorted(StdCharSets.CharSetDict.keys())

	if charSetName:
		charsetList = [ charSetName ]

	print("\tCharset Report: Note Yet Implemented.")




def read_options():
	global gDesignSpaceTolerance
	directory = logfilename = charSetName = ""
	nreport = mreport = preport = creport  = doFeatureReportOnly = 0
	doStemWidthChecks = 1
	singleTestList = []
	familyTestList = []
	flags = ['-h', '-u', '-d', '-rn', '-rm', '-rp', '-l', '-nohints', '-rc', '-rf', '-st', '-ft', '-tolerance',]
	i = 1
	directory = os.curdir # default
	while i < (len(sys.argv)):
		try:
			option = flags.index(sys.argv[i])
		except ValueError:
			print("Error. did not recognize option '%s'." % (sys.argv[i]))
			print(__usage__)
			sys.exit()
		else:
			if option == 1:
				print(__usage__)
				sys.exit()
			elif option == 0:
				print(__help__)
				sys.exit()
			elif option == 2:
				i = i + 1
				directory = sys.argv[i]
			elif option == 3:
				nreport = 1
			elif option == 4:
				mreport = 1
			elif option == 5:
				preport = 1
			elif option == 6:
				i = i + 1
				logfilename = sys.argv[i]
			elif option == 7:
				doStemWidthChecks = 0
			elif option == 8:
				creport = 1
				try:
					nextArg =  sys.argv[i+1]
					if nextArg[0] != "-":
						i = i + 1
						charSetName = sys.argv[i]
						charsetList =  StdCharSets.CharSetDict.keys()
						if charSetName and (charSetName not in charsetList):
							print("Charset name", charSetName,"is not in the known list of charsets:")
							for name in charsetList:
								print("\t" + name)
							sys.exit(2)
				except IndexError:
					pass

			elif option == 9:
				doFeatureReportOnly = 1
			elif option == 10:
				i = i + 1
				singleTestList = sys.argv[i].split(",")
			elif option == 11:
				i = i + 1
				familyTestList = sys.argv[i].split(",")
			elif option == 12:
				i = i + 1
				try:
					gDesignSpaceTolerance = abs(int(sys.argv[i]))
				except:
					print("Error: argument for option '-tolerance' must be an integer design space value.")
					sys.exit()
			else:
				print("Argument '%s'  is not recognized." % (sys.argv[i]))
				print(__usage__)
				sys.exit()
			i = i + 1
	return directory, nreport, mreport, preport, logfilename, creport, charSetName, doStemWidthChecks, doFeatureReportOnly, singleTestList, familyTestList


def main():
	global directory, nreport, mreport, preport, creport, logfilename, doStemWidthChecks
	global fontlist
	global preferredFamilyList1, compatibleFamilyList3
	global gAGDDict
	directory = logfilename = charSetName = ""
	nreport = mreport = preport = creport  = doStemWidthChecks = 0
	preferredFamilyList1 = {}
	compatibleFamilyList3 = {}
	fontlist = []
	directory, nreport, mreport, preport, logfilename, \
		creport, charSetName, doStemWidthChecks, doFeatureReportOnly, singleTestList, familyTestList = read_options()
	if directory != "":
			build_fontlist_from_dir(directory)
	else:
			print("No directory available for processing.")
			sys.exit(0)

	if not fontlist:
			print("No font available for processing.")
			sys.exit(0)

	try:
			if logfilename != "":
					logfile = open(logfilename, "w")
					sys.stdout = logfile
	except IOError:
			print("Invalid log file name")
			logfile = ""

	print("Directory:", os.path.abspath(directory))
	print("Loading Adobe Glyph Dict...")
	from afdko import agd
	resources_dir = fdkutils.get_resources_dir()
	kAGD_TXTPath = os.path.join(resources_dir, "AGD.txt")
	fp = open(kAGD_TXTPath, "rU")
	agdTextPath = fp.read()
	fp.close()
	gAGDDict = agd.dictionary(agdTextPath)

	print("building lists of preferred families...")
	fontlist = sort_font(fontlist)
	build_preferredFamilyList(fontlist)
	print("Number of preferred families:", len(preferredFamilyList1.keys()), "[%s]" % ', '.join(["'%s'" % name for name in preferredFamilyList1.keys()]))
	print("Number of font faces:", len(fontlist))
	print()
	if doFeatureReportOnly:
		doFamilyTest12()
		return
	doSingleFaceTests(singleTestList, familyTestList)
	doFamilyTests(singleTestList, familyTestList)
	if nreport:
			print_menu_name_report()
	if mreport:
			print_font_metric_report()
	if preport:
			print_panose_report()

	if creport:
			print_std_charset_report(charSetName)



if __name__=='__main__':
	main()
