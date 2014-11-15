#!/usr/local/bin/perl
# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

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
