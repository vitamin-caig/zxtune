#!/usr/bin/perl

use strict;

my $package = shift or die 'No package name specified';
my $startrev = shift;
my $cur_entry_date = undef;

sub StartEntry
{
  my $rev = shift or die 'No revision';
  print("$package ($rev) experimental; urgency=low\n\n");
}

sub WriteIssue
{
  return unless $cur_entry_date;
  my $type = shift or die 'No issue type';
  my $val = shift or die 'No issue text';
  print("  $type $val\n");
}

sub FinishEntry
{
  if ($cur_entry_date)
  {
    print(" -- Vitamin <vitamin.caig\@gmail.com>  $cur_entry_date\n\n");
    undef $cur_entry_date;
  }
}

sub ConvDate
{
  my $time = shift or die 'No input';
  if ($time =~ /(\d{2})\.(\d{2})\.(\d{4})/)
  {
    $time = "$3-$2-$1";
  }
  return qx(date -d $time -R);
}

sub OnEntryStart
{
  my $date = shift or die 'No date';
  my $rev = shift or die 'No rev';
  if ($startrev)
  {
    if ($startrev ne $rev)
    {
      StartEntry($startrev);
      $cur_entry_date = ConvDate('now');
      WriteIssue('*', 'Fixes');
    }
    undef $startrev;
  }
  FinishEntry();
  $cur_entry_date = ConvDate($date);
  StartEntry($rev);
}


while (<>)
{
  if (/Rev(\d+M?)\D+([\d.]+)/)
  {
    OnEntryStart($2, $1);
  }
  if (/Rev: (\d+M?)\D+([-\d+]+)/)
  {
    OnEntryStart($2, $1)
  }
  elsif (/^\[([-*+])\]\s+(.*)$/)
  {
    WriteIssue($1, $2);
  }
}
FinishEntry();
