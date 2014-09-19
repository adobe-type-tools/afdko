#########################################################################
#                                                                       #
# Copyright 2000-2001 Adobe Systems Incorporated.                       #
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
CFLAGS = $(STD_OPTS) -O2

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/pdfwrite
LIB_SRCS = $(SRC_DIR)/pdfwrite.c
LIB_OBJS = pdfwrite.o
LIB_TARGET = $(LIB_DIR)/pdfwrite.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
pdfwrite.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/pdfwrite.c -o $@
# AUTO-GENERATED DEPENDENCIES
pdfwrite.o: ../../../../../../../source/lib/pdfwrite/pdfwrite.c \
  ../../../../../../../public/api/pdfwrite.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/pdwerr.h \
  ../../../../../../../public/api/dynarr.h
