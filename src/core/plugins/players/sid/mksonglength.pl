#!/usr/bin/perl

#use strict;

my %Hashes = ();

while (my $line = <>)
{
  next unless $line =~ /([0-9a-f]{32})=(.*)/;
  my $hash = $1;
  my @times = split(' ',$2);
  $Hashes{${hash}} = \@times;
}

foreach my $hashstr (sort keys %Hashes)
{
  my $hash = $hashstr;
  $hash =~ s/([0-9a-f]{2})/0x$1,/g;
  my $timesstr = $Hashes{$hashstr};
  my @times = map {$_ ? $_ : 1} map {$_ =~ /([0-9]+):([0-9]+)/; 60 * $1 + $2;} @$timesstr;
  foreach my $time (@times)
  {
    print "{{{$hash}},$time},\n";
  }
}
