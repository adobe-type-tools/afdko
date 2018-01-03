#!/usr/bin/perl

# Written by Dr. Ken Lunde (lunde@adobe.com)
# Senior Computer Scientist 2, Adobe Systems Incorporated
# Version 11/08/2015
#
# This tool takes a valid CMap resource as STDIN, and outputs to STDOUT
# an optimized -- in terms of representing contiguous code points and
# contiguous CIDs as ranges -- CMap resource. Various error checking is
# performed, to include sorting the code points, detecting duplicate code
# points, and checking for out-of-range code points. If the "-e" command-
# line option is specified, a single "begincidchar/endcidchar" block that
# contains single mappings is output to STDOUT, which is considered the
# uncompiled form. If the tool detects that a UTF-32 CMap resource is
# being compiled, by having "UTF32" as part of the /CMapName, corresponding
# UTF-8 and UTF-16 CMap resources are compiled. If STDIN does not include
# a /UIDBase value or /XUID array, the CMap resources that are generated
# will not include them. The BSD license is included as part of the CMap
# resource header.
#
# Tool Dependencies: None

$year = (localtime)[5] + 1900;

$expand = $expanded = 0;
$outfile = "STDOUT";

# UTF-8 and UTF-16 code space ranges

$cs8 = "4 begincodespacerange\n  <00>       <7F>\n  <C080>     <DFBF>\n  <E08080>   <EFBFBF>\n  <F0808080> <F7BFBFBF>\nendcodespacerange\n";
$cs16 = "3 begincodespacerange\n  <0000>     <D7FF>\n  <D800DC00> <DBFFDFFF>\n  <E000>     <FFFF>\nendcodespacerange\n";

# UTF-8 and UTF-16 .notdef ranges for Adobe's public ROSes

$nd8 = "\n1 beginnotdefrange\n<00> <1f> 1\nendnotdefrange\n";
$nd16 = "\n1 beginnotdefrange\n<0000> <001f> 1\nendnotdefrange\n";

# UTF-8 and UTF-16 XUID arrays for Adobe's public ROSes

%xuid = ( # XUID array values for UTF-8 and UTF-16 CMap files
  "UniGB-UTF8-H"  => "1 10 25379",
  "UniGB-UTF8-V"  => "1 10 25450",
  "UniGB-UTF16-H" => "1 10 25601",
  "UniGB-UTF16-V" => "1 10 25602",
  "UniCNS-UTF8-H"  => "1 10 25397",
  "UniCNS-UTF8-V"  => "1 10 25398",
  "UniCNS-UTF16-H" => "1 10 25591",
  "UniCNS-UTF16-V" => "1 10 25592",
  "UniJIS-UTF8-H"  => "1 10 25359",
  "UniJIS-UTF8-V"  => "1 10 25440",
  "UniJIS-UTF16-H" => "1 10 25611",
  "UniJIS-UTF16-V" => "1 10 25612",
  "UniJIS2004-UTF8-H"  => "1 10 25631",
  "UniJIS2004-UTF8-V"  => "1 10 25632",
  "UniJIS2004-UTF16-H" => "1 10 25619",
  "UniJIS2004-UTF16-V" => "1 10 25630",
  "UniHojo-UTF8-H"  => "1 10 25428",
  "UniHojo-UTF8-V"  => "1 10 25429",
  "UniHojo-UTF16-H" => "1 10 25513",
  "UniHojo-UTF16-V" => "1 10 25514",
  "UniKS-UTF8-H"  => "1 10 25414",
  "UniKS-UTF8-V"  => "1 10 25415",
  "UniKS-UTF16-H" => "1 10 25544",
  "UniKS-UTF16-V" => "1 10 25545",
);

while ($ARGV[0]) {
  if ($ARGV[0] =~ /^-[huHU]/) {
    print STDERR "Usage: cmap-tool.pl [-e] < STDIN\n";
    exit;
  } elsif ($ARGV[0] =~ /^-[eE]/) {
    $expand = 1;
    shift;
  } else {
    print STDERR "Invalid option: $ARGV[0]! Skipping...\n";
    shift;
  }
}

