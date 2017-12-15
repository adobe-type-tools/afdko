#!/usr/bin/perl

# Written by Dr. Ken Lunde (lunde@adobe.com)
# Senior Computer Scientist 2, Adobe Systems Incorporated
# Version 04/11/2016
#
# Please invoke this script using the "-u" command-line option to see
# the command-line options, or "-h" to display more information.
#
# Tool Dependencies: None

$makefile = $number = $optimize = $hicount = $hiwidth = 0;
$newline = "\n";
$bottom = 9;
$top = 24;
$res = 72;

while (@ARGV and $ARGV[0] =~ /^-/) {
  my $arg = shift @ARGV;
  if (lc $arg eq "-u") {
      &ShowUsage;
      exit;
  } elsif (lc $arg eq "-h") {
      &ShowUsage;
      &ShowHelp;
      exit;
  } elsif (lc $arg eq "-o") {
      $optimize = 1;
  } elsif (lc $arg =~ /-b(\d+)/) {
      $bottom = $1;
  } elsif (lc $arg =~ /-t(\d+)/) {
      $top = $1;
  } elsif (lc $arg =~ /-r(\d+)/) {
      $res = $1;
  } elsif (lc $arg eq "-s") {
      $makefile = 1;
  } elsif (lc $arg eq "-n") {
      $newline = "";
  } else {
      die "Invalid option: $arg! Skipping\nExit\n";
  }
}

while(defined($line = <STDIN>)) {
    chomp $line;
    next if $line !~ /^\d+\s+\d+/;
    ($c,$w) = $line =~ /^(\d+)\s+(\d+)/;
    $w2c{$w} += $c;
}

if ($makefile) {
    print STDOUT "Count\tWidth\n";
    foreach $w (sort {$a <=> $b} keys %w2c) {
        print STDOUT "$w2c{$w}\t$w\n";
    }
} elsif (not $optimize) {
    foreach $w (keys %w2c) {
        $hicount = $w2c{$w} and $hiwidth = $w if $w2c{$w} > $hicount;
    }
    print STDOUT "$hiwidth (x $w2c{$hiwidth})$newline";
} else {
    $index = &getindex($bottom,$top);
    $buffer = "[$index]";
    foreach $size ($index - 10 .. $index + 10) {
        delete $w2c{$size};
    }
    $index = &getindex($bottom,$top);
    $buffer .= " " . $index;
    foreach $size ($index - 10 .. $index + 10) {
        delete $w2c{$size};
    }
    $index = &getindex($bottom,$top);
    $buffer .= " " . $index;
    print STDOUT "$buffer$newline";
}

sub scandata ($$) {
    my ($index,$range) = @_;
    $result = 0;
    foreach $c ($index - $range .. $index + $range) {
        if ($c > 0 and $c < 1000) {
            $result += $w2c{$c};
        }
    }
    return $result;
}

sub getrange ($) {
    my ($range) = @_;
    $number = 0;
    foreach $i (sort {$a <=> $b} keys %w2c) {
        $temp = &scandata($i,$range);
        if ($temp > $number) {
            $index = $i;
            $number = $temp;
        }
    }
    return $index;
}

sub getindex ($$) {
    my ($bottom,$top) = @_;
    foreach $size ($bottom * 2 .. $top * 2) {
        $count{&getrange(&rnd(0.35 * (72000 / (($size / 2) * $res))))} += 1;
    }
    foreach $i (sort {$a <=> $b} keys %count) {
        $index = $i and $number = $count{$i} if $count{$i} > $number;
    }
    undef %count;
    $number = 0;
    return $index;
}

sub rnd ($) {
    my ($arg) = @_;
    if ($arg =~ /\.[0-4]\d*/) {
	$arg =~ s/\.[0-4]\d*$//;
    } elsif ($arg =~ /\.[5-9]\d*/) {
	$arg =~ s/(\d+)\.[5-9]\d*$/$1 + 1/e;
    }
    return $arg;
}

sub fix {
    my ($string) = @_;
    $string =~ s/^  //gm;
    return $string;
}

sub ShowUsage {
  print STDOUT &fix(<<ENDUSAGE);
  setsnap.pl Version 04/11/2016

  setsnap.pl [-u] [-h]
  setsnap.pl < stemHist-generated-stemwidth-reports
  setsnap.pl [-o [-b<bot_pt_size>] [-t<top_pt_size>] [-r<dpi>]] < stemHist-generated-stemwidth-reports

  Calculates highest-frequency or optimal stem width values in one or more
  stemHist-generated stemwidth reports.

ENDUSAGE
}

sub ShowHelp {
  print STDOUT &fix(<<ENDHELP);
  OPTIONS:
   -u = Displays only the version and command-line options
   -h = Displays this help page
   -o = Calculate optimal values based on point size range and resolution
   -b = Bottom (lower) end of point size range; used only with '-o' option
   -t = Top (upper) end of point size range; used only with '-o' option
   -r = Resolution expressed in dpi; used only with '-o' option

  When no options are used, the highest-frequency stem width value is
  reported, along with the frequency in parentheses.

  When the '-o' option is used, the '-b', '-t', and '-r' options can also be
  used, and their default values are 9 (points), 24 (points), and 72 (dpi),
  respectively. The optimal value is output in brackets, followed by secondary
  and tertiary values.

ENDHELP
}
