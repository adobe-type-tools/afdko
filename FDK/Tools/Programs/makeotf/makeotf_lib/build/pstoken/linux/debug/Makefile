#########################################################################
#                                                                       #
# Copyright 1997-1999 Adobe Systems Incorporated.                       #
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
CFLAGS = $(STD_OPTS) -g -DPSTOKEN_DEBUG=1

# Library
SRC_DIR = $(ROOT_DIR)/source/pstoken
LIB_SRCS = $(SRC_DIR)/pstoken.c
LIB_OBJS = pstoken.o
LIB_TARGET = $(LIB_DIR)/pstoken.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../standard/linux.mak

# Auto-generated dependencies
include depend.mak

# Object rules
pstoken.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/pstoken.c -o $@