undef $/;
$data = <STDIN>;
%cmap = &GetData($data); # Read in CMap file's information

if (exists $cmap{UseCMap}) { # Read in primary CMap file's information
  open(HCMAP,"< $cmap{UseCMap}") or die "Cannot locate horizontal CMap file";
  $data2 = <HCMAP>;
  close(HCMAP);
  %cmap2 = &GetData($data2);
}

if (exists $cmap{CodeSpace}) { # Store codespace information
  &StoreCodeSpace($cmap{CodeSpace});
} else {
  &StoreCodeSpace($cmap2{CodeSpace});
}

%single_mappings = &StoreMappings($cmap{Mappings}) if exists $cmap{Mappings};

$cmap{NewMappings} = &CreateMappings(%single_mappings) if exists $cmap{Mappings};

if ($cmap{Name} eq "$cmap{Registry}-$cmap{Ordering}-$cmap{Supplement}") {
  &PrintIdentityCMap(%cmap);
} elsif (exists $cmap{UseCMap}) {
  &PrintUseCMap(%cmap);
} else {
  &PrintCMap(%cmap);
}

if ($cmap{Name} =~ /UTF32/ and not $expand) {
  foreach $utf (8, 16) {
    $outfile = "OUTFILE";
    $cmap{Name} =~ s/UTF32/UTF$utf/;
    print STDERR "Automatically creating $cmap{Name}...";
    $cmap{UseCMap} =~ s/UTF32/UTF$utf/ if $cmap{UseCMap};
    $cmap{XUID} = $xuid{$cmap{Name}};
    if (exists $cmap{CodeSpace}) {
      if ($utf == 8) {
        $cmap{CodeSpace} = $cs8;
      } else {
        $cmap{CodeSpace} = $cs16;
      }
      &StoreCodeSpace($cmap{CodeSpace});
    } else {
      if ($utf == 8) {
        $cmap2{CodeSpace} = $cs8;
      } else {
        $cmap2{CodeSpace} = $cs16;
      }
      &StoreCodeSpace($cmap2{CodeSpace});
    }
    if ($utf == 8) {
      $cmap{NotDef} = $nd8;
      %utf_mappings = &Convert2UTF8(%single_mappings);
    } else {
      $cmap{NotDef} = $nd16;
      %utf_mappings = &Convert2UTF16(%single_mappings);
    }
    $cmap{NewMappings} = &CreateMappings(%utf_mappings) if exists $cmap{Mappings};
    open($outfile,">$cmap{Name}") or die "Cannot open $cmap{Name} file!\n";
    if (exists $cmap{UseCMap}) {
      &PrintUseCMap(%cmap);
    } else {
      &PrintCMap(%cmap);
    }
    close($outfile);
    print STDERR "Done.\n";
    $cmap{Name} =~ s/UTF$utf/UTF32/;
    $cmap{UseCMap} =~ s/UTF$utf/UTF32/ if $cmap{UseCMap};
  }
}

