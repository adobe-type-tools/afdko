#!/usr/bin/perl

# Written by Dr. Ken Lunde (lunde@adobe.com)
# Senior Computer Scientist, Adobe Systems Incorporated
# Version 08/18/2014
#
# This tool takes a CFF resource/table (instantiated as a file) or an
# OpenType/CFF (name- or CID-keyed) as its only argument, and reports
# the number of global (for name- and CID-keyed fonts) and local (for
# CID-keyed fonts only) subroutines are present, their sizes in bytes,
# and whether the number of subroutines exceeds architectural (64K - 3
# = 65,533) or known implementation-specific (32K - 3 = 32,765)
# limits.
#
# Mac OS X Version 10.4 (aka, Tiger) and earlier, along with Adobe
# Acrobat Distiller Version 7.0 and earlier, are known implementations
# whose subroutine limit is 32K - 3 (32,765).
#
# Tool Dependencies: tx (AFDKO)

if ($ARGV[0] =~ /^-[huHU]/) {
  print STDERR "Usage: subr-check.pl <CFF|OpenTypeCFF>\n";
  exit;
} else {
  $file = "\"$ARGV[0]\"";
}

$fd = "global";

# Extract the number of global and local subroutines, and store in hash

open(FILE,"tx -dcf -1 -T gl $file |") or die "Cannot open $file input file!\n";

while(defined($line = <FILE>)) {
    next if $line !~ /^(?:---\sFD|count)/;
    if ($line =~ /^count\s+=(\d+)/) {
        $data->{$fd} = $1;
    } elsif ($line =~ /^---\sFD\[(\d+)\]/) {
        $fd = $1;
    }
}
close(FILE);

$fd = "global";

# Extract the sizes of the global and local subroutines, and store in hash

open(FILE,"tx -dcf -0 -T gl $file |") or die "Cannot open $file input file!\n";

while(defined($line = <FILE>)) {
    next if $line !~ /^(?:---\sFD|###)/;
    if ($line =~ /^### (?:Global|Local).+\(([0-9a-f]{8})-([0-9a-f]{8})\)/) {
        $size->{$fd} = hex($2) - hex($1) + 1;
    } elsif ($line =~ /^---\sFD\[(\d+)\]/) {
        $fd = $1;
    }
}
close(FILE);

printf STDOUT "Global Subroutines: %d (%d bytes)",$data->{"global"},$size->{"global"};

$totalsize = $size->{"global"};

if ($data->{"global"} > 65533) {
    print STDOUT " > 64K-3 limit";
} elsif ($data->{"global"} > 32765) {
    print STDOUT " > 32K-3 limit";
}

print STDOUT "\nLocal Subroutines:";

if (scalar keys %{ $data } == 1) {
    print STDOUT " 0\n";
} else {
    print STDOUT "\n";
    foreach $index (sort {$a <=> $b} keys %{ $data }) {
        next if $index eq "global";
        printf STDOUT "  FD=${index}: %d (%d bytes)",$data->{$index},$size->{$index};
        $totalsize += $size->{$index};
        if ($data->{$index} + $data->{"global"} > 65533) {
            printf STDOUT " > 64K-3 limit: %d (%d + %d)",$data->{$index} + $data->{"global"},$data->{$index},$data->{"global"};
        } elsif ($data->{$index} + $data->{"global"} > 32765) {
            printf STDOUT " > 32K-3 limit: %d (%d + %d)",$data->{$index} + $data->{"global"},$data->{$index},$data->{"global"};
        }
        print STDOUT "\n";
    }
}

print STDOUT "Total Subroutine Size: $totalsize bytes\n";
