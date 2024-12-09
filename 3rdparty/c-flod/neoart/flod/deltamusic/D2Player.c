/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.0 - 2012/03/30

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "D2Player.h"
#include "../flod_internal.h"

static const unsigned short PERIODS[] = {
           0,6848,6464,6096,5760,5424,5120,4832,4560,4304,4064,3840,
        3616,3424,3232,3048,2880,2712,2560,2416,2280,2152,2032,1920,
        1808,1712,1616,1524,1440,1356,1280,1208,1140,1076,1016, 960,
         904, 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480,
         452, 428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240,
         226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120,
         113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113,
         113,
};

void D2Player_loader(struct D2Player* self, struct ByteArray *stream);
void D2Player_process(struct D2Player* self);
void D2Player_initialize(struct D2Player* self);

void D2Player_defaults(struct D2Player* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void D2Player_ctor(struct D2Player* self, struct Amiga *amiga) {
	CLASS_CTOR_DEF(D2Player);
	// original constructor code goes here
	AmigaPlayer_ctor(&self->super, amiga);

	unsigned i;
	for (i = 0; i < D2PLAYER_MAX_VOICES; i++) {
		D2Voice_ctor(&self->voices[i], i);
		if(i) self->voices[i - 1].next = &self->voices[i];
	}
	
	//vtable
	self->super.super.loader = D2Player_loader;
	self->super.super.process = D2Player_process;
	self->super.super.initialize = D2Player_initialize;
	
	self->super.super.min_filesize = 3018;
	
}

struct D2Player* D2Player_new(struct Amiga *amiga) {
	CLASS_NEW_BODY(D2Player, amiga);
}

//override
void D2Player_process(struct D2Player* self) {
	struct AmigaChannel *chan = 0;
	int i = 0;
	int level = 0; 
	struct AmigaRow *row = 0; 
	struct D2Sample *sample = 0;
	int value = 0; 
	struct D2Voice *voice = &self->voices[0];
	/* >>> bitwise unsigned right shift Operator */

	for (; i < 64;) {
		self->noise = (self->noise << 7) | (self->noise >> 25);
		self->noise += 0x6eca756d;
		self->noise ^= 0x9e59a92b;

		value = (self->noise >> 24) & 255;
		if (value > 127) value |= -256;
		self->super.amiga->memory[i++] = value;

		value = (self->noise >> 16) & 255;
		if (value > 127) value |= -256;
		self->super.amiga->memory[i++] = value;

		value = (self->noise >> 8) & 255;
		if (value > 127) value |= -256;
		self->super.amiga->memory[i++] = value;

		value = self->noise & 255;
		if (value > 127) value |= -256;
		self->super.amiga->memory[i++] = value;
	}

	if (--(self->super.super.tick) < 0) self->super.super.tick = self->super.super.speed;

	while (voice) {
		if (voice->trackLen < 1) {
			voice = voice->next;
			continue;
		}

		chan = voice->channel;
		sample = voice->sample;

		if (sample->synth) {
			chan->pointer = sample->super.loopPtr;
			chan->length  = sample->super.repeat;
		}

		if (self->super.super.tick == 0) {
			unsigned idxx;
			if (voice->patternPos == 0) {
				idxx = voice->trackPtr + voice->trackPos;
				assert_op(idxx, <, D2PLAYER_MAX_TRACKS);
				voice->step = &self->tracks[idxx];

				if (++voice->trackPos == voice->trackLen)
				voice->trackPos = voice->restart;
			}
			idxx = voice->step->pattern + voice->patternPos;
			assert_op(idxx, <, D2PLAYER_MAX_PATTERNS);
			row = voice->row = &self->patterns[idxx];

			if (row->note) {
				AmigaChannel_set_enabled(chan, 0);
				voice->note = row->note;
				idxx = row->note + voice->step->transpose;
				assert_op(idxx, <, ARRAY_SIZE(PERIODS));
				voice->period = PERIODS[idxx];

				assert_op(row->sample, <, D2PLAYER_MAX_SAMPLES);
				sample = voice->sample = &self->samples[row->sample];

				if (sample->synth < 0) {
					chan->pointer = sample->super.pointer;
					chan->length  = sample->super.length;
				}

				voice->arpeggioPos    = 0;
				voice->tableCtr       = 0;
				voice->tablePos       = 0;
				voice->vibratoCtr     = sample->vibratos[1];
				voice->vibratoPos     = 0;
				voice->vibratoDir     = 0;
				voice->vibratoPeriod  = 0;
				voice->vibratoSustain = sample->vibratos[2];
				voice->volume         = 0;
				voice->volumePos      = 0;
				voice->volumeSustain  = 0;
			}

			switch (row->effect) {
				default:
				case -1:
					break;
				case 0:
					self->super.super.speed = row->param & 15;
					break;
				case 1:
					self->super.amiga->filter->active = row->param;
					break;
				case 2:
					voice->pitchBend = ~(row->param & 255) + 1;
					break;
				case 3:
					voice->pitchBend = row->param & 255;
					break;
				case 4:
					voice->portamento = row->param;
					break;
				case 5:
					voice->volumeMax = row->param & 63;
					break;
				case 6:
					Amiga_set_volume(self->super.amiga, row->param);
					break;
				case 7:
					voice->arpeggioPtr = (row->param & 63) << 4;
					break;
			}
			voice->patternPos = ++voice->patternPos & 15;
		}
		sample = voice->sample;

		if (sample->synth >= 0) {
			if (voice->tableCtr) {
				voice->tableCtr--;
			} else {
				voice->tableCtr = sample->index;
				value = sample->table[voice->tablePos];

				if (value == 0xff) {
					value = sample->table[++voice->tablePos];
					if (value != 0xff) {
						voice->tablePos = value;
						value = sample->table[voice->tablePos];
					}
				}

				if (value != 0xff) {
					chan->pointer = value << 8;
					chan->length  = sample->super.length;
					if (++voice->tablePos > 47) voice->tablePos = 0;
				}
			}
		}
		value = sample->vibratos[voice->vibratoPos];

		if (voice->vibratoDir) voice->vibratoPeriod -= value;
		else voice->vibratoPeriod += value;

		if (--voice->vibratoCtr == 0) {
			unsigned idxx = voice->vibratoPos + 1;
			assert_op(idxx, <, D2SAMPLE_MAX_VIBRATOS);
			voice->vibratoCtr = sample->vibratos[idxx];
			voice->vibratoDir = ~voice->vibratoDir;
		}

		if (voice->vibratoSustain) {
			voice->vibratoSustain--;
		} else {
			voice->vibratoPos += 3;
			if (voice->vibratoPos == 15) voice->vibratoPos = 12;
			unsigned idxx = voice->vibratoPos + 2;
			assert_op(idxx, <, D2SAMPLE_MAX_VIBRATOS);
			voice->vibratoSustain = sample->vibratos[idxx];
		}

		if (voice->volumeSustain) {
			voice->volumeSustain--;
		} else {
			unsigned idxx;
			assert_op(voice->volumePos + 1, <, D2SAMPLE_MAX_VOLUMES);
			value = sample->volumes[voice->volumePos];
			level = sample->volumes[voice->volumePos + 1];

			if (level < voice->volume) {
				voice->volume -= value;
				if (voice->volume < level) {
					voice->volume = level;
					voice->volumePos += 3;
					idxx = voice->volumePos - 1;
					assert_op(idxx, <, D2SAMPLE_MAX_VOLUMES);
					voice->volumeSustain = sample->volumes[idxx];
				}
			} else {
				voice->volume += value;
				if (voice->volume > level) {
					voice->volume = level;
					voice->volumePos += 3;
					if (voice->volumePos == 15) voice->volumePos = 12;
					idxx = voice->volumePos - 1;
					assert_op(idxx, <, D2SAMPLE_MAX_VOLUMES);
					voice->volumeSustain = sample->volumes[idxx];
				}
			}
		}

		if (voice->portamento) {
			if (voice->period < voice->finalPeriod) {
				voice->finalPeriod -= voice->portamento;
				if (voice->finalPeriod < voice->period) voice->finalPeriod = voice->period;
			} else {
				voice->finalPeriod += voice->portamento;
				if (voice->finalPeriod > voice->period) voice->finalPeriod = voice->period;
			}
		}
		unsigned idxx = voice->arpeggioPtr + voice->arpeggioPos;
		assert_op(idxx, <, D2PLAYER_MAX_ARPEGGIOS);
		value = self->arpeggios[idxx];

		if (value == -128) {
			voice->arpeggioPos = 0;
			value = self->arpeggios[voice->arpeggioPtr];
		}
		voice->arpeggioPos = ++voice->arpeggioPos & 15;

		if (voice->portamento == 0) {
			value = voice->note + voice->step->transpose + value;
			if (value < 0) value = 0;
			assert_op(value, <, ARRAY_SIZE(PERIODS));
			voice->finalPeriod = PERIODS[value];
		}

		voice->vibratoPeriod -= (sample->pitchBend - voice->pitchBend);
		AmigaChannel_set_period(chan, voice->finalPeriod + voice->vibratoPeriod);

		value = (voice->volume >> 2) & 63;
		if (value > voice->volumeMax) value = voice->volumeMax;
		AmigaChannel_set_volume(chan, value);
		AmigaChannel_set_enabled(chan, 1);

		voice = voice->next;
	}
}

//override
void D2Player_initialize(struct D2Player* self) {
	struct D2Voice *voice = &self->voices[0];
	
	CorePlayer_initialize(&self->super.super);
	
	self->super.super.speed = 5;
	self->super.super.tick  = 1;
	self->noise = 0;

	while (voice) {
		D2Voice_initialize(voice);
		voice->channel = &self->super.amiga->channels[voice->index];
		voice->sample  = &self->samples[self->samples_count - 1];
		voice->trackPtr = self->data[voice->index];
		voice->restart  = self->data[voice->index + 4];
		voice->trackLen = self->data[voice->index + 8];
		voice = voice->next;
	}
}

//fixed
#define STACK_MAX_OFFSETS 128
//override
void D2Player_loader(struct D2Player* self, struct ByteArray *stream) {
	int i = 0; 
	char id[4];
	int j = 0;
	int len = 0;
	int offsets[STACK_MAX_OFFSETS] = {0};
	int position = 0;
	struct AmigaRow *row = 0;
	struct D2Sample *sample = 0;
	struct AmigaStep *step = 0;
	int value = 0;
	
	ByteArray_set_position(stream, 3014);
	stream->readMultiByte(stream, id, 4);
	if (!is_str(id, ".FNL")) return;

	ByteArray_set_position(stream, 4042);
	//data = new Vector.<int>(12, true);

	for (i = 0; i < 4; ++i) {
		self->data[i + 4] = stream->readUnsignedShort(stream) >> 1;
		value = stream->readUnsignedShort(stream) >> 1;
		self->data[i + 8] = value;
		len += value;
	}

	value = len;
	for (i = 3; i > 0; --i) self->data[i] = (value -= self->data[i + 8]);
	assert_op(len, <=, D2PLAYER_MAX_TRACKS);
	//tracks = new Vector.<AmigaStep>(len, true);

	for (i = 0; i < len; ++i) {
		step = &self->tracks[i];
		AmigaStep_ctor(step);
		step->pattern   = stream->readUnsignedByte(stream) << 4;
		step->transpose = stream->readByte(stream);
	}

	len = stream->readUnsignedInt(stream) >> 2;
	self->patterns_count = len;
	assert_op(len, <=, D2PLAYER_MAX_PATTERNS);
	//patterns = new Vector.<AmigaRow>(len, true);

	for (i = 0; i < len; ++i) {
		row = &self->patterns[i];
		AmigaRow_ctor(row);
		row->note   = stream->readUnsignedByte(stream);
		row->sample = stream->readUnsignedByte(stream);
		row->effect = stream->readUnsignedByte(stream) - 1;
		row->param  = stream->readUnsignedByte(stream);
	}

	ByteArray_set_position_rel(stream, +254);
	value = stream->readUnsignedShort(stream);
	position = ByteArray_get_position(stream);
	ByteArray_set_position_rel(stream, -256);

	len = 1;
	//offsets = new Vector.<int>(128, true);

	for (i = 0; i < 128; ++i) {
		j = stream->readUnsignedShort(stream);
		if (j != value) offsets[len++] = j;
	}

	self->samples_count = len;
	// + 1 because we access samples[len] member ca 100 lines below
	assert_op(len + 1, <=, D2PLAYER_MAX_SAMPLES);
	//self->samples = new Vector.<D2Sample>(len);

	for (i = 0; i < len; ++i) {
		ByteArray_set_position(stream, position + offsets[i]);
		sample = &self->samples[i];
		D2Sample_ctor(sample);
		
		sample->super.length = stream->readUnsignedShort(stream) << 1;
		sample->super.loop   = stream->readUnsignedShort(stream);
		sample->super.repeat = stream->readUnsignedShort(stream) << 1;

		for (j = 0; j < 15; ++j)
		sample->volumes[j] = stream->readUnsignedByte(stream);
		for (j = 0; j < 15; ++j)
		sample->vibratos[j] = stream->readUnsignedByte(stream);

		sample->pitchBend = stream->readUnsignedShort(stream);
		sample->synth     = stream->readByte(stream);
		sample->index     = stream->readUnsignedByte(stream);

		for (j = 0; j < 48; ++j)
		sample->table[j] = stream->readUnsignedByte(stream);
	}

	len = stream->readUnsignedInt(stream);
	Amiga_store(self->super.amiga, stream, len, -1);

	ByteArray_set_position_rel(stream, +64);
	for (i = 0; i < 8; ++i)
		offsets[i] = stream->readUnsignedInt(stream);

	len = self->samples_count;
	position = ByteArray_get_position(stream);

	for (i = 0; i < len; ++i) {
		sample = &self->samples[i];
		if (sample->synth >= 0) continue;
		ByteArray_set_position(stream, position + offsets[sample->index]);
		sample->super.pointer = Amiga_store(self->super.amiga, stream, sample->super.length, -1);
		sample->super.loopPtr = sample->super.pointer + sample->super.loop;
	}

	ByteArray_set_position(stream, 3018);
	for (i = 0; i < 1024; ++i)
		self->arpeggios[i] = stream->readByte(stream);

	sample = &self->samples[len];
	D2Sample_ctor(sample);
	self->samples_count++;
	
	sample->super.pointer = sample->super.loopPtr = self->super.amiga->vector_count_memory;
	sample->super.length  = sample->super.repeat  = 2;

	len = self->patterns_count;
	j = self->samples_count - 1;

	for (i = 0; i < len; ++i) {
		row = &self->patterns[i];
		if (row->sample > j) row->sample = 0;
	}

	self->super.super.version = 2;

}

