#!/usr/bin/perl

use strict;
use Data::Dumper;

my $traits = {
              launcher => {'-mdpi' => '48x48', '-hdpi' => '72x72', '-xhdpi' => '96x96'},
              status => {'-mdpi' => '24x24', '-hdpi' => '36x36', '-xhdpi' => '48x48'},
              status_v8 => {'-mdpi' => '25x25'},
              status_v9 => {'-mdpi' => '16x25', '-hdpi' => '24x38'},
              icon => {'-mdpi' => '32x32', '-hdpi' => '48x48', '-xhdpi' => '64x64'},
             };

sub convert
{
  my $icon = shift or die 'No object';
  $icon->{source} =~ /([^:]+)(:(.*))?/;
  my $src = $1;
  my $srcid = $3;
  my $tgt = $icon->{target};
  my $ctg = $icon->{category};
  my $qual = $icon->{qualifiers};
  for my $dpi ('', '-ldpi', '-mdpi', '-hdpi', '-xhdpi')
  {
    next unless ${traits}->{${ctg}}->{${dpi}} =~ /(\d+)x(\d+)/;
    my $fld = "../drawable${dpi}${qual}";
    my $w = $1;
    my $h = $2;
    mkdir ${fld} unless -d ${fld};
    my $sz = ${w} >= ${h} ? ${h} : ${w};
    my $cmd = "--without-gui --file=${src} --export-png=${fld}/${tgt} --export-width=${sz} --export-height=${sz} --export-area-page";
    $cmd .= " --export-id=${srcid} --export-id-only" if ${srcid};
    system("inkscape ${cmd}");
    if (${h} > ${w})
    {
      system("convert ${fld}/${tgt} -bordercolor none -border 0x{${h}-${w}} -gravity center -crop ${w}x${h}+0+0 ${fld}/${tgt}");
    }
  }
}

#convert({source => 'logo.svg', target => 'ic_launcher.png', category => 'launcher'});
convert({source => 'status.svg:v11', target => 'ic_stat_notify_play.png', category => 'status'});
convert({source => 'status.svg:v9', target => 'ic_stat_notify_play.png', category => 'status_v9', qualifiers => '-v9'});
#convert({source => 'status.svg:v8', target => 'ic_stat_notify_play.png', category => 'status_v8', qualifiers => '-v8'});
#convert({source => 'folder.svg', target => 'ic_browser_folder.png', category => 'icon'});
#convert({source => 'play.svg', target => 'ic_play.png', category => 'icon'});
#convert({source => 'next.svg', target => 'ic_next.png', category => 'icon'});
#convert({source => 'prev.svg', target => 'ic_prev.png', category => 'icon'});
#convert({source => 'pause.svg', target => 'ic_pause.png', category => 'icon'});
#convert({source => 'vfs_local.svg', target => 'ic_browser_vfs_local.png', category => 'icon'});
#convert({source => 'vfs_zxtunes.svg', target => 'ic_browser_vfs_zxtunes.png', category => 'icon'});
