/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.0 - 2012/03/24

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "FEPlayer.h"
#include "../flod_internal.h"

static const unsigned short PERIODS[] = {
        8192,7728,7296,6888,6504,6136,5792,5464,5160,
        4872,4600,4336,4096,3864,3648,3444,3252,3068,
        2896,2732,2580,2436,2300,2168,2048,1932,1824,
        1722,1626,1534,1448,1366,1290,1218,1150,1084,
        1024, 966, 912, 861, 813, 767, 724, 683, 645,
         609, 575, 542, 512, 483, 456, 430, 406, 383,
         362, 341, 322, 304, 287, 271, 256, 241, 228,
         215, 203, 191, 181, 170, 161, 152, 143, 135,
};

void FEPlayer_loader(struct FEPlayer* self, struct ByteArray *stream);
void FEPlayer_process(struct FEPlayer* self);
void FEPlayer_initialize(struct FEPlayer* self);

void FEPlayer_defaults(struct FEPlayer* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void FEPlayer_ctor(struct FEPlayer* self, struct Amiga* amiga) {
	CLASS_CTOR_DEF(FEPlayer);
	// original constructor code goes here
	//super(amiga);
	AmigaPlayer_ctor(&self->super, amiga);
	
	//voices = new Vector.<FEVoice>(4, true);

	/*voices[3] = new FEVoice(3,8);
	voices[3].next = voices[2] = new FEVoice(2,4);
	voices[2].next = voices[1] = new FEVoice(1,2);
	voices[1].next = voices[0] = new FEVoice(0,1); */
	
	unsigned i = FEPLAYER_MAX_VOICES;
	while(1) {
		FEVoice_ctor(&self->voices[i], i, 1 << i);
		if(i) self->voices[i].next = &self->voices[i - 1];
		else break;
		i--;
	}
	
	//vtable
	self->super.super.loader = FEPlayer_loader;
	self->super.super.process = FEPlayer_process;
	self->super.super.initialize = FEPlayer_initialize;
	
	self->super.super.min_filesize = 2830;
	
}

struct FEPlayer* FEPlayer_new(struct Amiga* amiga) {
	CLASS_NEW_BODY(FEPlayer, amiga);
}

//override
void FEPlayer_process(struct FEPlayer* self) {
	struct AmigaChannel *chan = 0;
	int i = 0;
	int j = 0;
	int len = 0;
	int loop = 0;
	int pos = 0;
	struct FESample *sample = 0;
	int value = 0;
	struct FEVoice *voice = &self->voices[3];

	while (voice) {
		chan = voice->channel;
		loop = 0;

		do {
			ByteArray_set_position(self->patterns, voice->patternPos);
			sample = voice->sample;
			self->sampFlag = 0;

			if (!voice->busy) {
				voice->busy = 1;

				if (sample->loopPtr == 0) {
					chan->pointer = self->super.amiga->loopPtr;
					chan->length  = self->super.amiga->loopLen;
				} else if (sample->loopPtr > 0) {
					chan->pointer = (sample->type) ? voice->synth : sample->pointer + sample->loopPtr;
					chan->length  = sample->length - sample->loopPtr;
				}
			}

			if (--voice->tick == 0) {
				loop = 2;

				while (loop > 1) {
					value = self->patterns->readByte(self->patterns);

					if (value < 0) {
						switch (value) {
							case -125:
							{
								unsigned idxx = self->patterns->readUnsignedByte(self->patterns);
								assert_op(idxx, <, FEPLAYER_MAX_SAMPLES);
								
								voice->sample = sample = &self->samples[idxx];
							}
								self->sampFlag = 1;
								voice->patternPos = ByteArray_get_position(self->patterns);
								break;
							case -126:
								self->super.super.speed = self->patterns->readUnsignedByte(self->patterns);
								voice->patternPos = ByteArray_get_position(self->patterns);
								break;
							case -127:
								value = (sample) ? sample->relative : 428;
								voice->portaSpeed = self->patterns->readUnsignedByte(self->patterns) * self->super.super.speed;
								voice->portaNote  = self->patterns->readUnsignedByte(self->patterns);
								assert_op(voice->portaNote, <, ARRAY_SIZE(PERIODS));
								voice->portaLimit = (PERIODS[voice->portaNote] * value) >> 10;
								voice->portamento = 0;
								voice->portaDelay = self->patterns->readUnsignedByte(self->patterns) * self->super.super.speed;
								voice->portaFlag  = 1;
								voice->patternPos = ByteArray_get_position(self->patterns);
								break;
							case -124:
								AmigaChannel_set_enabled(chan, 0);
								voice->tick = self->super.super.speed;
								voice->busy = 1;
								voice->patternPos = ByteArray_get_position(self->patterns);
								loop = 0;
								break;
							case -128:
								voice->trackPos++;

								while (1) {
									assert_op(voice->index, <, FESONG_MAX_TRACKS);
									assert_op(voice->trackPos, <, FETRACK_MAX_DATA);
						
									value = self->song->tracks[voice->index].track_data[voice->trackPos];

									if (value == 65535) {
										CoreMixer_set_complete(&self->super.amiga->super, 1);
										//self->super.amiga->complete = 1;
									} else if (value > 32767) {
										voice->trackPos = (value ^ 32768) >> 1;

										if (!self->super.super.loopSong) {
											self->complete &= ~(voice->bitFlag);
											if (!self->complete) {
												//self->super.amiga->complete = 1;
												CoreMixer_set_complete(&self->super.amiga->super, 1);
											}
										}
									} else {
										voice->patternPos = value;
										voice->tick = 1;
										loop = 1;
										break;
									}
								}
								break;
							default:
								voice->tick =self->super.super.speed * -value;
								voice->patternPos = ByteArray_get_position(self->patterns);
								loop = 0;
								break;
						}
					} else {
						loop = 0;
						voice->patternPos = ByteArray_get_position(self->patterns);

						voice->note = value;
						voice->arpeggioPos =  0;
						voice->vibratoFlag = -1;
						voice->vibrato     =  0;

						voice->arpeggioSpeed = sample->arpeggioSpeed;
						voice->vibratoDelay  = sample->vibratoDelay;
						voice->vibratoSpeed  = sample->vibratoSpeed;
						voice->vibratoDepth  = sample->vibratoDepth;

						if (sample->type == 1) {
							if (self->sampFlag || (sample->synchro & 2)) {
								voice->pulseCounter = sample->pulseCounter;
								voice->pulseDelay = sample->pulseDelay;
								voice->pulseDir = 0;
								voice->pulsePos = sample->pulsePosL;
								voice->pulseSpeed = sample->pulseSpeed;

								i = voice->synth;
								len = i + sample->pulsePosL;
								assert_op(len, <=, AMIGA_MAX_MEMORY);
								for (; i < len; ++i) self->super.amiga->memory[i] = sample->pulseRateNeg;
								len += (sample->length - sample->pulsePosL);
								assert_op(len, <=, AMIGA_MAX_MEMORY);
								for (; i < len; ++i) self->super.amiga->memory[i] = sample->pulseRatePos;
								if(len > self->super.amiga->vector_count_memory)
									self->super.amiga->vector_count_memory = len;
							}

							chan->pointer = voice->synth;
							
						} else if (sample->type == 2) {
							voice->blendCounter = sample->blendCounter;
							voice->blendDelay = sample->blendDelay;
							voice->blendDir = 0;
							voice->blendPos = 1;

							i = sample->pointer;
							j = voice->synth;
							len = i + 31;
							assert_op(len, <=, AMIGA_MAX_MEMORY);
							assert_op(j + 31, <=, AMIGA_MAX_MEMORY);
							for (; i < len; ++i) self->super.amiga->memory[j++] = self->super.amiga->memory[i];
							if(len > self->super.amiga->vector_count_memory)
								self->super.amiga->vector_count_memory = len;
							if(j > self->super.amiga->vector_count_memory)
								self->super.amiga->vector_count_memory = j;
								
							chan->pointer = voice->synth;
							
						} else {
							chan->pointer = sample->pointer;
						}

						voice->tick = self->super.super.speed;
						voice->busy = 0;
						voice->period = (PERIODS[voice->note] * sample->relative) >> 10;

						voice->volume = 0;
						voice->envelopePos = 0;
						voice->sustainTime = sample->sustainTime;

						chan->length  = sample->length;
						AmigaChannel_set_period(chan, voice->period);
						AmigaChannel_set_volume(chan, 0);
						AmigaChannel_set_enabled(chan, 1);

						if (voice->portaFlag) {
							if (!voice->portamento) {
								voice->portamento   = voice->period;
								voice->portaCounter = 1;
								voice->portaPeriod  = voice->portaLimit - voice->period;
							}
						}
					}
				}
			} else if (voice->tick == 1) {
				//value = (self->patterns[voice->patternPos] - 160) & 255;
				value = (ByteArray_getUnsignedByte(self->patterns, voice->patternPos) - 160) & 255;
				if (value > 127) AmigaChannel_set_enabled(chan, 0);
			}
		} while (loop > 0);

		if (!AmigaChannel_get_enabled(chan)) {
			voice = voice->next;
			continue;
		}

		value = voice->note + sample->arpeggio[voice->arpeggioPos];

		if (--voice->arpeggioSpeed == 0) {
			voice->arpeggioSpeed = sample->arpeggioSpeed;

			if (++voice->arpeggioPos == sample->arpeggioLimit)
			voice->arpeggioPos = 0;
		}

		assert_op(value, <, ARRAY_SIZE(PERIODS));
		voice->period = (PERIODS[value] * sample->relative) >> 10;

		if (voice->portaFlag) {
			if (voice->portaDelay) {
				voice->portaDelay--;
			} else {
				voice->period += ((voice->portaCounter * voice->portaPeriod) / voice->portaSpeed);

				if (++voice->portaCounter > voice->portaSpeed) {
					voice->note = voice->portaNote;
					voice->portaFlag = 0;
				}
			}
		}

		if (voice->vibratoDelay) {
			voice->vibratoDelay--;
		} else {
			if (voice->vibratoFlag) {
				if (voice->vibratoFlag < 0) {
					voice->vibrato += voice->vibratoSpeed;

					if (voice->vibrato == voice->vibratoDepth)
						voice->vibratoFlag ^= 0x80000000;
				} else {
					voice->vibrato -= voice->vibratoSpeed;

					if (voice->vibrato == 0)
						voice->vibratoFlag ^= 0x80000000;
				}

				if (voice->vibrato == 0) voice->vibratoFlag ^= 1;

				if (voice->vibratoFlag & 1) {
					voice->period += voice->vibrato;
				} else {
					voice->period -= voice->vibrato;
				}
			}
		}

		AmigaChannel_set_period(chan, voice->period);

		switch (voice->envelopePos) {
			case 4: break;
			case 0:
				voice->volume += sample->attackSpeed;

				if (voice->volume >= sample->attackVol) {
					voice->volume = sample->attackVol;
					voice->envelopePos = 1;
				}
				break;
			case 1:
				voice->volume -= sample->decaySpeed;

				if (voice->volume <= sample->decayVol) {
					voice->volume = sample->decayVol;
					voice->envelopePos = 2;
				}
				break;
			case 2:
				if (voice->sustainTime) {
					voice->sustainTime--;
				} else {
					voice->envelopePos = 3;
				}
				break;
			case 3:
				voice->volume -= sample->releaseSpeed;

				if (voice->volume <= sample->releaseVol) {
					voice->volume = sample->releaseVol;
					voice->envelopePos = 4;
				}
				break;
			default:
				break;
		}

		value = sample->envelopeVol << 12;
		value >>= 8;
		value >>= 4;
		value *= voice->volume;
		value >>= 8;
		value >>= 1;
		AmigaChannel_set_volume(chan, value);

		if (sample->type == 1) {
			if (voice->pulseDelay) {
				voice->pulseDelay--;
			} else {
				if (voice->pulseSpeed) {
					voice->pulseSpeed--;
				} else {
					if (voice->pulseCounter || !(sample->synchro & 1)) {
						voice->pulseSpeed = sample->pulseSpeed;

						if (voice->pulseDir & 4) {
							while (1) {
								if (voice->pulsePos >= sample->pulsePosL) {
									loop = 1;
									break;
								}

								voice->pulseDir &= -5;
								voice->pulsePos++;
								voice->pulseCounter--;

								if (voice->pulsePos <= sample->pulsePosH) {
									loop = 2;
									break;
								}

								voice->pulseDir |= 4;
								voice->pulsePos--;
								voice->pulseCounter--;
							}
						} else {
						while (1) {
							if (voice->pulsePos <= sample->pulsePosH) {
								loop = 2;
								break;
							}

							voice->pulseDir |= 4;
							voice->pulsePos--;
							voice->pulseCounter--;

							if (voice->pulsePos >= sample->pulsePosL) {
								loop = 1;
								break;
							}

							voice->pulseDir &= -5;
							voice->pulsePos++;
							voice->pulseCounter++;
						}
						}

						pos = voice->synth + voice->pulsePos;
						assert_op(pos, <, AMIGA_MAX_MEMORY);
						if(pos >= self->super.amiga->vector_count_memory)
							self->super.amiga->vector_count_memory = pos + 1;

						if (loop == 1) {
							self->super.amiga->memory[pos] = sample->pulseRatePos;
							voice->pulsePos--;
						} else {
							self->super.amiga->memory[pos] = sample->pulseRateNeg;
							voice->pulsePos++;
						}
					}
				}
			}
		} else if (sample->type == 2) {
			if (voice->blendDelay) {
				voice->blendDelay--;
			} else {
				if (voice->blendCounter || !(sample->synchro & 4)) {
					if (voice->blendDir) {
						if (voice->blendPos != 1) {
							voice->blendPos--;
						} else {
							voice->blendDir ^= 1;
							voice->blendCounter--;
						}
					} else {
						if (voice->blendPos != (sample->blendRate << 1)) {
							voice->blendPos++;
						} else {
							voice->blendDir ^= 1;
							voice->blendCounter--;
						}
					}

					i = sample->pointer;
					j = voice->synth;
					len = i + 31;
					pos = len + 1;
					
					assert_op(len, <=, AMIGA_MAX_MEMORY);
					if(len > self->super.amiga->vector_count_memory)
						self->super.amiga->vector_count_memory = len;
					unsigned posmax = pos + ((len - i) * 2); // FIXME check if thats correct
					assert_op(posmax, <=, AMIGA_MAX_MEMORY);
					
					for (; i < len; ++i) {
						value = (voice->blendPos * self->super.amiga->memory[pos++]) >> sample->blendRate;
						self->super.amiga->memory[pos++] = value + self->super.amiga->memory[i];
					}
					assert_op(pos, <=, AMIGA_MAX_MEMORY);
					if(pos > self->super.amiga->vector_count_memory)
						self->super.amiga->vector_count_memory = pos;
				}
			}
		}

		voice = voice->next;
	}
}

//override
void FEPlayer_initialize(struct FEPlayer* self) {
	int i = 0; 
	int len = 0; 
	struct FEVoice *voice = &self->voices[3];
	
	CorePlayer_initialize(&self->super.super);
	//self->super->initialize();

	self->song  = &self->songs[self->super.super.playSong];
	self->super.super.speed = self->song->speed;

	self->complete = 15;

	while (voice) {
		//voice->initialize();
		FEVoice_initialize(voice);
		voice->channel = &self->super.amiga->channels[voice->index];
		voice->patternPos = self->song->tracks[voice->index].track_data[0];

		i = voice->synth;
		len = i + 64;
		assert_op(len, <=, AMIGA_MAX_MEMORY);
		for (; i < len; ++i) self->super.amiga->memory[i] = 0;
		if(len > self->super.amiga->vector_count_memory)
			self->super.amiga->vector_count_memory = len;

		voice = voice->next;
	}
}

//override
void FEPlayer_loader(struct FEPlayer* self, struct ByteArray *stream) {
	int basePtr = 0;
	int dataPtr = 0;
	int i = 0;
	int j = 0;
	int len = 0;
	int pos = 0;
	int ptr = 0;
	struct FESample *sample = 0;
	int size = 0; 
	struct FESong *song = 0;
	int tracksLen = 0;
	int value = 0;

	while (ByteArray_get_position(stream) < 16) {
		value = stream->readUnsignedShort(stream);
		ByteArray_set_position_rel(stream, + 2);
		if (value != 0x4efa) return;                                            //jmp
	}

	while (ByteArray_get_position(stream) < 1024) {
		value = stream->readUnsignedShort(stream);

		if (value == 0x123a) {                                                  //move->b $x,d1
			ByteArray_set_position_rel(stream, +2);
			value = stream->readUnsignedShort(stream);

			if (value == 0xb001) {                                                //cmp->b d1,d0
				ByteArray_set_position_rel(stream, -4);
				dataPtr = (ByteArray_get_position(stream) + stream->readUnsignedShort(stream)) - 0x895;
			}
		} else if (value == 0x214a) {                                           //move->l a2,(a0)
			ByteArray_set_position_rel(stream, +2);
			value = stream->readUnsignedShort(stream);

			if (value == 0x47fa) {                                                //lea $x,a3
				basePtr = ByteArray_get_position(stream) + stream->readShort(stream);
				self->super.super.version = 1;
				break;
			}
		}
	}

	if (!self->super.super.version) return;

	ByteArray_set_position(stream, dataPtr + 0x8a2);
	pos = stream->readUnsignedInt(stream);
	ByteArray_set_position(stream, basePtr + pos);
	//samples = new Vector.<FESample>();
	unsigned samples_count = 0;
	pos = 0x7fffffff;

	while (pos > ByteArray_get_position(stream)) {
		value = stream->readUnsignedInt(stream);

		if (value) {
			if ((value < ByteArray_get_position(stream)) || (value >= ByteArray_get_length(stream))) {
				ByteArray_set_position_rel(stream, -4);
				break;
			}

			if (value < pos) pos = basePtr + value;
		}
		
		assert_op(samples_count, <, FEPLAYER_MAX_SAMPLES);
		sample = &self->samples[samples_count];
		FESample_ctor(sample);
		
		//sample = new FESample();
		sample->pointer  = value;
		sample->loopPtr  = stream->readShort(stream);
		sample->length   = stream->readUnsignedShort(stream) << 1;
		sample->relative = stream->readUnsignedShort(stream);

		sample->vibratoDelay = stream->readUnsignedByte(stream);
		ByteArray_set_position_rel(stream, +1);
		sample->vibratoSpeed = stream->readUnsignedByte(stream);
		sample->vibratoDepth = stream->readUnsignedByte(stream);
		sample->envelopeVol  = stream->readUnsignedByte(stream);
		sample->attackSpeed  = stream->readUnsignedByte(stream);
		sample->attackVol    = stream->readUnsignedByte(stream);
		sample->decaySpeed   = stream->readUnsignedByte(stream);
		sample->decayVol     = stream->readUnsignedByte(stream);
		sample->sustainTime  = stream->readUnsignedByte(stream);
		sample->releaseSpeed = stream->readUnsignedByte(stream);
		sample->releaseVol   = stream->readUnsignedByte(stream);

		for (i = 0; i < 16; ++i) sample->arpeggio[i] = stream->readByte(stream);

		sample->arpeggioSpeed = stream->readUnsignedByte(stream);
		sample->type          = stream->readByte(stream);
		sample->pulseRateNeg  = stream->readByte(stream);
		sample->pulseRatePos  = stream->readUnsignedByte(stream);
		sample->pulseSpeed    = stream->readUnsignedByte(stream);
		sample->pulsePosL     = stream->readUnsignedByte(stream);
		sample->pulsePosH     = stream->readUnsignedByte(stream);
		sample->pulseDelay    = stream->readUnsignedByte(stream);
		sample->synchro       = stream->readUnsignedByte(stream);
		sample->blendRate     = stream->readUnsignedByte(stream);
		sample->blendDelay    = stream->readUnsignedByte(stream);
		sample->pulseCounter  = stream->readUnsignedByte(stream);
		sample->blendCounter  = stream->readUnsignedByte(stream);
		sample->arpeggioLimit = stream->readUnsignedByte(stream);

		ByteArray_set_position_rel(stream, +12);
		//self->samples->push(sample);
		samples_count++;
		if (!stream->bytesAvailable(stream)) break;
	}

	//self->samples->fixed = true;

	if (pos != 0x7fffffff) {
		Amiga_store(self->super.amiga, stream, ByteArray_get_length(stream) - pos, -1);
		len = samples_count; //self->samples->length;

		for (i = 0; i < len; ++i) {
			sample = &self->samples[i];
			if (sample->pointer) sample->pointer -= (basePtr + pos);
		}
	}

	pos = self->super.amiga->vector_count_memory; //self->super.amiga->memory->length;
	Amiga_memory_set_length(self->super.amiga, self->super.amiga->vector_count_memory + 256);
	//self->super.amiga->memory->length += 256;
	self->super.amiga->loopLen = 100;

	for (i = 0; i < 4; ++i) {
		self->voices[i].synth = pos;
		pos += 64;
	}

	//self->patterns = new ByteArray();
	self->patterns = &self->patterns_buf;
	ByteArray_ctor(self->patterns);
	//FIXME endian ?
	ByteArray_open_mem(self->patterns, self->patterns_memory, FEPLAYER_PATTERNS_MEMORY_MAX);
	
	ByteArray_set_position(stream, dataPtr + 0x8a2);
	len = stream->readUnsignedInt(stream);
	pos = stream->readUnsignedInt(stream);
	ByteArray_set_position(stream, basePtr + pos);
	
	assert_op((len - pos), <=, FEPLAYER_PATTERNS_MEMORY_MAX);
	
	stream->readBytes(stream, self->patterns, 0, (len - pos));
	
	pos += basePtr;

	ByteArray_set_position(stream, dataPtr + 0x895);
	self->super.super.lastSong = len = stream->readUnsignedByte(stream);

	//songs = new Vector.<FESong>(++len, true);
	++len;
	assert_op(len, <=, FEPLAYER_MAX_SONGS);
	
	
	basePtr = dataPtr + 0xb0e;
	tracksLen = pos - basePtr;
	pos = 0;

	for (i = 0; i < len; ++i) {
		//song = new FESong();
		song = &self->songs[i];
		FESong_ctor(song);

		for (j = 0; j < FESONG_MAX_TRACKS; ++j) {
			ByteArray_set_position(stream, basePtr + pos);
			value = stream->readUnsignedShort(stream);

			if (j == 3 && (i == (len - 1))) size = tracksLen;
			else size = stream->readUnsignedShort(stream);

			size = (size - value) >> 1;
			if (size > song->length) song->length = size;

			//song->tracks[j] = new Vector.<int>(size, true);
			assert_op(size, <=, FETRACK_MAX_DATA);
			
			ByteArray_set_position(stream, basePtr + value);

			for (ptr = 0; ptr < size; ++ptr)
				song->tracks[j].track_data[ptr] = stream->readUnsignedShort(stream);

			pos += 2;
		}

		ByteArray_set_position(stream, dataPtr + 0x897 + i);
		song->speed = stream->readUnsignedByte(stream);
		//self->songs[i] = song;
	}

	//stream->clear();
	//stream = null;
}

