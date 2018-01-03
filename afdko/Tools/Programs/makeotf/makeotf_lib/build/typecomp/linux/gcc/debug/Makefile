#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug

# Library
ROOT_DIR = ../../../../..
SRC_DIR = $(ROOT_DIR)/source/typecomp
OBJECT_DIR = .
LIB_TARGET = $(LIB_DIR)/typecomp.a

CFLAGS = $(STD_OPTS) -I$(ROOT_DIR)/api \
					 -I$(ROOT_DIR)/resource \
					 -I$(ROOT_DIR)/../../public/lib/api \
					 -I$(ROOT_DIR)/../../public/lib/resource \
					 -DTC_HINT_CHECK=1 \
					 -DTC_T13_SUPPORT=0 \
					 -DTC_EURO_SUPPORT=1 \
					 -DTC_SUBR_SUPPORT=1
					 -DTC_DEBUG=1 \
					 -g
					 
LIB_SRCS = \
	$(SRCDIR)/charset.c \
	$(SRCDIR)/cs.c \
	$(SRCDIR)/dict.c \
	$(SRCDIR)/encoding.c \
	$(SRCDIR)/fdselect.c \
	$(SRCDIR)/parse.c \
	$(SRCDIR)/recode.c \
	$(SRCDIR)/sindex.c \
	$(SRCDIR)/subr.c \
	$(SRCDIR)/t13.c \
	$(SRCDIR)/tc.c \

LIB_OBJS = \
	$(OBJECT_DIR)/charset.o \
	$(OBJECT_DIR)/cs.o \
	$(OBJECT_DIR)/dict.o \
	$(OBJECT_DIR)/encoding.o \
	$(OBJECT_DIR)/fdselect.o \
	$(OBJECT_DIR)/parse.o \
	$(OBJECT_DIR)/recode.o \
	$(OBJECT_DIR)/sindex.o \
	$(OBJECT_DIR)/subr.o \
	$(OBJECT_DIR)/t13.o \
	$(OBJECT_DIR)/tc.o \

# Standard definitions
include  $(ROOT_DIR)/../../public/lib/config/linux/gcc/gcc.mak

	
# Lib Object rules
charset.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/charset.c -o $@

cs.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cs.c -o $@

dict.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/dict.c -o $@

encoding.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/encoding.c -o $@

fdselect.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/fdselect.c -o $@

parse.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/parse.c -o $@

recode.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/recode.c -o $@

sindex.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sindex.c -o $@

subr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/subr.c -o $@

t13.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/t13.c -o $@

tc.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/tc.c -o $@

	
