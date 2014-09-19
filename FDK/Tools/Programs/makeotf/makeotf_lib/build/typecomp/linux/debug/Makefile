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
CFLAGS = $(STD_OPTS) -g -DTC_STATISTICS=0 -DTC_HINT_CHECK=1 \
	-DTC_T13_SUPPORT=1 -DTC_DEBUG=1 -DTC_EURO_SUPPORT=1 -DTC_SUBR_SUPPORT=1

# Library
SRC_DIR = $(ROOT_DIR)/source/typecomp
LIB_SRCS = $(SRC_DIR)/sindex.c \
	$(SRC_DIR)/parse.c \
	$(SRC_DIR)/cs.c \
	$(SRC_DIR)/recode.c \
	$(SRC_DIR)/subr.c \
	$(SRC_DIR)/encoding.c \
	$(SRC_DIR)/charset.c \
	$(SRC_DIR)/tc.c \
	$(SRC_DIR)/dict.c \
	$(SRC_DIR)/fdselect.c \
	$(SRC_DIR)/t13.c
LIB_OBJS = sindex.o parse.o cs.o recode.o subr.o encoding.o charset.o \
	tc.o dict.o fdselect.o t13.o
LIB_TARGET = $(LIB_DIR)/typecomp.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../standard/linux.mak

# Auto-generated dependencies
include depend.mak

# Object rules
sindex.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sindex.c -o $@

parse.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/parse.c -o $@

cs.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cs.c -o $@

recode.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/recode.c -o $@

subr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/subr.c -o $@

encoding.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/encoding.c -o $@

charset.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/charset.c -o $@

tc.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/tc.c -o $@

dict.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/dict.c -o $@

fdselect.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/fdselect.c -o $@

t13.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/t13.c -o $@
