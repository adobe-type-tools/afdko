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
