#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug

# Library
ROOT_DIR = ../../../..
SRC_DIR = $(ROOT_DIR)/source
OBJECT_DIR = .
LIB_TARGET = $(LIB_DIR)/autohintlib.a

CFLAGS = $(STD_OPTS) -I$(ROOT_DIR)/source/public/ac \
					-I$(ROOT_DIR)/source/public/extras \
					-I$(ROOT_DIR)/source/ac \
					-I$(ROOT_DIR)/source/bf \
					 -DAC_C_LIB=1 \
					 -DACLIB_EXPORTS \
					 -DIS_LIB \
					 -g
					 
LIB_SRCS = \
	$(SRC_DIR)/ac/ac.c \
	$(SRC_DIR)/ac/ac_C_lib.c \
	$(SRC_DIR)/ac/acfixed.c \
	$(SRC_DIR)/ac/auto.c \
	$(SRC_DIR)/ac/bbox.c \
	$(SRC_DIR)/ac/bfstub.c \
	$(SRC_DIR)/ac/charprop.c \
	$(SRC_DIR)/ac/check.c \
	$(SRC_DIR)/ac/control.c \
	$(SRC_DIR)/ac/cswrite.c \
	$(SRC_DIR)/ac/enum.c \
	$(SRC_DIR)/ac/eval.c \
	$(SRC_DIR)/ac/fix.c \
	$(SRC_DIR)/ac/flat.c \
	$(SRC_DIR)/ac/fontinfo.c \
	$(SRC_DIR)/ac/gen.c \
	$(SRC_DIR)/ac/head.c \
	$(SRC_DIR)/ac/merge.c \
	$(SRC_DIR)/ac/misc.c \
	$(SRC_DIR)/ac/path.c \
	$(SRC_DIR)/ac/pick.c \
	$(SRC_DIR)/ac/read.c \
	$(SRC_DIR)/ac/report.c \
	$(SRC_DIR)/ac/shuffle.c \
	$(SRC_DIR)/ac/stub.c \
	$(SRC_DIR)/ac/write.c \
	$(SRC_DIR)/bf/charlistpriv.c \
	$(SRC_DIR)/bf/charpath.c \
	$(SRC_DIR)/bf/charpathpriv.c \
	$(SRC_DIR)/bf/cryptprocs.c \
	$(SRC_DIR)/bf/fileops.c \
	$(SRC_DIR)/bf/filookup.c \
	$(SRC_DIR)/bf/illtobez.c \
	$(SRC_DIR)/bf/machinedep.c \
	$(SRC_DIR)/bf/rawPStobez.c
LIB_OBJS = \
	ac.o \
	ac_C_lib.o \
	acfixed.o \
	auto.o \
	bbox.o \
	bfstub.o \
	charprop.o \
	check.o \
	control.o \
	cswrite.o \
	enum.o \
	eval.o \
	fix.o \
	flat.o \
	fontinfo.o \
	gen.o \
	head.o \
	merge.o \
	misc.o \
	path.o \
	pick.o \
	read.o \
	report.o \
	shuffle.o \
	stub.o \
	write.o \
	charlistpriv.o \
	charpath.o \
	charpathpriv.o \
	cryptprocs.o \
	fileops.o \
	filookup.o \
	illtobez.o \
	machinedep.o \
	rawPStobez.o

# Standard definitions
include  $(ROOT_DIR)/../../public/lib/config/linux/gcc/gcc.mak

	
# Lib Object rules
absfont.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont.c -o $@

ac.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/ac.c -o $@

ac_C_lib.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/ac_C_lib.c -o $@

acfixed.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/acfixed.c -o $@

auto.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/auto.c -o $@

bbox.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/bbox.c -o $@

bfstub.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/bfstub.c -o $@

charprop.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/charprop.c -o $@

check.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/check.c -o $@

control.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/control.c -o $@

cswrite.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/cswrite.c -o $@

enum.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/enum.c -o $@

eval.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/eval.c -o $@

fix.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/fix.c -o $@

flat.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/flat.c -o $@

fontinfo.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/fontinfo.c -o $@

gen.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/gen.c -o $@

head.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/head.c -o $@

merge.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/merge.c -o $@

misc.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/misc.c -o $@

path.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/path.c -o $@

pick.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/pick.c -o $@

read.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/read.c -o $@

report.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/report.c -o $@

shuffle.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/shuffle.c -o $@

stub.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/stub.c -o $@

write.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/ac/write.c -o $@

charlistpriv.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/bf/charlistpriv.c -o $@

charpath.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/bf/charpath.c -o $@

charpathpriv.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/bf/charpathpriv.c -o $@

cryptprocs.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/bf/cryptprocs.c -o $@

fileops.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/bf/fileops.c -o $@

filookup.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/bf/filookup.c -o $@

illtobez.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/bf/illtobez.c -o $@

machinedep.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/bf/machinedep.c -o $@

rawPStobez.o:
	$(CC) $(CFLAGS) -c 	$(SRC_DIR)/bf/rawPStobez.c -o $@

	
	
