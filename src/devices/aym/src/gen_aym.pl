#!/usr/bin/perl

use strict;
use HTML::Template;

# $0 template_name

sub RoutineName
{
  my $idx = shift;
  sprintf("GetLevels%u%u%u", ($idx & 448) >> 6, ($idx & 56) >> 3, $idx & 7);
}

sub NoiseVar
{
  my $idx = shift;
  return 0 != ($idx & 56) ? 'const uint_t noise = GenN.GetLevel();' : '';
}

sub EnvelopeVar
{
  my $idx = shift;
  return 0 != ($idx & 448) ? 'const uint_t envelope = GenE.GetLevel();' : '';
}

sub OutVar
{
  my $flag = shift;
  my $chan = shift;
  my $name = chr(ord('A') + $chan);
  my $res = (0 != ($flag & (64 << $chan))) ? 'envelope' : 'Level'.$name;
  $res .= ' & noise' if (0 != ($flag & (8 << $chan)));
  $res .= ' & Gen'.$name.'.GetLevel()' if (0 != ($flag & (1 << $chan)));
  return $res;
}

my $templateName = shift or die 'No template name';

my $template = HTML::Template->new(filename => $templateName, die_on_bad_params => 0, global_vars => 1) or die 'Failed to open template';
my @functions = ();

for (my $idx = 0; $idx != 512; ++$idx)
{
  my $name = RoutineName($idx);
  my $noise = NoiseVar($idx);
  my $envelope = EnvelopeVar($idx);
  my $outA = OutVar($idx, 0);
  my $outB = OutVar($idx, 1);
  my $outC = OutVar($idx, 2);
  my %function = (NAME => $name, NOISE => $noise, ENVELOPE => $envelope, OUTA => $outA, OUTB => $outB, OUTC => $outC);
  push(@functions, \%function);
}

$template->param(FUNCTIONS => \@functions);
print $template->output();
