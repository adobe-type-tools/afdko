# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

all: pccts
ROOT_DIR     = ../..
HOT          = $(ROOT_DIR)/source/hotconv
# HOT          = ../hotconv
PCCTS		 = pccts
PCCTS_BIN	 = ${PCCTS}/bin
ANTLR		 = ${PCCTS_BIN}/antlr
DLG			 = ${PCCTS_BIN}/dlg

# -ga    Generate ANSI-compatible code (default=FALSE)
# -o     Directory where all output files should go (default=".")
# -f?    Rename output files
ANTLR_FLAGS = -ga \
			  -o ${HOT} \
			  -fe featerr.c \
			  -fl featpars.dlg \
			  -fm featmode.h \
			  -ft feattoks.h

# -ga    Generate ANSI-compatible code (default=FALSE)
# -C2	 Compression level 2
# -i	 Build interactive scanner
# -o     Directory where all output files should go (default=".")
# -m	 Rename lexical mode output file
DLG_FLAGS   = -ga -C2 -i \
			  -o ${HOT} \
			  -m featmode.h

ANTLR_SPAWN = ${HOT}/featpars.dlg ${HOT}/featgram.c ${HOT}/feattoks.h \
	${HOT}/featerr.c
DLG_SPAWN 	= ${HOT}/featscan.c ${HOT}/featmode.h

pccts: ${ANTLR_SPAWN} ${DLG_SPAWN}

${ANTLR_SPAWN}: featgram.g ${HOT}/feat.c ${HOT}/feat.h
	${ANTLR} ${ANTLR_FLAGS} featgram.g

${DLG_SPAWN}: ${HOT}/featpars.dlg
	${DLG} ${DLG_FLAGS} ${HOT}/featpars.dlg ${HOT}/featscan.c

scrub:
	rm -f ${ANTLR_SPAWN} ${DLG_SPAWN}
