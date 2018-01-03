#!/usr/bin/perl

$hintparam = $ARGV[0];

open(FILE,"<$hintparam") or die "Cannot open $hintparam input file!\n";

while(defined($line = <FILE>)) {
    chomp $line;
    if ($line =~ /^([A-Z][A-Za-z0-9]?.+)$/) {
        $hintdictname1 = $1;
    } elsif ($line =~ /(\/(?:BlueValues|OtherBlues|Std(?:H|V)W|StemSnap(?:H|V))\s+\[\s*.+\s*\]\s+def)/) {
        $hintdata{$hintdictname1} .= $1 . "\n";
    }
}

while(defined($line = <STDIN>)) {
    if ($line =~ /\/CIDFontName\s+\/(.+)\s+def/) {
        $cidfontname = $1;
        print STDERR "Detected CIDFontName: $cidfontname\n";
    } elsif ($line =~ /\/FontName \/${cidfontname}-(.+)\s+def/) {
        $hintdictname2 = $1;
        print STDERR "Modifying $hintdictname2 hinting parameters...\n";
    } elsif ($line =~ /\/Private \d+ dict dup begin/) {
        print STDOUT "/Private 15 dict dup begin\n";
        print STDOUT $hintdata{$hintdictname2};
        next;
    } elsif ($line =~/\/(?:Blue(?:Values|Scale)|(?:Family)?(?:Other)?Blues|Std(?:H|V)W|StemSnap(?:H|V)|RndStemUp)/) {
        next;
    }
    print STDOUT $line;
}
