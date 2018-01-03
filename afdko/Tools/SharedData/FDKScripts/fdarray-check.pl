#!/usr/bin/perl

# Written by Dr. Ken Lunde (lunde@adobe.com)
# Senior Computer Scientist, Adobe Systems Incorporated
# Version 08/18/2014
#
# This tool takes a CID-keyed font, which can be a CIDFont resource
# (instantiated as a file), CFF resource/table (instantiated as a
# file), or an OpenType/CFF font, as its only argument, and reports
# to STDOUT the name of each FDArray element, along with its index in
# parentheses, the CIDs that are assigned to it, and the total number
# of CIDs in parentheses. The CIDs are prefixed with a slash to
# explicitly indicate CIDs, as opposed to GIDs, which is useful when
# using the "-g" or "-gx" command-line options for many AFDKO tools,
# especially when GIDs do not equal CIDs in a particular font. The
# slash prefix can be suppressed by specifying the "-g" command-line
# option, and the values are still CIDs, not GIDs. The ROS (/Registry,
# /Ordering, and /Supplement) of the CID-keyed font is also reported
# to STDOUT. This tool also reports, via STDERR, whether GIDs do not
# equal CIDs in the specified font, and starting from which GID/CID.
#
# Tool Dependencies: tx (AFDKO)

$pre = "/";
$gidNEcid = 0;

while ($ARGV[0]) {
    if ($ARGV[0] =~ /^-[huHU]/) {
        print STDERR "Usage: fdarray-check.pl [-g] <CID-keyed font>\n";
        exit;
    } elsif ($ARGV[0] =~ /^-[gG]/) {
        $pre = "";
        shift;
    } else {
        $file = "\"$ARGV[0]\"";
        shift;
    }
}

open(FILE,"tx -1 $file |") or die "Cannot open $file input file!\n";

while(defined($line = <FILE>)) {
    chomp $line;
    if ($line =~ /^cid\.Registry\s+\"(.+)\"/) {
        $r = $1;
    } elsif ($line =~ /^cid\.Ordering\s+\"(.+)\"/) {
        $o = $1;
    } elsif ($line =~ /^cid\.Supplement\s+(\d+)/) {
        $s = $1;
    } elsif ($line =~ /^## FontDict\[(\d+)\]/) {
        $index = $1;
    } elsif ($line =~ /^FontName\s+\"(.+)\"/) {
        $fdarray->{$index}{NAME} = $1;
    } elsif ($line =~ /^glyph\[(\d+)\]\s+{(\d+),(\d+)(?:,[01])?}/) {
        $gid = $1;
        $cid = $2;
        $index = $3;
        if (not exists $fdarray->{$index}{CIDs}) {
            $fdarray->{$index}{CIDs} = $cid;
        } else {
            $fdarray->{$index}{CIDs} .= "," . $cid;
        }
        if (not $gidNEcid and $gid != $cid) {
            $gidNEcid = 1;
            print STDERR "NOTE: GIDs do not equal CIDs starting from GID+$gid/CID+$cid!\n";
        }
    } elsif ($line =~ /^glyph\[\d+\]\s+{[^0-9].*,/) {
	die "ERROR: name-keyed font! Quitting...\n";
    }
}

print STDOUT "Detected ROS: $r-$o-$s\n";

foreach $element (sort {$a <=> $b} keys %{ $fdarray }) {
    printf STDOUT "%s ($element): ",$fdarray->{$element}{NAME};
    @cids = split(/,/,$fdarray->{$element}{CIDs});
    $count = scalar @cids;
    $second = 0;
    foreach $cid (@cids) {
        if (not $second) {
            $orig = $previous = $cid;
            $second = 1;
            next;
        }
        if ($cid != $previous + 1) {
            if ($orig == $previous) {
                print STDOUT "$pre$orig,";
            } else {
                print STDOUT "$pre$orig-$pre$previous,";
            }
            $orig = $previous = $cid;
        } else {
            $previous = $cid;
        }
    }
    if ($orig == $previous) {
        print STDOUT "$pre$orig";
    } else {
        print STDOUT "$pre$orig-$pre$previous";
    }
    print STDOUT " ($count)\n";
}
