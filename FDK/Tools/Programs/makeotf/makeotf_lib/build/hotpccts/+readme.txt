Read Roberts 12/11/2007.

To build the new featXXX.[ch] files, just cd to this directory and run
"make".

The "make" refers to the 'antlr' program at pccts/bin/antlr.

If this does not run, try rebuilding antlr; just cd to 'pccts', and say
make clean
make

As of 12/1/2007, with Mac OSX 10.4.8, the 'sorcerer' program fails to
build, but this is fine, as we need only the antlr program.

I got this as "pccts133mr.zip", from:

http://www.polhode.com/pccts.html

The grammar changed enough in the transition to ANTL2 and ANTLR 3 that
the  PCCTS featgram.g  won't run under the ANTLR 3 programs.