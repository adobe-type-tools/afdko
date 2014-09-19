#########################################################################
#                                                                       #
# Copyright 2001 Adobe Systems Incorporated.                            #
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
SRC_DIR = $(ROOT_DIR)/source/lib/sfntwrite
LIB_SRCS = $(SRC_DIR)/sfntwrite.c
LIB_OBJS = sfntwrite.o
LIB_TARGET = $(LIB_DIR)/sfntwrite.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
sfntwrite.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sfntwrite.c -o $@
# AUTO-GENERATED DEPENDENCIES
sfntwrite.o: ../../../../../../../source/lib/sfntwrite/sfntwrite.c \
  ../../../../../../../public/api/sfntwrite.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/sfwerr.h \
  ../../../../../../../public/api/dynarr.h
