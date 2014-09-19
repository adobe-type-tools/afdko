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
