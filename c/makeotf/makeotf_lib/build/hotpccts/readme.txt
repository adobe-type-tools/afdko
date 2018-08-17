Read Roberts 8/14/2018

To build the new featXXX.[ch] files, just cd to this directory, under Mac OSX, and run
"make".

Note that this makefile will run the tools cpplint and clang-format in
order to clean up the old code format. To install these tools, use the
commands:
	'brew install clang-format'
	'pip install cpplint'
Note that clang-format does NOT fix all the issues. Any warnings
reported by cpplint must be fixed by hand. Currently, there are only
four of these.

The "make" references to the 'antlr' program at pccts/bin/antlr.

If this does not run, try rebuilding the antlr program; just cd to 'pccts', and say
	MACOSX_DEPLOYMENT_TARGET=10.9
	export MACOSX_DEPLOYMENT_TARGET
	make clean
	make

The value 'MACOSX_DEPLOYMENT_TARGET=10.9' works as of 9/14/2015. This will probably need
to change as Xcode evolves.

It is OK if only the  antlr program builds, as this is the only one we need.

I got this as "pccts133mr.zip", from:

http://www.polhode.com/pccts.html

In order to for the make file to work, I had to edit line 20 in the
file pccts/makefile from:
#COPT=-O2
to
COPT=-O2 -DPCCTS_USE_STDARG=1

and had to uncomment line 18:
#CC=cc
to
CC=cc

The grammar changed enough in the transition to ANTL2 and ANTLR 3 that
the  PCCTS featgram.g  won't run under the ANTLR 3 programs.