sub GetData ($) { # For extracting CMap information
  my ($cmapfile) = @_;
  my ($usecmap,$r,$o,$s,$name,$version,$uidoffset,$xuid,$wmode) = ("","","","","","","","","");
  my ($codespacerange,$notdefrange,$mappings) = ("","","");
  undef %data;

  ($usecmap) = $cmapfile =~ m{/([0-9a-zA-Z-]+)\s+usecmap};

  ($r,$o,$s) = $cmapfile =~ m{
    \s* /CIDSystemInfo \s+ \d+ \s+ dict \s+ dup \s+ begin
    \s* /Registry   \s+ \( (.+) \) \s+ def
    \s* /Ordering   \s+ \( (.+) \) \s+ def
    \s* /Supplement \s+ (\d+)      \s+ def
    \s* end \s+ def
  }msx;

  ($name) = $cmapfile =~ m{/CMapName\s+/([0-9a-zA-Z-]+)\s+def};
  ($version) = $cmapfile =~ m{/CMapVersion\s+(\d+(?:\.\d*))\s+def};
  ($uidoffset) = $cmapfile =~ m{(/UIDOffset\s+\d+)\s+def};
  ($xuid) = $cmapfile =~ m{/XUID\s+\[([0-9 ]+)\]\s+def};
  ($wmode) = $cmapfile =~ m{/WMode\s+([01])\s+def};
  ($codespacerange) = $cmapfile =~ m{
    (
      \d+
      \s+
      begincodespacerange
        .+
      endcodespacerange
      \s
    )
  }msx;
  ($notdefrange) = $cmapfile =~ m{
    (
      \s
      \d+
      \s+
      beginnotdefrange
        .+
      endnotdefrange
      \s
    )
  }msx;
  ($mappings) = $cmapfile =~ m{
    (
      \d+
      \s+
      begin(?:bf|cid)(?:char|range)
        .+
      end(?:bf|cid)(?:char|range)
    )
    \s+
    endcmap
  }msx;

  $data{UseCMap} = $usecmap if $usecmap;
  $data{Registry} = $r if $r;
  $data{Ordering} = $o if $o;
  $data{Supplement} = $s if $s or $s eq "0";
  $data{Name} = $name if $name;
  $data{Version} = $version if $version;
  $data{UIDOffset} = "$uidoffset def\n" if $uidoffset;
  $data{XUID} = $xuid if $xuid;
  $data{WMode} = $wmode if $wmode or $wmode eq "0";
  $data{CodeSpace} = $codespacerange if $codespacerange;
  $data{NotDef} = $notdefrange if $notdefrange;
  $data{Mappings} = $mappings if $mappings;

  return %data;
}

sub StoreCodeSpace ($) { # Store the codespace information
  my ($info) = @_;
  my $index = 0;

  @lines = split(/[\r\n]+/,$info);
  foreach $line (@lines) {
    if (($s,$e) = $line =~ /^\s*<([\dA-F]+)>\s+<([\dA-F]+)>\s*$/) {
      $enc_l[$index] = length($s);
      $enc_s[$index] = hex($s);
      $enc_e[$index] = hex($e);
      $index++;
    }
  }
}

sub StoreMappings ($) {
  my ($mappings) = @_;
  my $processed = 0;

  ($bforcid) = $mappings =~ /begin(bf|cid)(?:char|range)/;
  @lines = split(/[\r\n]+/,$mappings);

  print STDERR "Reading CMap file into a lookup structure...";
  foreach $line (@lines) {
    $line = uc $line;
    if ($line =~ /^\s*<([\dA-F]+)>\s+(\d+)\s*$/) { # char
      $processed++;
      $d = &GetLengthOnly(hex($1));
      $hex = sprintf("%0${d}X",hex($1));
      if (not defined $mapping{"0x" . $hex}) { # MODIFIED
        $mapping{"0x" . $hex} = $2; #MODIFIED
        $expanded++;
      } else {
        printf STDERR "Duplicate mapping at %08X!\n",hex($1);
      }
    } elsif ($line =~ /^\s*<([\dA-F]+)>\s+<([\dA-F]+)>\s+(\d+)\s*$/) { # range
      $processed++;
      $cid = $3;
      for ($dec = hex($1); $dec <= hex($2); $dec++) {
        $d = &GetLengthOnly($dec);
        $hex = sprintf("%0${d}X",$dec);
        if (not defined $mapping{"0x" . $hex}) { # MODIFIED "0x" .
          $mapping{"0x" . $hex} = $cid; # MODIFIED
        } else {
          printf STDERR "Duplicate mapping at %08X!\n",$dec;
        }
        $expanded++;
        $cid++;
      }
    }
  }
  print STDERR "Done.\n";
  print STDERR "Processed $processed CMap lines into $expanded mappings.\n";
  return %mapping;
}

