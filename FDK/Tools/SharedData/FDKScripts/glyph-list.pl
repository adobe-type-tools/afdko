#!/usr/bin/perl

# Written by Dr. Ken Lunde (lunde@adobe.com)
# Senior Computer Scientist, Adobe Systems Incorporated
# Version 03/30/2016
#
# This tool lists the glyphs in the specified font, which can be a
# CIDFont resource, a name-keyed Type 1 font (PFA), or an 'sfnt'
# (TrueType or OpenType) font. By default, glyphs are listed as CIDs
# or glyph names, depending on whether the font is CID- or name-keyed.
# CIDs are prefixed with a slash. The "-g" command-line option will
# list GIDs in lieu of CIDs or glyph names. The "-r" command-line
# option will turn the list of CIDs or GIDs into ranges. The "-s"
# command-line option will additionally output lists or ranges onto
# a single line with comma separators so that it can be repurposed, such
# as to be used as the argument of the "-g" or "-gx" command-line
# options that are supported by many AFDKO tools.
#
# Tool Dependencies: tx (AFDKO)

$usegid = $second = $range = 0;
$iscid = 1;
$sep = "\n";
$prefix = "/";
$data = "";

while ($ARGV[0]) {
    if ($ARGV[0] =~ /^-[huHU]/) {
        print STDERR "Usage: glyph-list.pl [-g] [-r] [-s] <font>\n";
        exit;
    } elsif ($ARGV[0] =~ /^-[gG]/) {
        $usegid = 1;
        $prefix = "";
        shift;
    } elsif ($ARGV[0] =~ /^-[rR]/) {
        $range = 1;
        shift;
    } elsif ($ARGV[0] =~ /^-[sS]/) {
        $sep = ",";
        shift;
    } else {
        $file = "\"$ARGV[0]\"";
        shift;
    }
}

open(FILE,"tx -1 $file |") or die "Cannot open $file input file!\n";

while(defined($line = <FILE>)) {
    chomp $line;
    if ($line =~ /^sup\.srcFontType/) {
        if ($line =~ /(?:TrueType|name-keyed)/) {
            $iscid = 0;
            $range = 0 if not $usegid;
            $prefix = "";
        }
        next;
    }
    next if $line !~ /glyph\[\d+\]/;
    if ($usegid) {
        ($glyph) = $line =~ /glyph\[(\d+)\]/;
    } else {
        ($glyph) = $line =~ /{(.+?),/;
    }
    if ($range) {
        if (not $second) {
            $orig = $previous = $glyph;
            $second = 1;
            next;
        }
        if ($glyph != $previous + 1) {
            if ($orig == $previous) {
                $data .= "$prefix$orig$sep";
            } else {
                $data .= "$prefix$orig-$prefix$previous$sep";
            }
            $orig = $previous = $glyph;
        } else {
            $previous = $glyph;
        }
    } else {
        if (not $sep) {
            $data .= "$prefix$glyph\n";
        } else {
            if (not $data) {
                $data .= "$prefix$glyph";
            } else {
                $data .= "$sep$prefix$glyph";
            }
        }
    }
}

if ($range) {
    if ($orig == $previous) {
        $data .= "$prefix$orig\n";
    } else {
        $data .= "$prefix$orig-$prefix$previous\n";
    }
} else {
    $data .= "\n";
}

print STDOUT $data;
