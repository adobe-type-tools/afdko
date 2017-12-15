OpenType CID_font_support Read Me v1.0 7/5/2000

Contents
========
1. Introduction
2. Installation Guide
3. Overview of the scripts

1. Introduction
===============
This download provides a set of Perl scripts that will allow you to build
the BASE, valt, palt, halt, vpal and vhal tables for an OpenType/CFF-CID 
font. The "Sample Input" folder contains examples, taken from Adobe fonts, 
of the files that are used as input.

These scripts are the same being used by the internal Adobe type
development group.

2. Installation Guide
=====================
These scripts can be installed to the location of your choice. However, since
they are Perl scripts, you will need to install a version of Perl appropriate
to your system (not provided in this download but widely available on the 
Web). Once you have installed Perl and these scripts, you are ready to run 
them following the instructions below:

Mac
---
Due to a bug in the Mac Perl 5.0.4, these scripts need to be run directly 
from the Finder (in other words, you cannot open them in MacPerl and run
them from that interface). To do so, you can either set the MacPerl 
preferences to run a script by default (Edit-Preferences-Scripts) or hold down
the option key while double clicking on the script icon whenever you run it.
You will then be presented with standard Macintosh file selection dialogs to
let you specify the files you wish to use as input (see "Overview of the 
scripts" below for details).

Windows
-------
On any version of Windows, these scripts will need to be run from the DOS 
prompt. Once you install Perl, you can "cd" to the script directory and 
execute the scripts of your choice by typing its name (including the .pl
extension) followed by the appropriate parameters (see "Overview of the 
scripts" below for details).

3. Overview of the Scripts
==========================

a) almx2gpos.pl
   ------------
This script takes as input a single almx file (see KozMinPro-Light.almx for 
format sample) and uses its contents to create a file called features.alts
which contains the pvalt, halt, vpal, vhal features data for that file.

b) mkvalt.pl
   ---------
This script takes as input a file containing a list of CIDs and an AFM file. 
The latter contains lines that look like: 
C -1 ; W0X 238 ; N 13 ; B 63 -106 179 107 ;
Output is to features.valt and contains the necessary adjustments in GPOS 
style (position \[CID] [0] [YPlacement] [0] [0];)
In order to vertically center the glyphs in the em-box, calculations are done
on the basis of the Character Bounding Box.
Note: If necessary the values for the em-box have to be adjusted, they are 
      currently set to 0,1000 and -120,880.

c) mkicf.pl
   --------
This script takes as input a CMAP file and an AFM file (in that order both
on the Mac and Windows) and processes their contents to generate BASE table
data for the corresponding font. 
The file named AFM in the sample input folder is provided as an example and
is taken from Adobe's KozMinPro-Light font. 
Output is to "features.BASE".

d) feature-merge.pl
   ----------------
This script takes an arbitrary number of file names as input (on Windows, 
just specify as many as necessary; on Mac, the file selection dialog will
continue to reappear until you select "Cancel") and concatenates their 
content in the appropriate features file order. Usually, the input to this
script would be the ouput from two or more of the other scripts in this
package. The ouput is named "features.auto" by default.
 

OTFDK/July 2002
©2002 Adobe Systems Incorporated

OpenType is a trademark of the Microsoft Corporation. Macintosh is a
trademark of Apple Computer Inc. Adobe, the Adobe logo, Adobe Type Manager,
PostScript, Adobe InDesign are trademarks of Adobe Systems Inc.
Windows is a trademark of Microsoft Corp.


