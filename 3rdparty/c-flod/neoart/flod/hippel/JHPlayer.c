/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.0 - 2012/03/11

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "JHPlayer.h"
#include "../flod_internal.h"

static const unsigned short PERIODS[] = {
        1712,1616,1524,1440,1356,1280,1208,1140,1076,1016,
         960, 906, 856, 808, 762, 720, 678, 640, 604, 570,
         538, 508, 480, 453, 428, 404, 381, 360, 339, 320,
         302, 285, 269, 254, 240, 226, 214, 202, 190, 180,
         170, 160, 151, 143, 135, 127, 120, 113, 113, 113,
         113, 113, 113, 113, 113, 113, 113, 113, 113, 113,
        3424,3232,3048,2880,2712,2560,2416,2280,2152,2032,
        1920,1812,6848,6464,6096,5760,5424,5120,4832,4560,
        4304,4064,3840,3624,
};

void JHPlayer_loader(struct JHPlayer* self, struct ByteArray *stream);
void JHPlayer_initialize(struct JHPlayer* self);
void JHPlayer_process(struct JHPlayer* self);

void JHPlayer_defaults(struct JHPlayer* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void JHPlayer_ctor(struct JHPlayer* self, struct Amiga *amiga) {
	CLASS_CTOR_DEF(JHPlayer);
	// original constructor code goes here
	AmigaPlayer_ctor(&self->super, amiga);
	unsigned i = 0;
	for(; i < JHPLAYER_MAX_VOICES; i++) {
		JHVoice_ctor(&self->voices[i], i);
		if(i) self->voices[i - 1].next = &self->voices[i];
	}
	
	//vtable
	self->super.super.loader = JHPlayer_loader;
	self->super.super.process = JHPlayer_process;
	self->super.super.initialize = JHPlayer_initialize;
	
	self->super.super.min_filesize = 8;
}

struct JHPlayer* JHPlayer_new(struct Amiga *amiga) {
	CLASS_NEW_BODY(JHPlayer, amiga);
}

struct ByteArray fake_stream;
char fake_stream_buffer[32];

static void fakeByteArray_readMultiByte(struct ByteArray* self, char* buffer, size_t len) {
	if(self->pos < 10) {
		assert_op(self->pos + len, <, sizeof(fake_stream_buffer));
		fake_stream.pos = self->pos;
		ByteArray_readMultiByte(&fake_stream, buffer, len);
		return;
	}
	ByteArray_readMultiByte(self, buffer, len);
}

//override
void JHPlayer_process(struct JHPlayer* self) {
	struct AmigaChannel *chan = 0;
	int loop = 0;
	int period = 0;
	int pos1 = 0;
	int pos2 = 0;
	struct AmigaSample *sample = 0;
	int value = 0;
	struct JHVoice *voice = &self->voices[0];
	
	if (--(self->super.super.tick) == 0) {
		self->super.super.tick = self->super.super.speed;

		while (voice) {
			chan = voice->channel;

			if (self->coso) {
				if (--voice->cosoCounter < 0) {
					voice->cosoCounter = voice->cosoSpeed;

					do {
						self->stream->set_position(self->stream, voice->patternPos);

						do {
							loop = 0;
							value = self->stream->readByte(self->stream);

							if (value == -1) {
								if (voice->trackPos == self->song->length) {
									voice->trackPos = 0;
									CoreMixer_set_complete(&self->super.amiga->super, 1);
								}

								self->stream->set_position(self->stream,  voice->trackPtr + voice->trackPos);
								value = self->stream->readUnsignedByte(self->stream);
								voice->trackTransp = self->stream->readByte(self->stream);
								unsigned idxx = ByteArray_get_position(self->stream);
								pos1 = ByteArray_getUnsignedByte(self->stream, idxx);

								if ((self->super.super.variant > 3) && (pos1 > 127)) {
									pos2 = (pos1 >> 4) & 15;
									pos1 &= 15;

									if (pos2 == 15) {
										pos2 = 100;

										if (pos1) {
											pos2 = (15 - pos1) + 1;
											pos2 <<= 1;
											pos1 = pos2;
											pos2 <<= 1;
											pos2 += pos1;
										}

										voice->volFade = pos2;
									} else if (pos2 == 8) {
										CoreMixer_set_complete(&self->super.amiga->super, 1);
									} else if (pos2 == 14) {
										self->super.super.speed = pos1;
									}
								} else {
									voice->volTransp = self->stream->readByte(self->stream);
								}

								self->stream->set_position(self->stream,  self->patterns + (value << 1));
								voice->patternPos = self->stream->readUnsignedShort(self->stream);
								voice->trackPos += 12;
								loop = 1;
							} else if (value == -2) {
								voice->cosoCounter = voice->cosoSpeed = self->stream->readUnsignedByte(self->stream);
								loop = 3;
							} else if (value == -3) {
								voice->cosoCounter = voice->cosoSpeed = self->stream->readUnsignedByte(self->stream);
								voice->patternPos = ByteArray_get_position(self->stream);
							} else {
								voice->note = value;
								voice->info = self->stream->readByte(self->stream);

								if (voice->info & 224) voice->infoPrev = self->stream->readByte(self->stream);

								voice->patternPos = ByteArray_get_position(self->stream);
								voice->portaDelta = 0;

								if (value >= 0) {
									if (self->super.super.variant == 1) AmigaChannel_set_enabled(chan, 0);
									value = (voice->info & 31) + voice->volTransp;
									self->stream->set_position(self->stream,  self->volseqs + (value << 1));
									self->stream->set_position(self->stream,  self->stream->readUnsignedShort(self->stream));

									voice->volCounter = voice->volSpeed = self->stream->readUnsignedByte(self->stream);
									voice->volSustain = 0;
									value = self->stream->readByte(self->stream);

									voice->vibSpeed = self->stream->readByte(self->stream);
									voice->vibrato  = 64;
									voice->vibDepth = voice->vibDelta = self->stream->readByte(self->stream);
									voice->vibDelay = self->stream->readUnsignedByte(self->stream);

									voice->volseqPtr = ByteArray_get_position(self->stream);
									voice->volseqPos = 0;

									if (value != -128) {
										if (self->super.super.variant > 1 && (voice->info & 64)) value = voice->infoPrev;
										int idxx = self->frqseqs + (value << 1);
										self->stream->set_position(self->stream,  idxx);

										voice->frqseqPtr = self->stream->readUnsignedShort(self->stream);
										voice->frqseqPos = 0;

										voice->tick = 0;
									}
								}
							}
						} while (loop > 2);
					} while (loop > 0);
				}
			} else {
				self->stream->set_position(self->stream,  voice->patternPtr + voice->patternPos);
				value = self->stream->readByte(self->stream);

				if (voice->patternPos == self->patternLen || (value & 127) == 1) {
					if (voice->trackPos == self->song->length) {
						voice->trackPos = 0;
						CoreMixer_set_complete(&self->super.amiga->super, 1);
					}

					self->stream->set_position(self->stream,  voice->trackPtr + voice->trackPos);
					value = self->stream->readUnsignedByte(self->stream);
					voice->trackTransp = self->stream->readByte(self->stream);
					voice->volTransp = self->stream->readByte(self->stream);

					if (voice->volTransp == -128) CoreMixer_set_complete(&self->super.amiga->super, 1);

					voice->patternPtr = self->patterns + (value * self->patternLen);
					voice->patternPos = 0;
					voice->trackPos += 12;

					self->stream->set_position(self->stream,  voice->patternPtr);
					value = self->stream->readByte(self->stream);
				}

				if (value & 127) {
					voice->note = value & 127;
					voice->portaDelta = 0;

					pos1 = ByteArray_get_position(self->stream);
					if (!voice->patternPos) self->stream->set_position_rel(self->stream, self->patternLen);
					self->stream->set_position_rel(self->stream,  -2);

					voice->infoPrev = self->stream->readByte(self->stream);
					self->stream->set_position(self->stream,  pos1);
					voice->info = self->stream->readByte(self->stream);

					if (value >= 0) {
						if (self->super.super.variant == 1) AmigaChannel_set_enabled(chan, 0);
						value = (voice->info & 31) + voice->volTransp;
						self->stream->set_position(self->stream,  self->volseqs + (value << 6));

						voice->volCounter = voice->volSpeed = self->stream->readUnsignedByte(self->stream);
						voice->volSustain = 0;
						value = self->stream->readByte(self->stream);

						voice->vibSpeed = self->stream->readByte(self->stream);
						voice->vibrato  = 64;
						voice->vibDepth = voice->vibDelta = self->stream->readByte(self->stream);
						voice->vibDelay = self->stream->readUnsignedByte(self->stream);

						voice->volseqPtr = ByteArray_get_position(self->stream);
						voice->volseqPos = 0;

						if (self->super.super.variant > 1 && (voice->info & 64)) value = voice->infoPrev;

						voice->frqseqPtr = self->frqseqs + (value << 6);
						voice->frqseqPos = 0;

						voice->tick = 0;
					}
				}
				voice->patternPos += 2;
			}
			voice = voice->next;
		}
		voice = &self->voices[0];
	}

	while (voice) {
		chan = voice->channel;
		voice->enabled = 0;

		do {
			loop = 0;

			if (voice->tick) {
			voice->tick--;
		} else {
			self->stream->set_position(self->stream,  voice->frqseqPtr + voice->frqseqPos);

			do {
				value = self->stream->readByte(self->stream);
				if (value == -31) break;
				loop = 3;

				if (self->super.super.variant == 3 && self->coso) {
					if (value == -27) {
						value = -30;
					} else if (value == -26) {
						value = -28;
					}
				}

				switch (value) {
					case -32:
						voice->frqseqPos = (self->stream->readUnsignedByte(self->stream) & 63);
						self->stream->set_position(self->stream,  voice->frqseqPtr + voice->frqseqPos);
						break;
					case -30:
					{
						unsigned idxx = self->stream->readUnsignedByte(self->stream);
						assert_op(idxx, <, JHPLAYER_MAX_SAMPLES);
						sample = &self->samples[idxx];
					}
						voice->sample = -1;

						voice->loopPtr = sample->loopPtr;
						voice->repeat  = sample->repeat;
						voice->enabled = 1;

						AmigaChannel_set_enabled(chan, 0);
						chan->pointer = sample->pointer;
						chan->length  = sample->length;

						voice->volseqPos  = 0;
						voice->volCounter = 1;
						voice->slide = 0;
						voice->frqseqPos += 2;
						break;
					case -29:
						voice->vibSpeed = self->stream->readByte(self->stream);
						voice->vibDepth = self->stream->readByte(self->stream);
						voice->frqseqPos += 3;
						break;
					case -28:
					{
						unsigned idxx = self->stream->readUnsignedByte(self->stream);
						assert_op(idxx, <, JHPLAYER_MAX_SAMPLES);
						
						sample = &self->samples[idxx];
					}
						voice->loopPtr = sample->loopPtr;
						voice->repeat  = sample->repeat;

						chan->pointer = sample->pointer;
						chan->length  = sample->length;

						voice->slide = 0;
						voice->frqseqPos += 2;
						break;
					case -27:
						if (self->super.super.variant < 2) break;
						sample = &self->samples[self->stream->readUnsignedByte(self->stream)];
						AmigaChannel_set_enabled(chan, 0);
						voice->enabled = 1;

						if (self->super.super.variant == 2) {
							pos1 = self->stream->readUnsignedByte(self->stream) * sample->length;

							voice->loopPtr = sample->loopPtr + pos1;
							voice->repeat  = sample->repeat;

							chan->pointer = sample->pointer + pos1;
							chan->length  = sample->length;

							voice->frqseqPos += 3;
						} else {
							voice->sldPointer = sample->pointer;
							voice->sldEnd = sample->pointer + sample->length;
							value = self->stream->readUnsignedShort(self->stream);

							if (value == 0xffff) {
								voice->sldLoopPtr = sample->length;
							} else {
								voice->sldLoopPtr = value << 1;
							}

							voice->sldLen     = self->stream->readUnsignedShort(self->stream) << 1;
							voice->sldDelta   = self->stream->readShort(self->stream) << 1;
							voice->sldActive  = 0;
							voice->sldCounter = 0;
							voice->sldSpeed   = self->stream->readUnsignedByte(self->stream);
							voice->slide      = 1;
							voice->sldDone    = 0;

							voice->frqseqPos += 9;
						}
						voice->volseqPos  = 0;
						voice->volCounter = 1;
						break;
					case -26:
						if (self->super.super.variant < 3) break;

						voice->sldLen     = self->stream->readUnsignedShort(self->stream) << 1;
						voice->sldDelta   = self->stream->readShort(self->stream) << 1;
						voice->sldActive  = 0;
						voice->sldCounter = 0;
						voice->sldSpeed   = self->stream->readUnsignedByte(self->stream);
						voice->sldDone    = 0;

						voice->frqseqPos += 6;
						break;
					case -25:
						if (self->super.super.variant == 1) {
							voice->frqseqPtr = self->frqseqs + (self->stream->readUnsignedByte(self->stream) << 6);
							voice->frqseqPos = 0;

							self->stream->set_position(self->stream,  voice->frqseqPtr);
							loop = 3;
						} else {
							value = self->stream->readUnsignedByte(self->stream);

							if (value != voice->sample) {
								assert_op(value, <, JHPLAYER_MAX_SAMPLES);
								sample = &self->samples[value];
								voice->sample = value;

								voice->loopPtr = sample->loopPtr;
								voice->repeat  = sample->repeat;
								voice->enabled = 1;

								AmigaChannel_set_enabled(chan, 0);
								chan->pointer = sample->pointer;
								chan->length  = sample->length;
							}

							voice->volseqPos  = 0;
							voice->volCounter = 1;
							voice->slide = 0;
							voice->frqseqPos += 2;
						}
						break;
					case -24:
						voice->tick = self->stream->readUnsignedByte(self->stream);
						voice->frqseqPos += 2;
						loop = 1;
						break;
					case -23:
						if (self->super.super.variant < 2) break;
						unsigned idxx = self->stream->readUnsignedByte(self->stream);
						assert_op(idxx, <, JHPLAYER_MAX_SAMPLES);
						sample = &self->samples[idxx];
						voice->sample = -1;
						voice->enabled = 1;

						pos2 = self->stream->readUnsignedByte(self->stream);
						pos1 = ByteArray_get_position(self->stream);
						AmigaChannel_set_enabled(chan, 0);

						self->stream->set_position(self->stream,  self->samplesData + sample->pointer + 4);
						
						value = (self->stream->readUnsignedShort(self->stream) * 24) + (self->stream->readUnsignedShort(self->stream) << 2);
						self->stream->set_position_rel(self->stream, pos2 * 24);

						voice->loopPtr = self->stream->readUnsignedInt(self->stream) & 0xfffffffe;
						chan->length   = (self->stream->readUnsignedInt(self->stream) & 0xfffffffe) - voice->loopPtr;
						voice->loopPtr += (sample->pointer + value + 8);
						chan->pointer  = voice->loopPtr;
						voice->repeat  = 2;

						self->stream->set_position(self->stream,  pos1);
						pos1 = voice->loopPtr + 1;
						assert_op(pos1, <, AMIGA_MAX_MEMORY);
						assert_op(voice->loopPtr, <, AMIGA_MAX_MEMORY);
						self->super.amiga->memory[pos1] = self->super.amiga->memory[voice->loopPtr];

						voice->volseqPos  = 0;
						voice->volCounter = 1;
						voice->slide = 0;
						voice->frqseqPos += 3;
						break;
					default:
						voice->transpose = value;
						voice->frqseqPos++;
						loop = 0;
				}
			} while (loop > 2);
		}
		} while (loop > 0);

		if (voice->slide) {
			if (!voice->sldDone) {
				if (--voice->sldCounter < 0) {
					voice->sldCounter = voice->sldSpeed;

					if (voice->sldActive) {
						value = voice->sldLoopPtr + voice->sldDelta;

						if (value < 0) {
							voice->sldDone = 1;
							value = voice->sldLoopPtr - voice->sldDelta;
						} else {
							pos1 = voice->sldPointer + voice->sldLen + value;

							if (pos1 > voice->sldEnd) {
								voice->sldDone = 1;
								value = voice->sldLoopPtr - voice->sldDelta;
							}
						}
						voice->sldLoopPtr = value;
					} else {
						voice->sldActive = 1;
					}

					voice->loopPtr = voice->sldPointer + voice->sldLoopPtr;
					voice->repeat  = voice->sldLen;
					chan->pointer  = voice->loopPtr;
					chan->length   = voice->repeat;
				}
			}
		}

		do {
			loop = 0;

			if (voice->volSustain) {
				voice->volSustain--;
			} else {
				if (--voice->volCounter) break;
				voice->volCounter = voice->volSpeed;

				do {
					self->stream->set_position(self->stream,  voice->volseqPtr + voice->volseqPos);
					value = self->stream->readByte(self->stream);
					if (value <= -25 && value >= -31) break;

					switch (value) {
						case -24:
							voice->volSustain = self->stream->readUnsignedByte(self->stream);
							voice->volseqPos += 2;
							loop = 1;
							break;
						case -32:
							voice->volseqPos = (self->stream->readUnsignedByte(self->stream) & 63) - 5;
							loop = 3;
							break;
						default:
							voice->volume = value;
							voice->volseqPos++;
							loop = 0;
					}
				} while (loop > 2);
			}
		} while (loop > 0);

		value = voice->transpose;
		if (value >= 0) value += (voice->note + voice->trackTransp);
		value &= 127;

		if (self->coso) {
			if (value > 83) value = 0;
			period = PERIODS[value];
			value <<= 1;
		} else {
			value <<= 1;
			self->stream->set_position(self->stream,  self->periods + value);
			period = self->stream->readUnsignedShort(self->stream);
		}

		if (voice->vibDelay) {
			voice->vibDelay--;
		} else {
			if (self->super.super.variant > 3) {
				if (voice->vibrato & 32) {
					value = voice->vibDelta + voice->vibSpeed;

					if (value > voice->vibDepth) {
						voice->vibrato &= ~32;
						value = voice->vibDepth;
					}
				} else {
					value = voice->vibDelta - voice->vibSpeed;

					if (value < 0) {
						voice->vibrato |= 32;
						value = 0;
					}
				}

				voice->vibDelta = value;
				value = (value - (voice->vibDepth >> 1)) * period;
				period += (value >> 10);
			} else if (self->super.super.variant > 2) {
				value = voice->vibSpeed;

				if (value < 0) {
					value &= 127;
					voice->vibrato ^= 1;
				}

				if (!(voice->vibrato & 1)) {
					if (voice->vibrato & 32) {
						voice->vibDelta += value;
						pos1 = voice->vibDepth << 1;

						if (voice->vibDelta > pos1) {
							voice->vibrato &= ~32;
							voice->vibDelta = pos1;
						}
					} else {
						voice->vibDelta -= value;

						if (voice->vibDelta < 0) {
							voice->vibrato |= 32;
							voice->vibDelta = 0;
						}
					}
				}

				period += (value - voice->vibDepth);
			} else {
				if (voice->vibrato >= 0 || !(voice->vibrato & 1)) {
					if (voice->vibrato & 32) {
						voice->vibDelta += voice->vibSpeed;
						pos1 = voice->vibDepth << 1;

						if (voice->vibDelta >= pos1) {
							voice->vibrato &= ~32;
							voice->vibDelta = pos1;
						}
					} else {
						voice->vibDelta -= voice->vibSpeed;

						if (voice->vibDelta < 0) {
							voice->vibrato |= 32;
							voice->vibDelta = 0;
						}
					}
				}

				pos1 = voice->vibDelta - voice->vibDepth;

				if (pos1) {
					value += 160;

					while (value < 256) {
						pos1 += pos1;
						value += 24;
					}

					period += pos1;
				}
			}
		}

		if (self->super.super.variant < 3) voice->vibrato ^= 1;

		if (voice->info & 32) {
			value = voice->infoPrev;

			if (self->super.super.variant > 3) {
				if (value < 0) {
					voice->portaDelta += (-value);
					value = voice->portaDelta * period;
					period += (value >> 10);
				} else {
					voice->portaDelta += value;
					value = voice->portaDelta * period;
					period -= (value >> 10);
				}
			} else {
				if (value < 0) {
					voice->portaDelta += (-value << 11);
					period += (voice->portaDelta >> 16);
				} else {
					voice->portaDelta += (value << 11);
					period -= (voice->portaDelta >> 16);
				}
			}
		}

		if (self->super.super.variant > 3) {
			value = (voice->volFade * voice->volume) / 100;
		} else {
			value = voice->volume;
		}

		AmigaChannel_set_period(chan, period);
		AmigaChannel_set_volume(chan, value);

		if (voice->enabled) {
			AmigaChannel_set_enabled(chan, 1);
			chan->pointer = voice->loopPtr;
			chan->length  = voice->repeat;
		}

		voice = voice->next;
	}
}

//override
void JHPlayer_initialize(struct JHPlayer* self) {
	struct JHVoice *voice = &self->voices[0];
	
	CorePlayer_initialize(&self->super.super);

	self->song  = &self->songs[self->super.super.playSong];
	self->super.super.speed = self->song->speed;
	self->super.super.tick  = (self->coso || self->super.super.variant > 1) ? 1 : self->super.super.speed;

	while (voice) {
		JHVoice_initialize(voice);
		assert_op(voice->index, <, AMIGA_MAX_CHANNELS);
		voice->channel = &self->super.amiga->channels[voice->index];
		voice->trackPtr = self->song->pointer + (voice->index * 3);

		if (self->coso) {
			voice->trackPos   = 0;
			voice->patternPos = 8;
		} else {
			self->stream->set_position(self->stream,  voice->trackPtr);
			voice->patternPtr = self->patterns + (self->stream->readUnsignedByte(self->stream) * self->patternLen);
			voice->trackTransp = self->stream->readByte(self->stream);
			voice->volTransp = self->stream->readByte(self->stream);

			voice->frqseqPtr = self->base;
			voice->volseqPtr = self->base;
		}

		voice = voice->next;
	}
}

//override
void JHPlayer_loader(struct JHPlayer* self, struct ByteArray *stream) {
	int headers = 0;
	int i = 0;
	int id = 0;
	int len = 0;
	int pos = 0;
	struct AmigaSample *sample = 0;
	struct JHSong *song = 0;
	int songsData = 0; 
	int tracks = 0; 
	int value = 0;
	char test[4];

	self->base = self->periods = 0;
	
	ByteArray_set_position(stream, 0);
	
	stream->readMultiByte(stream, test, 4);
	if(is_str(test, "COSO")) self->coso = 1;
	else {
		ByteArray_set_position(stream, 0);
		value = stream->readUnsignedShort(stream);
		if (value == 0x6000 || value == 0x6002 || value == 0x600e || value == 0x6016) ;
		else return;
	}
	
	ByteArray_set_position(stream, 0);
	
	//self->coso = int(stream->readMultiByte(stream, 4, ENCODING) == "COSO");
	value = 0;

	if (self->coso) {
		for (i = 0; i < 7; ++i) value += stream->readInt(stream);

		if (value == 16942) {
			ByteArray_set_position(stream,  47);
			value += stream->readUnsignedByte(stream);
		}

		switch (value) {
			case 22666:
			case 18842:
			case 30012:
			case 22466:
			case 3546:
				self->super.super.variant = 1;
				break;
			case 16948:
			case 18332:
			case 13698:
				self->super.super.variant = 2;
				break;
			case 18546:   //Wings of Death
			case 13926:
			case 8760:
			case 17242:
			case 11394:
			case 14494:
			case 14392:
			case 13576:  //Dragonflight
			case 6520:
				self->super.super.variant = 3;
				break;
			default:
				self->super.super.variant = 4;
		}

		self->super.super.version = 2;
		ByteArray_set_position(stream,  4);

		self->frqseqs     = stream->readUnsignedInt(stream);
		self->volseqs     = stream->readUnsignedInt(stream);
		self->patterns    = stream->readUnsignedInt(stream);
		tracks      = stream->readUnsignedInt(stream);
		songsData   = stream->readUnsignedInt(stream);
		headers     = stream->readUnsignedInt(stream);
		self->samplesData = stream->readUnsignedInt(stream);

		ByteArray_set_position(stream,  0);

		/* ugly hack:
		 * the original code wrote 10 bytes into the beginning of the stream.
		 * 
		 * since our stream is possibly a file, write access is not allowed.
		 * to circumvent the issue i see two possible fixes
		 * 
		 * 1) create "copy-on-write" in ByteArray. this is a clean design
		 *    but requires a second copy, which is wasteful.
		 *    also it would need an additional buffer or usage of malloc().
		 * 2) create a dummy overlay buffer for the first few bytes.
		 *    this is what i implemented below.
		 *    to make it possible, the vtable entries of 
		 *    the readfuncs is manipulated to use the overlay buffer.
		 *    since we can't put an additional argument in the function call,
		 *    the buffer and fakestream are global variables.
		 *    thus there can be no 2 hippel players be active at the same time
		 *    (not "threadsafe"). an additional disadvantage are a couple of
		 *    instructions wasted, when the COSO code gets used.
		 */
		
		/* we copy some bytes into the fake ByteArray, which will be accessed whenever 
		 * pos < 10, instead of stream. the first 10 bytes are overwritten. */
		
		ByteArray_ctor(&fake_stream);
		fake_stream.endian = stream->endian;
		
		/* copy slightly more then 10 bytes into the fake stream */
		ByteArray_open_mem(&fake_stream, fake_stream_buffer, sizeof(fake_stream_buffer));
		ByteArray_readBytes(stream, &fake_stream, 0, sizeof(fake_stream_buffer));

		/* overwrite the first 10 bytes with the needed crap */
		fake_stream.pos = 0;
		fake_stream.writeInt(&fake_stream, 0x1000000);
		fake_stream.writeInt(&fake_stream, 0xe1);
		fake_stream.writeShort(&fake_stream, 0xffff);
		
		stream->readMultiByte = fakeByteArray_readMultiByte;
		
		/* ugly hack done */

		len = ((self->samplesData - headers) / 10) - 1;
		self->super.super.lastSong = (headers - songsData) / 6;
	} else {
		while (stream->bytesAvailable(stream) > 12) {
			value = stream->readUnsignedShort(stream);

			switch (value) {
				case 0x0240:                                                        //andi->w #x,d0
					value = stream->readUnsignedShort(stream);

					if (value == 0x007f) {                                            //andi->w #$7f,d0
						ByteArray_set_position_rel(stream,  +2);
						self->periods = ByteArray_get_position(stream) + stream->readUnsignedShort(stream);
					}
					break;
				case 0x7002:                                                        //moveq #2,d0
				case 0x7003:                                                        //moveq #3,d0
					self->super.super.channels = value & 0xff;
					value = stream->readUnsignedShort(stream);
					if (value == 0x7600) value = stream->readUnsignedShort(stream);          //moveq #0,d3

					if (value == 0x41fa) {                                            //lea x,a0
						ByteArray_set_position_rel(stream,  +4);
						self->base = ByteArray_get_position(stream) + stream->readUnsignedShort(stream);
					}
					break;
				case 0x5446:                                                        //"TF"
					value = stream->readUnsignedShort(stream);

					if (value == 0x4d58) {                                            //"MX"
						id = ByteArray_get_position(stream) - 4;
						ByteArray_set_position(stream,  ByteArray_get_length(stream));
					}
					break;
				default:
					break;
			}
		}

		if (!id || !self->base || !self->periods) return;
		self->super.super.version = 1;

		ByteArray_set_position(stream,  id + 4);
		self->frqseqs = pos = id + 32;
		value = stream->readUnsignedShort(stream);
		self->volseqs = (pos += (++value << 6));

		value = stream->readUnsignedShort(stream);
		self->patterns = (pos += (++value << 6));
		value = stream->readUnsignedShort(stream);
		ByteArray_set_position_rel(stream,  +2);
		self->patternLen = stream->readUnsignedShort(stream);
		tracks = (pos += (++value * self->patternLen));

		ByteArray_set_position_rel(stream,  -4);
		value = stream->readUnsignedShort(stream);
		songsData = (pos += (++value * 12));

		ByteArray_set_position(stream,  id + 16);
		self->super.super.lastSong = stream->readUnsignedShort(stream);
		headers = (pos += (++(self->super.super.lastSong) * 6));

		len = stream->readUnsignedShort(stream);
		self->samplesData = pos + (len * 30);
	}

	ByteArray_set_position(stream,  headers);
	//self->samples = new Vector.<AmigaSample>(len, true);
	assert_op(len, <=, JHPLAYER_MAX_SAMPLES);
	value = 0;

	for (i = 0; i < len; ++i) {
		//sample = new AmigaSample();
		sample = &self->samples[i];
		AmigaSample_ctor(sample);
		
		if (!self->coso) {
			stream->readMultiByte(stream, self->sample_names[i], 18);
			sample->name = self->sample_names[i];
		}

		sample->pointer = stream->readUnsignedInt(stream);
		sample->length  = stream->readUnsignedShort(stream) << 1;
		if (!self->coso) sample->volume  = stream->readUnsignedShort(stream);
		sample->loopPtr = stream->readUnsignedShort(stream) + sample->pointer;
		sample->repeat  = stream->readUnsignedShort(stream) << 1;

		if (sample->loopPtr & 1) sample->loopPtr--;
		value += sample->length;
		//self->samples[i] = sample;
	}

	ByteArray_set_position(stream,  self->samplesData);
	Amiga_store(self->super.amiga, stream, value, -1);

	ByteArray_set_position(stream,  songsData);
	//self->songs = new Vector.<JHSong>();
	assert_op(self->super.super.lastSong, <=, JHPLAYER_MAX_SONGS);
	value = 0;

	for (i = 0; i < self->super.super.lastSong; ++i) {
		//song = new JHSong();
		song = &self->songs[i];
		JHSong_ctor(song);
		
		song->pointer = stream->readUnsignedShort(stream);
		song->length  = stream->readUnsignedShort(stream) - song->pointer + 1;
		song->speed   = stream->readUnsignedShort(stream);

		song->pointer = (song->pointer * 12) + tracks;
		song->length *= 12;
	}
	self->super.super.lastSong--;
	assert_op(self->super.super.lastSong, >=, 0);
	
	for(; self->super.super.lastSong; self->super.super.lastSong--)
		if(self->songs[self->super.super.lastSong].length > 12) break;

	if (!self->coso) {
		ByteArray_set_position(stream,  0);
		self->super.super.variant = 1;

		while (ByteArray_get_position(stream) < id) {
			value = stream->readUnsignedShort(stream);

			if (value == 0xb03c || value == 0x0c00) {                             //cmp->b #x,d0 | cmpi->b #x,d0
				value = stream->readUnsignedShort(stream);

				if (value == 0x00e5 || value == 0x00e6 || value == 0x00e9) {        //effects
					self->super.super.variant = 2;
					break;
				}
			} else if (value == 0x4efb) {                                         //jmp $(pc,d0.w)
				self->super.super.variant = 3;
				break;
			}
		}
	}

	self->stream = stream;
}

