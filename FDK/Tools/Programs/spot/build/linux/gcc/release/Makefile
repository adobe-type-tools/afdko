#########################################################################
#                                                                       #
# Copyright 1997-2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = release
ROOT_DIR = ../../../..
OBJECT_DIR = .
PRG_TARGET = $(EXE_DIR)/spot
# Build targets
TARGETS = $(PRG_TARGET)

CFLAGS = $(STD_OPTS) -m32 -g -I$(ROOT_DIR)/sfnt_includes -I$(SRC_DIR) -DEXECUTABLE=1

# Program
PRG_SRCS = $(SRC_DIR)/BASE.c \
	$(SRC_DIR)/BBOX.c \
	$(SRC_DIR)/BLND.c \
	$(SRC_DIR)/CFF_.c \
	$(SRC_DIR)/CID_.c \
	$(SRC_DIR)/EBLC.c \
	$(SRC_DIR)/ENCO.c \
	$(SRC_DIR)/FNAM.c \
	$(SRC_DIR)/GDEF.c \
	$(SRC_DIR)/GLOB.c \
	$(SRC_DIR)/GPOS.c \
	$(SRC_DIR)/GSUB.c \
	$(SRC_DIR)/HFMX.c \
	$(SRC_DIR)/LTSH.c \
	$(SRC_DIR)/META.c \
	$(SRC_DIR)/MMFX.c \
	$(SRC_DIR)/MMSD.c \
	$(SRC_DIR)/MMVR.c \
	$(SRC_DIR)/OS_2.c \
	$(SRC_DIR)/SING.c \
	$(SRC_DIR)/TTdumpinstrs.c \
	$(SRC_DIR)/TYP1.c \
	$(SRC_DIR)/VORG.c \
	$(SRC_DIR)/WDTH.c \
	$(SRC_DIR)/cffread.c \
	$(SRC_DIR)/cmap.c \
	$(SRC_DIR)/da.c \
	$(SRC_DIR)/desc.c \
	$(SRC_DIR)/dump.c \
	$(SRC_DIR)/fdsc.c \
	$(SRC_DIR)/feat.c \
	$(SRC_DIR)/file.c \
	$(SRC_DIR)/fvar.c \
	$(SRC_DIR)/gasp.c \
	$(SRC_DIR)/global.c \
	$(SRC_DIR)/glyf.c \
	$(SRC_DIR)/hdmx.c \
	$(SRC_DIR)/head.c \
	$(SRC_DIR)/hhea.c \
	$(SRC_DIR)/hmtx.c \
	$(SRC_DIR)/kern.c \
	$(SRC_DIR)/loca.c \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/map.c \
	$(SRC_DIR)/maxp.c \
	$(SRC_DIR)/memcheck.c \
	$(SRC_DIR)/name.c \
	$(SRC_DIR)/opt.c \
	$(SRC_DIR)/pathbuild.c \
	$(SRC_DIR)/post.c \
	$(SRC_DIR)/proof.c \
	$(SRC_DIR)/res.c \
	$(SRC_DIR)/sfnt.c \
	$(SRC_DIR)/spotmsgs.c \
	$(SRC_DIR)/strftime.c \
	$(SRC_DIR)/sys.c \
	$(SRC_DIR)/trak.c \
	$(SRC_DIR)/tto.c \
	$(SRC_DIR)/vhea.c \
	$(SRC_DIR)/vmtx.c


