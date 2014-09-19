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
CFLAGS = $(STD_OPTS) -g -DTTR_DEBUG=1

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/ttread
LIB_SRCS = $(SRC_DIR)/ttread.c
LIB_OBJS = ttread.o
LIB_TARGET = $(LIB_DIR)/ttread.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
ttread.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/ttread.c -o $@

# AUTO-GENERATED DEPENDENCIES
ttread.o: ../../../../../../../source/lib/ttread/ttread.c \
  ../../../../../../../public/api/ttread.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/ttrerr.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../public/api/sfntread.h \
  ../../../../../../../public/api/sfrerr.h \
  ../../../../../../../public/resource/applestd.h \
  ../../../../../../../public/resource/uv2agl.h \
  ../../../../../../../public/resource/dblmapuv.h \
  ../../../../../../../public/resource/macromn0.h
