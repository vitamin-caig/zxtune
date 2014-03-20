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

print <<'__HEADER__';
/**
* 
* @file
*
* @brief  SID modules lengths database. Automatically generated from HSVC database
*
* @author
*
**/

//common includes
#include <types.h>
//boost includes
#include <boost/array.hpp>

namespace Module
{
namespace Sid
{
  typedef boost::array<uint8_t, 16> MD5Hash;
  
  struct SongEntry
  {
    const MD5Hash Hash;
    const unsigned Seconds;
    
    bool operator < (const MD5Hash& hash) const
    {
      return Hash < hash;
    }
  };
  
  const SongEntry SONGS[] =
  {
__HEADER__

foreach my $hashstr (sort keys %Hashes)
{
  my $hash = $hashstr;
  $hash =~ s/([0-9a-f]{2})/0x$1,/g;
  my $timesstr = $Hashes{$hashstr};
  my @times = map {$_ ? $_ : 1} map {$_ =~ /([0-9]+):([0-9]+)/; 60 * $1 + $2;} @$timesstr;
  foreach my $time (@times)
  {
    print "    {{{$hash}},$time},\n";
  }
}
print <<'__FOOTER__';
  };//SONGS
}//namespace Sid
}//namespace Module
__FOOTER__