sub CreateMappings (%) {
  my (%single_mappings) = @_;
  my ($e,$s,$char,$range,$charbuff,$rangebuff) = ("","","","","","");
  my ($collapsed,$count,$i,$tallyi,$j,$tallyj,$tally,$start) = (0,0,0,0,0,0,0,0);
  $cmap{NewMappings} = "";

  print STDERR "Processing the mappings...";
  if ($expand) {
    if ($expanded) {
      $cmap{NewMappings} .= "$expanded begin${bforcid}char\n";
      foreach $entry (sort ByThings keys %single_mappings) {
        $d = &GetLength(hex($entry),hex($entry));
        $cmap{NewMappings} .= sprintf("<%0${d}x> $single_mappings{$entry}\n",hex($entry));
      }
      $cmap{NewMappings} .= "end${bforcid}char\n";
    }
    print STDERR "Done.\n";
  } elsif ($expanded) {
    foreach $entry (sort ByThings keys %single_mappings) {
      $e = $s = hex($entry); $c = $single_mappings{$entry};
      unless (not $start) {
        if ((($s % 256) == (($oe % 256) + 1)) && ($s == ($oe + 1)) &&
        ($c == ($oc + ($oe - $os) + 1))) {
           $c = $oc; $s = $os;
           $collapsed++;
         } else {
           $d = &GetLength($os,$oe);
           if ($os == $oe) {
             $j++;
             if ($j < 100) {
               $char .= sprintf("<%0${d}x> $oc\n",$os);
               $count++;
             } else {
               $char .= sprintf("<%0${d}x> $oc\n",$os);
               $char .= "end${bforcid}char\n\n";
               $count++;
               $charbuff .= "$j begin${bforcid}char\n$char" if $j;
               $tallyj += $j;
               $char = "";
               $j = 0;
             }
           } else {
             $i++;
             if ($i < 100) {
               $range .= sprintf("<%0${d}x> <%0${d}x> $oc\n",$os,$oe);
               $count += ($oe - $os) + 1;
             } else {
               $range .= sprintf("<%0${d}x> <%0${d}x> $oc\n",$os,$oe);
               $range .= "end${bforcid}range\n\n";
               $count += ($oe - $os) + 1;
               $rangebuff .= "$i begin${bforcid}range\n$range" if $i;
               $tallyi += $i;
               $range = "";
               $i = 0;
             }
           }
         }
      }
      $oe = $e; $os = $s; $oc = $c; $start = 1;
    }
  }

  if (not $expand) {
    if ($expanded) {
      $d = &GetLength($os,$oe);
      if ($os == $oe) {
        $char .= sprintf("<%0${d}x> $oc\n",$os);
        $count++;
        $j++;
      } else {
        $range .= sprintf("<%0${d}x> <%0${d}x> $oc\n",$os,$oe);
        $count += ($oe - $os) + 1;
        $i++;
      }
      $tallyi += $i;
      $tallyj += $j;
      $charbuff .= "$j begin${bforcid}char\n${char}end${bforcid}char\n" if $j;
      $rangebuff .= "$i begin${bforcid}range\n${range}end${bforcid}range\n" if $i;
      $cmap{NewMappings} .= "$charbuff\n" if $tallyj;
      $cmap{NewMappings} .= "$rangebuff\n" if $tallyi;
    }
    $tally = $tallyi + $tallyj;
    print STDERR "Done.\n";
    print STDERR "Collapsed $collapsed of $expanded mappings.\n";
    print STDERR "Total $count character mappings in $tally CMap lines.\n";
    printf STDERR "%3.2f average mappings per CMap line.\n",($count / $tally);
  }
  return $cmap{NewMappings};
}

sub GetLength ($$) {
  my ($one,$two) = @_;
  my $index = 0;

  foreach $index (0 .. $#enc_s) {
    if (($one >= $enc_s[$index]) && ($two <= $enc_e[$index])) {
      return $enc_l[$index];
    }
  }
  printf STDERR "Codes 0x%X ($one) thru 0x%X ($two) not in range!\n",$one,$two;
  print STDERR "WARNING: Defaulting to eight-digit representation...\n";
  return 8;
}

