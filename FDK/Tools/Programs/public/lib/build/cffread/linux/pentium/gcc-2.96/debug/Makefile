#########################################################################
#                                                                       #
# Copyright 1997-2001 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
# Patents Pending                                                       #
#                                                                       #
# NOTICE: All information contained herein is the property of Adobe     #
# Systems Incorporated. Many of the intellectual and technical          #
# concepts contained herein are proprietary to Adobe, are protected     #
# as trade secrets, and are made available only to Adobe licensees      #
# for their internal use. Any reproduction or dissemination of this     #
# software is strictly forbidden unless prior written permission is     #
# obtained from Adobe.                                                  #
#                                                                       #
# PostScript and Display PostScript are trademarks of Adobe Systems     #
# Incorporated or its subsidiaries and may be registered in certain     #
# jurisdictions.                                                        #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g -DCFR_DEBUG=1

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/cffread
LIB_SRCS = $(SRC_DIR)/cffread.c
LIB_OBJS = cffread.o
LIB_TARGET = $(LIB_DIR)/cffread.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
cffread.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffread.c -o $@

# AUTO-GENERATED DEPENDENCIES
cffread.o: ../../../../../../../source/lib/cffread/cffread.c \
  ../../../../../../../public/api/cffread.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/cfrerr.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/t2cstr.h \
  ../../../../../../../public/api/t2cerr.h \
  ../../../../../../../public/resource/dictops.h \
  ../../../../../../../public/resource/txops.h \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../public/api/sfntread.h \
  ../../../../../../../public/api/sfrerr.h \
  ../../../../../../../public/resource/stdenc0.h \
  ../../../../../../../public/resource/exenc0.h \
  ../../../../../../../public/resource/stdstr1.h \
  ../../../../../../../public/resource/isocs0.h \
  ../../../../../../../public/resource/excs0.h \
  ../../../../../../../public/resource/exsubcs0.h
