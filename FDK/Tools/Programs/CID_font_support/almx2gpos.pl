#!/usr/local/bin/perl
# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

#print STDERR "$^O\n";

if ($^O eq "MacOS")
{
	print "You need to set your Lib path (in Preferences)\n" .

		"if you want this to work É\n\n" if ($#INC == 0);
	require "GUSI.ph"; # || die "GUSI.ph not found.";
}
$palt = "feature palt {\n  script latn;\n  lookup ALL_PALT {\n";
$halt = "feature halt {\n  script latn;\n  lookup ALL_HALT {\n";
$vpal = "feature vpal {\n  script latn;\n  lookup ALL_VPAL {\n";
$vhal = "feature vhal {\n  script latn;\n  lookup ALL_VHAL {\n";

foreach $num (12000 .. 12069, 12784 .. 12831) {
  $ishw{$num} = 1;
}

if ($^O eq "MacOS")
{
		$file=(&MacPerl'Choose(&GUSI'AF_FILE, 0, "Choose an almx file:")); 
}else{
	#print "$#ARGV";
	if ($#ARGV<0)
	{
	 die "You need to specify the input almx file.";
	}
	$file=$ARGV[0];
}

open(ALMX, $file) || die "Error opening almx file.\n";
open(STDOUT,">features.alts") || die "Could not open output file!\n";

print STDERR "Processing ALMX file...\n";

while(defined($line = <ALMX>)) {
  next if $line !~ /^\s+\@/;
  ($vcid,$cid,$ha,$ho,$va,$vo) = $line =~ m{
    ^
      \s+
      \@([0-9]+)
      \s+
      \@([0-9]+)
      \s+
      \[(-?[0-9]+)\]\s+\[(-?[0-9]+)\]\s+\[(-?[0-9]+)\]\s+\[(-?[0-9]+)\]
      ;
    $
  }x;
  next if $ha == 1000 and $ho == 0 and $va == -1000 and $vo == 0;

  if ($ha == 1000) {
    $xa = 0;
  } else {
    $xa = $ha - 1000;
  }
  if ($ho == 0) {
    $xp = 0;
  } else {
    $xp = -($ho);
  }

  if ($va == -1000) {
    $ya = 0;
  } else {
    $ya = -($va + 1000);
  }
  if ($vo == 0) {
    $yp = 0;
  } else {
    $yp = -($vo); # Modified to reverse sign
  }

  $xp = 0 if $xp eq "-0";
  $xa = 0 if $xa eq "-0";
  $yp = 0 if $yp eq "-0";
  $ya = 0 if $ya eq "-0";

  if (exists $ishw{$vcid}) {
    $halt{$cid} = "$xp 0 $xa 0;" unless $xp == 0 and $xa == 0;
    $vhal{$cid} = "0 $yp 0 $ya;" unless $yp == 0 and $ya == 0;
  } else {
    $palt{$cid} = "$xp 0 $xa 0;" unless $xp == 0 and $xa == 0;
    $vpal{$cid} = "0 $yp 0 $ya;" unless $yp == 0 and $ya == 0;
  }
}

foreach $element (sort {$a <=> $b} keys %halt) {
  $halt .= "    position \\$element $halt{$element}\n";
}
foreach $element (sort {$a <=> $b} keys %palt) {
  $palt .= "    position \\$element $palt{$element}\n";
}
foreach $element (sort {$a <=> $b} keys %vhal) {
  $vhal .= "    position \\$element $vhal{$element}\n";
}
foreach $element (sort {$a <=> $b} keys %vpal) {
  $vpal .= "    position \\$element $vpal{$element}\n";
}

$halt .= "  } ALL_HALT;\n\n  script kana;\n  lookup ALL_HALT;\n\n";
$halt .= "  script hani;\n  lookup ALL_HALT;\n\n  script cyrl;\n  lookup ALL_HALT;\n\n";
$halt .= "  script grek;\n  lookup ALL_HALT;\n\n} halt;\n";

$palt .= "  } ALL_PALT;\n\n  script kana;\n  lookup ALL_PALT;\n\n";
$palt .= "  script hani;\n  lookup ALL_PALT;\n\n  script cyrl;\n  lookup ALL_PALT;\n\n";
$palt .= "  script grek;\n  lookup ALL_PALT;\n\n} palt;\n";

$vhal .= "  } ALL_VHAL;\n\n  script kana;\n  lookup ALL_VHAL;\n\n";
$vhal .= "  script hani;\n  lookup ALL_VHAL;\n\n  script cyrl;\n  lookup ALL_VHAL;\n\n";
$vhal .= "  script grek;\n  lookup ALL_VHAL;\n\n} vhal;\n";

$vpal .= "  } ALL_VPAL;\n\n  script kana;\n  lookup ALL_VPAL;\n\n";
$vpal .= "  script hani;\n  lookup ALL_VPAL;\n\n  script cyrl;\n  lookup ALL_VPAL;\n\n";
$vpal .= "  script grek;\n  lookup ALL_VPAL;\n\n} vpal;\n";

print STDOUT "$halt\n$palt\n$vhal\n$vpal";
close(ALMX);
print STDERR "Done. Output to features.alts.\n";