sub GetLengthOnly ($) {
  my ($code) = @_;
  my $index = 0;

  foreach $index (0 .. $#enc_s) {
    if (($code >= $enc_s[$index]) && ($code <= $enc_e[$index])) {
      return $enc_l[$index];
    }
  }
  return 8;
}

sub ByThings {
  substr($a,2,4) cmp substr($b,2,4) or hex($a) <=> hex($b);
}

sub PrintCMap (%) {
  my (%cmap) = @_;
  my $uid;
  if (defined $cmap{UIDOffset}) {
    $uid = "\n$cmap{UIDOffset}/XUID [$cmap{XUID}] def\n";
  } elsif (defined $cmap{XUID}) {
    $uid = "\n/XUID [$cmap{XUID}] def\n";
  }

  print $outfile <<EOD;
\%!PS-Adobe-3.0 Resource-CMap
\%\%DocumentNeededResources: ProcSet (CIDInit)
\%\%IncludeResource: ProcSet (CIDInit)
\%\%BeginResource: CMap ($cmap{Name})
\%\%Title: ($cmap{Name} $cmap{Registry} $cmap{Ordering} $cmap{Supplement})
\%\%Version: $cmap{Version}
\%\%Copyright: -----------------------------------------------------------
\%\%Copyright: Copyright 1990-$year Adobe Systems Incorporated.
\%\%Copyright: All rights reserved.
\%\%Copyright:
\%\%Copyright: Redistribution and use in source and binary forms, with or
\%\%Copyright: without modification, are permitted provided that the
\%\%Copyright: following conditions are met:
\%\%Copyright:
\%\%Copyright: Redistributions of source code must retain the above
\%\%Copyright: copyright notice, this list of conditions and the following
\%\%Copyright: disclaimer.
\%\%Copyright:
\%\%Copyright: Redistributions in binary form must reproduce the above
\%\%Copyright: copyright notice, this list of conditions and the following
\%\%Copyright: disclaimer in the documentation and/or other materials
\%\%Copyright: provided with the distribution. 
\%\%Copyright:
\%\%Copyright: Neither the name of Adobe Systems Incorporated nor the names
\%\%Copyright: of its contributors may be used to endorse or promote
\%\%Copyright: products derived from this software without specific prior
\%\%Copyright: written permission. 
\%\%Copyright:
\%\%Copyright: THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
\%\%Copyright: CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
\%\%Copyright: INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
\%\%Copyright: MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
\%\%Copyright: DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
\%\%Copyright: CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
\%\%Copyright: SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
\%\%Copyright: NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
\%\%Copyright: LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
\%\%Copyright: HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
\%\%Copyright: CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
\%\%Copyright: OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
\%\%Copyright: SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\%\%Copyright: -----------------------------------------------------------
\%\%EndComments

/CIDInit /ProcSet findresource begin

12 dict begin

begincmap

/CIDSystemInfo 3 dict dup begin
  /Registry ($cmap{Registry}) def
  /Ordering ($cmap{Ordering}) def
  /Supplement $cmap{Supplement} def
end def

/CMapName /$cmap{Name} def
/CMapVersion $cmap{Version} def
/CMapType 1 def
$uid
/WMode $cmap{WMode} def

$cmap{CodeSpace}$cmap{NotDef}
$cmap{NewMappings}endcmap
CMapName currentdict /CMap defineresource pop
end
end

\%\%EndResource
\%\%EOF
EOD
}

