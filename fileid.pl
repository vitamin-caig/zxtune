#!/usr/bin/perl

use strict;

sub BaseName {return 'tagbase.db';}

my %TagHash = ();

sub scan
{
  my $rootdir = shift or die 'No rootdir specified';
  for my $file (glob($rootdir.'/*'))
  {
    if (-d $file)
    {
      scan($file);
    }
    elsif ($file =~ /\.(cpp|h|cxx|hpp|h|c)$/i)
    {
      open FILE, '<'.$file or die "Failed to open $file";
      my @cont = <FILE>;
      chomp @cont;
      my $val = join ' ',@cont;
      if ($val =~ /define\s+FILE_TAG\s+([a-fA-F0-9]+)/)
      {
        die "Duplicated tags in files ${TagHash{hex $1}} and $file" if exists $TagHash{hex $1};
        $TagHash{hex $1} = $file;
        print "Matched in $file\n";
        my $size = length $1;
        print "Warning! Tag size invalid ($size)\n" if $size != 8;
      }
    }
  }
}

if (0 == scalar @ARGV) #no params
{
  srand(time());
  printf "#define FILE_TAG %08X\n", rand(1 << 16) + (rand(1 << 16) << 16);
  exit 0;
}

if (!-e BaseName() || $ARGV[0] eq 'scan')
{
  scan('.');
  open FILE, '>'.BaseName() or die 'Failed to open '.BaseName();
  while (my ($id, $name) = each %TagHash)
  {
    print FILE "$id $name\n";
  }
  exit 0;
}
else
{
  open FILE, '<'.BaseName() or die 'Failed to open '.BaseName();
  while (<FILE>)
  {
    /(\d+)\s+(.*)/;
    $TagHash{$1} = $2;
  }
}

my $tag = undef;

if ($ARGV[0] =~ /([a-fA-F0-9]{8})$/)
{
  $tag = hex($1);
}
if ($ARGV[0] =~ /^0x([a-fA-F0-9]+)$/)
{
  $tag = hex($1);
}
elsif ($ARGV[0] =~ /^[0-9]+$/)
{
  $tag = int($ARGV[0]);
}
if (defined $tag)
{
  my $basetag = $tag & ~0xffff;
  while (my ($id, $name) = each %TagHash)
  {
    my $diff = $tag - $id;
    if ($basetag == (int($id) & ~0xffff) && $diff >= 0)
    {
      print "$name:$diff\n";
      exit 0;
    }
  }
  print "Tag not found\n";
  exit 1;
}
print "Usage:\n$0 scan -- to force base creating\n$0 <tag> -- to find tag location\n";
0;