PRG_INCLUDES = $(SRC_DIR)/BASE.h \
	$(SRC_DIR)/BBOX.h \
	$(SRC_DIR)/BLND.h \
	$(SRC_DIR)/CFF_.h \
	$(SRC_DIR)/CID_.h \
	$(SRC_DIR)/EBLC.h \
	$(SRC_DIR)/ENCO.h \
	$(SRC_DIR)/FNAM.h \
	$(SRC_DIR)/GDEF.h \
	$(SRC_DIR)/GLOB.h \
	$(SRC_DIR)/GPOS.h \
	$(SRC_DIR)/GSUB.h \
	$(SRC_DIR)/HFMX.h \
	$(SRC_DIR)/LTSH.h \
	$(SRC_DIR)/META.h \
	$(SRC_DIR)/MMFX.h \
	$(SRC_DIR)/MMSD.h \
	$(SRC_DIR)/MMVR.h \
	$(SRC_DIR)/OS_2.h \
	$(SRC_DIR)/SING.h \
	$(SRC_DIR)/TTdumpinstrs.h \
	$(SRC_DIR)/TYP1.h \
	$(SRC_DIR)/VORG.h \
	$(SRC_DIR)/WDTH.h \
	$(SRC_DIR)/bitstr.h \
	$(SRC_DIR)/cffread.h \
	$(SRC_DIR)/cmap.h \
	$(SRC_DIR)/da.h \
	$(SRC_DIR)/desc.h \
	$(SRC_DIR)/dictops.h \
	$(SRC_DIR)/dump.h \
	$(SRC_DIR)/excs.h \
	$(SRC_DIR)/excs0.h \
	$(SRC_DIR)/exenc0.h \
	$(SRC_DIR)/exenccd.h \
	$(SRC_DIR)/exsubcs.h \
	$(SRC_DIR)/exsubcs0.h \
	$(SRC_DIR)/fdsc.h \
	$(SRC_DIR)/feat.h \
	$(SRC_DIR)/file.h \
	$(SRC_DIR)/fvar.h \
	$(SRC_DIR)/gasp.h \
	$(SRC_DIR)/global.h \
	$(SRC_DIR)/glyf.h \
	$(SRC_DIR)/hdmx.h \
	$(SRC_DIR)/head.h \
	$(SRC_DIR)/hhea.h \
	$(SRC_DIR)/hmtx.h \
	$(SRC_DIR)/isocs.h \
	$(SRC_DIR)/isocs0.h \
	$(SRC_DIR)/kern.h \
	$(SRC_DIR)/loca.h \
	$(SRC_DIR)/map.h \
	$(SRC_DIR)/maxp.h \
	$(SRC_DIR)/memcheck.h \
	$(SRC_DIR)/name.h \
	$(SRC_DIR)/nicename.h \
	$(SRC_DIR)/numtypes.h \
	$(SRC_DIR)/opt.h \
	$(SRC_DIR)/package.h \
	$(SRC_DIR)/pathbuild.h \
	$(SRC_DIR)/post.h \
	$(SRC_DIR)/proof.h \
	$(SRC_DIR)/res.h \
	$(SRC_DIR)/sfnt.h \
	$(SRC_DIR)/spot.h \
	$(SRC_DIR)/spotmsgs.h \
	$(SRC_DIR)/stdenc0.h \
	$(SRC_DIR)/stdenccd.h \
	$(SRC_DIR)/stdstr.h \
	$(SRC_DIR)/stdstr1.h \
	$(SRC_DIR)/sys.h \
	$(SRC_DIR)/trak.h \
	$(SRC_DIR)/tto.h \
	$(SRC_DIR)/txops.h \
	$(SRC_DIR)/vhea.h \
	$(SRC_DIR)/vmtx.h


