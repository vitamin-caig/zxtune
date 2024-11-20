// Game_Music_Emu https://bitbucket.org/mpyne/game-music-emu/

#include "Music_Player.h"

#include <string.h>
#include <ctype.h>
#include "SDL_rwops.h"
#include "Archive_Reader.h"

/* Copyright (C) 2005-2010 by Shay Green. Permission is hereby granted, free of
charge, to any person obtaining a copy of this software module and associated
documentation files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and
to permit persons to whom the Software is furnished to do so, subject to the
following conditions: The above copyright notice and this permission notice
shall be included in all copies or substantial portions of the Software. THE
SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#undef RETURN_ERR
#define RETURN_ERR( expr ) \
	do {\
		gme_err_t err_ = (expr);\
		if ( err_ )\
			return err_;\
	} while ( 0 )

// Number of audio buffers per second. Adjust if you encounter audio skipping.
// Note that this sets the floor on how often you'll see changes to the audio
// scope
static const int fill_rate = 80;

// Simple sound driver using SDL
typedef void (*sound_callback_t)( void* data, short* out, int count );
static const char* sound_init( long sample_rate, int buf_size, sound_callback_t, void* data );
static void sound_start();
static void sound_stop();
static void sound_cleanup();

struct arc_type_t {
	long header;
	Archive_Reader* (*new_arc)();
};

#ifdef RARDLL
static Archive_Reader* new_rar_reader() { return BLARGG_NEW Rar_Reader; }
#endif

static const arc_type_t arcs[] = {
#ifdef RARDLL
	{ BLARGG_4CHAR('R','a','r','!'), &new_rar_reader },
#endif
	{ 0, nullptr }
};

Music_Player::Music_Player()
{
	emu_        = 0;
	scope_buf   = 0;
	paused      = false;
	track_info_ = NULL;
}

gme_err_t Music_Player::init( long rate )
{
	sample_rate = rate;

	int min_size = sample_rate * 2 / fill_rate;
	int buf_size = 512;
	while ( buf_size < min_size )
		buf_size *= 2;

	return sound_init( sample_rate, buf_size, fill_buffer, this );
}

void Music_Player::stop()
{
	sound_stop();
	gme_delete( emu_ );
	emu_ = NULL;
}

Music_Player::~Music_Player()
{
	stop();
	sound_cleanup();
	gme_free_info( track_info_ );
}

// check if file is an archive
const arc_type_t* identify_archive( const char* path )
{
	long header;
	char h[4];
	FILE *in = fopen( path, "rb" );
	if ( !in )
		return nullptr;
	fread( h, 1, sizeof h, in );
	fclose( in );
	header = BLARGG_4CHAR( h[0], h[1], h[2], h[3] );
	for ( const arc_type_t* arc = arcs; arc->header; arc++ )
		if ( arc->header == header )
			return arc;
	return nullptr;
}

gme_err_t Music_Player::load_file(const char* path , bool by_mem)
{
	stop();

	if ( by_mem )
	{
		printf( "Loading file %s by memory...\n", path );
		fflush( stdout );

		SDL_RWops *file = SDL_RWFromFile(path, "rb");

		if ( !file )
			return "Can't load the file";

		size_t fileSize = SDL_RWsize(file);
		Uint8 *buf = (Uint8 *)SDL_malloc(fileSize);

		if ( !buf )
			return "Out of memory";

		if ( SDL_RWread(file, buf, 1, fileSize) < fileSize)
		{
			SDL_free(buf);
			SDL_RWclose(file);
			return "Can't read a file";
		}

		SDL_RWclose(file);

		const char *ret = gme_open_data( buf, (long)fileSize, &emu_, sample_rate );
		SDL_free(buf);
		RETURN_ERR( ret );
	}
	else
	{
		printf( "Loading file %s by file path...\n", path );
		fflush( stdout );

		const arc_type_t* arc = identify_archive( path );
		if ( arc )
		{
			Archive_Reader* ptr = arc->new_arc();
			if ( !ptr )
				return "Failed to create archive reader";
			Archive_Reader& in = *ptr;
			blargg_vector<long> sizes;
			blargg_vector<uint8_t> buf;
			RETURN_ERR( in.open( path ) );
			RETURN_ERR( sizes.resize( in.count() ) );
			RETURN_ERR( buf.resize( in.size() ) );

			int n = 0;
			uint8_t *bp = buf.begin();
			gme_type_t emu_type = nullptr;
			while( in.next_entry() )
			{ // copy data and file sizes
				gme_type_t t;
				RETURN_ERR( in.read( bp ) );
				if ( (t = gme_identify_extension( in.entry_name() ))
				&& gme_fixed_track_count( t ) == 1 )
				{
					if ( !emu_type )
						emu_type = t;
					if ( t == emu_type )
						bp += (sizes[n++] = in.entry_size());
				}
			}
			delete ptr;

			if ( !emu_type )
				return gme_wrong_file_type;
			emu_ = gme_new_emu( emu_type, sample_rate );
			if ( !emu_ )
				return "Out of memory";
			RETURN_ERR( gme_load_tracks( emu_, buf.begin(), sizes.begin(), n ) );
		}
		else
			RETURN_ERR( gme_open_file( path, &emu_, sample_rate ) );
	}

	char m3u_path [256 + 5];
	strncpy( m3u_path, path, 256 );
	m3u_path [256] = 0;
	char* p = strrchr( m3u_path, '.' );
	if ( !p )
		p = m3u_path + strlen( m3u_path );
	strcpy( p, ".m3u" );
	if ( gme_load_m3u( emu_, m3u_path ) ) { } // ignore error

	return 0;
}

int Music_Player::track_count() const
{
	return emu_ ? gme_track_count( emu_ ) : false;
}

gme_err_t Music_Player::start_track( int track )
{
	if ( emu_ )
	{
		// Sound must not be running when operating on emulator
		sound_stop();
		RETURN_ERR( gme_start_track( emu_, track ) );

		gme_free_info( track_info_ );
		track_info_ = nullptr;
		RETURN_ERR( gme_track_info( emu_, &track_info_, track ) );

		// Calculate track length
		if ( track_info_->length <= 0 )
			track_info_->length = track_info_->intro_length +
						track_info_->loop_length * 2;

		if ( track_info_->length <= 0 )
			track_info_->length = (long) (2.5 * 60 * 1000);
		gme_set_fade_msecs( emu_, track_info_->length, 8000 );

		paused = false;
		sound_start();
	}
	return 0;
}

void Music_Player::pause( int b )
{
	paused = b;
	if ( b )
		sound_stop();
	else
		sound_start();
}

void Music_Player::suspend()
{
	if ( !paused )
		sound_stop();
}

void Music_Player::resume()
{
	if ( !paused )
		sound_start();
}

bool Music_Player::track_ended() const
{
	return emu_ ? gme_track_ended( emu_ ) : false;
}

void Music_Player::set_stereo_depth( double tempo )
{
	suspend();
	gme_set_stereo_depth( emu_, tempo );
	resume();
}

void Music_Player::enable_accuracy( bool b )
{
	suspend();
	gme_enable_accuracy( emu_, b );
	resume();
}

void Music_Player::set_tempo( double tempo )
{
	suspend();
	gme_set_tempo( emu_, tempo );
	resume();
}

void Music_Player::set_echo_disable( bool d )
{
	suspend();
	gme_disable_echo( emu_, d );
	resume();
}

void Music_Player::mute_voices( int mask )
{
	suspend();
	gme_mute_voices( emu_, mask );
	gme_ignore_silence( emu_, mask != 0 );
	resume();
}

void Music_Player::seek_forward()
{
	suspend();
	int pos = gme_tell( emu_ );
	if ( pos > 0 )
		gme_seek( emu_, pos + 1000 );
	resume();
}

void Music_Player::seek_backward()
{
	suspend();
	int pos = gme_tell( emu_ );
	if ( pos > 0 )
		gme_seek( emu_, pos - 1000 );
	resume();
}

void Music_Player::set_fadeout( bool fade )
{
	gme_set_fade_msecs( emu_, fade ? track_info_->length : -1, 8000 );
}

void Music_Player::fill_buffer( void* data, sample_t* out, int count )
{
	Music_Player* self = (Music_Player*) data;
	if ( self->emu_ )
	{
		if ( gme_play( self->emu_, count, out ) ) { } // ignore error

		if ( self->scope_buf )
			memcpy( self->scope_buf, out, self->scope_buf_size * sizeof *self->scope_buf );
	}
}

// Sound output driver using SDL

#include "SDL.h"

static sound_callback_t sound_callback;
static void* sound_callback_data;

static void sdl_callback( void* /* data */, Uint8* out, int count )
{
	if ( sound_callback )
		sound_callback( sound_callback_data, (short*) out, count / 2 );
}

static const char* sound_init( long sample_rate, int buf_size,
		sound_callback_t cb, void* data )
{
	sound_callback = cb;
	sound_callback_data = data;

	static SDL_AudioSpec as; // making static clears all fields to 0
	as.freq     = sample_rate;
	as.format   = AUDIO_S16SYS;
	as.channels = 2;
	as.callback = sdl_callback;
	as.samples  = buf_size;
	if ( SDL_OpenAudio( &as, 0 ) < 0 )
	{
		const char* err = SDL_GetError();
		if ( !err )
			err = "Couldn't open SDL audio";
		return err;
	}

	return 0;
}

static void sound_start()
{
	SDL_PauseAudio( false );
}

static void sound_stop()
{
	SDL_PauseAudio( true );

	// be sure audio thread is not active
	SDL_LockAudio();
	SDL_UnlockAudio();
}

static void sound_cleanup()
{
	sound_stop();
	SDL_CloseAudio();
}
