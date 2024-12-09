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

#include "D1Player.h"
#include "../flod_internal.h"

static const unsigned short PERIODS[] = {
	   0,6848,6464,6096,5760,5424,5120,4832,4560,4304,4064,3840,
	3616,3424,3232,3048,2880,2712,2560,2416,2280,2152,2032,1920,
	1808,1712,1616,1524,1440,1356,1280,1208,1140,1076, 960, 904,
	 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 452,
	 428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
	 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
	 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113,
};

void D1Player_loader(struct D1Player* self, struct ByteArray *stream);
void D1Player_initialize(struct D1Player* self);
void D1Player_process(struct D1Player* self);

void D1Player_defaults(struct D1Player* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void D1Player_ctor(struct D1Player* self, struct Amiga *amiga) {
	CLASS_CTOR_DEF(D1Player);
	// original constructor code goes here
	AmigaPlayer_ctor(&self->super, amiga);

	unsigned i;
	for (i = 0; i < D1PLAYER_MAX_VOICES; i++) {
		D1Voice_ctor(&self->voices[i], i);
		if(i) self->voices[i - 1].next = &self->voices[i];
	}
	
	//vtable
	self->super.super.loader = D1Player_loader;
	self->super.super.process = D1Player_process;
	self->super.super.initialize = D1Player_initialize;
	self->super.super.min_filesize = 64;
}

struct D1Player* D1Player_new(struct Amiga *amiga) {
	CLASS_NEW_BODY(D1Player, amiga);
}


//override
void D1Player_process(struct D1Player* self) {
	int adsr = 0; 
	struct AmigaChannel *chan = 0;
	int loop = 0; 
	struct AmigaRow *row = 0;
	struct D1Sample *sample = 0;
	int value = 0; 
	struct D1Voice *voice = &self->voices[0];

	while (voice) {
		chan = voice->channel;

		if (--voice->speed == 0) {
			voice->speed = self->super.super.speed;

			if (voice->patternPos == 0) {
				assert_op(voice->index, <, D1PLAYER_MAX_VOICES);
				unsigned idxx = self->pointers[voice->index] + voice->trackPos;
				assert_op(idxx, <, D1PLAYER_MAX_TRACKS);
				voice->step = &self->tracks[idxx];

				if (voice->step->pattern < 0) {
					voice->trackPos = voice->step->transpose;
					assert_op(voice->index, <, D1PLAYER_MAX_VOICES);
					idxx = self->pointers[voice->index] + voice->trackPos;
					assert_op(idxx, <, D1PLAYER_MAX_TRACKS);
					voice->step = &self->tracks[idxx];
				}
				voice->trackPos++;
			}

			unsigned idxx = voice->step->pattern + voice->patternPos;
			assert_op(idxx, <, D1PLAYER_MAX_PATTERNS);
			row = &self->patterns[idxx];
			if (row->effect) voice->row = row;

			if (row->note) {
				AmigaChannel_set_enabled(chan, 0);
				voice->row = row;
				voice->note = row->note + voice->step->transpose;
				voice->arpeggioPos = voice->pitchBend = voice->status = 0;

				assert_op(row->sample, <, D1PLAYER_MAX_SAMPLES);
				sample = voice->sample = &self->samples[row->sample];
				if (!sample->synth) chan->pointer = sample->super.pointer;
				chan->length = sample->super.length;

				voice->tableCtr   = voice->tablePos = 0;
				voice->vibratoCtr = sample->vibratoWait;
				voice->vibratoPos = sample->vibratoLen;
				voice->vibratoDir = sample->vibratoLen << 1;
				voice->volume     = voice->attackCtr = voice->decayCtr = voice->releaseCtr = 0;
				voice->sustain    = sample->sustain;
			}
			if (++voice->patternPos == 16) voice->patternPos = 0;
		}
		sample = voice->sample;

		if (sample->synth) {
			if (voice->tableCtr == 0) {
				voice->tableCtr = sample->tableDelay;

				do {
					loop = 1;
					if (voice->tablePos >= 48) voice->tablePos = 0;
					value = sample->table[voice->tablePos];
					voice->tablePos++;

					if (value >= 0) {
						chan->pointer = sample->super.pointer + (value << 5);
						loop = 0;
					} else if (value != -1) {
						sample->tableDelay = value & 127;
					} else {
						voice->tablePos = sample->table[voice->tablePos];
					}
				} while (loop);
			} else
			voice->tableCtr--;
		}

		if (sample->portamento) {
			value = PERIODS[voice->note] + voice->pitchBend;

			if (voice->period != 0) {
				if (voice->period < value) {
					voice->period += sample->portamento;
					if (voice->period > value) voice->period = value;
				} else {
					voice->period -= sample->portamento;
					if (voice->period < value) voice->period = value;
				}
			} else
			voice->period = value;
		}

		if (voice->vibratoCtr == 0) {
			voice->vibratoPeriod = voice->vibratoPos * sample->vibratoStep;

			if ((voice->status & 1) == 0) {
				voice->vibratoPos++;
				if (voice->vibratoPos == voice->vibratoDir) voice->status ^= 1;
			} else {
				voice->vibratoPos--;
				if (voice->vibratoPos == 0) voice->status ^= 1;
			}
		} else {
			voice->vibratoCtr--;
		}

		if (sample->pitchBend < 0) voice->pitchBend += sample->pitchBend;
		else voice->pitchBend -= sample->pitchBend;

		if (voice->row) {
			row = voice->row;

			switch (row->effect) {
				default:
				case 0:
					break;
				case 1:
					value = row->param & 15;
					if (value) self->super.super.speed = value;
					break;
				case 2:
					voice->pitchBend -= row->param;
					break;
				case 3:
					voice->pitchBend += row->param;
					break;
				case 4:
					self->super.amiga->filter->active = row->param;
					break;
				case 5:
					sample->vibratoWait = row->param;
					break;
				case 6:
					sample->vibratoStep = row->param;
					//FIXME forgotten break ?
				case 7:
					sample->vibratoLen = row->param;
					break;
				case 8:
					sample->pitchBend = row->param;
					break;
				case 9:
					sample->portamento = row->param;
					break;
				case 10:
					value = row->param;
					if (value > 64) value = 64;
					sample->super.volume = 64;
					break;
				case 11:
					sample->arpeggio[0] = row->param;
					break;
				case 12:
					sample->arpeggio[1] = row->param;
					break;
				case 13:
					sample->arpeggio[2] = row->param;
					break;
				case 14:
					sample->arpeggio[3] = row->param;
					break;
				case 15:
					sample->arpeggio[4] = row->param;
					break;
				case 16:
					sample->arpeggio[5] = row->param;
					break;
				case 17:
					sample->arpeggio[6] = row->param;
					break;
				case 18:
					sample->arpeggio[7] = row->param;
					break;
				case 19:
					sample->arpeggio[0] = sample->arpeggio[4] = row->param;
					break;
				case 20:
					sample->arpeggio[1] = sample->arpeggio[5] = row->param;
					break;
				case 21:
					sample->arpeggio[2] = sample->arpeggio[6] = row->param;
					break;
				case 22:
					sample->arpeggio[3] = sample->arpeggio[7] = row->param;
					break;
				case 23:
					value = row->param;
					if (value > 64) value = 64;
					sample->attackStep = value;
					break;
				case 24:
					sample->attackDelay = row->param;
					break;
				case 25:
					value = row->param;
					if (value > 64) value = 64;
					sample->decayStep = value;
					break;
				case 26:
					sample->decayDelay = row->param;
					break;
				case 27:
					sample->sustain = row->param & (sample->sustain & 255);
					break;
				case 28:
					sample->sustain = (sample->sustain & 65280) + row->param;
					break;
				case 29:
					value = row->param;
					if (value > 64) value = 64;
					sample->releaseStep = value;
					break;
				case 30:
					sample->releaseDelay = row->param;
					break;
			}
		}

		if (sample->portamento)
			value = voice->period;
		else {
			assert_op(voice->arpeggioPos, <, D1SAMPLE_MAX_ARPEGGIO);
			unsigned idxx = voice->note + sample->arpeggio[voice->arpeggioPos];
			assert_op(idxx, <, ARRAY_SIZE(PERIODS));
			value = PERIODS[idxx];
			voice->arpeggioPos = ++voice->arpeggioPos & 7;
			value -= (sample->vibratoLen * sample->vibratoStep);
			value += voice->pitchBend;
			voice->period = 0;
		}

		AmigaChannel_set_period(chan, value + voice->vibratoPeriod);
		adsr  = voice->status & 14;
		value = voice->volume;

		if (adsr == 0) {
			if (voice->attackCtr == 0) {
				voice->attackCtr = sample->attackDelay;
				value += sample->attackStep;

				if (value >= 64) {
				adsr |= 2;
				voice->status |= 2;
				value = 64;
				}
			} else {
				voice->attackCtr--;
			}
		}

		if (adsr == 2) {
			if (voice->decayCtr == 0) {
				voice->decayCtr = sample->decayDelay;
				value -= sample->decayStep;

				if (value <= sample->super.volume) {
				adsr |= 6;
				voice->status |= 6;
				value = sample->super.volume;
				}
			} else {
				voice->decayCtr--;
			}
		}

		if (adsr == 6) {
			if (voice->sustain == 0) {
				adsr |= 14;
				voice->status |= 14;
			} else {
				voice->sustain--;
			}
		}

		if (adsr == 14) {
			if (voice->releaseCtr == 0) {
				voice->releaseCtr = sample->releaseDelay;
				value -= sample->releaseStep;

				if (value < 0) {
					voice->status &= 9;
					value = 0;
				}
			} else {
				voice->releaseCtr--;
			}
		}

		AmigaChannel_set_volume(chan, (voice->volume = value));
		AmigaChannel_set_enabled(chan, 1);

		if (!sample->synth) {
			if (sample->super.loop) {
				chan->pointer = sample->super.loopPtr;
				chan->length  = sample->super.repeat;
			} else {
				chan->pointer = self->super.amiga->loopPtr;
				chan->length  = 2;
			}
		}
		voice = voice->next;
	}
}

//override
void D1Player_initialize(struct D1Player* self) {
	struct D1Voice *voice = &self->voices[0];
	
	CorePlayer_initialize(&self->super.super);
	
	self->super.super.speed = 6;

	while (voice) {
		D1Voice_initialize(voice);
		voice->channel = &self->super.amiga->channels[voice->index];
		voice->sample  = &self->samples[20];
		voice = voice->next;
	}
}

//fixed
#define D1PLAYER_MAX_STACKDATA 25
//override
void D1Player_loader(struct D1Player* self, struct ByteArray *stream) {
	//data:Vector.<int>, 
	int data[D1PLAYER_MAX_STACKDATA];
	int i = 0;
	char id[4];
	int index = 0;
	int j = 0;
	int len = 0;
	int position = 0; 
	struct AmigaRow *row = 0;
	struct D1Sample *sample = 0;
	struct AmigaStep *step = 0;
	int value = 0;
	
	stream->readMultiByte(stream, id, 4);
	if (!is_str(id, "ALL ")) return;

	position = 104;
	//data = new Vector.<int>(25 ,true);
	for (i = 0; i < 25; ++i) data[i] = stream->readUnsignedInt(stream);

	//pointers = new Vector.<int>(4, true);
	for (i = 1; i < 4; ++i)
		self->pointers[i] = self->pointers[j] + (data[j++] >> 1) - 1;

	len = self->pointers[3] + (data[3] >> 1) - 1;
	//tracks = new Vector.<AmigaStep>(len, true);
	assert_op(len, <=, D1PLAYER_MAX_TRACKS);
	
	index = position + data[1] - 2;
	ByteArray_set_position(stream, position);
	j = 1;

	for (i = 0; i < len; ++i) {
		//step  = new AmigaStep();
		step = &self->tracks[i];
		AmigaStep_ctor(step);
		
		value = stream->readUnsignedShort(stream);

		if (value == 0xffff || ByteArray_get_position(stream) == index) {
			step->pattern   = -1;
			step->transpose = stream->readUnsignedShort(stream);
			index += data[j++];
		} else {
			ByteArray_set_position_rel(stream, -1);
			step->pattern   = ((value >> 2) & 0x3fc0) >> 2;
			step->transpose = stream->readByte(stream);
		}
		//self->tracks[i] = step;
	}

	len = data[4] >> 2;
	//self->patterns = new Vector.<AmigaRow>(len, true);
	assert_op(len, <=, D1PLAYER_MAX_PATTERNS);

	for (i = 0; i < len; ++i) {
		//row = new AmigaRow();
		row = &self->patterns[i];
		AmigaRow_ctor(row);
		
		row->sample = stream->readUnsignedByte(stream);
		row->note   = stream->readUnsignedByte(stream);
		row->effect = stream->readUnsignedByte(stream) & 31;
		row->param  = stream->readUnsignedByte(stream);
		//self->patterns[i] = row;
	}

	index = 5;

	for (i = 0; i < 20; ++i) {
		//self->samples[i] = null;

		if (data[index] != 0) {
			//sample = new D1Sample();
			sample = &self->samples[i];
			D1Sample_ctor(sample);
			
			sample->attackStep   = stream->readUnsignedByte(stream);
			sample->attackDelay  = stream->readUnsignedByte(stream);
			sample->decayStep    = stream->readUnsignedByte(stream);
			sample->decayDelay   = stream->readUnsignedByte(stream);
			sample->sustain      = stream->readUnsignedShort(stream);
			sample->releaseStep  = stream->readUnsignedByte(stream);
			sample->releaseDelay = stream->readUnsignedByte(stream);
			sample->super.volume = stream->readUnsignedByte(stream);
			sample->vibratoWait  = stream->readUnsignedByte(stream);
			sample->vibratoStep  = stream->readUnsignedByte(stream);
			sample->vibratoLen   = stream->readUnsignedByte(stream);
			sample->pitchBend    = stream->readByte(stream);
			sample->portamento   = stream->readUnsignedByte(stream);
			sample->synth        = stream->readUnsignedByte(stream);
			sample->tableDelay   = stream->readUnsignedByte(stream);

			for (j = 0; j < 8; ++j)
				sample->arpeggio[j] = stream->readByte(stream);

			sample->super.length = stream->readUnsignedShort(stream);
			sample->super.loop   = stream->readUnsignedShort(stream);
			sample->super.repeat = stream->readUnsignedShort(stream) << 1;
			sample->synth  = sample->synth ? 0 : 1;

			if (sample->synth) {
				for (j = 0; j < 48; ++j)
					sample->table[j] = stream->readByte(stream);

				len = data[index] - 78;
			} else {
				len = sample->super.length;
			}

			sample->super.pointer = Amiga_store(self->super.amiga, stream, len, -1);
			sample->super.loopPtr = sample->super.pointer + sample->super.loop;
			//self->samples[i] = sample;
		}
		index++;
	}

	sample = &self->samples[20];//new D1Sample();
	sample->super.pointer = sample->super.loopPtr = self->super.amiga->vector_count_memory;
	sample->super.length  = sample->super.repeat  = 2;
	//self->samples[20] = sample;
	self->super.super.version = 1;
}
