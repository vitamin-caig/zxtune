#!/usr/bin/perl

sub do_move
{
  my $in_file = shift or die;
  my $out_file = shift or die;
  system("mv -f $in_file $out_file\n") if $in_file ne $out_file;
}

sub do_remove
{
  my $file = shift or die;
  system("rm -f $file");
}

foreach my $i (glob '*.gcov')
{
  my $t = $i;
  $t =~ s/#/\//g;
  if ($t =~ /(.*?)\/\/(.*)/)
  {
    my($base, $file) = ($1, $2);
    if ($file !~ /^\/usr/)
    {
      do_move($i, $file);
    }
    else
    {
      do_remove($i);
    }
  }
  else
  {
    do_move($i, $t);
  }
}
