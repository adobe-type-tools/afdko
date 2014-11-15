# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g  -DSTAGE=DEVELOP 

# Program
SRC_DIR = $(ROOT_DIR)/source/exe/tx
PRG_SRCS = $(SRC_DIR)/tx.c
PRG_OBJS = tx.o
PRG_TARGET = $(EXE_DIR)/tx
PRG_LIBS = \
	$(LIB_DIR)/dynarr.a \
	$(LIB_DIR)/t1read.a \
	$(LIB_DIR)/t1cstr.a \
	$(LIB_DIR)/cffread.a \
	$(LIB_DIR)/t2cstr.a \
	$(LIB_DIR)/ttread.a \
	$(LIB_DIR)/pstoken.a \
	$(LIB_DIR)/cffwrite.a \
	$(LIB_DIR)/absfont.a \
	$(LIB_DIR)/ctutil.a \
	$(LIB_DIR)/cfembed.a \
	$(LIB_DIR)/pdfwrite.a \
	$(LIB_DIR)/sfntread.a \
	$(LIB_DIR)/sfntwrite.a \
	$(LIB_DIR)/t1write.a \
	$(LIB_DIR)/buildch.a \
	$(LIB_DIR)/support.a \
	$(LIB_DIR)/sha1.a
MAKEALL = \
	lib/dynarr \
	lib/t1read \
	lib/t1cstr \
	lib/cffread \
	lib/t2cstr \
	lib/ttread \
	lib/pstoken \
	lib/absfont \
	lib/ctutil \
	lib/cffwrite \
	lib/cfembed \
	lib/pdfwrite \
	lib/sfntread \
	lib/sfntwrite \
	lib/t1write \
	lib/buildch \
	lib/support \
	lib/sha1 \
	exe/tx
SYS_LIBS = -lm

# Build targets
TARGETS = $(PRG_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
tx.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/tx.c -o $@

$(SRC_DIR)/usage.h:	$(SRC_DIR)/usage.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/help.h:	$(SRC_DIR)/help.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/dump.h:	$(SRC_DIR)/dump.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/ps.h:	$(SRC_DIR)/ps.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/afm.h:	$(SRC_DIR)/afm.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/path.h:	$(SRC_DIR)/path.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/cff.h:	$(SRC_DIR)/cff.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/cef.h:	$(SRC_DIR)/cef.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/pdf.h:	$(SRC_DIR)/pdf.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/mtx.h:	$(SRC_DIR)/mtx.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/t1.h:	$(SRC_DIR)/t1.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/bc.h:	$(SRC_DIR)/bc.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/dcf.h:	$(SRC_DIR)/dcf.txt
	$(MKCSTRS) $? $@

# AUTO-GENERATED DEPENDENCIES
tx.o: ../../../../../../../source/exe/tx/tx.c \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/cfembed.h \
  ../../../../../../../public/api/sfntwrite.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/sfwerr.h \
  ../../../../../../../public/api/ceferr.h \
  ../../../../../../../public/api/cffread.h \
  ../../../../../../../public/api/cfrerr.h \
  ../../../../../../../public/api/cffwrite.h \
  ../../../../../../../public/api/cfwerr.h \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/pdfwrite.h \
  ../../../../../../../public/api/pdwerr.h \
  ../../../../../../../public/api/sfntread.h \
  ../../../../../../../public/api/sfrerr.h \
  ../../../../../../../public/api/t1read.h \
  ../../../../../../../public/api/t1rerr.h \
  ../../../../../../../public/api/ttread.h \
  ../../../../../../../public/api/ttrerr.h \
  ../../../../../../../public/api/t1write.h \
  ../../../../../../../public/api/t1werr.h \
  ../../../../../../../public/resource/txops.h \
  ../../../../../../../public/resource/dictops.h \
  ../../../../../../../public/api/buildch.h \
  ../../../../../../../public/api/supportpublictypes.h \
  ../../../../../../../public/api/supportenvironment.h \
  ../../../../../../../public/api/supportfp.h \
  ../../../../../../../public/api/supportaszone.h \
  ../../../../../../../public/api/supportasbasic.h \
  ../../../../../../../public/api/supportframepixeltypes.h \
  ../../../../../../../public/api/sha1.h \
  ../../../../../../../public/api/atmqred.h \
  ../../../../../../../source/exe/tx/dump.h \
  ../../../../../../../source/exe/tx/ps.h \
  ../../../../../../../source/exe/tx/afm.h \
  ../../../../../../../source/exe/tx/path.h \
  ../../../../../../../source/exe/tx/cff.h \
  ../../../../../../../public/resource/agl2uv.h \
  ../../../../../../../source/exe/tx/cef.h \
  ../../../../../../../source/exe/tx/pdf.h \
  ../../../../../../../source/exe/tx/mtx.h \
  ../../../../../../../source/exe/tx/t1.h \
  ../../../../../../../public/resource/stdenc2.h \
  ../../../../../../../source/exe/tx/bc.h \
  ../../../../../../../source/exe/tx/dcf.h \
  ../../../../../../../source/exe/tx/usage.h \
  ../../../../../../../source/exe/tx/help.h
