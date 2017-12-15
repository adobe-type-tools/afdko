Adobe Font Development Kit for OpenType (AFDKO)
===============================================

A set of tools for building OpenType font files from PostScript and TrueType font data.

This directory tree contains the data files, Python scripts, Perl scripts, and
sources for the command line programs that comprise the AFDKO.

Please refer to the file [AFDKO-Overview.html](https://rawgit.com/adobe-type-tools/afdko/master/FDK/AFDKO-Overview.html)
for a more detailed description of what is in the AFDKO.

The file [FDK Build Notes.txt](https://raw.githubusercontent.com/adobe-type-tools/afdko/master/FDK/FDK%20Build%20Notes.txt)
explains how to assemble a working version of the AFDKO. Note that batteries are NOT included:
you will need to configure a Python interpreter to work with the AFDKO, and build the command
line tools from the sources. The [FDK Build Notes.txt](https://raw.githubusercontent.com/adobe-type-tools/afdko/master/FDK/FDK%20Build%20Notes.txt)
file explains how to do this.

The file [Read_Me_First.html](https://rawgit.com/adobe-type-tools/afdko/master/FDK/Read_Me_First.html)
explains how to install a copy of the AFDKO, once you have assembled a working version.

You can also download and install a working version from http://www.adobe.com/devnet/opentype/afdko.html.
This download contains two command line programs, `IS` and `checkOutlines`, that are
not in the open source branch of the AFDKO.
