/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.0 - 2012/03/31

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "S2Player.h"
#include "../flod_internal.h"

void S2Player_process(struct S2Player* self);
void S2Player_initialize(struct S2Player* self);
void S2Player_loader(struct S2Player* self, struct ByteArray *stream);

void S2Player_defaults(struct S2Player* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void S2Player_ctor(struct S2Player* self, struct Amiga *amiga) {
	CLASS_CTOR_DEF(S2Player);
	// original constructor code goes here
	AmigaPlayer_ctor(&self->super, amiga);
	//super(amiga);
	
	//arpeggioFx = new Vector.<int>(4, true);
	//voices     = new Vector.<S2Voice>(4, true);
	unsigned i = 0;
	for(; i < 4; i++) {
		S2Voice_ctor(&self->voices[i], i);
		if(i) self->voices[i - 1].next = &self->voices[i];
	}

	self->super.super.loader = S2Player_loader;
	self->super.super.process = S2Player_process;
	self->super.super.initialize = S2Player_initialize;
	
	self->super.super.min_filesize = 86;
}

struct S2Player* S2Player_new(struct Amiga *amiga) {
	CLASS_NEW_BODY(S2Player, amiga);
}

static const unsigned short PERIODS[] = {0,
        5760,5424,5120,4832,4560,4304,4064,3840,3616,3424,3232,3048,
        2880,2712,2560,2416,2280,2152,2032,1920,1808,1712,1616,1524,
        1440,1356,1280,1208,1140,1076,1016, 960, 904, 856, 808, 762,
         720, 678, 640, 604, 570, 538, 508, 480, 453, 428, 404, 381,
         360, 339, 320, 302, 285, 269, 254, 240, 226, 214, 202, 190,
         180, 170, 160, 151, 143, 135, 127, 120, 113, 107, 101,  95,
};

//override
void S2Player_process(struct S2Player* self) {
	struct AmigaChannel *chan = 0;
	struct S2Instrument *instr = 0;
	struct SMRow *row = 0;
	struct S2Sample *sample = 0;
	int value = 0; 
	struct S2Voice *voice = &self->voices[0];
	
	self->arpeggioPos = ++(self->arpeggioPos) & 3;

	if (++(self->super.super.tick) >= self->super.super.speed) {
		self->super.super.tick = 0;

		while (voice) {
			chan = voice->channel;
			voice->enabled = voice->note = 0;

			if (!self->patternPos) {
				unsigned idxx = self->trackPos + voice->index * self->length;
				assert_op(idxx, <, S2PLAYER_MAX_TRACKS);
				voice->step    = &self->tracks[idxx];
				voice->pattern = voice->step->super.pattern;
				voice->speed   = 0;
			}
			if (--voice->speed < 0) {
				
				assert_op(voice->pattern, <, S2PLAYER_MAX_PATTERNS);
				voice->row   = row = &self->patterns[voice->pattern];
				voice->pattern++;
				voice->speed = row->speed;

				if (row->super.note) {
					voice->enabled = 1;
					voice->note    = row->super.note + voice->step->super.transpose;
					AmigaChannel_set_enabled(chan, 0);
				}
			}
			voice->pitchBend = 0;

			if (voice->note) {
				voice->waveCtr      = voice->sustainCtr     = 0;
				voice->arpeggioCtr  = voice->arpeggioPos    = 0;
				voice->vibratoCtr   = voice->vibratoPos     = 0;
				voice->pitchBendCtr = voice->noteSlideSpeed = 0;
				voice->adsrPos = 4;
				voice->volume  = 0;

				if (row->super.sample) {
					voice->instrument = row->super.sample;
					unsigned idxx = voice->instrument + voice->step->soundTranspose;
					assert_op(idxx, <, S2PLAYER_MAX_INSTRUMENTS);
					voice->instr  = &self->instruments[idxx];
					assert_op(voice->instr->wave, <, S2PLAYER_MAX_WAVES);
					idxx = self->waves[voice->instr->wave];
					assert_op(idxx, <, S2PLAYER_MAX_SAMPLES);
					voice->sample = &self->samples[idxx];
				}
				assert_op(voice->instr->arpeggio, <, S2PLAYER_MAX_ARPEGGIOS);
				voice->original = voice->note + self->arpeggios[voice->instr->arpeggio];
				
				assert_op(voice->original, <, ARRAY_SIZE(PERIODS));
				voice->period = PERIODS[voice->original];
				AmigaChannel_set_period(chan, voice->period);

				sample = voice->sample;
				assert_op(sample, !=, 0);
				chan->pointer = sample->super.pointer;
				chan->length  = sample->super.length;
				AmigaChannel_set_enabled(chan, voice->enabled);
				chan->pointer = sample->super.loopPtr;
				chan->length  = sample->super.repeat;
			}
			voice = voice->next;
		}

		if (++(self->patternPos) == self->patternLen) {
			self->patternPos = 0;

			if (++(self->trackPos) == self->length) {
				self->trackPos = 0;
				CoreMixer_set_complete(&self->super.amiga->super, 1);
				//self->amiga->complete = 1;
			}
		}
	}
	voice = &self->voices[0];

	while (voice) {
		if (!voice->sample) {
			voice = voice->next;
			continue;
		}
		chan   = voice->channel;
		sample = voice->sample;

		if (sample->negToggle) {
			voice = voice->next;
			continue;
		}
		sample->negToggle = 1;

		if (sample->negCtr) {
			sample->negCtr = --sample->negCtr & 31;
		} else {
			sample->negCtr = sample->negSpeed;
			if (!sample->negDir) {
				voice = voice->next;
				continue;
			}

			value = sample->negStart + sample->negPos;
			assert_op(value, <, AMIGA_MAX_MEMORY);
			self->super.amiga->memory[value] = ~self->super.amiga->memory[value];
			sample->negPos += sample->negOffset;
			value = sample->negLen - 1;

			if (sample->negPos < 0) {
				if (sample->negDir == 2) {
					sample->negPos = value;
				} else {
					sample->negOffset = -sample->negOffset;
					sample->negPos += sample->negOffset;
				}
			} else if (value < sample->negPos) {
				if (sample->negDir == 1) {
					sample->negPos = 0;
				} else {
					sample->negOffset = -sample->negOffset;
					sample->negPos += sample->negOffset;
				}
			}
		}
		voice = voice->next;
	}
	voice = &self->voices[0];

	while (voice) {
		if (!voice->sample) {
			voice = voice->next;
			continue;
		}
		voice->sample->negToggle = 0;
		voice = voice->next;
	}
	voice = &self->voices[0];

	while (voice) {
		chan  = voice->channel;
		instr = voice->instr;

		switch (voice->adsrPos) {
			case 0:
				break;
			case 4:   //attack
				voice->volume += instr->attackSpeed;
				if (instr->attackMax <= voice->volume) {
					voice->volume = instr->attackMax;
					voice->adsrPos--;
				}
				break;
			case 3:   //decay
				if (!instr->decaySpeed) {
					voice->adsrPos--;
				} else {
					voice->volume -= instr->decaySpeed;
					if (instr->decayMin >= voice->volume) {
						voice->volume = instr->decayMin;
						voice->adsrPos--;
					}
				}
				break;
			case 2:   //sustain
				if (voice->sustainCtr == instr->sustain) voice->adsrPos--;
				else voice->sustainCtr++;
				break;
			case 1:   //release
				voice->volume -= instr->releaseSpeed;
				if (instr->releaseMin >= voice->volume) {
					voice->volume = instr->releaseMin;
					voice->adsrPos--;
				}
				break;
			default:
				break;
		}
		
		AmigaChannel_set_volume(chan, voice->volume >> 2);

		if (instr->waveLen) {
			if (voice->waveCtr == instr->waveDelay) {
				voice->waveCtr = instr->waveDelay - instr->waveSpeed;
				if (voice->wavePos == instr->waveLen) voice->wavePos = 0;
				else voice->wavePos++;
				
				unsigned idxx = instr->wave + voice->wavePos;
				assert_op(idxx, <, S2PLAYER_MAX_WAVES);
				idxx = self->waves[idxx];
				assert_op(idxx, <, S2PLAYER_MAX_WAVES);
				voice->sample = sample = &self->samples[idxx];
				chan->pointer = sample->super.pointer;
				chan->length  = sample->super.length;
			} else
				voice->waveCtr++;
		}

		if (instr->arpeggioLen) {
			if (voice->arpeggioCtr == instr->arpeggioDelay) {
				voice->arpeggioCtr = instr->arpeggioDelay - instr->arpeggioSpeed;
				if (voice->arpeggioPos == instr->arpeggioLen) voice->arpeggioPos = 0;
				else voice->arpeggioPos++;

				unsigned idxx = instr->arpeggio + voice->arpeggioPos;
				assert_op(idxx, <, S2PLAYER_MAX_ARPEGGIOS);
				value = voice->original + self->arpeggios[idxx];
				assert_op(value, <, ARRAY_SIZE(PERIODS));
				voice->period = PERIODS[value];
			} else
				voice->arpeggioCtr++;
		}
		row = voice->row;

		if (self->super.super.tick) {
			switch (row->super.effect) {
				case 0:
					break;
				case 0x70:  //arpeggio
					self->arpeggioFx[0] = row->super.param >> 4;
					self->arpeggioFx[2] = row->super.param & 15;
					assert_op(self->arpeggioPos, <, S2PLAYER_MAX_ARPEGGIOFX);
					value = voice->original + self->arpeggioFx[self->arpeggioPos];
					assert_op(value, <, ARRAY_SIZE(PERIODS));
					voice->period = PERIODS[value];
					break;
				case 0x71:  //pitch up
					voice->pitchBend = -row->super.param;
					break;
				case 0x72:  //pitch down
					voice->pitchBend = row->super.param;
					break;
				case 0x73:  //volume up
					if (voice->adsrPos != 0) break;
					if (voice->instrument != 0) voice->volume = instr->attackMax;
					voice->volume += row->super.param << 2;
					if (voice->volume >= 256) voice->volume = -1;
					break;
				case 0x74:  //volume down
					if (voice->adsrPos != 0) break;
					if (voice->instrument != 0) voice->volume = instr->attackMax;
					voice->volume -= row->super.param << 2;
					if (voice->volume < 0) voice->volume = 0;
					break;
				default:
					break;
			}
		}

		switch (row->super.effect) {
			default:
			case 0:
				break;
			case 0x75:  //set adsr attack
				instr->attackMax   = row->super.param;
				instr->attackSpeed = row->super.param;
				break;
			case 0x76:  //set pattern length
				self->patternLen = row->super.param;
				break;
			case 0x7c:  //set volume
				AmigaChannel_set_volume(chan, row->super.param);
				voice->volume = row->super.param << 2;
				if (voice->volume >= 255) voice->volume = 255;
				break;
			case 0x7f:  //set speed
				value = row->super.param & 15;
				if (value) self->super.super.speed = value;
				break;
		}

		if (instr->vibratoLen) {
			if (voice->vibratoCtr == instr->vibratoDelay) {
				voice->vibratoCtr = instr->vibratoDelay - instr->vibratoSpeed;
				if (voice->vibratoPos == instr->vibratoLen) voice->vibratoPos = 0;
				else voice->vibratoPos++;

				unsigned idxx = instr->vibrato + voice->vibratoPos;
				assert_op(idxx, <, S2PLAYER_MAX_VIBRATOS);
				voice->period += self->vibratos[idxx];
			} else
				voice->vibratoCtr++;
		}

		if (instr->pitchBend) {
			if (voice->pitchBendCtr == instr->pitchBendDelay) {
				voice->pitchBend += instr->pitchBend;
			} else
				voice->pitchBendCtr++;
		}

		if (row->super.param) {
			if (row->super.effect && row->super.effect < 0x70) {
				unsigned idxx = row->super.effect + voice->step->super.transpose;
				assert_op(idxx, <, ARRAY_SIZE(PERIODS));
				voice->noteSlideTo = PERIODS[idxx];
				value = row->super.param;
				if ((voice->noteSlideTo - voice->period) < 0) value = -value;
					voice->noteSlideSpeed = value;
			}
		}

		if (voice->noteSlideTo && voice->noteSlideSpeed) {
			voice->period += voice->noteSlideSpeed;

			if ((voice->noteSlideSpeed < 0 && voice->period < voice->noteSlideTo) ||
				(voice->noteSlideSpeed > 0 && voice->period > voice->noteSlideTo)) {
				voice->noteSlideSpeed = 0;
				voice->period = voice->noteSlideTo;
			}
		}

		voice->period += voice->pitchBend;

		if (voice->period < 95) voice->period = 95;
		else if (voice->period > 5760) voice->period = 5760;

		AmigaChannel_set_period(chan, voice->period);
		voice = voice->next;
	}
}

//override
void S2Player_initialize(struct S2Player* self) {
	struct S2Voice *voice = &self->voices[0];
	CorePlayer_initialize(&self->super.super);
	//self->super->initialize();

	self->super.super.speed      = self->speedDef;
	self->super.super.tick       = self->speedDef;
	self->trackPos   = 0;
	self->patternPos = 0;
	self->patternLen = 64;

	while (voice) {
		S2Voice_initialize(voice);
		voice->channel = &self->super.amiga->channels[voice->index];
		voice->instr   = &self->instruments[0];

		self->arpeggioFx[voice->index] = 0;
		voice = voice->next;
	}
}

//override
void S2Player_loader(struct S2Player* self, struct ByteArray *stream) {
	int higher = 0; 
	int i = 0;
	char id[32];
	struct S2Instrument *instr = 0;
	int j = 0;
	int len = 0; 
	//pointers:Vector.<int>;
#define STACK_MAX_POINTERS 258
	int pointers[STACK_MAX_POINTERS]; // FIXME
	int position = 0; 
	int pos = 0; 
	struct SMRow *row = 0;
	struct S2Step *step = 0;
	struct S2Sample *sample = 0;
	int sampleData = 0;
	int value = 0;
	unsigned samples_count;
	unsigned tracks_count;
	
	ByteArray_set_position(stream, 58);
	stream->readMultiByte(stream, id, 28);
	if (!is_str(id, "SIDMON II - THE MIDI VERSION")) return;

	ByteArray_set_position(stream, 2);
	self->length   = stream->readUnsignedByte(stream);
	self->speedDef = stream->readUnsignedByte(stream);
	
	samples_count = stream->readUnsignedShort(stream) >> 6;
	assert_op(samples_count, <=, S2PLAYER_MAX_SAMPLES);
	
	//self->samples  = new Vector.<S2Sample>(stream->readUnsignedShort(stream) >> 6, true);

	ByteArray_set_position(stream, 14);
	len = stream->readUnsignedInt(stream);
	
	tracks_count = len;
	assert_op(tracks_count, <=, S2PLAYER_MAX_TRACKS);
	
	//self->tracks = new Vector.<S2Step>(len, true);
	ByteArray_set_position(stream, 90);

	for (; i < len; ++i) {
		//step = new S2Step();
		step = &self->tracks[i];
		S2Step_ctor(step);
		
		step->super.pattern = stream->readUnsignedByte(stream);
		if (step->super.pattern > higher) higher = step->super.pattern;
		//self->tracks[i] = step;
	}

	for (i = 0; i < len; ++i) {
		step = &self->tracks[i];
		step->super.transpose = stream->readByte(stream);
	}

	for (i = 0; i < len; ++i) {
		step = &self->tracks[i];
		step->soundTranspose = stream->readByte(stream);
	}

	position = ByteArray_get_position(stream);
	ByteArray_set_position(stream, 26);
	len = stream->readUnsignedInt(stream) >> 5;
	//self->instruments = new Vector.<S2Instrument>(++len, true);
	++len;
	assert_op(len, <=, S2PLAYER_MAX_INSTRUMENTS);
	ByteArray_set_position(stream, position);

	//self->instruments[0] = new S2Instrument();
	S2Instrument_ctor(&self->instruments[0]);

	for(i = 1; i < len; i++) {
	//for (i = 0; ++i < len;) {
		//instr = new S2Instrument();
		instr = &self->instruments[i];
		S2Instrument_ctor(instr);
		
		instr->wave           = stream->readUnsignedByte(stream) << 4;
		instr->waveLen        = stream->readUnsignedByte(stream);
		instr->waveSpeed      = stream->readUnsignedByte(stream);
		instr->waveDelay      = stream->readUnsignedByte(stream);
		instr->arpeggio       = stream->readUnsignedByte(stream) << 4;
		instr->arpeggioLen    = stream->readUnsignedByte(stream);
		instr->arpeggioSpeed  = stream->readUnsignedByte(stream);
		instr->arpeggioDelay  = stream->readUnsignedByte(stream);
		instr->vibrato        = stream->readUnsignedByte(stream) << 4;
		instr->vibratoLen     = stream->readUnsignedByte(stream);
		instr->vibratoSpeed   = stream->readUnsignedByte(stream);
		instr->vibratoDelay   = stream->readUnsignedByte(stream);
		instr->pitchBend      = stream->readByte(stream);
		instr->pitchBendDelay = stream->readUnsignedByte(stream);
		stream->readByte(stream);
		stream->readByte(stream);
		instr->attackMax      = stream->readUnsignedByte(stream);
		instr->attackSpeed    = stream->readUnsignedByte(stream);
		instr->decayMin       = stream->readUnsignedByte(stream);
		instr->decaySpeed     = stream->readUnsignedByte(stream);
		instr->sustain        = stream->readUnsignedByte(stream);
		instr->releaseMin     = stream->readUnsignedByte(stream);
		instr->releaseSpeed   = stream->readUnsignedByte(stream);
		//self->instruments[i] = instr;
		ByteArray_set_position_rel(stream, + 9);
	}

	position = ByteArray_get_position(stream);
	ByteArray_set_position(stream, 30);
	len = stream->readUnsignedInt(stream);
	//self->waves = new Vector.<int>(len, true);
	assert_op(len, <=, S2PLAYER_MAX_WAVES);
	ByteArray_set_position(stream, position);

	for (i = 0; i < len; ++i) self->waves[i] = stream->readUnsignedByte(stream);

	position = ByteArray_get_position(stream);
	ByteArray_set_position(stream, 34);
	len = stream->readUnsignedInt(stream);
	//self->arpeggios = new Vector.<int>(len, true);
	assert_op(len, <=, S2PLAYER_MAX_ARPEGGIOS);
	ByteArray_set_position(stream, position);

	for (i = 0; i < len; ++i) self->arpeggios[i] = stream->readByte(stream);

	position = ByteArray_get_position(stream);
	ByteArray_set_position(stream, 38);
	len = stream->readUnsignedInt(stream);
	//vibratos = new Vector.<int>(len, true);
	assert_op(len, <=, S2PLAYER_MAX_VIBRATOS);
	ByteArray_set_position(stream, position);

	for (i = 0; i < len; ++i) self->vibratos[i] = stream->readByte(stream);

	len = samples_count; //self->samples->length;
	position = 0;
	
	assert_op(len, <=, S2PLAYER_MAX_SAMPLES);

	for (i = 0; i < len; ++i) {
		//sample = new S2Sample();
		sample = &self->samples[i];
		S2Sample_ctor(sample);
		
		stream->readUnsignedInt(stream);
		sample->super.length    = stream->readUnsignedShort(stream) << 1;
		sample->super.loop      = stream->readUnsignedShort(stream) << 1;
		sample->super.repeat    = stream->readUnsignedShort(stream) << 1;
		sample->negStart  = position + (stream->readUnsignedShort(stream) << 1);
		sample->negLen    = stream->readUnsignedShort(stream) << 1;
		sample->negSpeed  = stream->readUnsignedShort(stream);
		sample->negDir    = stream->readUnsignedShort(stream);
		sample->negOffset = stream->readShort(stream);
		sample->negPos    = stream->readUnsignedInt(stream);
		sample->negCtr    = stream->readUnsignedShort(stream);
		ByteArray_set_position_rel(stream, + 6);
		stream->readMultiByte(stream, sample->sample_name, 32);
		sample->super.name = sample->sample_name;

		sample->super.pointer = position;
		sample->super.loopPtr = position + sample->super.loop;
		position += sample->super.length;
		//self->samples[i] = sample;
	}

	sampleData = position;
	len = ++higher;
	//pointers = new Vector.<int>(++higher, true);
	++higher;
	assert_op(higher + 1, <=, STACK_MAX_POINTERS);
	for (i = 0; i < len; ++i) pointers[i] = stream->readUnsignedShort(stream);

	position = ByteArray_get_position(stream);
	ByteArray_set_position(stream, 50);
	len = stream->readUnsignedInt(stream);
	//self->patterns = new Vector.<SMRow>();
	
	ByteArray_set_position(stream, position);
	j = 1;
	
	unsigned pattern_count = pos; // not finished yet
	assert_op(len + pos, <, S2PLAYER_MAX_PATTERNS);

	for (i = 0; i < len; ++i) {
		//row   = new SMRow();
		row = &self->patterns[pos];
		SMRow_ctor(row);
		
		value = stream->readByte(stream);

		if (!value) {
			row->super.effect = stream->readByte(stream);
			row->super.param  = stream->readUnsignedByte(stream);
			i += 2;
		} else if (value < 0) {
			row->speed = ~value;
		} else if (value < 112) {
			row->super.note = value;
			value = stream->readByte(stream);
			i++;

			if (value < 0) {
				row->speed = ~value;
			} else if (value < 112) {
				row->super.sample = value;
				value = stream->readByte(stream);
				i++;

				if (value < 0) {
					row->speed = ~value;
				} else {
					row->super.effect = value;
					row->super.param  = stream->readUnsignedByte(stream);
					i++;
				}
			} else {
				row->super.effect = value;
				row->super.param  = stream->readUnsignedByte(stream);
				i++;
			}
		} else {
			row->super.effect = value;
			row->super.param  = stream->readUnsignedByte(stream);
			i++;
		}

		//self->patterns[pos++] = row;
		pos++;
		assert_op(j + 1, <, STACK_MAX_POINTERS);
		if ((position + pointers[j]) == ByteArray_get_position(stream)) pointers[j++] = pos;
	}
	pattern_count = pos - pattern_count;
	//pointers[j] = self->patterns->length;
	pointers[j] = pattern_count;
	//self->patterns->fixed = true;
	 
	if ((ByteArray_get_position(stream) & 1) != 0) ByteArray_set_position_rel(stream, +1);
	Amiga_store(self->super.amiga, stream, sampleData, -1);

	//len = self->tracks.length;
	len = tracks_count;

	for (i = 0; i < len; ++i) {
		step = &self->tracks[i];
		assert_op(step->super.pattern, <, STACK_MAX_POINTERS);
		step->super.pattern = pointers[step->super.pattern];
	}

	self->length++;
	self->super.super.version = 2;
}

