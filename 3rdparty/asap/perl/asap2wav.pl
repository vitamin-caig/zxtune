# asap2wav.pl - converter of ASAP-supported formats to WAV files
#
# Copyright (C) 2013  Piotr Fusik
#
# This file is part of ASAP (Another Slight Atari Player),
# see http://asap.sourceforge.net
#
# ASAP is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published
# by the Free Software Foundation; either version 2 of the License,
# or (at your option) any later version.
#
# ASAP is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with ASAP; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

use Getopt::Long;
use Asap;
use strict;

my $output_filename;
my $output_header = 1;
my $song;
my $format = ASAPSampleFormat::S16_L_E();
my $duration;
my $mute_mask = 0;

sub print_help() {
	print <<END;
Usage: perl asap2wav.pl [OPTIONS] INPUTFILE...
Each INPUTFILE must be in a supported format:
SAP, CMC, CM3, CMR, CMS, DMC, DLT, MPT, MPD, RMT, TMC, TM8, TM2 or FC.
Options:
-o FILE     --output=FILE      Set output file name
-s SONG     --song=SONG        Select subsong number (zero-based)
-t TIME     --time=TIME        Set output length (MM:SS format)
-b          --byte-samples     Output 8-bit samples
-w          --word-samples     Output 16-bit samples (default)
            --raw              Output raw audio (no WAV header)
-m CHANNELS --mute=CHANNELS    Mute POKEY chanels (1-8)
-h          --help             Display this information
-v          --version          Display version information
END
}

sub set_mute_mask($) {
	my ($s) = @_;
	$mute_mask = 0;
	$mute_mask |= 1 << ($1 - 1) while $s =~ /([1-8])/g;
}

sub process_file($) {
	my ($input_filename) = @_;
	my $module;
	open F, '<', $input_filename and binmode F and read F, $module, ASAPInfo::MAX_MODULE_LENGTH() and close F or die "$input_filename: $!\n";
	my @module = unpack 'C*', $module;

	my $asap = ASAP->new();
	$asap->load($input_filename, \@module, scalar(@module));
	my $info = $asap->get_info();
	$song = $info->get_default_song() unless defined $song;
	unless (defined $duration) {
		$duration = $info->get_duration($song);
		$duration = 180_000 if $duration < 0;
	}
	$asap->play_song($song, $duration);
	$asap->mute_pokey_channels($mute_mask);

	unless (defined $output_filename) {
		($output_filename = $input_filename) =~ s/\.\w+$/$output_header ? '.wav' : '.raw'/e;
	}
	open F, '>', $output_filename and binmode F or die "$output_filename: $!\n";
	my @buffer = [];
	my $n;
	if ($output_header) {
		$n = $asap->get_wav_header(\@buffer, $format, 0);
		print F pack 'C*', @buffer[0 .. $n - 1];
	}
	do {
		$n = $asap->generate(\@buffer, 8192, $format);
		print F pack 'C*', @buffer[0 .. $n - 1];
	} while $n == 8192;
	close F;

	$output_filename = undef;
	$song = undef;
	$duration = undef;
}

GetOptions(
	'output|o' => \$output_filename,
	'song|s' => \$song,
	'time|t' => sub { $duration = ASAPInfo::ParseDuration($_[1]); },
	'byte-samples|b' => sub { $format = ASAPSampleFormat::U8(); },
	'word-samples|w' => sub { $format = ASAPSampleFormat::S16_L_E(); },
	'raw' => sub { $output_header = 0; },
	'mute|m' => sub { set_mute_mask($_[1]); },
	'help|h' => sub { print_help(); exit 0; },
	'version|v' => sub { print 'ASAP2WAV (Perl) ', ASAPInfo::VERSION(), "\n"; exit 0; }
) && @ARGV or print_help(), exit 1;

process_file($_) for @ARGV;