PRG_OBJS = $(OBJECT_DIR)/BASE.o \
	$(OBJECT_DIR)/BBOX.o \
	$(OBJECT_DIR)/BLND.o \
	$(OBJECT_DIR)/CFF_.o \
	$(OBJECT_DIR)/CID_.o \
	$(OBJECT_DIR)/EBLC.o \
	$(OBJECT_DIR)/ENCO.o \
	$(OBJECT_DIR)/FNAM.o \
	$(OBJECT_DIR)/GDEF.o \
	$(OBJECT_DIR)/GLOB.o \
	$(OBJECT_DIR)/GPOS.o \
	$(OBJECT_DIR)/GSUB.o \
	$(OBJECT_DIR)/HFMX.o \
	$(OBJECT_DIR)/LTSH.o \
	$(OBJECT_DIR)/META.o \
	$(OBJECT_DIR)/MMFX.o \
	$(OBJECT_DIR)/MMSD.o \
	$(OBJECT_DIR)/MMVR.o \
	$(OBJECT_DIR)/OS_2.o \
	$(OBJECT_DIR)/SING.o \
	$(OBJECT_DIR)/TTdumpinstrs.o \
	$(OBJECT_DIR)/TYP1.o \
	$(OBJECT_DIR)/VORG.o \
	$(OBJECT_DIR)/WDTH.o \
	$(OBJECT_DIR)/cffread.o \
	$(OBJECT_DIR)/cmap.o \
	$(OBJECT_DIR)/da.o \
	$(OBJECT_DIR)/desc.o \
	$(OBJECT_DIR)/dump.o \
	$(OBJECT_DIR)/fdsc.o \
	$(OBJECT_DIR)/feat.o \
	$(OBJECT_DIR)/file.o \
	$(OBJECT_DIR)/fvar.o \
	$(OBJECT_DIR)/gasp.o \
	$(OBJECT_DIR)/global.o \
	$(OBJECT_DIR)/glyf.o \
	$(OBJECT_DIR)/hdmx.o \
	$(OBJECT_DIR)/head.o \
	$(OBJECT_DIR)/hhea.o \
	$(OBJECT_DIR)/hmtx.o \
	$(OBJECT_DIR)/kern.o \
	$(OBJECT_DIR)/loca.o \
	$(OBJECT_DIR)/main.o \
	$(OBJECT_DIR)/map.o \
	$(OBJECT_DIR)/maxp.o \
	$(OBJECT_DIR)/memcheck.o \
	$(OBJECT_DIR)/name.o \
	$(OBJECT_DIR)/opt.o \
	$(OBJECT_DIR)/pathbuild.o \
	$(OBJECT_DIR)/post.o \
	$(OBJECT_DIR)/proof.o \
	$(OBJECT_DIR)/res.o \
	$(OBJECT_DIR)/sfnt.o \
	$(OBJECT_DIR)/spotmsgs.o \
	$(OBJECT_DIR)/strftime.o \
	$(OBJECT_DIR)/sys.o \
	$(OBJECT_DIR)/trak.o \
	$(OBJECT_DIR)/tto.o \
	$(OBJECT_DIR)/vhea.o \
	$(OBJECT_DIR)/vmtx.o
	
# Standard definitions
include ../../../../../public/config/linux/gcc/gcc.mak

# program rule is in gcc.mak

# Object rules

$(OBJECT_DIR)/BASE.o: $(SRC_DIR)/BASE.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/BASE.c -o $(OBJECT_DIR)/BASE.o

$(OBJECT_DIR)/BBOX.o: $(SRC_DIR)/BBOX.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/BBOX.c -o $(OBJECT_DIR)/BBOX.o

$(OBJECT_DIR)/BLND.o: $(SRC_DIR)/BLND.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/BLND.c -o $(OBJECT_DIR)/BLND.o

$(OBJECT_DIR)/CFF_.o: $(SRC_DIR)/CFF_.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/CFF_.c -o $(OBJECT_DIR)/CFF_.o

$(OBJECT_DIR)/CID_.o: $(SRC_DIR)/CID_.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/CID_.c -o $(OBJECT_DIR)/CID_.o

$(OBJECT_DIR)/EBLC.o: $(SRC_DIR)/EBLC.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/EBLC.c -o $(OBJECT_DIR)/EBLC.o

$(OBJECT_DIR)/ENCO.o: $(SRC_DIR)/ENCO.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/ENCO.c -o $(OBJECT_DIR)/ENCO.o

$(OBJECT_DIR)/FNAM.o: $(SRC_DIR)/FNAM.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/FNAM.c -o $(OBJECT_DIR)/FNAM.o

$(OBJECT_DIR)/GDEF.o: $(SRC_DIR)/GDEF.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/GDEF.c -o $(OBJECT_DIR)/GDEF.o

