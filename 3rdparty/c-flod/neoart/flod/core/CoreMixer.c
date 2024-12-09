/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 3.0 - 2012/02/08

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/
#include "../flod.h"
#include "../flod_internal.h"
#include "CoreMixer.h"
#include "Amiga.h"

void CoreMixer_defaults(struct CoreMixer* self) {
	CLASS_DEF_INIT();
	/* static initializers go here */
}

void CoreMixer_ctor(struct CoreMixer* self) {
	CLASS_CTOR_DEF(CoreMixer);
	/* original constructor code goes here */
	PFUNC();
	CoreMixer_set_bufferSize(self, COREMIXER_MAX_BUFFER);
	//self->bufferSize = 8192;
	
	//vtable
	self->fast = CoreMixer_fast;
	self->accurate = CoreMixer_accurate;
}

struct CoreMixer* CoreMixer_new(void) {
	CLASS_NEW_BODY(CoreMixer);
}

/* stubs */
void CoreMixer_reset(struct CoreMixer* self) {
	PFUNC();
	if(self->type == CM_AMIGA)
		Amiga_reset((struct Amiga*)self);
}

void CoreMixer_fast(struct CoreMixer* self) { (void) self; }
void CoreMixer_accurate(struct CoreMixer* self) {(void) self; }

//js function reset
void CoreMixer_initialize(struct CoreMixer* self) {
	PFUNC();
	struct Sample* sample = &self->buffer[0];

	self->samplesLeft = 0;
	self->remains     = 0;
	self->completed   = 0;

	while (sample) {
		sample->l = sample->r = 0.0;
		sample = sample->next;
	}
}

int CoreMixer_get_complete(struct CoreMixer* self) {
	return self->completed;
}

void CoreMixer_set_complete(struct CoreMixer* self, int value) {
	self->completed = value ^ self->player->loopSong;
}

int CoreMixer_get_bufferSize(struct CoreMixer* self) {
	(void) self;
	return COREMIXER_MAX_BUFFER;
	//return self->vector_count_buffer;
	//return self->buffer->length; 
}

void CoreMixer_set_bufferSize(struct CoreMixer* self, int value) {
	PFUNC();
	int i = 0; int len = 0;
	//if (value == len || value < 2048) return;
	assert_op(value, <=, COREMIXER_MAX_BUFFER);

	len = self->vector_count_buffer;
	self->vector_count_buffer = value;

	if (value > len) {
		Sample_ctor(&self->buffer[len]);
		for(i = ++len; i < value; i++) {
			Sample_ctor(&self->buffer[i]);
			self->buffer[i - 1].next = &self->buffer[i];
		}
	}
}

