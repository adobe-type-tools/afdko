#!/usr/bin/perl

# Written by Dr. Ken Lunde (lunde@adobe.com)
# Senior Computer Scientist, Adobe Systems Incorporated
# Version 08/18/2014
#
# This script takes a CIDFont resource as its only argument, and
# outputs to STDOUT a CIDFont resource with a corrected /FontBBox
# array. The original and corrected /FontBBox array values are output
# to STDERR.
#
# Tool Dependencies: tx (AFDKO)

$file = $ARGV[0];

# Set initial coordinates to the center of [0,-120,1000,880]

$llx = $urx = 500;
$lly = $ury = 380;

# Create and open AFM file

open(AFM,"tx -afm $file |") or die "Cannot open $file input file!\n";

while(defined($line = <AFM>)) {
    chomp $line;
    if ($line =~ /FontBBox/) {
        ($bbox1) = $line =~ /FontBBox\s+(.+)/;
    } elsif ($line =~ /^C\s+.+;\s+N\s+.+\s+;\s+B\s+(.+)\s+;/) {
        ($a,$b,$c,$d) = split(/\s+/,$1);
        if ($a < $llx) { $llx = $a; }
        if ($b < $lly) { $lly = $b; }
        if ($c > $urx) { $urx = $c; }
        if ($d > $ury) { $ury = $d; }
    }
}

close(AFM);

print STDERR "Original FontBBox: $bbox1\n";
print STDERR "Correct FontBBox: $llx $lly $urx $ury\n";

# Open CIDFont resource to change the /FontBBox array in its header

open(FILE,"<$file") or die "Cannot open $file input file!\n";

while(defined($line = <FILE>)) {
    if ($line =~ /\/FontBBox/) {
        $line = "/FontBBox {$llx $lly $urx $ury} def\n";
        print STDERR "Corrected FontBBox\n";
    }
    print STDOUT $line;
}

close(FILE);
