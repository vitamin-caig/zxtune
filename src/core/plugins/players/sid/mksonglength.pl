#!/usr/bin/perl

#use strict;
use String::CRC32;

my %Hashes = ();

while (my $line = <>)
{
  next unless $line =~ /([0-9a-f]{32})=(.*)/;
  my $hash = $1;
  my $crc = sprintf("%08x", crc32($hash));
  my @times = split(' ', $2);
  if (exists $Hashes{$crc})
  {
    die('Duplicated hash crc32');
  }
  $Hashes{$crc} = {'hash' => $hash, 'times' => \@times};
}

foreach my $crc (sort keys %Hashes)
{
  my $hash = $Hashes{$crc}{'hash'};
  my $timesstr = $Hashes{$crc}{'times'};
  my @times = map {$_ ? $_ : 1} map {$_ =~ /([0-9]+):([0-9]+)/; 60 * $1 + $2;} @$timesstr;
  print "//$hash\n";
  foreach my $time (@times)
  {
    printf("{0x%s, %u},\n", $crc, $time);
  }
}
