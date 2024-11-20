/* C example that opens a game music file and records 10 seconds to "out.wav" */

#include "gme/gme.h"

#include "Wave_Writer.h" /* wave_ functions for writing sound file */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void handle_error( const char *str )
{
	if ( str )
	{
		printf( "Error: %s\n", str ); getchar();
		exit( EXIT_FAILURE );
	}
}

short avg( short *ptr, int n )
{
	int sum = 0, i = n;
	while ( i-- )
		sum += ptr[i];
	return sum / n;
}

int main( int argc, char *argv[] )
{
	const char *filename = "test.nsf"; /* Default file to open */
	if ( argc >= 2 )
		filename = argv[1];

	int sample_rate = 44100; /* number of samples per second */
	/* index of track to play (0 = first) */
	int track = argc >= 3 ? atoi(argv[2]) : 0;

	/* Open music file in new multi-channel emulator */
	gme_type_t file_type = gme_identify_extension( filename );
	Music_Emu* emu = gme_new_emu_multi_channel( file_type, sample_rate );
	handle_error( gme_load_file( emu, filename ) );

	/* Start track */
	handle_error( gme_start_track( emu, track ) );

	/* Begin writing to wave file */
	wave_open( sample_rate, "out.wav" );
	wave_enable_stereo();

	const int frames = 64;
	const int voices = 8;
	const int channels = 2;
	const int buf_size = frames * voices * channels;

	/* Record 10 seconds of track */
	while ( gme_tell( emu ) < 10 * 1000L )
	{
		/* Sample buffer */
		short buf [buf_size], *in = buf, *out = buf;

		/* Fill sample buffer */
		handle_error( gme_play( emu, buf_size, buf ) );

		/* Render frames for stereo output by sending
		   even-numbered voices to the left channel, and
		   odd-numbered voices to the right channel */
		for ( int f = frames; f--; out += channels )
		{
			int i = 0;
			short ch [channels];
			memset( ch, 0, channels * sizeof( short ) );
			for ( int v = voices; v--; in += channels )
			{
				ch[i] += avg( in, channels );
				i = ( ++i >= channels ) ? 0 : i;
			}
			for ( int c = channels; c--; )
				out[c] = ch[c];
		}

		/* Write samples to wave file */
		wave_write( buf, frames * channels );
	}

	/* Cleanup */
	gme_delete( emu );
	wave_close();

	return 0;
}
