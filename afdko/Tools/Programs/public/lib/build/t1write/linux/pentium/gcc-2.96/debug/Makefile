# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g -DT1W_DEBUG=1

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/t1write
LIB_SRCS = $(SRC_DIR)/t1write.c
LIB_OBJS = t1write.o
LIB_TARGET = $(LIB_DIR)/t1write.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
t1write.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/t1write.c -o $@

$(SRC_DIR)/t1write_hintothers.h:	$(SRC_DIR)/t1write_hintothers.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/t1write_flexothers.h:	$(SRC_DIR)/t1write_flexothers.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/t1write_gcothers.h:	$(SRC_DIR)/t1write_gcothers.txt
	$(MKCSTRS) $? $@

$(SRC_DIR)/t1write_procsetothers.h:	$(SRC_DIR)/t1write_procsetothers.txt
	$(MKCSTRS) $? $@
# AUTO-GENERATED DEPENDENCIES
t1write.o: ../../../../../../../source/lib/t1write/t1write.c \
  ../../../../../../../public/api/t1write.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/t1werr.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/resource/dictops.h \
  ../../../../../../../public/resource/txops.h \
  ../../../../../../../public/resource/stdenc3.h \
  ../../../../../../../source/lib/t1write/t1write_hintothers.h \
  ../../../../../../../source/lib/t1write/t1write_flexothers.h \
  ../../../../../../../source/lib/t1write/t1write_gcothers.h \
  ../../../../../../../source/lib/t1write/t1write_procsetothers.h