$(OBJECT_DIR)/GLOB.o: $(SRC_DIR)/GLOB.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/GLOB.c -o $(OBJECT_DIR)/GLOB.o

$(OBJECT_DIR)/GPOS.o: $(SRC_DIR)/GPOS.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/GPOS.c -o $(OBJECT_DIR)/GPOS.o

$(OBJECT_DIR)/GSUB.o: $(SRC_DIR)/GSUB.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/GSUB.c -o $(OBJECT_DIR)/GSUB.o

$(OBJECT_DIR)/HFMX.o: $(SRC_DIR)/HFMX.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/HFMX.c -o $(OBJECT_DIR)/HFMX.o

$(OBJECT_DIR)/LTSH.o: $(SRC_DIR)/LTSH.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/LTSH.c -o $(OBJECT_DIR)/LTSH.o

$(OBJECT_DIR)/META.o: $(SRC_DIR)/META.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/META.c -o $(OBJECT_DIR)/META.o

$(OBJECT_DIR)/MMFX.o: $(SRC_DIR)/MMFX.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/MMFX.c -o $(OBJECT_DIR)/MMFX.o

$(OBJECT_DIR)/MMSD.o: $(SRC_DIR)/MMSD.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/MMSD.c -o $(OBJECT_DIR)/MMSD.o

$(OBJECT_DIR)/MMVR.o: $(SRC_DIR)/MMVR.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/MMVR.c -o $(OBJECT_DIR)/MMVR.o

$(OBJECT_DIR)/OS_2.o: $(SRC_DIR)/OS_2.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/OS_2.c -o $(OBJECT_DIR)/OS_2.o

$(OBJECT_DIR)/SING.o: $(SRC_DIR)/SING.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/SING.c -o $(OBJECT_DIR)/SING.o

$(OBJECT_DIR)/TTdumpinstrs.o: $(SRC_DIR)/TTdumpinstrs.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/TTdumpinstrs.c -o $(OBJECT_DIR)/TTdumpinstrs.o

$(OBJECT_DIR)/TYP1.o: $(SRC_DIR)/TYP1.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/TYP1.c -o $(OBJECT_DIR)/TYP1.o

$(OBJECT_DIR)/VORG.o: $(SRC_DIR)/VORG.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/VORG.c -o $(OBJECT_DIR)/VORG.o

$(OBJECT_DIR)/WDTH.o: $(SRC_DIR)/WDTH.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/WDTH.c -o $(OBJECT_DIR)/WDTH.o

$(OBJECT_DIR)/cffread.o: $(SRC_DIR)/cffread.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/cffread.c -o $(OBJECT_DIR)/cffread.o

$(OBJECT_DIR)/cmap.o: $(SRC_DIR)/cmap.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/cmap.c -o $(OBJECT_DIR)/cmap.o

$(OBJECT_DIR)/da.o: $(SRC_DIR)/da.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/da.c -o $(OBJECT_DIR)/da.o

$(OBJECT_DIR)/desc.o: $(SRC_DIR)/desc.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/desc.c -o $(OBJECT_DIR)/desc.o

$(OBJECT_DIR)/dump.o: $(SRC_DIR)/dump.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/dump.c -o $(OBJECT_DIR)/dump.o

$(OBJECT_DIR)/fdsc.o: $(SRC_DIR)/fdsc.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/fdsc.c -o $(OBJECT_DIR)/fdsc.o

$(OBJECT_DIR)/feat.o: $(SRC_DIR)/feat.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/feat.c -o $(OBJECT_DIR)/feat.o

$(OBJECT_DIR)/file.o: $(SRC_DIR)/file.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/file.c -o $(OBJECT_DIR)/file.o

$(OBJECT_DIR)/fvar.o: $(SRC_DIR)/fvar.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/fvar.c -o $(OBJECT_DIR)/fvar.o

$(OBJECT_DIR)/gasp.o: $(SRC_DIR)/gasp.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/gasp.c -o $(OBJECT_DIR)/gasp.o

