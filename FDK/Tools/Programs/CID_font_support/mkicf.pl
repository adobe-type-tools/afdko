#!perl --			# -*- Perl -*- 
# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# This program was written by Ken Lunde (lunde@adobe.com) for building
# ACF information using only a CID-keyed AFM file and CMap file as input.

$num = 0;

if ($^O eq "MacOS")
{
print "You need to set your Lib path (in Preferences)\n" .

	"if you want this to work É\n\n" if ($#INC == 0);
require "GUSI.ph"; # || die "GUSI.ph not found.";
}

use integer;

if ($^O eq "MacOS")
{
	$file1=(&MacPerl'Choose(&GUSI'AF_FILE, 0, "Choose a CMAP file:"));

 $file2=(&MacPerl'Choose(&GUSI'AF_FILE, 0, "Choose AFM file:"));
}else{
	if ($#ARGV<1){
		die "You need to specify both the CMAP file and the AFM file";
	}
	$file1=$ARGV[0];
 $file2=$ARGV[1];
 
}

$count = $llx = $urx = $lly = $ury = 0;

open (STDOUT, ">features.BASE") || die "Error opening output file.\n";

open(CMAP, $file1) || die "Error opening CMap file.\n";
print STDERR "Storing CMap mappings into lookup structure...";
while (defined($line = <CMAP>)) {
  if ($line =~ /begincidrange$/) {
    chomp($line = <CMAP>);
    until ($line =~ /^endcidrange/) {
      ($begin,$end,$cid) = $line =~ m{
        ^
          < ([0-9A-Fa-f]+) > \s+
          < ([0-9A-Fa-f]+) > \s+
          (\d+)
        $
      }x;
      foreach $char (hex($begin) .. hex($end)) {
        if ($kana) {
          if ($char >= hex("3041") and $char <= hex("3094") or
              $char >= hex("30A1") and $char <= hex("30FA")) {
            $code = sprintf("%02X",$char);
            $code2cid{$code} = $cid;
            $cid2code{$cid} = $code;
            $count++;
            $cid++;
          }
	} elsif ($hangul) {
          if ($char >= hex("AC00") and $char <= hex("D7A3")) {
            $code = sprintf("%02X",$char);
            $code2cid{$code} = $cid;
            $cid2code{$cid} = $code;
            $count++;
            $cid++;
          }
        } else {
          if ($char >= hex("4E00") and $char <= hex("9FA5") or
              $char >= hex("F900") and $char <= hex("FA2D") or
              $char >= hex("3041") and $char <= hex("3094") or
              $char >= hex("30A1") and $char <= hex("30FA") or
              $char >= hex("AC00") and $char <= hex("D7A3")) {
            $code = sprintf("%02X",$char);
            $code2cid{$code} = $cid;
            $cid2code{$cid} = $code;
            $count++;
            $cid++;
          }
        }
      }
      chomp($line = <CMAP>);
    }
  }
}
close(CMAP);
print STDERR "Done.\n";

open(AFM, $file2) || die "Error opening AFM file.\n";

print STDERR "Storing AFM records for ";
while (defined($line = <AFM>)) {
  chomp $line;
  if ($line =~ /^FontName/) {
    ($fontname) = $line =~ /^FontName\s+(.*)$/;
    print STDERR "\"$fontname\" CIDFont into lookup structure...";
  } elsif ($line =~ /^Version/) {
    ($version) = $line =~ /^Version\s+(.*)$/;
  } elsif ($line =~ /^Notice/) {
    ($notice) = $line =~ /^Notice\s+(.*)$/;
    $notice =~ s/\([Cc]\)\s+//;
  } elsif ($line =~ /^StartCharMetrics/) {
    while ($line !~ /^EndCharMetrics/) {
      chomp($line = <AFM>);
      ($width,$cid,$bbox,$a,$b,$c,$d) = $line =~ m{
        ^
          \s* C \s+ -1 \s+ ; \s+
          W0X \s+ (\d+) \s+ ; \s+
          N \s+ (\d+) \s+ ; \s+
          B \s+ ((-?\d+) \s+ (-?\d+) \s+ (-?\d+) \s+ (-?\d+)) \s+ ; \s*
        $
      }x;
      if (exists $cid2code{$cid}) {
        $num++;
        $data{$cid} = "W0X $width ; B $bbox ;";
        if ($bbox ne "0 0 0 0") {
          $llx += $a;
          $lly += $b;
          $urx += $c;
          $ury += $d;
        }
      }
    }
  }
}
print STDERR "Done.\n";

$left = $llx / $num;
$right = 1000 - ($urx / $num);
$bottom = 120 + ($lly / $num);
$top = 880 - ($ury / $num);

$result = ($left + $right + $bottom + $top) / 4;

$left = $result;
$right = 1000 - $result;
$bottom = -120 + $result;
$top = 880 - $result;

print STDOUT <<EOF;
table BASE {
  HorizAxis.BaseTagList                 icfb  icft  ideo  romn;
  HorizAxis.BaseScriptList  hani  ideo   $bottom  $top   -120  0,
                            kana  ideo   $bottom  $top   -120  0,
                            latn  romn   $bottom  $top   -120  0,
                            cyrl  romn   $bottom  $top   -120  0,
                            grek  romn   $bottom  $top   -120  0;

  VertAxis.BaseTagList                  icfb  icft  ideo  romn;
  VertAxis.BaseScriptList   hani  ideo  $left    $right   0     120,
                            kana  ideo  $left    $right   0     120,
                            latn  romn  $left    $right   0     120,
                            cyrl  romn  $left    $right   0     120,
                            grek  romn  $left    $right   0     120;
} BASE;
EOF

close(CMAP);