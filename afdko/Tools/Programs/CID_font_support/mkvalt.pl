#!/usr/local/bin/perl 
# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# This script requires an input file in the style of AFM as created by
# mkcidfont and saved in the Metrics directory:
# such a line looks like: C -1 ; W0X 238 ; N 13 ; B 63 -106 179 107 ;
# Output is STDOUT and contains the necessary adjustments in 
# GPOS style (position \[CID] [0] [YPlacement] [0] [0];)
# in order to vertically center the glyphs in the em-box, calculations are done
# on the basis of the Character Bounding Box.
# If run on a Mac that file can be dropped onto the mkvalt.pld droplet.
# This scripts also requires an input file named CIDlist which must reside
# in the same folder as the script.
# Note: If necessary the values for the em-box have to be adjusted,
# they are currently set to 0,1000 and -120,880.
if ($^O eq "MacOS")
{
print "You need to set your Lib path (in Preferences)\n" .

	"if you want this to work É\n\n" if ($#INC == 0);
require "GUSI.ph"; # || die "GUSI.ph not found.";
}
if ($^O eq "MacOS")
{
	$file1=(&MacPerl'Choose(&GUSI'AF_FILE, 0, "Choose CIDList file:"));

 $file2=(&MacPerl'Choose(&GUSI'AF_FILE, 0, "Choose AFM file:"));
}else{
	if ($#ARGV<1){
		die "You need to specify both the CIDList and AFM file";
	}
	$file1=$ARGV[0];
 $file2=$ARGV[1];
 
}

open(CIDS,"<$file1") || help("CIDlist not found.\n");
open(STDOUT,">features.valt") || die "Could not open output file!\n";

print STDERR "Loading CIDs ...\n";

$count = 0;
while ($line = <CIDS>) {
  chomp $line;
  ++$count;
  $line =~ s/^\s*(.*)\s*$/$1/;
  $line =~ s/\s+/ /;

  if ($line =~ /-/) {
    ($cidstart,$cidend) = split /-/, $line; 
  } else {
    $cidstart = $line;
    $cidend = $cidstart; }
  

  foreach $cid ($cidstart .. $cidend) {
				$cidrange{$cid} = 1;
  }
}

print STDOUT <<EOF;
feature valt {
  script latn;
  lookup ALL_VALT {
EOF

open(AFM,"<$file2") || help("Input file missing\n");

while ($line = <AFM>) {
  if ($line =~ /^C.*/) {
    chomp $line;
    ($t1,$t2,$t3,$t4,$width,$t5,$t6,$cid,$t7,$t8,$xmin,$ymin,$xmax,$ymax,$t9) = split / /, $line;
    if (exists $cidrange{$cid}) { 
						$ybottomlimit = -120;
      $ytoplimit = 880;

      $ycenter = ($ybottomlimit + $ytoplimit) / 2;

      $halfy = ($ymax - $ymin) / 2;
      
      $yminnew = $ycenter - $halfy;
      $ymaxnew = $ycenter + $halfy;

      $pushy = $yminnew - $ymin;

      $pushy = round($pushy);
      $pushy =~ s/^\+(\d+)/$1/;
      $pushy = "0" if $pushy eq "-0";
      
      print STDOUT "    position \\$cid 0 $pushy 0 0;\n" if $pushy !~ /^-?[0-4]$/;
  		}
  }
}

print STDOUT <<EOF;
  } ALL_VALT;

  script kana;
  lookup ALL_VALT;

  script hani;
  lookup ALL_VALT;

  script cyrl;
  lookup ALL_VALT;

  script grek;
  lookup ALL_VALT;

} valt;
EOF

sub round {
  my ($arg) = @_;
  if ($arg =~ /\.[0-4]\d*/) {
    $arg =~ s/\.[0-4]\d*$//;
  } elsif ($arg =~ /\.[5-9]\d*/) {
    $arg =~ s/(\d+)\.[5-9]\d*$/$1 + 1/e;
  } return $arg;
}
sub help {
	my ($arg) = @_;
	print STDERR $arg;
	print STDERR "\n";
	print STDERR <<EOF;
mkvalt.pl
---------
This script is used to create the valt feature used to build an OpenType font.
It requires an AFM-style file as input. Such a file is composed of lines such as:
C -1 ; W0X 238 ; N 13 ; B 63 -106 179 107 ;
Output is the file features.valt and contains the necessary adjustments in 
GPOS style (position \[CID] [0] [YPlacement] [0] [0];)
in order to vertically center the glyphs in the em-box, calculations are done
on the basis of the Character Bounding Box.
If run on a Mac that file must be dropped onto the mkvalt.pl droplet.
This scripts also requires an input file named CIDlist which must reside
in the same folder as the script.
Note: If necessary the values for the em-box have to be adjusted,
they are currently set to 0,1000 and -120,880.
EOF
	die;	
}

print STDERR "Done. Output is 'features.valt'.";
close(CIDS);
close(AFM);

