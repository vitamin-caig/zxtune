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

#include "S1Player.h"
#include "S1Player_const.h"
#include "../flod_internal.h"

void S1Player_loader(struct S1Player* self, struct ByteArray *stream);
void S1Player_process(struct S1Player* self);
void S1Player_initialize(struct S1Player* self);

void S1Player_defaults(struct S1Player* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void S1Player_ctor(struct S1Player* self, struct Amiga *amiga) {
	CLASS_CTOR_DEF(S1Player);
	// original constructor code goes here
	AmigaPlayer_ctor(&self->super, amiga);
	//super(amiga);


	//tracksPtr = new Vector.<int>(4, true);
	//voices    = new Vector.<S1Voice>(4, true);
	
	unsigned i = 0;
	for(; i < S1PLAYER_MAX_VOICES; i++) {
		S1Voice_ctor(&self->voices[i], i);
		if(i) self->voices[i - 1].next = &self->voices[i];
	}

	//vtable
	self->super.super.loader = S1Player_loader;
	self->super.super.process = S1Player_process;
	self->super.super.initialize = S1Player_initialize;
	
	self->super.super.min_filesize = 5220;
	
}

struct S1Player* S1Player_new(struct Amiga *amiga) {
	CLASS_NEW_BODY(S1Player, amiga);
}

//override
void S1Player_process(struct S1Player* self) {
	struct AmigaChannel *chan = 0;
	int dst = 0;
	int i = 0;
	int index = 0; 
	//memory:Vector.<int> = amiga->memory;
	signed char *memory = self->super.amiga->memory;
	struct SMRow *row = 0;
	struct S1Sample *sample = 0;
	int src1 = 0; 
	int src2 = 0; 
	struct AmigaStep *step = 0;
	int value = 0; 
	struct S1Voice *voice = &self->voices[0];

	while (voice) {
		chan = voice->channel;
		self->audPtr = -1;
		self->audLen = self->audPer = self->audVol = 0;

		if (self->super.super.tick == 0) {
			if (self->patternEnd) {
				if (self->trackEnd) voice->step = self->tracksPtr[voice->index];
				else voice->step++;

				assert_op(voice->step, <, S1PLAYER_MAX_TRACKS);
				step = &self->tracks[voice->step];
				assert_op(step->pattern, <, S1PLAYER_MAX_PATTERNSPTR);
				voice->row = self->patternsPtr[step->pattern];
				if (self->doReset) voice->noteTimer = 0;
			}

			if (voice->noteTimer == 0) {
				assert_op(voice->row, <, S1PLAYER_MAX_PATTERNS);
				row = &self->patterns[voice->row];

				if (row->super.sample == 0) {
					if (row->super.note) {
						voice->noteTimer = row->speed;

						if (voice->waitCtr) {
							assert_op(voice->sample, <, S1PLAYER_MAX_SAMPLES);
							sample = &self->samples[voice->sample];
							self->audPtr = sample->super.pointer;
							self->audLen = sample->super.length;
							voice->samplePtr = sample->super.loopPtr;
							voice->sampleLen = sample->super.repeat;
							voice->waitCtr = 1;
							AmigaChannel_set_enabled(chan, 0);
						}
					}
				} else {
					assert_op(row->super.sample, <, S1PLAYER_MAX_SAMPLES);
					sample = &self->samples[row->super.sample];
					if (voice->waitCtr) {
						voice->waitCtr = 0;
						AmigaChannel_set_enabled(chan, 0);
					}

					if (sample->waveform > 15) {
						self->audPtr = sample->super.pointer;
						self->audLen = sample->super.length;
						voice->samplePtr = sample->super.loopPtr;
						voice->sampleLen = sample->super.repeat;
						voice->waitCtr = 1;
					} else {
						voice->wavePos = 0;
						voice->waveList = sample->waveform;
						index = voice->waveList << 4;
						assert_op(index + 1, <, S1PLAYER_MAX_WAVELISTS);
						self->audPtr = self->waveLists[index] << 5;
						self->audLen = 32;
						voice->waveTimer = self->waveLists[++index];
					}

					voice->noteTimer   = row->speed;
					voice->sample      = row->super.sample;
					voice->envelopeCtr = voice->pitchCtr = voice->pitchFallCtr = 0;
				}

				if (row->super.note) {
					voice->noteTimer = row->speed;

					if (row->super.note != 0xff) {
						assert_op(voice->sample, <, S1PLAYER_MAX_SAMPLES);
						sample = &self->samples[voice->sample];
						assert_op(voice->step, <, S1PLAYER_MAX_TRACKS);
						step = &self->tracks[voice->step];

						voice->note = row->super.note + step->transpose;
						unsigned idxx = 1 + sample->finetune + voice->note;
						assert_op(idxx, <, ARRAY_SIZE(PERIODS));
						
						voice->period = self->audPer = PERIODS[idxx];
						voice->phaseSpeed = sample->phaseSpeed;

						voice->bendSpeed   = voice->volume = 0;
						voice->envelopeCtr = voice->pitchCtr = voice->pitchFallCtr = 0;

						switch (row->super.effect) {
							case 0:
								if (row->super.param == 0) break;
								sample->attackSpeed = row->super.param;
								sample->attackMax   = row->super.param;
								voice->waveTimer    = 0;
								break;
							case 2:
								self->super.super.speed = row->super.param;
								voice->waveTimer = 0;
								break;
							case 3:
								self->patternLen = row->super.param;
								voice->waveTimer = 0;
								break;
							default:
								voice->bendTo = row->super.effect + step->transpose;
								voice->bendSpeed = row->super.param;
								break;
						}
					}
				}
				voice->row++;
			} else {
				voice->noteTimer--;
			}
		}
		
		assert_op(voice->sample, <, S1PLAYER_MAX_SAMPLES);
		sample = &self->samples[voice->sample];
		self->audVol = voice->volume;

		switch (voice->envelopeCtr) {
			default:
			case 8:
				break;
			case 0:   //attack
				self->audVol += sample->attackSpeed;

				if (self->audVol > sample->attackMax) {
					self->audVol = sample->attackMax;
					voice->envelopeCtr += 2;
				}
				break;
			case 2:   //decay
				self->audVol -= sample->decaySpeed;

				if (self->audVol <= sample->decayMin || self->audVol < -256) {
					self->audVol = sample->decayMin;
					voice->envelopeCtr += 2;
					voice->sustainCtr = sample->sustain;
				}
				break;
			case 4:   //sustain
				voice->sustainCtr--;
				if (voice->sustainCtr == 0 || voice->sustainCtr == -256) voice->envelopeCtr += 2;
				break;
			case 6:   //release
				self->audVol -= sample->releaseSpeed;

				if (self->audVol <= sample->releaseMin || self->audVol < -256) {
					self->audVol = sample->releaseMin;
					voice->envelopeCtr = 8;
				}
				break;
		}

		voice->volume = self->audVol;
		voice->arpeggioCtr = ++voice->arpeggioCtr & 15;
		index = sample->finetune + sample->arpeggio[voice->arpeggioCtr] + voice->note;
		voice->period = self->audPer = PERIODS[index];

		if (voice->bendSpeed) {
			unsigned idxx = sample->finetune + voice->bendTo;
			assert_op(idxx, <, ARRAY_SIZE(PERIODS));
			value = PERIODS[idxx];
			index = ~voice->bendSpeed + 1;
			if (index < -128) index &= 255;
			voice->pitchCtr += index;
			voice->period   += voice->pitchCtr;

			if ((index < 0 && voice->period <= value) || (index > 0 && voice->period >= value)) {
				voice->note   = voice->bendTo;
				voice->period = value;
				voice->bendSpeed = 0;
				voice->pitchCtr  = 0;
			}
		}

		if (sample->phaseShift) {
			if (voice->phaseSpeed) {
				voice->phaseSpeed--;
			} else {
				voice->phaseTimer = ++voice->phaseTimer & 31;
				index = (sample->phaseShift << 5) + voice->phaseTimer;
				voice->period += memory[index] >> 2;
			}
		}
		voice->pitchFallCtr -= sample->pitchFall;
		if (voice->pitchFallCtr < -256) voice->pitchFallCtr += 256;
		voice->period += voice->pitchFallCtr;

		if (voice->waitCtr == 0) {
			if (voice->waveTimer) {
				voice->waveTimer--;
			} else {
				if (voice->wavePos < 16) {
					index = (voice->waveList << 4) + voice->wavePos;
					value = self->waveLists[index++];

					if (value == 0xff) {
						voice->wavePos = self->waveLists[index] & 254;
					} else {
						self->audPtr = value << 5;
						voice->waveTimer = self->waveLists[index];
						voice->wavePos += 2;
					}
				}
			}
		}
		if (self->audPtr > -1) chan->pointer = self->audPtr;
		if (self->audPer != 0) AmigaChannel_set_period(chan, voice->period);
		if (self->audLen != 0) chan->length  = self->audLen;

		if (sample->super.volume) AmigaChannel_set_volume(chan, sample->super.volume);
		else AmigaChannel_set_volume(chan, self->audVol >> 2);

		AmigaChannel_set_enabled(chan, 1);
		voice = voice->next;
	}

	self->trackEnd = self->patternEnd = 0;

	if (++(self->super.super.tick) > self->super.super.speed) {
		self->super.super.tick = 0;

		if (++(self->patternPos) == self->patternLen) {
			self->patternPos = 0;
			self->patternEnd = 1;

			if (++(self->trackPos) == self->trackLen) {
				self->trackPos = self->trackEnd = 1;
				CoreMixer_set_complete(&self->super.amiga->super, 1);
			}
		}
	}

	if (self->mix1Speed) {
		if (self->mix1Ctr == 0) {
			self->mix1Ctr = self->mix1Speed;
			index = self->mix1Pos = ++(self->mix1Pos) & 31;
			dst  = (self->mix1Dest    << 5) + 31;
			src1 = (self->mix1Source1 << 5) + 31;
			src2 =  self->mix1Source2 << 5;

			for (i = 31; i > -1; --i) {
				unsigned idxx = src2 + index;
				assert_op(idxx, <, AMIGA_MAX_MEMORY);
				memory[dst--] = (memory[src1--] + memory[idxx]) >> 1;
				index = --index & 31;
			}
		}
		self->mix1Ctr--;
	}

	if (self->mix2Speed) {
		if (self->mix2Ctr == 0) {
			self->mix2Ctr = self->mix2Speed;
			index = self->mix2Pos = ++(self->mix2Pos) & 31;
			dst  = (self->mix2Dest    << 5) + 31;
			src1 = (self->mix2Source1 << 5) + 31;
			src2 =  self->mix2Source2 << 5;

			for (i = 31; i > -1; --i) {
				unsigned idxx = src2 + index;
				assert_op(idxx, <, AMIGA_MAX_MEMORY);
				memory[dst--] = (memory[src1--] + memory[idxx]) >> 1;
				index = --index & 31;
			}
		}
		self->mix2Ctr--;
	}

	if (self->doFilter) {
		index = self->mix1Pos + 32;
		assert_op(index, <, AMIGA_MAX_MEMORY);		
		memory[index] = ~memory[index] + 1;
	}
	voice = &self->voices[0];

	while (voice) {
		chan = voice->channel;

		if (voice->waitCtr == 1) {
			voice->waitCtr++;
		} else if (voice->waitCtr == 2) {
			voice->waitCtr++;
			chan->pointer = voice->samplePtr;
			chan->length  = voice->sampleLen;
		}
		voice = voice->next;
	}
}

//override
void S1Player_initialize(struct S1Player* self) {
	struct AmigaChannel *chan = 0;
	struct AmigaStep *step = 0;
	struct S1Voice *voice = &self->voices[0];
	
	//super->initialize();
	CorePlayer_initialize(&self->super.super);

	self->super.super.speed      =  self->speedDef;
	self->super.super.tick       =  self->speedDef;
	self->trackPos   =  1;
	self->trackEnd   =  0;
	self->patternPos = -1;
	self->patternEnd =  0;
	self->patternLen =  self->patternDef;

	self->mix1Ctr = self->mix1Pos = 0;
	self->mix2Ctr = self->mix2Pos = 0;

	while (voice) {
		S1Voice_initialize(voice);
		//voice->initialize();
		assert_op(voice->index, <, AMIGA_MAX_CHANNELS);
		chan = &self->super.amiga->channels[voice->index];

		voice->channel = chan;
		assert_op(voice->index, <, S1PLAYER_MAX_TRACKSPTR);
		voice->step    = self->tracksPtr[voice->index];
		
		assert_op(voice->step, <, S1PLAYER_MAX_TRACKS);
		step = &self->tracks[voice->step];
		
		assert_op(step->pattern, <, S1PLAYER_MAX_PATTERNSPTR);
		voice->row    = self->patternsPtr[step->pattern];
		
		assert_op(voice->row, <, S1PLAYER_MAX_PATTERNS);
		voice->sample = self->patterns[voice->row].super.sample;

		chan->length  = 32;
		AmigaChannel_set_period(chan, voice->period);
		AmigaChannel_set_enabled(chan, 1);

		voice = voice->next;
	}
}

//override
void S1Player_loader(struct S1Player* self, struct ByteArray *stream) {
	int data = 0;
	int i = 0;
	char id[32];
	int j = 0; 
	int headers = 0; 
	int len = 0;
	int position = 0;
	struct SMRow *row = 0;
	struct S1Sample *sample = 0;
	int start = 0;
	struct AmigaStep *step = 0;
	int totInstruments = 0;
	int totPatterns = 0; 
	int totSamples = 0; 
	int totWaveforms = 0; 
	int ver = 0;

	while (stream->bytesAvailable(stream) > 8) {
		start = stream->readUnsignedShort(stream);
		if (start != 0x41fa) continue;
		j = stream->readUnsignedShort(stream);

		start = stream->readUnsignedShort(stream);
		if (start != 0xd1e8) continue;
		start = stream->readUnsignedShort(stream);

		if (start == 0xffd4) {
			if (j == 0x0fec) ver = SIDMON_0FFA;
			else if (j == 0x1466) ver = SIDMON_1444;
			else ver = j;

			position = j + ByteArray_get_position(stream) - 6;
			break;
		}
	}
	if (!position) return;
	ByteArray_set_position(stream, position);

	stream->readMultiByte(stream, id, 32);
	if (!is_str(id, " SID-MON BY R.v.VLIET  (c) 1988 ") &&
	    !is_str(id, " Ripped with SCX Ripper") && 
	    !is_str(id, " Ripped with SidmRipper")) return;

	ByteArray_set_position(stream, position - 44);
	start = stream->readUnsignedInt(stream);

	for (i = 1; i < 4; ++i)
		self->tracksPtr[i] = (stream->readUnsignedInt(stream) - start) / 6;

	ByteArray_set_position(stream, position - 8);
	start = stream->readUnsignedInt(stream);
	len   = stream->readUnsignedInt(stream);
	if (len < start) len = ByteArray_get_length(stream) - position;

	totPatterns = (len - start) >> 2;
	
	assert_op(totPatterns, <=, S1PLAYER_MAX_PATTERNSPTR);
	
	//self->patternsPtr = new Vector.<int>(totPatterns);
	ByteArray_set_position(stream, position + start + 4);

	for (i = 1; i < totPatterns; ++i) {
		start = stream->readUnsignedInt(stream) / 5;

		if (start == 0) {
			totPatterns = i;
			break;
		}
		self->patternsPtr[i] = start;
	}

	//self->patternsPtr->length = totPatterns;
	//self->patternsPtr->fixed  = true;

	ByteArray_set_position(stream, position - 44);
	start = stream->readUnsignedInt(stream);
	ByteArray_set_position(stream, position - 28);
	len = (stream->readUnsignedInt(stream) - start) / 6;

	//self->tracks = new Vector.<AmigaStep>(len, true);
	assert_op(len, <=, S1PLAYER_MAX_TRACKS);
	ByteArray_set_position(stream, position + start);

	for (i = 0; i < len; ++i) {
		//step = new AmigaStep();
		step = &self->tracks[i];
		AmigaStep_ctor(step);
		step->pattern = stream->readUnsignedInt(stream);
		if (step->pattern >= totPatterns) step->pattern = 0;
		stream->readByte(stream);
		step->transpose = stream->readByte(stream);
		if (step->transpose < -99 || step->transpose > 99) step->transpose = 0;
		// self->tracks[i] = step;
	}

	ByteArray_set_position(stream, position - 24);
	start = stream->readUnsignedInt(stream);
	totWaveforms = stream->readUnsignedInt(stream) - start;

	//self->super.amiga->memory->length = 32;
	Amiga_memory_set_length(self->super.amiga, 32);
	Amiga_store(self->super.amiga, stream, totWaveforms, position + start);
	totWaveforms >>= 5;

	ByteArray_set_position(stream, position - 16);
	start = stream->readUnsignedInt(stream);
	len = (stream->readUnsignedInt(stream) - start) + 16;
	j = (totWaveforms + 2) << 4;
	
	unsigned waveLists_len = len < j ? j : len;
	assert_op(waveLists_len, <=, S1PLAYER_MAX_WAVELISTS);

	//self->waveLists = new Vector.<int>(len < j ? j : len, true);
	ByteArray_set_position(stream, position + start);
	i = 0;

	assert_op(j + (j % 4), <=, S1PLAYER_MAX_WAVELISTS);
	while (i < j) {
		self->waveLists[i] = (i + 1) >> 4;
		i++;
		self->waveLists[i++] = 0xff;
		self->waveLists[i++] = 0xff;
		self->waveLists[i++] = 0x10;
		i += 12;
	}

	assert_op(len, <=, S1PLAYER_MAX_WAVELISTS);
	for (i = 16; i < len; ++i)
		self->waveLists[i] = stream->readUnsignedByte(stream);

	ByteArray_set_position(stream, position - 20);
	ByteArray_set_position(stream, position + stream->readUnsignedInt(stream));

	self->mix1Source1 = stream->readUnsignedInt(stream);
	self->mix2Source1 = stream->readUnsignedInt(stream);
	self->mix1Source2 = stream->readUnsignedInt(stream);
	self->mix2Source2 = stream->readUnsignedInt(stream);
	self->mix1Dest    = stream->readUnsignedInt(stream);
	self->mix2Dest    = stream->readUnsignedInt(stream);
	self->patternDef  = stream->readUnsignedInt(stream);
	self->trackLen    = stream->readUnsignedInt(stream);
	self->speedDef    = stream->readUnsignedInt(stream);
	self->mix1Speed   = stream->readUnsignedInt(stream);
	self->mix2Speed   = stream->readUnsignedInt(stream);

	if (self->mix1Source1 > totWaveforms) self->mix1Source1 = 0;
	if (self->mix2Source1 > totWaveforms) self->mix2Source1 = 0;
	if (self->mix1Source2 > totWaveforms) self->mix1Source2 = 0;
	if (self->mix2Source2 > totWaveforms) self->mix2Source2 = 0;
	if (self->mix1Dest > totWaveforms) self->mix1Speed = 0;
	if (self->mix2Dest > totWaveforms) self->mix2Speed = 0;
	if (self->speedDef == 0) self->speedDef = 4;

	ByteArray_set_position(stream, position - 28);
	j = stream->readUnsignedInt(stream);
	totInstruments = (stream->readUnsignedInt(stream) - j) >> 5;
	if (totInstruments > 63) totInstruments = 63;
	len = totInstruments + 1;

	ByteArray_set_position(stream, position - 4);
	start = stream->readUnsignedInt(stream);

	if (start == 1) {
		ByteArray_set_position(stream, 0x71c);
		start = stream->readUnsignedShort(stream);

		if (start != 0x4dfa) {
			ByteArray_set_position(stream, 0x6fc);
			start = stream->readUnsignedShort(stream);

			if (start != 0x4dfa) {
				self->super.super.version = 0;
				return;
			}
		}
		ByteArray_set_position_rel(stream, stream->readUnsignedShort(stream));
		assert_op(len + 3, <=, S1PLAYER_MAX_SAMPLES);
		//self->samples = new Vector.<S1Sample>(len + 3, true);

		for (i = 0; i < 3; ++i) {
			//sample = new S1Sample();
			sample = &self->samples[len + i];
			S1Sample_ctor(sample);
			
			sample->waveform = 16 + i;
			sample->super.length   = EMBEDDED[i];
			sample->super.pointer  = Amiga_store(self->super.amiga, stream, sample->super.length, -1);
			sample->super.loop     = sample->super.loopPtr = 0;
			sample->super.repeat   = 4;
			sample->super.volume   = 64;
			//self->samples[int(len + i)] = sample;
			ByteArray_set_position_rel(stream, sample->super.length);
		}
	} else {
		//samples = new Vector.<S1Sample>(len, true);
		assert_op(len, <=, S1PLAYER_MAX_SAMPLES);

		ByteArray_set_position(stream, position + start);
		data = stream->readUnsignedInt(stream);
		totSamples = (data >> 5) + 15;
		headers = ByteArray_get_position(stream);
		data += headers;
	}

	//sample = new S1Sample();
	//self->samples[0] = sample;
	S1Sample_ctor(&self->samples[0]);
	
	ByteArray_set_position(stream, position + j);

	for (i = 1; i < len; ++i) {
		//sample = new S1Sample();
		sample = &self->samples[i];
		S1Sample_ctor(sample);
		
		sample->waveform = stream->readUnsignedInt(stream);
		for (j = 0; j < 16; ++j) sample->arpeggio[j] = stream->readUnsignedByte(stream);

		sample->attackSpeed  = stream->readUnsignedByte(stream);
		sample->attackMax    = stream->readUnsignedByte(stream);
		sample->decaySpeed   = stream->readUnsignedByte(stream);
		sample->decayMin     = stream->readUnsignedByte(stream);
		sample->sustain      = stream->readUnsignedByte(stream);
		stream->readByte(stream);
		sample->releaseSpeed = stream->readUnsignedByte(stream);
		sample->releaseMin   = stream->readUnsignedByte(stream);
		sample->phaseShift   = stream->readUnsignedByte(stream);
		sample->phaseSpeed   = stream->readUnsignedByte(stream);
		sample->finetune     = stream->readUnsignedByte(stream);
		sample->pitchFall    = stream->readByte(stream);

		if (ver == SIDMON_1444) {
			sample->pitchFall = sample->finetune;
			sample->finetune = 0;
		} else {
			if (sample->finetune > 15) sample->finetune = 0;
			sample->finetune *= 67;
		}

		if (sample->phaseShift > totWaveforms) {
			sample->phaseShift = 0;
			sample->phaseSpeed = 0;
		}

		if (sample->waveform > 15) {
			if ((totSamples > 15) && (sample->waveform > totSamples)) {
				sample->waveform = 0;
			} else {
				start = headers + ((sample->waveform - 16) << 5);
				//FIXME examine if this needs samples[i] to be cleared
				// the original code malloc'd a S1Sample at loop start, but doesnt
				// assign it to the array in this case here.
				if (start >= ByteArray_get_length(stream)) continue;
				j = ByteArray_get_position(stream);

				ByteArray_set_position(stream, start);
				sample->super.pointer  = stream->readUnsignedInt(stream);
				sample->super.loop     = stream->readUnsignedInt(stream);
				sample->super.length   = stream->readUnsignedInt(stream);
				stream->readMultiByte(stream, sample->sample_name, 20);
				sample->super.name     = sample->sample_name;

				if (sample->super.loop == 0      ||
					sample->super.loop == 99999  ||
					sample->super.loop == 199999 ||
					sample->super.loop >= sample->super.length) {

					sample->super.loop   = 0;
					sample->super.repeat = ver == SIDMON_0FFA ? 2 : 4;
				} else {
					sample->super.repeat = sample->super.length - sample->super.loop;
					sample->super.loop  -= sample->super.pointer;
				}

				sample->super.length -= sample->super.pointer;
				if (sample->super.length < (sample->super.loop + sample->super.repeat))
				sample->super.length = sample->super.loop + sample->super.repeat;

				sample->super.pointer = Amiga_store(self->super.amiga, stream, sample->super.length, data + sample->super.pointer);
				if (sample->super.repeat < 6 || sample->super.loop == 0) sample->super.loopPtr = 0;
				else sample->super.loopPtr = sample->super.pointer + sample->super.loop;

				ByteArray_set_position(stream, j);
			}
		} else if (sample->waveform > totWaveforms) {
			sample->waveform = 0;
		}
		//self->samples[i] = sample;
	}

	ByteArray_set_position(stream, position - 12);
	start = stream->readUnsignedInt(stream);
	len = (stream->readUnsignedInt(stream) - start) / 5;
	
	assert_op(len, <=, S1PLAYER_MAX_PATTERNS);
	
	//self->patterns = new Vector.<SMRow>(len, true);
	
	ByteArray_set_position(stream, position + start);

	for (i = 0; i < len; ++i) {
		//row = new SMRow();
		row = &self->patterns[i];
		SMRow_ctor(row);
		
		row->super.note   = stream->readUnsignedByte(stream);
		row->super.sample = stream->readUnsignedByte(stream);
		row->super.effect = stream->readUnsignedByte(stream);
		row->super.param  = stream->readUnsignedByte(stream);
		row->speed  = stream->readUnsignedByte(stream);

		if (ver == SIDMON_1444) {
			if (row->super.note > 0 && row->super.note < 255) row->super.note += 469;
			if (row->super.effect > 0 && row->super.effect < 255) row->super.effect += 469;
			if (row->super.sample > 59) row->super.sample = totInstruments + (row->super.sample - 60);
		} else if (row->super.sample > totInstruments) {
			row->super.sample = 0;
		}
		//self->patterns[i] = row;
	}

	if (ver == SIDMON_1170 || ver == SIDMON_11C6 || ver == SIDMON_1444) {
		if (ver == SIDMON_1170) self->mix1Speed = self->mix2Speed = 0;
		self->doReset = self->doFilter = 0;
	} else {
		self->doReset = self->doFilter = 1;
	}
	self->super.super.version = 1;
}
