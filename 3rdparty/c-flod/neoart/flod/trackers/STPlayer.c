/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.1 - 2012/04/13

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "STPlayer.h"
#include "../flod_internal.h"

static const unsigned short PERIODS[] = {
        856,808,762,720,678,640,604,570,538,508,480,453,
        428,404,381,360,339,320,302,285,269,254,240,226,
        214,202,190,180,170,160,151,143,135,127,120,113,
        0,0,0
};


void STPlayer_defaults(struct STPlayer* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

/* amiga default is null */
void STPlayer_ctor(struct STPlayer* self, struct Amiga *amiga) {
	CLASS_CTOR_DEF(STPlayer);
	// original constructor code goes here
	//super(amiga);
	AmigaPlayer_ctor((struct AmigaPlayer *) self, amiga);

	//self->track   = new Vector.<int>(128, true);
	//self->samples = new Vector.<AmigaSample>(16, true);
	//self->voices  = new Vector.<STVoice>(4, true);
	
	unsigned i;
	for (i = 0; i < STPLAYER_MAX_VOICES; i ++) {
		STVoice_ctor(&self->voices[i], i);
		if(i) self->voices[i - 1].next = &self->voices[i];
	}
	
	//vtable
	self->super.super.set_ntsc = STPlayer_set_ntsc;
	self->super.super.set_force = STPlayer_set_force;
	self->super.super.process = STPlayer_process;
	self->super.super.initialize = STPlayer_initialize;
	self->super.super.loader = STPlayer_loader;
	
	self->super.super.min_filesize = 1625;
}

struct STPlayer* STPlayer_new(struct Amiga *amiga) {
	CLASS_NEW_BODY(STPlayer, amiga);
}

static int isLegal(char *text) {
	int i = 0;
	if (!text[i]) return 0;

	while(text[i]) {
		if (text[i] != '\n' && (text[i] < 32 || text[i] > 127)) return 0;
		i++;
	}
	return 1;
}


//override
void STPlayer_set_force(struct STPlayer* self, int value) {
	if (value < ULTIMATE_SOUNDTRACKER)
		value = ULTIMATE_SOUNDTRACKER;
	else if (value > DOC_SOUNDTRACKER_20)
		value = DOC_SOUNDTRACKER_20;

	self->super.super.version = value;
}

//override
void STPlayer_set_ntsc(struct STPlayer* self, int value) {
	AmigaPlayer_set_ntsc(&self->super, value);

	if (self->super.super.version < DOC_SOUNDTRACKER_9)
		self->super.amiga->super.samplesTick = (240 - self->super.super.tempo) * (value ? 7.5152005551f : 7.58437970472f);
}

//override
void STPlayer_process(struct STPlayer* self) {
	struct AmigaChannel *chan = NULL;
	struct AmigaRow *row = NULL;
	struct AmigaSample *sample = NULL;
	int value; 
	struct STVoice *voice = &self->voices[0];

	if (!self->super.super.tick) {
		value = self->track[self->trackPos] + self->patternPos;

		while (voice) {
			chan = voice->channel;
			voice->enabled = 0;

			row = &self->patterns[value + voice->index];
			voice->period = row->note;
			voice->effect = row->effect;
			voice->param  = row->param;

			if (row->sample) {
				sample = voice->sample = &self->samples[row->sample];

				if (((self->super.super.version & 2) == 2) && voice->effect == 12) 
					AmigaChannel_set_volume(chan, voice->param);
				else 
					AmigaChannel_set_volume(chan, sample->volume);
			} else {
				sample = voice->sample;
			}

			if (voice->period) {
				voice->enabled = 1;

				AmigaChannel_set_enabled(chan, 0);
				chan->pointer = sample->pointer;
				chan->length  = sample->length;
				voice->last = voice->period;
				AmigaChannel_set_period(chan, voice->period);
			}

			if (voice->enabled) AmigaChannel_set_enabled(chan, 1);
			chan->pointer = sample->loopPtr;
			chan->length  = sample->repeat;

			if (self->super.super.version < DOC_SOUNDTRACKER_20) {
				voice = voice->next;
				continue;
			}

			switch (voice->effect) {
				case 11:  //position jump
					self->trackPos = voice->param - 1;
					self->jumpFlag ^= 1;
					break;
				case 12:  //set volume
					AmigaChannel_set_volume(chan, voice->param);
					break;
				case 13:  //pattern break
					self->jumpFlag ^= 1;
					break;
				case 14:  //set filter
					self->super.amiga->filter->active = voice->param ^ 1;
					break;
				case 15:  //set speed
					if (!voice->param) break;
					self->super.super.speed = voice->param & 0x0f;
					self->super.super.tick = 0;
					break;
				default:
					break;
			}
			voice = voice->next;
		}
	} else {
		while (voice) {
			if (!voice->param) {
				voice = voice->next;
				continue;
			}
			chan = voice->channel;

			if (self->super.super.version == ULTIMATE_SOUNDTRACKER) {
				if (voice->effect == 1) {
					STPlayer_arpeggio(self, voice);
				} else if (voice->effect == 2) {
					value = voice->param >> 4;

					if (value) voice->period += value;
					else voice->period -= (voice->param & 0x0f);

					AmigaChannel_set_period(chan, voice->period);
				}
			} else {
				switch (voice->effect) {
					case 0: //arpeggio
						STPlayer_arpeggio(self, voice);
						break;
					case 1: //portamento up
						voice->last -= voice->param & 0x0f;
						if (voice->last < 113) voice->last = 113;
						AmigaChannel_set_period(chan, voice->last);
						break;
					case 2: //portamento down
						voice->last += voice->param & 0x0f;
						if (voice->last > 856) voice->last = 856;
						AmigaChannel_set_period(chan, voice->last);
						break;
					default:
						break;
				}

				if ((self->super.super.version & 2) != 2) {
					voice = voice->next;
					continue;
				}

				switch (voice->effect) {
					case 12:  //set volume
						AmigaChannel_set_volume(chan, voice->param);
						break;
					case 14:  //set filter
						self->super.amiga->filter->active = 0;
						break;
					case 15:  //set speed
						self->super.super.speed = voice->param & 0x0f;
						break;
					default:
						break;
				}
			}
			voice = voice->next;
		}
	}

	if (++(self->super.super.tick) == self->super.super.speed) {
		self->super.super.tick = 0;
		self->patternPos += 4;

		if (self->patternPos == 256 || self->jumpFlag) {
			self->patternPos = self->jumpFlag = 0;

			if (++(self->trackPos) == self->length) {
				self->trackPos = 0;
				CoreMixer_set_complete(&self->super.amiga->super, 1);
			}
		}
	}
}

//override
void STPlayer_initialize(struct STPlayer* self) {
	struct STVoice *voice = &self->voices[0];
	CorePlayer_initialize(&self->super.super);
	//super->initialize();
	AmigaPlayer_set_ntsc((struct AmigaPlayer*) self, self->super.standard);

	self->super.super.speed = 6;
	self->trackPos   = 0;
	self->patternPos = 0;
	self->jumpFlag   = 0;

	while (voice) {
		STVoice_initialize(voice);
		voice->channel = &self->super.amiga->channels[voice->index];
		voice->sample  = &self->samples[0];
		voice = voice->next;
	}
}

static int is_ust_tune(struct STPlayer* self, struct ByteArray *stream) {
	int score = 0; 
	char zero_a = 0;
	char buf[15];
	char zero_b = 0;
	
	if (ByteArray_get_length(stream) <= self->super.super.min_filesize) return 0;
	
	self->title_buf[21] = 0;
	stream->readMultiByte(stream, self->title_buf, 20);
	self->super.super.title = self->title_buf;
	score += self->super.super.title[0] && isLegal(self->super.super.title);
	
	stream->readMultiByte(stream, buf, sizeof(buf));
	score += buf[0] && isLegal(buf) && strlen(buf) >= 4;
	if(score && is_str(buf, "st-0")) score += 2;
	
	ByteArray_set_position(stream, 0x32 - 1);
	stream->readMultiByte(stream, buf, sizeof(buf));
	score += buf[0] < 32 && buf[1] && isLegal(buf + 1) && strlen(buf + 1) >= 4;
	if(score && is_str(buf + 1, "st-0")) score += 2;
	
	ByteArray_set_position(stream, 0x50 - 1);
	stream->readMultiByte(stream, buf, sizeof(buf));
	score += buf[0] < 32 && buf[1] && isLegal(buf + 1) && strlen(buf + 1) >= 4;
	if(score && is_str(buf + 1, "st-0")) score += 2;
	
	ByteArray_set_position(stream, 0x6e - 1);
	stream->readMultiByte(stream, buf, sizeof(buf));
	score += buf[0] < 32 && buf[1] && isLegal(buf + 1) && strlen(buf + 1) >= 4;
	if(score && is_str(buf + 1, "st-0")) score += 2;
	return score;
}

//override
void STPlayer_loader(struct STPlayer* self, struct ByteArray *stream) {
	int higher = 0;
	int i = 0; 
	int j = 0; 
	struct AmigaRow *row = NULL;
	struct AmigaSample *sample = NULL;
	int score = 0; 
	int size = 0; 
	int value = 0;
	
	if((score = is_ust_tune(self, stream)) < 3) return;

	self->super.super.version = ULTIMATE_SOUNDTRACKER;
	ByteArray_set_position(stream, 42);

	for (i = 1; i < 16; ++i) {
		value = stream->readUnsignedShort(stream);

		if (!value) {
			// FIXME !!!
			//self->samples[i] = null;
			ByteArray_set_position_rel(stream, 28);
			continue;
		}
		sample = &self->samples[i];
		AmigaSample_ctor(sample);
		//sample = new AmigaSample();
		ByteArray_set_position_rel(stream, -24);
		
		ByteArray_readMultiByte(stream, self->sample_names[i], 22);

		sample->name = self->sample_names[i];
		sample->length = value << 1;
		ByteArray_set_position_rel(stream, 3);

		sample->volume = stream->readUnsignedByte(stream);
		sample->loop   = stream->readUnsignedShort(stream);
		sample->repeat = stream->readUnsignedShort(stream) << 1;

		ByteArray_set_position_rel(stream, 22);
		sample->pointer = size;
		size += sample->length;
		//self->samples[i] = sample;

		score += isLegal(sample->name);
		if (sample->length > 9999) self->super.super.version = MASTER_SOUNDTRACKER;
	}

	ByteArray_set_position(stream, 470);
	self->length = stream->readUnsignedByte(stream);
	self->super.super.tempo  = stream->readUnsignedByte(stream);

	for (i = 0; i < 128; ++i) {
		value = stream->readUnsignedByte(stream) << 8;
		if (value > 16384) score--;
		self->track[i] = value;
		if (value > higher) higher = value;
	}

	ByteArray_set_position(stream, 600);
	higher += 256;
	//FIXME
	//patterns = new Vector.<AmigaRow>(higher, true);
	//if(higher >= STPLAYER_MAX_PATTERNS) {
		/* the format detection is very weak, therefore we do this check
		 * to prevent a crash. */
	//	return;
	//}
	assert_op(higher, <=, STPLAYER_MAX_PATTERNS);
	
	i = (ByteArray_get_length(stream) - size - 600) >> 2;
	if (higher > i) higher = i;
	
	// "weak spot"
	assert_op(higher, <=, STPLAYER_MAX_PATTERNS);
	

	for (i = 0; i < higher; ++i) {
		//FIXME
		//row = new AmigaRow();
		row = &self->patterns[i];
		AmigaRow_ctor(row);

		row->note   = stream->readUnsignedShort(stream);
		value       = stream->readUnsignedByte(stream);
		row->param  = stream->readUnsignedByte(stream);
		row->effect = value & 0x0f;
		row->sample = value >> 4;

		//self->patterns[i] = row;

		if (row->effect > 2 && row->effect < 11) score--;
		if (row->note) {
			if (row->note < 113 || row->note > 856) score--;
		}

		//FIXME need to create a lookup table to see which samples are assigned
		if (row->sample && (row->sample > 15 /* || !self->samples[row->sample]*/)) {
			if (row->sample > 15) score--;
			row->sample = 0;
		}

		if (row->effect > 2 || (!row->effect && row->param != 0))
			self->super.super.version = DOC_SOUNDTRACKER_9;

		if (row->effect == 11 || row->effect == 13)
			self->super.super.version = DOC_SOUNDTRACKER_20;
	}

	Amiga_store(self->super.amiga, stream, size, -1);

	for (i = 1; i < 16; ++i) {
		sample = &self->samples[i];
		if (!sample) continue;

		if (sample->loop) {
			sample->loopPtr = sample->pointer + sample->loop;
			sample->pointer = sample->loopPtr;
			sample->length  = sample->repeat;
		} else {
			sample->loopPtr = self->super.amiga->vector_count_memory;
			sample->repeat  = 2;
		}

		size = sample->pointer + 4;
		for (j = sample->pointer; j < size; ++j) self->super.amiga->memory[j] = 0;
	}

	sample = &self->samples[0];
	AmigaSample_ctor(sample);
	//sample = new AmigaSample();
	sample->pointer = sample->loopPtr = self->super.amiga->vector_count_memory;
	sample->length  = sample->repeat  = 2;
	//self->samples[0] = sample;

	if (score < 1) self->super.super.version = 0;
}

void STPlayer_arpeggio(struct STPlayer* self, struct STVoice *voice) {
	struct AmigaChannel *chan = voice->channel;
	int i = 0;
	int param = self->super.super.tick % 3;

	if (!param) {
		AmigaChannel_set_period(chan, voice->last);
		return;
	}

	if (param == 1) param = voice->param >> 4;
	else param = voice->param & 0x0f;

	while (voice->last != PERIODS[i]) i++;
	AmigaChannel_set_period(chan, PERIODS[i + param]);
}
