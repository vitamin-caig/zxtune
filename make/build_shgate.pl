#!/usr/bin/perl

use strict;

# $0 GetPtrFunc <functions>

my $GetPtrFunc;
my %FuncsHash = ();

#main
$GetPtrFunc = shift or die 'No GetPtrFunc specified';
while (my $func = shift)
{
  $FuncsHash{$func} = [];
}

while (my $line = <>)
{
  $line =~ /^(.*?)(#.*)?$/;
  my $func = $1;
  next unless $func;
  die "Invalid function format ($_)" unless $func =~ /([\w_][\w\d_]*)\s*\((.*)\)/;
  my $funcName = $1;
  my @params = split /,\s*/,$2;
  my @paramNames = ();
  while (my $param = shift @params)
  {
    next if $param =~ /^\s*void\s*$/;
    die "Invalid parameters format ($param)" unless $param =~ /^[\w\d_ *]+[\s*]+([\w_][\w\d_]*)(\[\])?$/;
    push @paramNames, $1;
  }
  #print("Func($1) = ".join(',',@paramNames)."\n");
  next if %FuncsHash && !defined $FuncsHash{$funcName};
  printf "%s\n{\n  return %s(%s)(%s);\n}\n\n", $func, $GetPtrFunc, $funcName, join(', ', @paramNames);
}