sub PrintUseCMap (%) {
  my (%cmap) = @_;
  my $uid;
  if (defined $cmap{UIDOffset}) {
    $uid = "\n$cmap{UIDOffset}/XUID [$cmap{XUID}] def\n";
  } elsif (defined $cmap{XUID}) {
    $uid = "\n/XUID [$cmap{XUID}] def\n";
  }

  print $outfile <<EOD;
\%!PS-Adobe-3.0 Resource-CMap
\%\%DocumentNeededResources: ProcSet (CIDInit)
\%\%DocumentNeededResources: CMap ($cmap{UseCMap})
\%\%IncludeResource: ProcSet (CIDInit)
\%\%IncludeResource: CMap ($cmap{UseCMap})
\%\%BeginResource: CMap ($cmap{Name})
\%\%Title: ($cmap{Name} $cmap{Registry} $cmap{Ordering} $cmap{Supplement})
\%\%Version: $cmap{Version}
\%\%Copyright: -----------------------------------------------------------
\%\%Copyright: Copyright 1990-$year Adobe Systems Incorporated.
\%\%Copyright: All rights reserved.
\%\%Copyright:
\%\%Copyright: Redistribution and use in source and binary forms, with or
\%\%Copyright: without modification, are permitted provided that the
\%\%Copyright: following conditions are met:
\%\%Copyright:
\%\%Copyright: Redistributions of source code must retain the above
\%\%Copyright: copyright notice, this list of conditions and the following
\%\%Copyright: disclaimer.
\%\%Copyright:
\%\%Copyright: Redistributions in binary form must reproduce the above
\%\%Copyright: copyright notice, this list of conditions and the following
\%\%Copyright: disclaimer in the documentation and/or other materials
\%\%Copyright: provided with the distribution. 
\%\%Copyright:
\%\%Copyright: Neither the name of Adobe Systems Incorporated nor the names
\%\%Copyright: of its contributors may be used to endorse or promote
\%\%Copyright: products derived from this software without specific prior
\%\%Copyright: written permission. 
\%\%Copyright:
\%\%Copyright: THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
\%\%Copyright: CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
\%\%Copyright: INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
\%\%Copyright: MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
\%\%Copyright: DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
\%\%Copyright: CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
\%\%Copyright: SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
\%\%Copyright: NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
\%\%Copyright: LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
\%\%Copyright: HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
\%\%Copyright: CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
\%\%Copyright: OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
\%\%Copyright: SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\%\%Copyright: -----------------------------------------------------------
\%\%EndComments

/CIDInit /ProcSet findresource begin

12 dict begin

begincmap

/$cmap{UseCMap} usecmap

/CIDSystemInfo 3 dict dup begin
  /Registry ($cmap{Registry}) def
  /Ordering ($cmap{Ordering}) def
  /Supplement $cmap{Supplement} def
end def

/CMapName /$cmap{Name} def
/CMapVersion $cmap{Version} def
/CMapType 1 def
$uid
/WMode $cmap{WMode} def

$cmap{NewMappings}endcmap
CMapName currentdict /CMap defineresource pop
end
end

\%\%EndResource
\%\%EOF
EOD
}

