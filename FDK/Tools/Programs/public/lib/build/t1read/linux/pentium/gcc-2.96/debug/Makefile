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
CFLAGS = $(STD_OPTS) -g -DT1R_DEBUG=1

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/t1read
LIB_SRCS = $(SRC_DIR)/t1read.c
LIB_OBJS = t1read.o
LIB_TARGET = $(LIB_DIR)/t1read.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
t1read.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/t1read.c -o $@

# AUTO-GENERATED DEPENDENCIES
t1read.o: ../../../../../../../source/lib/t1read/t1read.c \
  ../../../../../../../public/api/t1read.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/t1rerr.h \
  ../../../../../../../public/api/t1cstr.h \
  ../../../../../../../public/api/t1cerr.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/pstoken.h \
  ../../../../../../../public/api/psterr.h \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../public/resource/txops.h \
  ../../../../../../../public/resource/stdenc3.h
