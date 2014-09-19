#########################################################################
#                                                                       #
# Copyright 1997-2003 Adobe Systems Incorporated.                       #
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
CONFIG = release
CFLAGS = $(STD_OPTS) -O2 -DOS=os_linux -DISP=isp_i80486 -DSTAGE=EXPORT

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/support
LIB_SRCS = $(SRC_DIR)/bitarray.c \
	$(SRC_DIR)/canthappen.c \
	$(SRC_DIR)/except.c \
	$(SRC_DIR)/fixed.c
LIB_OBJS = bitarray.o canthappen.o except.o fixed.o

LIB_TARGET = $(LIB_DIR)/support.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-3.2.mak

# Object rules
bitarray.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/bitarray.c -o $@

canthappen.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/canthappen.c -o $@

except.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/except.c -o $@

fixed.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/fixed.c -o $@

# AUTO-GENERATED DEPENDENCIES
bitarray.o: ../../../../../../../source/lib/support/bitarray.c \
  ../../../../../../../public/api/supportenvironment.h \
  ../../../../../../../public/api/supportframepixel.h \
  ../../../../../../../public/api/supportpublictypes.h \
  ../../../../../../../public/api/supportframepixeltypes.h
canthappen.o: ../../../../../../../source/lib/support/canthappen.c \
  ../../../../../../../public/api/supportenvironment.h \
  ../../../../../../../public/api/supportexcept.h \
  ../../../../../../../public/api/supportpublictypes.h \
  ../../../../../../../public/api/supportcanthappen.h
except.o: ../../../../../../../source/lib/support/except.c \
  ../../../../../../../public/api/supportenvironment.h \
  ../../../../../../../public/api/supportexcept.h \
  ../../../../../../../public/api/supportpublictypes.h \
  ../../../../../../../public/api/supportcanthappen.h
fixed.o: ../../../../../../../source/lib/support/fixed.c \
  ../../../../../../../public/api/supportpublictypes.h \
  ../../../../../../../public/api/supportenvironment.h \
  ../../../../../../../public/api/supportfp.h \
  ../../../../../../../public/api/supportasbasic.h
