#!/usr/bin/perl

use strict;
use Data::Dumper;

my $traits = {
              launcher => {'-mdpi' => '48x48', '-hdpi' => '72x72', '-xhdpi' => '96x96', '-xxhdpi' => '144x144'},
              status => {'-mdpi' => '24x24', '-hdpi' => '36x36', '-xhdpi' => '48x48', '-xxhdpi' => '72x72'},
              icon => {'-mdpi' => '32x32', '-hdpi' => '48x48', '-xhdpi' => '64x64', '-xxhdpi' => '96x96'},
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
  for my $dpi ('', '-mdpi', '-hdpi', '-xhdpi', '-xxhdpi')
  {
    next unless ${traits}->{${ctg}}->{${dpi}} =~ /(\d+)x(\d+)/;
    my $fld = "../drawable${dpi}${qual}";
    my $w = $1;
    my $h = $2;
    mkdir ${fld} unless -d ${fld};
    my $sz = ${w} >= ${h} ? ${h} : ${w};
    my $cmd = "--without-gui --file=${src} --export-png=${fld}/${tgt} --export-width=${sz} --export-height=${sz} --export-area-page --export-background-opacity=0";
    $cmd .= " --export-id=${srcid} --export-id-only" if ${srcid};
    system("inkscape ${cmd}");
    if (${h} > ${w})
    {
      system("convert ${fld}/${tgt} -bordercolor none -border 0x{${h}-${w}} -gravity center -crop ${w}x${h}+0+0 ${fld}/${tgt}");
    }
  }
}

#convert({source => 'logo.svg', target => 'ic_launcher.png', category => 'launcher'});

#convert({source => 'restorer/play.svg', target => 'ic_play.png', category => 'icon'});
#convert({source => 'restorer/next.svg', target => 'ic_next.png', category => 'icon'});
#convert({source => 'restorer/prev.svg', target => 'ic_prev.png', category => 'icon'});
#convert({source => 'restorer/pause.svg', target => 'ic_pause.png', category => 'icon'});
#convert({source => 'restorer/folder.svg', target => 'ic_browser_folder.png', category => 'icon'});

#convert({source => 'status.svg', target => 'ic_stat_notify_play.png', category => 'status'});
#convert({source => 'scanning.svg', target => 'ic_stat_notify_scan.png', category => 'status'});
#convert({source => 'vfs_local.svg', target => 'ic_browser_vfs_local.png', category => 'icon'});
#convert({source => 'vfs_zxtunes.svg', target => 'ic_browser_vfs_zxtunes.png', category => 'icon'});
convert({source => 'drag_handler.svg', target => 'ic_drag_handler.png', category => 'icon'});
convert({source => 'status.svg', target => 'ic_playing.png', category => 'icon'});

#convert({source => 'track_regular.svg', target => 'ic_track_regular.png', category => 'icon'});
#convert({source => 'track_looped.svg', target => 'ic_track_looped.png', category => 'icon'});
#convert({source => 'sequence_ordered.svg', target => 'ic_sequence_ordered.png', category => 'icon'});
#convert({source => 'sequence_looped.svg', target => 'ic_sequence_looped.png', category => 'icon'});
#convert({source => 'sequence_shuffle.svg', target => 'ic_sequence_shuffle.png', category => 'icon'});
