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
CFLAGS = $(STD_OPTS) -g -DCEF_DEBUG=1

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/cfembed
LIB_SRCS = $(SRC_DIR)/cfembed.c
LIB_OBJS = cfembed.o
LIB_TARGET = $(LIB_DIR)/cfembed.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
cfembed.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cfembed.c -o $@
# AUTO-GENERATED DEPENDENCIES
cfembed.o: ../../../../../../../source/lib/cfembed/cfembed.c \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/cfembed.h \
  ../../../../../../../public/api/sfntwrite.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/sfwerr.h \
  ../../../../../../../public/api/ceferr.h \
  ../../../../../../../public/api/t1read.h \
  ../../../../../../../public/api/t1rerr.h \
  ../../../../../../../public/api/cffread.h \
  ../../../../../../../public/api/cfrerr.h \
  ../../../../../../../public/api/ttread.h \
  ../../../../../../../public/api/ttrerr.h \
  ../../../../../../../public/api/cffwrite.h \
  ../../../../../../../public/api/cfwerr.h \
  ../../../../../../../public/api/sfntread.h \
  ../../../../../../../public/api/sfrerr.h
