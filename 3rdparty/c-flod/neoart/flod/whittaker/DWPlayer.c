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

#include "DWPlayer.h"
#include "../flod_internal.h"
//extends AmigaPlayer

void DWPlayer_defaults(struct DWPlayer* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

// amiga default value: NULL
void DWPlayer_ctor(struct DWPlayer* self, struct Amiga* amiga) {
	CLASS_CTOR_DEF(DWPlayer);
	// original constructor code goes here
	//super(amiga);
	PFUNC();
	AmigaPlayer_ctor(&self->super, amiga);
	unsigned i;
	
	//self->voices = new Vector.<DWVoice>(4, true);
/*	
	self->voices[0] = DWVoice_new(0,1);
	self->voices[1] = DWVoice_new(1,2);
	self->voices[2] = DWVoice_new(2,4);
	self->voices[3] = DWVoice_new(3,8);	*/
	for(i = 0; i < DWPLAYER_MAX_VOICES; i++) {
		DWVoice_ctor(&self->voices[i], i, 1 << i);
	}

	//add vtable
	self->super.super.process = (void*) DWPlayer_process;
	self->super.super.loader = (void*) DWPlayer_loader;
	
	self->super.super.initialize = (void*) DWPlayer_initialize;
	
	// TODO: this is not very accurate...
	self->super.super.min_filesize = 64;
}

struct DWPlayer* DWPlayer_new(struct Amiga* amiga) {
	CLASS_NEW_BODY(DWPlayer, amiga);
}

//override
void DWPlayer_process(struct DWPlayer* self) {
	PFUNC();
	struct AmigaChannel *chan = NULL;
	int loop = 0;
	int pos = 0;
	struct DWSample *sample = NULL;
	int value = 0; 
	struct DWVoice *voice = NULL;
	int volume = 0;
	
	assert_op(self->active, <, DWPLAYER_MAX_VOICES);
	voice = &self->voices[self->active];

	if (self->slower) {
		if (--(self->slowerCounter) == 0) {
			self->slowerCounter = 6;
			return;
		}
	}

	if ((self->delayCounter += self->delaySpeed) > 255) {
		self->delayCounter -= 256;
		return;
	}

	if (self->fadeSpeed) {
		if (--(self->fadeCounter) == 0) {
			self->fadeCounter = self->fadeSpeed;
			self->songvol--;
		}

		if (!self->songvol) {
			if (!self->super.super.loopSong) {
				CoreMixer_set_complete(&self->super.amiga->super, 1);
				//self->super.amiga->complete = 1;
				return;
			} else {
				CorePlayer_initialize(&self->super.super);
				//self->initialize();
			}
		}
	}

	if (self->wave) {
		if (self->waveDir) {
			self->super.amiga->memory[self->wavePos++] = self->waveRatePos;
			if (self->waveLen > 1) self->super.amiga->memory[self->wavePos++] = self->waveRatePos;
			if ((self->wavePos -= (self->waveLen << 1)) == self->waveLo) self->waveDir = 0;
		} else {
			self->super.amiga->memory[self->wavePos++] = self->waveRateNeg;
			if (self->waveLen > 1) self->super.amiga->memory[self->wavePos++] = self->waveRateNeg;
			if (self->wavePos == self->waveHi) self->waveDir = 1;
		}
	}

	while (voice) {
		chan = voice->channel;
		ByteArray_set_position(self->stream, voice->patternPos);
		sample = voice->sample;

		if (!voice->busy) {
			voice->busy = 1;

			if (sample->super.loopPtr < 0) {
				chan->pointer = self->super.amiga->loopPtr;
				assert_op(chan->pointer, <, (int) AMIGA_MAX_MEMORY);
				chan->length  = self->super.amiga->loopLen;
			} else {
				chan->pointer = sample->super.pointer + sample->super.loopPtr;
				assert_op(chan->pointer, <, (int) AMIGA_MAX_MEMORY);
				chan->length  = sample->super.length  - sample->super.loopPtr;
			}
		}

		if (--(voice->tick) == 0) {
			voice->flags = 0;
			loop = 1;

			while (loop > 0) {
				value = self->stream->readByte(self->stream);

				if (value < 0) {
					if (value >= -32) {
						voice->speed = self->super.super.speed * (value + 33);
					} else if (value >= self->com2) {
						value -= self->com2;
						assert_op(value, <, (int) self->vector_count_samples);
						voice->sample = sample = &self->samples[value];
					} else if (value >= self->com3) {
						pos = ByteArray_get_position(self->stream);

						ByteArray_set_position(self->stream, self->volseqs + ((value - self->com3) << 1));
						ByteArray_set_position(self->stream, self->base + self->stream->readUnsignedShort(self->stream));
						voice->volseqPtr = ByteArray_get_position(self->stream);

						ByteArray_set_position_rel(self->stream, -1);
						voice->volseqSpeed = self->stream->readUnsignedByte(self->stream);

						ByteArray_set_position(self->stream, pos);
					} else if (value >= self->com4) {
						pos = ByteArray_get_position(self->stream);

						ByteArray_set_position(self->stream, self->frqseqs + ((value - self->com4) << 1));
						voice->frqseqPtr = self->base + self->stream->readUnsignedShort(self->stream);
						voice->frqseqPos = voice->frqseqPtr;

						ByteArray_set_position(self->stream, pos);
					} else {
						switch (value) {
							case -128:
								ByteArray_set_position(self->stream, voice->trackPtr + voice->trackPos);
								if(self->readLen == 2) {
									value = ByteArray_readUnsignedShort(self->stream);
								} else if (self->readLen == 4) {
									value = ByteArray_readUnsignedInt(self->stream);
								}
								//value = self->stream[self->readMix]();

								if (value) {
									ByteArray_set_position(self->stream, self->base + value);
									voice->trackPos += self->readLen;
								} else {
									ByteArray_set_position(self->stream, voice->trackPtr);
									unsigned int temp;
									if(self->readLen == 2) {
										temp = ByteArray_readUnsignedShort(self->stream);
									} else if (self->readLen == 4) {
										temp = ByteArray_readUnsignedInt(self->stream);
									} else abort();
									ByteArray_set_position(self->stream, self->base + temp);
									//ByteArray_set_position(self->stream, self->base + self->stream[readMix]());
									voice->trackPos = self->readLen;

									if (!self->super.super.loopSong) {
										self->complete &= ~(voice->bitFlag);
										if (!self->complete) {
											//self->super.amiga->complete = 1;
											// FIXME: seems to set complete to the opposite of self->complete
											CoreMixer_set_complete(&self->super.amiga->super, 1);
										}
										
									}
								}
								break;
							case -127:
								if (self->super.super.variant > 0) voice->portaDelta = 0;
								voice->portaSpeed = self->stream->readByte(self->stream);
								voice->portaDelay = self->stream->readUnsignedByte(self->stream);
								voice->flags |= 2;
								break;
							case -126:
								voice->tick = voice->speed;
								voice->patternPos = ByteArray_get_position(self->stream);

								if (self->super.super.variant == 41) {
									voice->busy = 1;
									AmigaChannel_set_enabled(chan, 0);
									//chan->enabled = 0;
								} else {
									chan->pointer = self->super.amiga->loopPtr;
									assert_op(chan->pointer, <, (int) AMIGA_MAX_MEMORY);
									chan->length  = self->super.amiga->loopLen;
								}

								loop = 0;
								break;
							case -125:
								if (self->super.super.variant > 0) {
									voice->tick = voice->speed;
									voice->patternPos = ByteArray_get_position(self->stream);
									AmigaChannel_set_enabled(chan, 1);
									//chan->enabled = 1;
									loop = 0;
								}
								break;
							case -124:
								CoreMixer_set_complete(&self->super.amiga->super, 1);
								//self->super.amiga->complete = 1;
								break;
							case -123:
								if (self->super.super.variant > 0) self->transpose = self->stream->readByte(self->stream);
								break;
							case -122:
								voice->vibrato = -1;
								voice->vibratoSpeed = self->stream->readUnsignedByte(self->stream);
								voice->vibratoDepth = self->stream->readUnsignedByte(self->stream);
								voice->vibratoDelta = 0;
								break;
							case -121:
								voice->vibrato = 0;
								break;
							case -120:
								if (self->super.super.variant == 21) {
									voice->halve = 1;
								} else if (self->super.super.variant == 11) {
									self->fadeSpeed = self->stream->readUnsignedByte(self->stream);
								} else {
									voice->transpose = self->stream->readByte(self->stream);
								}
							break;
							case -119:
								if (self->super.super.variant == 21) {
									voice->halve = 0;
								} else {
									voice->trackPtr = self->base + self->stream->readUnsignedShort(self->stream);
									voice->trackPos = 0;
								}
								break;
							case -118:
								if (self->super.super.variant == 31) {
									self->delaySpeed = self->stream->readUnsignedByte(self->stream);
								} else {
									self->super.super.speed = self->stream->readUnsignedByte(self->stream);
								}
								break;
							case -117:
								self->fadeSpeed = self->stream->readUnsignedByte(self->stream);
								self->fadeCounter = self->fadeSpeed;
								break;
							case -116:
								value = self->stream->readUnsignedByte(self->stream);
								if (self->super.super.variant != 32) self->songvol = value;
								break;
							default:
								break;
						}
					}
				} else {
					voice->patternPos = ByteArray_get_position(self->stream);
					voice->note = (value += sample->finetune);
					voice->tick = voice->speed;
					voice->busy = 0;

					if (self->super.super.variant >= 20) {
						value = (value + self->transpose + voice->transpose) & 0xff;
						ByteArray_set_position(self->stream, voice->volseqPtr);
						volume = self->stream->readUnsignedByte(self->stream);

						voice->volseqPos = ByteArray_get_position(self->stream);
						voice->volseqCounter = voice->volseqSpeed;

						if (voice->halve) volume >>= 1;
						volume = (volume * self->songvol) >> 6;
					} else {
						volume = sample->super.volume;
					}

					chan->pointer = sample->super.pointer;
					assert_op(chan->pointer, <, (int) AMIGA_MAX_MEMORY);
					chan->length  = sample->super.length;
					AmigaChannel_set_volume(chan, volume);
					//chan->volume  = volume;

					ByteArray_set_position(self->stream, self->periods + (value << 1));
					value = (self->stream->readUnsignedShort(self->stream) * sample->relative) >> 10;
					if (self->super.super.variant < 10) voice->portaDelta = value;

					AmigaChannel_set_period(chan, value);
					//chan->period  = value;
					AmigaChannel_set_enabled(chan, 1);
					//chan->enabled = 1;
					loop = 0;
				}
			}
		} else if (voice->tick == 1) {
			if (self->super.super.variant < 30) {
				//chan->enabled = 0;
				AmigaChannel_set_enabled(chan, 0);
			} else {
				value = self->stream->readUnsignedByte(self->stream);

				if (value != 131) {
				if (self->super.super.variant < 40 || value < 224 || (self->stream->readUnsignedByte(self->stream) != 131))
					AmigaChannel_set_enabled(chan, 0);
					//chan->enabled = 0;
				}
			}
		} else if (self->super.super.variant == 0) {
			if (voice->flags & 2) {
				if (voice->portaDelay) {
					voice->portaDelay--;
				} else {
					voice->portaDelta -= voice->portaSpeed;
					AmigaChannel_set_period(chan, voice->portaDelta);
					//chan->period = voice->portaDelta;
				}
			}
		} else {
			ByteArray_set_position(self->stream, voice->frqseqPos);
			value = self->stream->readByte(self->stream);

			if (value < 0) {
				value &= 0x7f;
				ByteArray_set_position(self->stream, voice->frqseqPtr);
			}

			voice->frqseqPos = ByteArray_get_position(self->stream);

			value = (value + voice->note + self->transpose + voice->transpose) & 0xff;
			ByteArray_set_position(self->stream, self->periods + (value << 1));
			value = (self->stream->readUnsignedShort(self->stream) * sample->relative) >> 10;

			if (voice->flags & 2) {
				if (voice->portaDelay) {
					voice->portaDelay--;
				} else {
					voice->portaDelta += voice->portaSpeed;
					value -= voice->portaDelta;
				}
			}

			if (voice->vibrato) {
				if (voice->vibrato > 0) {
					voice->vibratoDelta -= voice->vibratoSpeed;
					if (!voice->vibratoDelta) voice->vibrato ^= 0x80000000;
				} else {
					voice->vibratoDelta += voice->vibratoSpeed;
					if (voice->vibratoDelta == voice->vibratoDepth) voice->vibrato ^= 0x80000000;
				}

				if (!voice->vibratoDelta) voice->vibrato ^= 1;

				if (voice->vibrato & 1) {
					value += voice->vibratoDelta;
				} else {
					value -= voice->vibratoDelta;
				}
			}

			AmigaChannel_set_period(chan, value);

			if (self->super.super.variant >= 20) {
				if (--(voice->volseqCounter) < 0) {
					ByteArray_set_position(self->stream, voice->volseqPos);
					volume = self->stream->readByte(self->stream);

					if (volume >= 0) voice->volseqPos = ByteArray_get_position(self->stream);
					voice->volseqCounter = voice->volseqSpeed;
					volume &= 0x7f;

					if (voice->halve) volume >>= 1;
					AmigaChannel_set_volume(chan, (volume * self->songvol) >> 6);
					//chan->volume = (volume * self->songvol) >> 6;
				}
			}
		}

		voice = voice->next;
	}
}

//override
void DWPlayer_initialize(struct DWPlayer* self) {
	int i = 0;
	int len = 0;
	struct DWVoice *voice = NULL;
	
	PFUNC();
	
	assert_op(self->active, <, DWPLAYER_MAX_VOICES);
	voice = &self->voices[self->active];
	CorePlayer_initialize(&self->super.super);
	//self->super->initialize();

	if (self->super.super.playSong >= DWPLAYER_MAX_SONGS) abort();
	self->song    = &self->songs[self->super.super.playSong];
	self->songvol = self->master;
	self->super.super.speed   = self->song->speed;

	self->transpose     = 0;
	self->slowerCounter = 6;
	self->delaySpeed    = self->song->delay;
	self->delayCounter  = 0;
	self->fadeSpeed     = 0;
	self->fadeCounter   = 0;
	
	if (self->wave) {
		self->waveDir = 0;
		self->wavePos = self->wave->super.pointer + self->waveCenter;
		i = self->wave->super.pointer;

		len = self->wavePos;
		for (; i < len; ++i) self->super.amiga->memory[i] = self->waveRateNeg;

		len += self->waveCenter;
		for (; i < len; ++i) self->super.amiga->memory[i] = self->waveRatePos;
	}

	while (voice) {
		DWVoice_initialize(voice);
		voice->channel = &self->super.amiga->channels[voice->index];
		voice->sample  = &self->samples[0];
		self->complete += voice->bitFlag;

		voice->trackPtr   = self->song->tracks[voice->index];
		voice->trackPos   = self->readLen;
		ByteArray_set_position(self->stream, voice->trackPtr);
		unsigned int temp;
		if(self->readLen == 2) {
			temp = ByteArray_readUnsignedShort(self->stream);
		} else if (self->readLen == 4) {
			temp = ByteArray_readUnsignedInt(self->stream);
		} else {
			abort();
		}
		//voice->patternPos = self->base + self->stream[readMix]();
		voice->patternPos = self->base + temp;

		if (self->frqseqs) {
			ByteArray_set_position(self->stream, self->frqseqs);
			voice->frqseqPtr = self->base + self->stream->readUnsignedShort(self->stream);
			voice->frqseqPos = voice->frqseqPtr;
		}
		assert_op(voice, !=, voice->next);
		voice = voice->next;
	}
}

//override
void DWPlayer_loader(struct DWPlayer* self, struct ByteArray *stream) {
	PFUNC();
	int flag = 0;
	int headers = 0;
	int i = 0;
	int index= 0;
	int info = 0;
	int lower = 0;
	int pos = 0; 
	struct DWSample *sample = NULL;
	int size = 10;
	struct DWSong *song = NULL;
	int total = 0;
	int value = 0;
	off_t savepos;

	self->master  = 64;
	//self->readMix = "readUnsignedShort";
	self->readLen = 2;
	self->super.super.variant = 0;
	self->vector_count_songs = 0;
	self->vector_count_samples = 0;

	if (stream->readUnsignedShort(stream) == 0x48e7) {                               //movem.l
		ByteArray_set_position(stream, 4);
		int insn = stream->readUnsignedShort(stream);
		if(insn != 0x4240) { //clr.w
			if (insn != 0x6100) return;                       //bsr.>w
			savepos = ByteArray_get_position(stream);
			ByteArray_set_position(stream, savepos + stream->readUnsignedShort(stream));
			self->super.super.variant = 30;
		}
	} else {
		ByteArray_set_position(stream, 0);
	}

	while (value != 0x4e75) {                                                 //rts
		value = stream->readUnsignedShort(stream);

		switch (value) {
			case 0x47fa:                                                          //lea x,a3
				self->base = ByteArray_get_position(stream) + stream->readShort(stream);
				break;
			case 0x6100:                                                          //bsr.w
				ByteArray_set_position_rel(stream, 2);
				info = ByteArray_get_position(stream);

				if (stream->readUnsignedShort(stream) == 0x6100)                           //bsr.w
				info = ByteArray_get_position(stream) + stream->readUnsignedShort(stream);
				break;
			case 0xc0fc:                                                          //mulu->w #x,d0
				size = stream->readUnsignedShort(stream);

				if (size == 18) {
					//self->readMix = "readUnsignedInt";
					self->readLen = 4;
				} else {
					self->super.super.variant = 10;
				}

				if (stream->readUnsignedShort(stream) == 0x41fa)
					headers = ByteArray_get_position(stream) + stream->readUnsignedShort(stream);

				if (stream->readUnsignedShort(stream) == 0x1230) flag = 1;
				break;
			case 0x1230:                                                          //move.b (a0,d0.w),d1
				ByteArray_set_position_rel(stream, -6);

				if (stream->readUnsignedShort(stream) == 0x41fa) {
					headers = ByteArray_get_position(stream) + stream->readUnsignedShort(stream);
					flag = 1;
				}

				ByteArray_set_position_rel(stream, 4);
				break;
			case 0xbe7c:                                                          //cmp.w #x,d7
				self->super.super.channels = stream->readUnsignedShort(stream);
				ByteArray_set_position_rel(stream, 2);

				if (stream->readUnsignedShort(stream) == 0x377c)
					self->master = stream->readUnsignedShort(stream);
				break;
			default:
				break;
		}

		if (ByteArray_bytesAvailable(stream) < 20) return;
	}

	index = ByteArray_get_position(stream);
	//songs = new Vector.<DWSong>();
	lower = 0x7fffffff;
	total = 0;
	ByteArray_set_position(stream, headers);

	while (1) {
		//song = DWSong_new();
		if(self->vector_count_songs >= DWPLAYER_MAX_SONGS) abort();
		song = &self->songs[self->vector_count_songs];
		DWSong_ctor(song);
		//song->tracks = new Vector.<int>(channels, true);
		//song->tracks = new Vector.<int>(channels, true);

		if (flag) {
			song->speed = stream->readUnsignedByte(stream);
			song->delay = stream->readUnsignedByte(stream);
		} else {
			song->speed = stream->readUnsignedShort(stream);
		}

		if (song->speed > 255) break;

		for (i = 0; i < self->super.super.channels; ++i) {
			unsigned int temp;
			if(self->readLen == 2) {
				temp = ByteArray_readUnsignedShort(stream);
			} else if (self->readLen == 4) {
				temp = ByteArray_readUnsignedInt(stream);
			} else abort();
			//value = self->base + stream[readMix]();
			value = self->base + temp;
			if (value < lower) lower = value;
			if(i >= DWSONG_MAXTRACKS) abort();
			song->tracks[i] = value;
			song->vector_count_tracks++;
		}

		//self->songs[total++] = song;
		total++;
		self->vector_count_songs++;
		if ((lower - ByteArray_get_position(stream)) < size) break;
	}

	if (!total) return;

	//self->songs->fixed = true;
	//self->super.super.lastSong = self->songs->length - 1;
	if(self->vector_count_songs == 0) abort();
	self->super.super.lastSong = self->vector_count_songs - 1;

	ByteArray_set_position(stream, info);
	if (stream->readUnsignedShort(stream) != 0x4a2b) return;                         //tst->b x(a3)
	headers = size = 0;
	self->wave = null;

	while (value != 0x4e75) {                                                 //rts
		value = stream->readUnsignedShort(stream);

		switch (value) {
			case 0x4bfa:
				if (headers) break;
				info = ByteArray_get_position(stream) + stream->readShort(stream);
				ByteArray_set_position_rel(stream, 1);
				total = stream->readUnsignedByte(stream);

				ByteArray_set_position_rel(stream, -10);
				value = stream->readUnsignedShort(stream);
				pos = ByteArray_get_position(stream);

				if (value == 0x41fa || value == 0x207a) {                           //lea x,a0 | movea->l x,a0
					headers = ByteArray_get_position(stream) + stream->readUnsignedShort(stream);
				} else if (value == 0xd0fc) {                                       //adda->w #x,a0
					headers = (64 + stream->readUnsignedShort(stream));
					ByteArray_set_position_rel(stream, -18);
					headers += (ByteArray_get_position(stream) + stream->readUnsignedShort(stream));
				}

				ByteArray_set_position(stream, pos);
				break;
			case 0x84c3:                                                          //divu->w d3,d2
				if (size) break;
				ByteArray_set_position_rel(stream, 4);
				value = stream->readUnsignedShort(stream);

				if (value == 0xdafc) {                                              //adda->w #x,a5
					size = stream->readUnsignedShort(stream);
				} else if (value == 0xdbfc) {                                       //adda->l #x,a5
					size = stream->readUnsignedInt(stream);
				}

				if (size == 12 && self->super.super.variant < 30) self->super.super.variant = 20;

				pos = ByteArray_get_position(stream);
				
				++total;
				assert_op(total, <=, DWPLAYER_MAX_SAMPLES);
				self->vector_count_samples = total;
				//samples = new Vector.<DWSample>(++total, true);
				ByteArray_set_position(stream, headers);
				/* 
				sample
				length | relative | sample_data | 
				U32    | U16      | U8 x length |
				
				info
				??? | loopPtr |
				x32 | S32 ?   |
				 */
				for (i = 0; i < total; ++i) {
					//sample = DWSample_new();
					sample = &self->samples[i];
					DWSample_ctor(sample);
					
					sample->super.length   = stream->readUnsignedInt(stream);
					sample->relative = 3579545 / stream->readUnsignedShort(stream);
					//sample->super.pointer  = self->super.amiga->store(stream, sample->length);
					sample->super.pointer  = Amiga_store(self->super.amiga, stream, sample->super.length, -1);

					value = ByteArray_get_position(stream);
					ByteArray_set_position(stream, info + (i * size) + 4);
					sample->super.loopPtr = stream->readInt(stream);
					/* 
					 FIXME: there are a lot of mods that have sample->super.loopPtr
					 out of bounds, and it is later used to assign an array index into mem
					 however setting it to 0 causes mods to misplay. one of them is dogsofwar
					 they do not seem to trigger the code tho that assigns the array index.
					 only jaws.dw does. 
					 jaws.dw: either info or size is not calculated right, the values in info
					 look very random at this point
					*/
					//if(sample->super.loopPtr >= AMIGA_MAX_MEMORY) {
					//	sample->super.loopPtr = AMIGA_MAX_MEMORY - 64;
					//}
					/* jaws has another problem, the subsong 5/6 loops indefinitely, but shouldnt */
					//assert_op(sample->super.loopPtr, <, AMIGA_MAX_MEMORY);

					if (self->super.super.variant == 0) {
						ByteArray_set_position_rel(stream, 6);
						sample->super.volume = stream->readUnsignedShort(stream);
					} else if (self->super.super.variant == 10) {
						ByteArray_set_position_rel(stream, 4);
						sample->super.volume = stream->readUnsignedShort(stream);
						sample->finetune = stream->readByte(stream);
					}

					ByteArray_set_position(stream, value);
					//self->samples[i] = sample;
				}

				self->super.amiga->loopLen = 64;
				ByteArray_set_length(stream, headers);
				//stream->length = headers;
				ByteArray_set_position(stream, pos);
				break;
			case 0x207a:                                                          //movea->l x,a0
				value = ByteArray_get_position(stream) + stream->readShort(stream);

				if (stream->readUnsignedShort(stream) != 0x323c) {                         //move->w #x,d1
					ByteArray_set_position_rel(stream, -2);
					break;
				}
				unsigned int temp = (value - info) / size;
				assert_op(temp, <, self->vector_count_samples);
				self->wave = &self->samples[temp];
				self->waveCenter = (stream->readUnsignedShort(stream) + 1) << 1;

				ByteArray_set_position_rel(stream, 2);
				self->waveRateNeg = stream->readByte(stream);
				ByteArray_set_position_rel(stream, 12);
				self->waveRatePos = stream->readByte(stream);
				break;
			case 0x046b:                                                          //subi->w #x,x(a3)
			case 0x066b:                                                          //addi->w #x,x(a3)
				total = stream->readUnsignedShort(stream);
				unsigned int temp2;
				temp2 = (stream->readUnsignedShort(stream) - info) / size;
				assert_op(temp2, <, self->vector_count_samples);
				sample = &self->samples[temp2];

				if (value == 0x066b) {
					sample->relative += total;
				} else {
					sample->relative -= total;
				}
				break;
			default:
				break;
		}
	}

	if (!self->vector_count_samples) return;
	ByteArray_set_position(stream, index);

	self->periods = 0;
	self->frqseqs = 0;
	self->volseqs = 0;
	self->slower  = 0;

	self->com2 = 0xb0;
	self->com3 = 0xa0;
	self->com4 = 0x90;

	while (ByteArray_bytesAvailable(stream) > 16) {
		value = stream->readUnsignedShort(stream);

		switch (value) {
			case 0x47fa:                                                          //lea x,a3
				ByteArray_set_position_rel(stream, 2);
				if (stream->readUnsignedShort(stream) != 0x4a2b) break;                    //tst->b x(a3)

				pos = ByteArray_get_position(stream);
				ByteArray_set_position_rel(stream, 4);
				value = stream->readUnsignedShort(stream);

				if (value == 0x103a) {                                              //move->b x,d0
					ByteArray_set_position_rel(stream, 4);

					if (stream->readUnsignedShort(stream) == 0xc0fc) {                       //mulu->w #x,d0
						value = stream->readUnsignedShort(stream);
						//total = self->songs->length;
						total = self->vector_count_songs;
						for (i = 0; i < total; ++i) self->songs[i].delay *= value;
						ByteArray_set_position_rel(stream, 6);
					}
				} else if (value == 0x532b) {                                       //subq->b #x,x(a3)
					ByteArray_set_position_rel(stream, -8);
				}

				value = stream->readUnsignedShort(stream);

				if (value == 0x4a2b) {                                              //tst->b x(a3)
					ByteArray_set_position(stream, self->base + stream->readUnsignedShort(stream));
					self->slower = stream->readByte(stream);
				}

				ByteArray_set_position(stream, pos);
				break;
			case 0x0c6b:                                                          //cmpi->w #x,x(a3)
				ByteArray_set_position_rel(stream, -6);
				value = stream->readUnsignedShort(stream);

				if (value == 0x546b || value == 0x526b) {                           //addq->w #2,x(a3) | addq->w #1,x(a3)
					ByteArray_set_position_rel(stream, 4);
					self->waveHi = self->wave->super.pointer + stream->readUnsignedShort(stream);
				} else if (value == 0x556b || value == 0x536b) {                    //subq->w #2,x(a3) | subq->w #1,x(a3)
					ByteArray_set_position_rel(stream, 4);
					self->waveLo = self->wave->super.pointer + stream->readUnsignedShort(stream);
				}

				self->waveLen = (value < 0x546b) ? 1 : 2;
				break;
			case 0x7e00:                                                          //moveq #0,d7
			case 0x7e01:                                                          //moveq #1,d7
			case 0x7e02:                                                          //moveq #2,d7
			case 0x7e03:                                                          //moveq #3,d7
				self->active = value & 0xf;
				total = self->super.super.channels - 1;
				assert_op(total, <, DWPLAYER_MAX_VOICES);

				if (self->active) {
					self->voices[0].next = null;
					for (i = total; i > 0; i--) self->voices[i].next = &self->voices[i - 1];
				} else {
					self->voices[total].next = null;
					for (i = 0; i < total;i++) self->voices[i].next = &self->voices[i + 1];
				}
				break;
			case 0x0c68:                                                          //cmpi->w #x,x(a0)
				ByteArray_set_position_rel(stream, 22);
				if (stream->readUnsignedShort(stream) == 0x0c11) self->super.super.variant = 40;
				break;
			case 0x322d:                                                          //move->w x(a5),d1
				pos = ByteArray_get_position(stream);
				value = stream->readUnsignedShort(stream);

				if (value == 0x000a || value == 0x000c) {                           //10 | 12
					ByteArray_set_position_rel(stream, -8);

					if (stream->readUnsignedShort(stream) == 0x45fa) {                        //lea x,a2
						self->periods = ByteArray_get_position(stream) + stream->readUnsignedShort(stream);
					}
				}

				ByteArray_set_position(stream, pos + 2);
				break;
			case 0x0400:                                                          //subi->b #x,d0
			case 0x0440:                                                          //subi->w #x,d0
			case 0x0600:                                                          //addi->b #x,d0
				value = stream->readUnsignedShort(stream);

				if (value == 0x00c0 || value == 0x0040) {                           //192 | 64
					self->com2 = 0xc0;
					self->com3 = 0xb0;
					self->com4 = 0xa0;
				} else if (value == self->com3) {
					ByteArray_set_position_rel(stream, 2);

					if (stream->readUnsignedShort(stream) == 0x45fa) {                       //lea x,a2
						self->volseqs = ByteArray_get_position(stream) + stream->readUnsignedShort(stream);
						if (self->super.super.variant < 40) self->super.super.variant = 30;
					}
				} else if (value == self->com4) {
					ByteArray_set_position_rel(stream, 2);

					if (stream->readUnsignedShort(stream) == 0x45fa)                         //lea x,a2
						self->frqseqs = ByteArray_get_position(stream) + stream->readUnsignedShort(stream);
				}
				break;
			case 0x4ef3:                                                          //jmp (a3,a2.w)
				ByteArray_set_position_rel(stream, 2);
				// FIXME forgot break ?
			case 0x4ed2:                                                          //jmp a2
				lower = ByteArray_get_position(stream);
				ByteArray_set_position_rel(stream, -10);
				savepos = ByteArray_get_position(stream);
				ByteArray_set_position(stream, savepos + stream->readUnsignedShort(stream));
				pos = ByteArray_get_position(stream);                                //jump table address

				ByteArray_set_position(stream, pos + 2);                               //effect -126
				ByteArray_set_position(stream, self->base + stream->readUnsignedShort(stream) + 10);
				if (stream->readUnsignedShort(stream) == 0x4a14) self->super.super.variant = 41;             //tst->b (a4)

				ByteArray_set_position(stream, pos + 16);                                         //effect -120
				value = self->base + stream->readUnsignedShort(stream);

				if (value > lower && value < pos) {
					ByteArray_set_position(stream, value);
					value = stream->readUnsignedShort(stream);

					if (value == 0x50e8) {                                            //st x(a0)
						self->super.super.variant = 21;
					} else if (value == 0x1759) {                                     //move->b (a1)+,x(a3)
						self->super.super.variant = 11;
					}
				}

				ByteArray_set_position(stream, pos + 20);                                         //effect -118
				value = self->base + stream->readUnsignedShort(stream);

				if (value > lower && value < pos) {
					ByteArray_set_position(stream, value + 2);
					if (stream->readUnsignedShort(stream) != 0x4880) self->super.super.variant = 31;           //ext->w d0
				}

				ByteArray_set_position(stream, pos + 26);                                         //effect -115
				value = stream->readUnsignedShort(stream);
				if (value > lower && value < pos) self->super.super.variant = 32;

				if (self->frqseqs) ByteArray_set_position(stream, ByteArray_get_length(stream));
				break;
			default:
				break;
		}
	}

	if (!self->periods) return;
	self->com2 -= 256;
	self->com3 -= 256;
	self->com4 -= 256;

	self->stream = stream;
	self->super.super.version = 1;
}
