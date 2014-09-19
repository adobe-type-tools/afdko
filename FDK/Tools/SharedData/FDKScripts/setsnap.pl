#!/usr/bin/perl

$makefile = $number = $optimize = $hicount = $hiwidth = 0;
$newline = "\n";
$lower = 9;
$upper = 24;
$res = 72;

while (@ARGV and $ARGV[0] =~ /^-/) {
  my $arg = shift @ARGV;
  if (lc $arg eq "-h") {
      &ShowHelp;
      exit;
  } elsif (lc $arg eq "-o") {
      $optimize = 1;
  } elsif (lc $arg =~ /-l(\d+)/) {
      $lower = $1;
  } elsif (lc $arg =~ /-u(\d+)/) {
      $upper = $1;
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
    $index = &getindex($lower,$upper);
    $buffer = "[$index]";
    foreach $size ($index - 10 .. $index + 10) {
        delete $w2c{$size};
    }
    $index = &getindex($lower,$upper);
    $buffer .= " " . $index;
    foreach $size ($index - 10 .. $index + 10) {
        delete $w2c{$size};
    }
    $index = &getindex($lower,$upper);
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
    my ($lower,$upper) = @_;
    foreach $size ($lower * 2 .. $upper * 2) {
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

sub ShowHelp {
  print STDERR &fix(<<ENDHELP);
  setsnap.pl Version 1.0 (10/23/2006)
  Written by Ken Lunde (lunde\@adobe.com)
  Program Copyright 2006 Adobe Systems Incorporated. All Rights Reserved.

  Calculates highest-frequency or optimal stem width values in one or more
  stemHist-generated stemwidth reports.

  setsnap.pl -h
  setsnap.pl < stemHist-generated-stemwidth-reports
  setsnap.pl [-o [-l<lo_pt_size>] [-u<up_pt_size>] [-r<dpi>]] < stemHist-generated-stemwidth-reports

  OPTIONS:
   -h = Displays this help page
   -o = Calculate optimal values based on point size range and resolution
   -l = Lower end of point size range; used only with '-o' option
   -u = Upper end of point size range; used only with '-o' option
   -r = Resolution expressed in dpi; used only with '-o' option

  When no options are used, the highest-frequency stem width value is
  reported, along with the frequency in parentheses.

  When the '-o' option is used, the '-l', '-u', and '-r' options can also be
  used, and their default values are 9 (points), 24 (points), and 72 (dpi),
  respectively. The optimal value is output in brackets, followed by secondary
  and tertiary values.
ENDHELP
}