$(OBJECT_DIR)/global.o: $(SRC_DIR)/global.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/global.c -o $(OBJECT_DIR)/global.o

$(OBJECT_DIR)/glyf.o: $(SRC_DIR)/glyf.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/glyf.c -o $(OBJECT_DIR)/glyf.o

$(OBJECT_DIR)/hdmx.o: $(SRC_DIR)/hdmx.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/hdmx.c -o $(OBJECT_DIR)/hdmx.o

$(OBJECT_DIR)/head.o: $(SRC_DIR)/head.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/head.c -o $(OBJECT_DIR)/head.o

$(OBJECT_DIR)/hhea.o: $(SRC_DIR)/hhea.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/hhea.c -o $(OBJECT_DIR)/hhea.o

$(OBJECT_DIR)/hmtx.o: $(SRC_DIR)/hmtx.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/hmtx.c -o $(OBJECT_DIR)/hmtx.o

$(OBJECT_DIR)/kern.o: $(SRC_DIR)/kern.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/kern.c -o $(OBJECT_DIR)/kern.o

$(OBJECT_DIR)/loca.o: $(SRC_DIR)/loca.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/loca.c -o $(OBJECT_DIR)/loca.o

$(OBJECT_DIR)/main.o: $(SRC_DIR)/main.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/main.c -o $(OBJECT_DIR)/main.o

$(OBJECT_DIR)/map.o: $(SRC_DIR)/map.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/map.c -o $(OBJECT_DIR)/map.o

$(OBJECT_DIR)/maxp.o: $(SRC_DIR)/maxp.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/maxp.c -o $(OBJECT_DIR)/maxp.o

$(OBJECT_DIR)/memcheck.o: $(SRC_DIR)/memcheck.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/memcheck.c -o $(OBJECT_DIR)/memcheck.o

$(OBJECT_DIR)/name.o: $(SRC_DIR)/name.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/name.c -o $(OBJECT_DIR)/name.o

$(OBJECT_DIR)/opt.o: $(SRC_DIR)/opt.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/opt.c -o $(OBJECT_DIR)/opt.o

$(OBJECT_DIR)/pathbuild.o: $(SRC_DIR)/pathbuild.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/pathbuild.c -o $(OBJECT_DIR)/pathbuild.o

$(OBJECT_DIR)/post.o: $(SRC_DIR)/post.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/post.c -o $(OBJECT_DIR)/post.o

$(OBJECT_DIR)/proof.o: $(SRC_DIR)/proof.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/proof.c -o $(OBJECT_DIR)/proof.o

$(OBJECT_DIR)/res.o: $(SRC_DIR)/res.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/res.c -o $(OBJECT_DIR)/res.o

$(OBJECT_DIR)/sfnt.o: $(SRC_DIR)/sfnt.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/sfnt.c -o $(OBJECT_DIR)/sfnt.o

$(OBJECT_DIR)/spotmsgs.o: $(SRC_DIR)/spotmsgs.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/spotmsgs.c -o $(OBJECT_DIR)/spotmsgs.o

$(OBJECT_DIR)/strftime.o: $(SRC_DIR)/strftime.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/strftime.c -o $(OBJECT_DIR)/strftime.o

$(OBJECT_DIR)/sys.o: $(SRC_DIR)/sys.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/sys.c -o $(OBJECT_DIR)/sys.o

$(OBJECT_DIR)/trak.o: $(SRC_DIR)/trak.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/trak.c -o $(OBJECT_DIR)/trak.o

$(OBJECT_DIR)/tto.o: $(SRC_DIR)/tto.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/tto.c -o $(OBJECT_DIR)/tto.o

$(OBJECT_DIR)/vhea.o: $(SRC_DIR)/vhea.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/vhea.c -o $(OBJECT_DIR)/vhea.o

$(OBJECT_DIR)/vmtx.o: $(SRC_DIR)/vmtx.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/vmtx.c -o $(OBJECT_DIR)/vmtx.o

