# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Changed:    %G% %U%
#########################################################################

BIN_EXEC_PROGRAM_INSTALL = spot
BIN_EXEC_PROGRAM_SET = \
	global.c main.c opt.c res.c sfnt.c fdsc.c feat.c BLND.c fvar.c MMVR.c \
	TYP1.c name.c cmap.c head.c hhea.c hmtx.c maxp.c post.c hdmx.c LTSH.c \
	trak.c OS_2.c HFMX.c ENCO.c FNAM.c loca.c glyf.c da.c WDTH.c BBOX.c \
	CID_.c GLOB.c kern.c sys.c file.c desc.c gasp.c tto.c GSUB.c BASE.c \
	vhea.c vmtx.c proof.c CFF_.c GPOS.c cffread.c pathbuild.c EBLC.c MMFX.c dump.c map.c

CFLAGS += -g -Wall -ansi -DSUNOS=1 -DAUTOSPOOL=0 -D__USE_FIXED_PROTOTYPES__ -DCFF_T13_SUPPORT
LDLIBS = -lm
INC_DIRS = -I..

include $(MAKE_RULES)

# common dependencies
F=../sfntfmt
G=global.h file.h

$(BE)/BBOX.o: $G BBOX.h $F/BBOX.h
$(BE)/BLND.o: $G BLND.h $F/BLND.h sfnt.h opt.h
$(BE)/CID_.o: $G CID_.h $F/CID_.h sfnt.h opt.h
$(BE)/ENCO.o: $G ENCO.h $F/ENCO.h FNAM.h
$(BE)/FNAM.o: $G FNAM.h $F/FNAM.h sfnt.h opt.h
$(BE)/GLOB.o: $G GLOB.h $F/GLOB.h
$(BE)/HFMX.o: $G HFMX.h $F/HFMX.h BLND.h
$(BE)/LTSH.o: $G LTSH.h $F/LTSH.h
$(BE)/MMVR.o: $G MMVR.h $F/MMVR.h
$(BE)/OS_2.o: $G OS_2.h $F/OS_2.h
$(BE)/TYP1.o: $G TYP1.h $F/TYP1.h sfnt.h opt.h
$(BE)/WDTH.o: $G WDTH.h $F/WDTH.h
$(BE)/cffread.o: cffread.c cffread.h stdstr1.h txops.h isocs0.h exsubcs0.h exenc0.h excs0.h dictops.h stdenc0.h t13supp.c
$(BE)/cmap.o: $G cmap.h $F/cmap.h $F/unicode.h post.h
$(BE)/da.o: $G da.h
$(BE)/fdsc.o: $G fdsc.h $F/fdsc.h
$(BE)/feat.o: $G feat.h $F/feat.h
$(BE)/fvar.o: $G fvar.h $F/fvar.h
$(BE)/global.o: $G opt.h sfnt.h maxp.h CID_.h map.h
$(BE)/glyf.o: $G glyf.h opt.h $F/glyf.h maxp.h loca.h head.h hmtx.h post.h da.h proof.h
$(BE)/hdmx.o: $G hdmx.h $F/hdmx.h maxp.h
$(BE)/head.o: $G head.h $F/head.h sfnt.h opt.h
$(BE)/hhea.o: $G hhea.h $F/hhea.h sfnt.h opt.h
$(BE)/hmtx.o: $G hmtx.h $F/hmtx.h hhea.h maxp.h sfnt.h opt.h
$(BE)/kern.o: $G kern.h $F/kern.h BLND.h sfnt.h post.h
$(BE)/loca.o: $G loca.h $F/loca.h maxp.h head.h sfnt.h opt.h
$(BE)/main.o: $G sfnt.h opt.h $F/sfnt.h res.h glyf.h da.h
$(BE)/maxp.o: $G maxp.h $F/maxp.h sfnt.h opt.h
$(BE)/mort.o: $G mort.h $F/mort.h
$(BE)/name.o: $G name.h $F/name.h
$(BE)/opt.o: $G opt.h
$(BE)/post.o: $G post.h $F/post.h maxp.h sfnt.h opt.h $F/applestd.h CID_.h
$(BE)/res.o: $G res.h opt.h opt.h sfnt.h
$(BE)/sfnt.o: $G sfnt.h opt.h $F/sfnt.h da.h BBOX.h BLND.h \
	CID_.h ENCO.h FNAM.h HFMX.h LTSH.h MMVR.h OS_2.h TYP1.h \
	WDTH.h cmap.h fdsc.h feat.h fvar.h glyf.h hdmx.h head.h \
	hhea.h hmtx.h kern.h loca.h maxp.h name.h post.h trak.h
$(BE)/trak.o: $G trak.h $F/trak.h
$(BE)/sys.o: $G sys.h
$(BE)/file.o: $G file.h sys.h
$(BE)/desc.o: $G desc.h
$(BE)/gasp.o: $G $F/gasp.h gasp.h
$(BE)/tto.o: $G $F/tto.h
$(BE)/GSUB.o: $G tto.h $F/GSUB.h $F/tto.h GSUB.h
$(BE)/BASE.o: $G tto.h $F/BASE.h $F/tto.h BASE.h
$(BE)/vhea.o: $G vhea.h $F/vhea.h sfnt.h
$(BE)/vmtx.o: $G vmtx.h $F/vmtx.h vhea.h maxp.h sfnt.h
$(BE)/EBLC.o: $G sfnt.h $F/EBLC.h EBLC.h
$(BE)/map.o: $G map.h nicename.h
