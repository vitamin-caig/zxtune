/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.0 - 2012/03/09

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "CorePlayer.h"
#include "../flod_internal.h"
#include "Amiga.h"
#include "Soundblaster.h"

void CorePlayer_defaults(struct CorePlayer* self) {
	CLASS_DEF_INIT();
	// static initializers go here
	self->title = "";
	self->soundPos = 0.0;
}

void CorePlayer_ctor(struct CorePlayer* self, struct CoreMixer *hardware) {
	CLASS_CTOR_DEF(CorePlayer);
	// original constructor code goes here
	PFUNC();
	hardware->player = self;
	self->hardware = hardware;
	
	//add vtable
	self->process = CorePlayer_process;
	self->fast = CorePlayer_fast;
	self->accurate = CorePlayer_accurate;
	self->setup = CorePlayer_setup;
	self->set_ntsc = CorePlayer_set_ntsc;
	self->set_stereo = CorePlayer_set_stereo;
	self->set_volume = CorePlayer_set_volume;
	self->toggle = CorePlayer_toggle;
	self->reset = CorePlayer_reset;
	self->loader = CorePlayer_loader;
	self->initialize = CorePlayer_initialize;
	self->set_force = CorePlayer_set_force;
}

struct CorePlayer* CorePlayer_new(struct CoreMixer *hardware) {
	CLASS_NEW_BODY(CorePlayer, hardware);
}

void CorePlayer_set_force(struct CorePlayer* self, int value) {
	self->version = 0;
	(void) value;
}

/* stubs */
void CorePlayer_process(struct CorePlayer* self) { (void) self; }

void CorePlayer_setup(struct CorePlayer* self) { (void) self; }

void CorePlayer_set_ntsc(struct CorePlayer* self, int value) { (void) self; (void) value; }

void CorePlayer_set_stereo(struct CorePlayer* self, Number value) { (void) self; (void) value; }

void CorePlayer_set_volume(struct CorePlayer* self, Number value) { (void) self; (void) value; }

void CorePlayer_toggle(struct CorePlayer* self, int index) { (void) self; (void) index; }

void CorePlayer_reset(struct CorePlayer* self) { (void) self; }

void CorePlayer_loader(struct CorePlayer* self, struct ByteArray *stream) { (void) self; (void) stream; }

/* callback function for EVENT_SAMPLE_DATA */
void CorePlayer_fast(struct CorePlayer* self) { (void) self; }
void CorePlayer_accurate(struct CorePlayer* self) { (void) self; }

int CorePlayer_load(struct CorePlayer* self, struct ByteArray *stream) {
	PFUNC();
	CoreMixer_reset(self->hardware);
	ByteArray_set_position(stream, 0);

	self->version  = 0;
	self->playSong = 0;
	self->lastSong = 0;
#ifdef SUPPORT_COMPRESSION
	struct ZipFile* zip;	
	if (stream->readUnsignedInt(stream) == 67324752) {
		zip = ZipFile_new(stream);
		stream = zip->uncompress(zip->entries[0]);
	}
#endif

	if (stream) {
		stream->endian = self->endian;
		ByteArray_set_position(stream, 0);
		self->loader(self, stream);
		if (self->version) self->setup(self);
	}
	return self->version;
}

    //js function reset
void CorePlayer_initialize(struct CorePlayer* self) {
	self->tick = 0;
	CoreMixer_initialize(self->hardware);
	PFUNC();
// FIXME we shouldnt pull in sb/amiga code if we don't need it
// using a compile-time flag like below is only a workaround.
#ifndef FLOD_NO_AMIGA
	if(self->hardware->type == CM_AMIGA)
		Amiga_initialize((struct Amiga*) self->hardware);
	else
#endif
#ifndef FLOD_NO_SOUNDBLASTER
	if(self->hardware->type == CM_SOUNDBLASTER)
		Soundblaster_initialize((struct Soundblaster*) self->hardware);
	else
#endif
		abort();

	self->hardware->samplesTick = 110250 / self->tempo;
}

#if 0
/* processor default : NULL */
int CorePlayer_play(struct CorePlayer* self) {
	PFUNC();
	if (!self->version) return 0;
	if (self->soundPos == 0.0) {
		//self->initialize();
		self->initialize(self);
	}
	self->soundPos = 0.0;
	return 1;
}

void CorePlayer_stop(struct CorePlayer* self) {
	PFUNC();
	if (!self->version) return;
	self->soundPos = 0.0;
	self->reset(self);
}
#endif