sub PrintIdentityCMap (%) {
  my (%cmap) = @_;
  my $uid;
  if (defined $cmap{UIDOffset}) {
    $uid = "\n$cmap{UIDOffset}/XUID [$cmap{XUID}] def\n";
  } elsif (defined $cmap{XUID}) {
    $uid = "\n/XUID [$cmap{XUID}] def\n";
  }

  print $outfile <<EOD;
\%!PS-Adobe-3.0 Resource-CMap
\%\%DocumentNeededResources: ProcSet (CIDInit)
\%\%IncludeResource: ProcSet (CIDInit)
\%\%BeginResource: CMap (Identity)
\%\%Title: (Identity $cmap{Registry} $cmap{Ordering} $cmap{Supplement})
\%\%Version: $cmap{Version}
\%\%Copyright: -----------------------------------------------------------
\%\%Copyright: Copyright 1990-$year Adobe Systems Incorporated.
\%\%Copyright: All rights reserved.
\%\%Copyright:
\%\%Copyright: Redistribution and use in source and binary forms, with or
\%\%Copyright: without modification, are permitted provided that the
\%\%Copyright: following conditions are met:
\%\%Copyright:
\%\%Copyright: Redistributions of source code must retain the above
\%\%Copyright: copyright notice, this list of conditions and the following
\%\%Copyright: disclaimer.
\%\%Copyright:
\%\%Copyright: Redistributions in binary form must reproduce the above
\%\%Copyright: copyright notice, this list of conditions and the following
\%\%Copyright: disclaimer in the documentation and/or other materials
\%\%Copyright: provided with the distribution. 
\%\%Copyright:
\%\%Copyright: Neither the name of Adobe Systems Incorporated nor the names
\%\%Copyright: of its contributors may be used to endorse or promote
\%\%Copyright: products derived from this software without specific prior
\%\%Copyright: written permission. 
\%\%Copyright:
\%\%Copyright: THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
\%\%Copyright: CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
\%\%Copyright: INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
\%\%Copyright: MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
\%\%Copyright: DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
\%\%Copyright: CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
\%\%Copyright: SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
\%\%Copyright: NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
\%\%Copyright: LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
\%\%Copyright: HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
\%\%Copyright: CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
\%\%Copyright: OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
\%\%Copyright: SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\%\%Copyright: -----------------------------------------------------------
\%\%EndComments

/CIDInit /ProcSet findresource begin

12 dict begin

begincmap

/CIDSystemInfo 3 dict dup begin
  /Registry ($cmap{Registry}) def
  /Ordering ($cmap{Ordering}) def
  /Supplement $cmap{Supplement} def
end def

/CMapName /$cmap{Name} def
/CMapVersion $cmap{Version} def
/CMapType 1 def
$uid
/WMode $cmap{WMode} def

/CIDCount $expanded def

$cmap{CodeSpace}
$cmap{NewMappings}endcmap
CMapName currentdict /CMap defineresource pop
end
end

\%\%EndResource
\%\%EOF
EOD
}

sub Convert2UTF8 (%) {
  my (%single_mappings) = @_;
  my %new_mappings;

  foreach $entry (keys %single_mappings) {
    $new = $entry;
    $new =~ s/^0x//;
    $code = sprintf("0x%s\n",uc unpack("H*",&UTF32toUTF8(pack("H*",$new))));
    $new_mappings{$code} = $single_mappings{$entry};
  }
  return %new_mappings;
}

sub Convert2UTF16 (%) {
  my (%single_mappings) = @_;
  my %new_mappings;

  foreach $entry (keys %single_mappings) {
    $new = $entry;
    $new =~ s/^0x//;
    $code = sprintf("0x%s\n",uc unpack("H*",&UTF32toUTF16(pack("H*",$new))));
    $new_mappings{$code} = $single_mappings{$entry};
  }
  return %new_mappings;
}

sub UTF32toUTF8 ($) {
  my ($ch) = unpack("N",$_[0]);

  if ($ch <= 127) {
    chr($ch);
  } elsif ($ch <= 2047) {
    pack("C*", 192 | ($ch >> 6), 128 | ($ch & 63));
  } elsif ($ch <= 65535) {
    pack("C*", 224 | ($ch >> 12), 128 | (($ch >> 6) & 63), 128 | ($ch & 63));
  } elsif ($ch <= 1114111) {
    pack("C*", 240 | ($ch >> 18), 128 | (($ch >> 12) & 63), 128 | (($ch >> 6) & 63), 128 | ($ch & 63));
  } else {
    die "Whoah! Bad UTF-32 data! Perhaps outside of Unicode (UCS-4).\n";
  }
}

sub UTF32toUTF16 ($) {
  my ($ch) = unpack("N",$_[0]);

  if ($ch <= 65535) {
    pack("n", $ch);
  } elsif ($ch <= 1114111) {
    pack("n*", ((($ch - 65536) / 1024) + 55296),(($ch % 1024) + 56320));
  } else {
    die "Whoah! Bad UTF-32 data! Perhaps outside of Unicode (UCS-4).\n";
  }
}
