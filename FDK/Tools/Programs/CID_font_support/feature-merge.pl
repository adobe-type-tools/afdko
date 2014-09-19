#!/usr/local/bin/perl
#***********************************************************************#
#*                                                                     *#
#* Copyright 1996-2002 Adobe Systems Incorporated.                     *#
#* All rights reserved.                                                *#
#*                                                                     *#
#* Patents Pending                                                     *#
#*                                                                     *#
#* NOTICE: All information contained herein is the property of Adobe   *#
#* Systems Incorporated. Many of the intellectual and technical        *#
#* concepts contained herein are proprietary to Adobe, are protected   *#
#* as trade secrets, and are made available only to Adobe licensees    *#
#* for their internal use. Any reproduction or dissemination of this   *#
#* software is strictly forbidden unless pfrior written permission is   *#
#* obtained from Adobe.                                                *#
#*                                                                     *#
#* PostScript and Display PostScript are trademarks of Adobe Systems   *#
#* Incorporated or its subsidiaries and may be registered in certain   *#
#* jurisdictions.                                                      *#
#*                                                                     *#
#***********************************************************************#

if ($^O eq "MacOS")
{
print "You need to set your Lib path (in Preferences)\n" .

	"if you want this to work É\n\n" if ($#INC == 0);
require "GUSI.ph"; # || die "GUSI.ph not found.";
}

@table_order = qw<
  head name vmtx BASE OS/2
>;

@feature_order = qw<
  aalt frac numr dnom dlig expt fwid hkna hwid jp78 jp83 nalt pwid
  ital liga qwid ruby subs sups trad twid zero vert vkna vrt2
  halt kern palt valt vhal vkrn vpal
>;

undef $/;

my($data)=" ";
print STDERR "Starting...\n";

if ($^O eq "MacOS") {
	FILE:while (1)
	{
		$file=(&MacPerl'Choose(&GUSI'AF_FILE, 0, "Choose an input file:")); 
		open(FEAT, $file) || last FILE;
 		while($line=<FEAT>){
			#print STDERR $line;
			$data=sprintf("%s%s",$data,$line);
		}
	}
}else{
	for ($i=0; $i<=$#ARGV; $i++){
		$file=$ARGV[$i];
		open(FEAT, $file) || last FILE;
 		while($line=<FEAT>){
			$data=sprintf("%s%s",$data,$line);
		}
	}
}
	#print STDERR ">$data<\n\n\n";


open(STDOUT,">features.auto") || die "Error opening output file\n";

@features = $data =~ m{
  \s*(?:feature|table)\s+
  (
    [a-zA-Z0-9/]{4}
    (?:\s+useExtension)?
    \s*{.+?}\s*
    [a-zA-Z0-9/]{4};
  )
  \s*
}gsx;

foreach $whole (@features) {
  ($feature,$subs) = $whole =~ m{
    ^
      ([a-zA-Z0-9/][a-zA-Z0-9/][a-zA-Z0-9/][a-zA-Z0-9/])
      (?:\s+useExtension)?
      \s*{\n+
      (.+)
      }\s*\1;\s*
    $
  }sx;
  $ext{$feature} = "useExtension " if $whole =~ /useExtension/s;
  $features{$feature} = $subs;
}

foreach $feature (@table_order) {
  if (exists $features{$feature}) {
    print STDOUT "table $feature {\n$features{$feature}} $feature;\n\n";
  }
}

foreach $feature (@feature_order) {
  if (exists $features{$feature}) {
    print STDOUT "feature $feature $ext{$feature}" . "{\n";
    print STDOUT "$features{$feature}} $feature;\n\n";
  }
}
print STDERR "Done.\n";
