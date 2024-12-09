/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.1 - 2012/04/09

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "Amiga.h"
#include "../flod_internal.h"
#include "../whittaker/DWPlayer.h"

void Amiga_defaults(struct Amiga* self) {
	CLASS_DEF_INIT();
	/* static initializers go here */	
	self->model = MODEL_A1200;
	self->loopLen = 4;
	self->clock = 0.0f;
	self->master = 0.00390625f;
}

void Amiga_ctor(struct Amiga* self) {
	CLASS_CTOR_DEF(Amiga);
	/* original constructor code goes here */	
	//super();
	
	PFUNC();
	
	CoreMixer_ctor(&self->super);
	self->super.type = CM_AMIGA;
	CoreMixer_set_bufferSize((struct CoreMixer*) self, COREMIXER_MAX_BUFFER);
	self->filter = AmigaFilter_new();
	
	unsigned int i;
	for(i = 0; i < AMIGA_MAX_CHANNELS; i++) {
		AmigaChannel_ctor(&self->channels[i], i);
		if(i) self->channels[i -1].next = &self->channels[i];
	}
	/*
	self->channels = new Vector.<AmigaChannel>(4, true);

	self->channels[0] = AmigaChannel_new(0);
	self->channels[0].next = self->channels[1] = AmigaChannel_new(1);
	self->channels[1].next = self->channels[2] = AmigaChannel_new(2);
	self->channels[2].next = self->channels[3] = AmigaChannel_new(3); */
	
	// vtable
	self->super.fast = (void*) Amiga_fast;
	self->super.accurate = (void*) Amiga_fast; // simply use the fast one
}

struct Amiga* Amiga_new(void) {
	CLASS_NEW_BODY(Amiga);
}


void Amiga_set_volume(struct Amiga* self, int value) {
	if (value > 0) {
		if (value > 64) value = 64;
		self->master = ((float) value / 64.f) * 0.00390625f;
	} else {
		self->master = 0.0f;
	}
}

void Amiga_memory_set_length(struct Amiga *self, unsigned len) {
	assert_op(len, <=, AMIGA_MAX_MEMORY);
	self->vector_count_memory = len;
}

// pointer default value: -1
int Amiga_store(struct Amiga* self, struct ByteArray *stream, int len, int pointer) {
	int add = 0; 
	int i = 0;
	int pos = ByteArray_get_position(stream);
	//int start = self->memory->length;
	int start = self->vector_count_memory;
	int total = 0;

	if (pointer > -1) ByteArray_set_position(stream, pointer);
	total = ByteArray_get_position(stream) + len;

	if (total >= ByteArray_get_length(stream)) {
		add = total - ByteArray_get_length(stream);
		len = ByteArray_get_length(stream) - ByteArray_get_position(stream);
	}
	
	len += start;
	Amiga_memory_set_length(self, len);
	//as3 memory.length is automatically increased + 1 when a byte gets written
	for (i = start; i < len; ++i)
		self->memory[i] = stream->readByte(stream);

	//self->memory->length += add;
	Amiga_memory_set_length(self, self->vector_count_memory + add); // FIXME dubious, why add and not len ?
	assert_op(self->vector_count_memory, <=, AMIGA_MAX_MEMORY);
	if (pointer > -1) ByteArray_set_position(stream, pos);
	return start;
}

//override
void Amiga_initialize(struct Amiga* self) {
	PFUNC();
	//self->super->initialize();
	//if(!self->super.player->record) ByteArray_clear(self->super.wave);
	AmigaFilter_initialize(self->filter);

	if (!self->memory_fixed) { 
		// we mimic the boolean behaviour of memory.fixed using that variable
		self->loopPtr = self->vector_count_memory;
		//self->loopPtr = 54686;
		Amiga_memory_set_length(self, self->vector_count_memory + self->loopLen);
		self->memory_fixed = true;
	}
	
	unsigned i;
	for(i = 0; i < AMIGA_MAX_CHANNELS; i++) AmigaChannel_initialize(&self->channels[i]);
}

//override
void Amiga_reset(struct Amiga* self) {
	PFUNC();
      //self->memory = new Vector.<int>();
      //memset(self->memory, 0, sizeof(self->memory));
      self->vector_count_memory = 0;
      // we need to set memory_fixed to false so that Amiga_initialize will set correct memory bounds.
      self->memory_fixed = false;
}

#define HARDWARE_FROM_AMIGA
#include "Hardware.h"

    //override
void Amiga_fast(struct Amiga* self) {
	struct AmigaChannel *chan = NULL;
	int i = 0;
	Number lvol = NAN;
	int mixed = 0;
	int mixLen = 0;
	int mixPos = 0;
	Number rvol = NAN;
	struct Sample *sample = NULL;
	int size = CoreMixer_get_bufferSize(&self->super);
	Number speed = NAN;
	int toMix = 0;
	Number value = NAN;
	
	PFUNC();

	if (self->super.completed) {
		if (!self->super.remains) return;
		size = self->super.remains;
	}

	while (mixed < size) {
		if (unlikely(!self->super.samplesLeft)) {

			self->super.player->process(self->super.player);
			self->super.samplesLeft = self->super.samplesTick;

			if (self->super.completed) {
				size = mixed + self->super.samplesTick;

				if (size > CoreMixer_get_bufferSize((struct CoreMixer*) self)) {
					self->super.remains = size - CoreMixer_get_bufferSize((struct CoreMixer*) self);
					size = CoreMixer_get_bufferSize((struct CoreMixer*) self);
				}
			}
		}

		toMix = self->super.samplesLeft;
		if ((mixed + toMix) >= size) toMix = size - mixed;
		mixLen = mixPos + toMix;
		chan = &self->channels[0];

		while (chan) {
			assert_op(mixPos, <, COREMIXER_MAX_BUFFER);
			sample = &self->super.buffer[mixPos];

			if (chan->audena && chan->audper > 60) {
				if (unlikely(chan->mute)) {
					chan->ldata = 0.0f;
					chan->rdata = 0.0f;
				}

				speed = chan->audper / self->clock;

				value = chan->audvol * self->master;
				lvol = value * (1 - chan->level);
				rvol = value * (1 + chan->level);

				for (i = mixPos; i < mixLen; ++i) {
					if (chan->delay) {
						chan->delay--;
					} else if (--chan->timer < 1.0f) { 
						if (!chan->mute) {
							assert_op(chan->audloc, <, (int) self->vector_count_memory);
							//assert_op(chan->audloc, <, AMIGA_MAX_MEMORY);
							value = self->memory[chan->audloc] * 0.0078125f;
							chan->ldata = value * lvol;
							chan->rdata = value * rvol;
						}

						chan->audloc++;
						chan->timer += speed;

						if (chan->audloc >= chan->audcnt) {
							chan->audloc = chan->pointer;
							chan->audcnt = chan->pointer + chan->length;
						}
					}

					sample->l += chan->ldata;
					sample->r += chan->rdata;
					sample = sample->next;
				}
			} else {
				for (i = mixPos; i < mixLen; ++i) {
					sample->l += chan->ldata;
					sample->r += chan->rdata;
					sample = sample->next;
				}
			}
			chan = chan->next;
		}

		mixPos = mixLen;
		mixed += toMix;
		self->super.samplesLeft -= toMix;
	}
	
	process_wave(self, size);

}

