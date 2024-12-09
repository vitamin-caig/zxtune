/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.1 - 2012/04/14

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "FXPlayer.h"
#include "../flod_internal.h"

void FXPlayer_set_force(struct FXPlayer* self, int value);
void FXPlayer_set_ntsc(struct FXPlayer* self, int value);
void FXPlayer_process(struct FXPlayer* self);
void FXPlayer_initialize(struct FXPlayer* self);
void FXPlayer_loader(struct FXPlayer* self, struct ByteArray *stream);

static const signed short PERIODS[] = {
        1076,1016,960,906,856,808,762,720,678,640,604,570,
         538, 508,480,453,428,404,381,360,339,320,302,285,
         269, 254,240,226,214,202,190,180,170,160,151,143,
         135, 127,120,113,113,113,113,113,113,113,113,113,
         113, 113,113,113,113,113,113,113,113,113,113,113,
         113, 113,113,113,113,113,-1,
};

void FXPlayer_defaults(struct FXPlayer* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void FXPlayer_ctor(struct FXPlayer* self, struct Amiga *amiga) {
	CLASS_CTOR_DEF(FXPlayer);
	// original constructor code goes here
	AmigaPlayer_ctor(&self->super, amiga);
	//super(amiga);

	//track  = new Vector.<int>(128, true);
	//voices = new Vector.<FXVoice>(4, true);
	
	unsigned i = 0;
	for(; i < FXPLAYER_MAX_VOICES; i++) {
		FXVoice_ctor(&self->voices[i], i);
		if(i) self->voices[i - 1].next = &self->voices[i];
	}

	//vtable
	self->super.super.loader = FXPlayer_loader;
	self->super.super.process = FXPlayer_process;
	self->super.super.initialize = FXPlayer_initialize;
	self->super.super.set_force = FXPlayer_set_force;
	self->super.super.set_ntsc = FXPlayer_set_ntsc;
	
	self->super.super.min_filesize = 1685;
}

struct FXPlayer* FXPlayer_new(struct Amiga *amiga) {
	CLASS_NEW_BODY(FXPlayer, amiga);
}

//override
void FXPlayer_set_force(struct FXPlayer* self, int value) {
	if (value < SOUNDFX_10)
		value = SOUNDFX_10;
	else if (value > SOUNDFX_20)
		value = SOUNDFX_20;

	self->super.super.version = value;
}

//override
void FXPlayer_set_ntsc(struct FXPlayer* self, int value) {
	AmigaPlayer_set_ntsc(&self->super, value);
	//self->super->ntsc = value;

	// FIXME : check whether this is supposed to do float math
	self->super.amiga->super.samplesTick = (self->super.super.tempo / 122) * (value ? 7.5152005551 : 7.58437970472);
}

//override
void FXPlayer_process(struct FXPlayer* self) {
	struct AmigaChannel *chan = 0;
	int index = 0; 
	int period = 0; 
	struct AmigaRow *row = 0;
	struct AmigaSample *sample = 0;
	int value = 0; 
	struct FXVoice *voice = &self->voices[0];

	if (!self->super.super.tick) {
		value = self->track[self->trackPos] + self->patternPos;

		while (voice) {
			chan = voice->channel;
			voice->enabled = 0;
			
			unsigned idxx = value + voice->index;
			assert_op(idxx, <, FXPLAYER_MAX_PATTERNS);

			row = &self->patterns[idxx];
			voice->period = row->note;
			voice->effect = row->effect;
			voice->param  = row->param;

			if (row->note == -3) {
				voice->effect = 0;
				voice = voice->next;
				continue;
			}

			if (row->sample) {
				assert_op(row->sample, <, FXPLAYER_MAX_SAMPLES);
				sample = voice->sample = &self->samples[row->sample];
				voice->volume = sample->volume;

				if (voice->effect == 5)
				voice->volume += voice->param;
				else if (voice->effect == 6)
				voice->volume -= voice->param;

				AmigaChannel_set_volume(chan, voice->volume);
			} else {
				sample = voice->sample;
			}

			if (voice->period) {
				voice->last = voice->period;
				voice->slideSpeed = 0;
				voice->stepSpeed  = 0;

				voice->enabled = 1;
				AmigaChannel_set_enabled(chan, 0);

				switch (voice->period) {
					case -2:
						AmigaChannel_set_volume(chan, 0);
						break;
					case -4:
						self->jumpFlag = 1;
						break;
					case -5:
						break;
					default:
						chan->pointer = sample->pointer;
						chan->length  = sample->length;

						if (self->delphine) AmigaChannel_set_period(chan, voice->period << 1);
						else AmigaChannel_set_period(chan, voice->period);
						break;
				}

				if (voice->enabled) AmigaChannel_set_enabled(chan, 1);
				chan->pointer = sample->loopPtr;
				chan->length  = sample->repeat;
			}
			voice = voice->next;
		}
	} else {
		while (voice) {
			chan = voice->channel;

			if (self->super.super.version == SOUNDFX_18 && voice->period == -3) {
				voice = voice->next;
				continue;
			}

			if (voice->stepSpeed) {
				voice->stepPeriod += voice->stepSpeed;

				if (voice->stepSpeed < 0) {
					if (voice->stepPeriod < voice->stepWanted) {
						voice->stepPeriod = voice->stepWanted;
						if (self->super.super.version > SOUNDFX_18) voice->stepSpeed = 0;
					}
				} else {
					if (voice->stepPeriod > voice->stepWanted) {
						voice->stepPeriod = voice->stepWanted;
						if (self->super.super.version > SOUNDFX_18) voice->stepSpeed = 0;
					}
				}

				if (self->super.super.version > SOUNDFX_18) voice->last = voice->stepPeriod;
				AmigaChannel_set_period(chan, voice->stepPeriod);
			} else {
				if (voice->slideSpeed) {
					value = voice->slideParam & 0x0f;

					if (value) {
						if (++voice->slideCtr == value) {
							voice->slideCtr = 0;
							value = (voice->slideParam << 4) << 3;

							if (!voice->slideDir) {
								voice->slidePeriod += 8;
								AmigaChannel_set_period(chan, voice->slidePeriod);
								value += voice->slideSpeed;
								if (value == voice->slidePeriod) voice->slideDir = 1;
							} else {
								voice->slidePeriod -= 8;
								AmigaChannel_set_period(chan, voice->slidePeriod);
								value -= voice->slideSpeed;
								if (value == voice->slidePeriod) voice->slideDir = 0;
							}
						} else {
							voice = voice->next;
							continue;
						}
					}
				}

				value = 0;

				switch (voice->effect) {
					case 0:
						break;
					case 1:   //arpeggio
						value = self->super.super.tick % 3;
						index = 0;

						if (value == 2) {
							AmigaChannel_set_period(chan, voice->last);
							voice = voice->next;
							continue;
						}

						if (value == 1) value = voice->param & 0x0f;
						else value = voice->param >> 4;

						while (index < ARRAY_SIZE(PERIODS) && voice->last != PERIODS[index]) index++;
						assert_op(index + value, <, ARRAY_SIZE(PERIODS));
						AmigaChannel_set_period(chan, PERIODS[index + value]);
						break;
					case 2:   //pitchbend
						value = voice->param >> 4;
						if (value) voice->period += value;
						else voice->period -= voice->param & 0x0f;
						AmigaChannel_set_period(chan, voice->period);
						break;
					case 3:   //filter on
						self->super.amiga->filter->active = 1;
						break;
					case 4:   //filter off
						self->super.amiga->filter->active = 0;
						break;
					case 8:   //step down
						value = -1;
					case 7:   //step up
						voice->stepSpeed  = voice->param & 0x0f;
						voice->stepPeriod = self->super.super.version > SOUNDFX_18 ? voice->last : voice->period;
						if (value < 0) voice->stepSpeed = -voice->stepSpeed;
						index = 0;

						while (true) {
							assert_op(index, <, ARRAY_SIZE(PERIODS));
							period = PERIODS[index];
							if (period == voice->stepPeriod) break;
							if (period < 0) {
								index = -1;
								break;
							} else
								index++;
						}

						if (index > -1) {
							period = voice->param >> 4;
							if (value > -1) period = -period;
							index += period;
							if (index < 0) index = 0;
							assert_op(index, <, ARRAY_SIZE(PERIODS));
							voice->stepWanted = PERIODS[index];
						} else
							voice->stepWanted = voice->period;
						break;
					case 9:   //auto slide
						voice->slideSpeed = voice->slidePeriod = voice->period;
						voice->slideParam = voice->param;
						voice->slideDir = 0;
						voice->slideCtr = 0;
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
void FXPlayer_initialize(struct FXPlayer* self) {
	struct FXVoice *voice = &self->voices[0];
	
	CorePlayer_initialize(&self->super.super);
	//self->super->initialize();
	FXPlayer_set_ntsc(self, self->super.standard);
	//self->ntsc = standard;

	self->super.super.speed      = 6;
	self->trackPos   = 0;
	self->patternPos = 0;
	self->jumpFlag   = 0;

	while (voice) {
		FXVoice_initialize(voice);
		voice->channel = &self->super.amiga->channels[voice->index];
		voice->sample  = &self->samples[0];
		voice = voice->next;
	}
}

//override
void FXPlayer_loader(struct FXPlayer* self, struct ByteArray *stream) {
	int higher = 0; 
	int i = 0; 
	char id[4];
	int j = 0; 
	int len = 0; 
	int offset = 0; 
	struct AmigaRow *row = 0;
	struct AmigaSample *sample = 0;
	int size = 0; 
	int value = 0;
	
	if (ByteArray_get_length(stream) < 1686) return;

	ByteArray_set_position(stream, 60);
	stream->readMultiByte(stream, id, 4);

	if (!is_str(id, "SONG")) {
		ByteArray_set_position(stream, 124);
		stream->readMultiByte(stream, id, 4);
		if (!is_str(id, "SO31")) return;
		if (ByteArray_get_length(stream) < 2350) return;

		offset = 544;
		len = 32;
		self->super.super.version = SOUNDFX_20;
	} else {
		offset = 0;
		len = 16;
		self->super.super.version = SOUNDFX_10;
	}

	unsigned samples_count = len;
	assert_op(len, <=, FXPLAYER_MAX_SAMPLES);
	//self->samples = new Vector.<AmigaSample>(len, true);
	self->super.super.tempo = stream->readUnsignedShort(stream);
	ByteArray_set_position(stream, 0);

	for (i = 1; i < len; ++i) {
		value = stream->readUnsignedInt(stream);

		if (value) {
			//sample = new AmigaSample();
			sample = &self->samples[i];
			AmigaSample_ctor(sample);
			
			sample->pointer = size;
			size += value;
			//self->samples[i] = sample;
		} else {
			//self->samples[i] = null;
		}
	}
	ByteArray_set_position_rel(stream, +20);

	for (i = 1; i < len; ++i) {
		sample = &self->samples[i];
		/*
		if (sample == null) {
			ByteArray_set_position_rel(stream, +30);
			continue;
		}
		*/
		stream->readMultiByte(stream, self->sample_names[i], 22);
		sample->name   = self->sample_names[i];
		sample->length = stream->readUnsignedShort(stream) << 1;
		sample->volume = stream->readUnsignedShort(stream);
		sample->loop   = stream->readUnsignedShort(stream);
		sample->repeat = stream->readUnsignedShort(stream) << 1;
	}

	ByteArray_set_position(stream, 530 + offset);
	self->length = len = stream->readUnsignedByte(stream);
	ByteArray_set_position_rel(stream, +1);

	for (i = 0; i < len; ++i) {
		value = stream->readUnsignedByte(stream) << 8;
		self->track[i] = value;
		if (value > higher) higher = value;
	}

	if (offset) offset += 4;
	ByteArray_set_position(stream, 660 + offset);
	higher += 256;
	assert_op(higher, <=, FXPLAYER_MAX_PATTERNS);
	//patterns = new Vector.<AmigaRow>(higher, true);

	//len = self->samples->length;
	len = samples_count;

	for (i = 0; i < higher; ++i) {
		//row = new AmigaRow();
		row = &self->patterns[i];
		AmigaRow_ctor(row);
		
		row->note   = stream->readShort(stream);
		value      = stream->readUnsignedByte(stream);
		row->param  = stream->readUnsignedByte(stream);
		row->effect = value & 0x0f;
		row->sample = value >> 4;

		//self->patterns[i] = row;

		if (self->super.super.version == SOUNDFX_20) {
			if (row->note & 0x1000) {
				row->sample += 16;
				if (row->note > 0) row->note &= 0xefff;
			}
		} else {
			if (row->effect == 9 || row->note > 856)
			self->super.super.version = SOUNDFX_18;

			if (row->note < -3)
			self->super.super.version = SOUNDFX_19;
		}
		//if (row->sample >= len || self->samples[row->sample] == null) row->sample = 0;
		if (row->sample >= len) row->sample = 0;
	}

	Amiga_store(self->super.amiga, stream, size, -1);

	assert_op(len, <=, FXPLAYER_MAX_SAMPLES);
	for (i = 1; i < len; ++i) {
		sample = &self->samples[i];
		//if (sample == null) continue; // FIXME this check is non-working with by-value buffers

		if (sample->loop)
			sample->loopPtr = sample->pointer + sample->loop;
		else {
			sample->loopPtr = self->super.amiga->vector_count_memory;  //self->super.amiga->memory->length;
			sample->repeat  = 2;
		}
		size = sample->pointer + 4;
		assert_op(size, <=, AMIGA_MAX_MEMORY);
		for (j = sample->pointer; j < size; ++j) self->super.amiga->memory[j] = 0;
	}

	//sample = new AmigaSample();
	sample = &self->samples[0];
	AmigaSample_ctor(sample);
	
	sample->pointer = sample->loopPtr = self->super.amiga->vector_count_memory; //self->super.amiga->memory->length;
	sample->length  = sample->repeat  = 2;
	//self->samples[0] = sample;

	ByteArray_set_position(stream, 0);
	higher = self->delphine = 0;
	
	for (i = 0; i < 265; ++i) higher += stream->readUnsignedShort(stream);

	switch (higher) {
		case 172662:
		case 1391423:
		case 1458300:
		case 1706977:
		case 1920077:
		case 1920694:
		case 1677853:
		case 1931956:
		case 1926836:
		case 1385071:
		case 1720635:
		case 1714491:
		case 1731874:
		case 1437490:
			self->delphine = 1;
			break;
		default:
			break;
	}
}
