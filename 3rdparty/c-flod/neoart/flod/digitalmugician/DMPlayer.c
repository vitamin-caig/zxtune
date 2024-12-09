/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.0 - 2012/03/10

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "DMPlayer.h"
#include "../flod_internal.h"
#include "DMPlayer_const.h"

static void tables(struct DMPlayer* self);
void DMPlayer_process(struct DMPlayer* self);
void DMPlayer_initialize(struct DMPlayer* self);
void DMPlayer_loader(struct DMPlayer* self, struct ByteArray *stream);

void DMPlayer_defaults(struct DMPlayer* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

// amiga default null
void DMPlayer_ctor(struct DMPlayer* self, struct Amiga *amiga) {
	CLASS_CTOR_DEF(DMPlayer);
	// original constructor code goes here
	//super(amiga);
	AmigaPlayer_ctor(&self->super, amiga);

	//songs     = new Vector.<DMSong>(8, true);
	//arpeggios = new Vector.<int>(256, true);
	//voices    = new Vector.<DMVoice>(7, true);
	unsigned i = 0;
	for(; i < DMPLAYER_MAX_VOICES; i++)
		DMVoice_ctor(&self->voices[i]);

	tables(self);
	
	//vtable
	self->super.super.process = DMPlayer_process;
	self->super.super.loader = DMPlayer_loader;
	self->super.super.initialize = DMPlayer_initialize;
	
	self->super.super.min_filesize = 204;
	
}

struct DMPlayer* DMPlayer_new(struct Amiga *amiga) {
	CLASS_NEW_BODY(DMPlayer, amiga);
}

//override
void DMPlayer_process(struct DMPlayer* self) {
	struct AmigaChannel *chan = 0;
	int dst = 0;
	int i = 0; 
	int idx = 0; 
	int j = 0; 
	int len = 0;
	signed char *memory = self->super.amiga->memory;
	//memory:Vector.<int> = amiga->memory;
	int r = 0; 
	struct AmigaRow *row = 0;
	int src1 = 0;
	int src2 = 0;
	struct DMSample *sample = 0;
	int value = 0;
	struct DMVoice *voice = 0;

	for (i = 0; i < self->numChannels; ++i) {
		voice = &self->voices[i];
		sample = voice->sample;

		if (i < 3 || self->numChannels == 4) {
			chan = voice->channel;
			assert_op((self->trackPos + i), <, DMSONG_MAX_TRACKS);
			if (self->stepEnd) voice->step = &self->song1->tracks[(self->trackPos + i)];

			if (sample->wave > 31) {
				chan->pointer = sample->super.loopPtr;
				chan->length  = sample->super.repeat;
			}
		} else {
			chan = &self->mixChannel;
			assert_op(self->trackPos + (i - 3), <, DMSONG_MAX_TRACKS);
			if (self->stepEnd) voice->step = &self->song2->tracks[(self->trackPos + (i - 3))];
		}

		if (self->patternEnd) {
			assert_op(voice->step->pattern + self->patternPos, <, DMPLAYER_MAX_PATTERNS);
			row = &self->patterns[voice->step->pattern + self->patternPos];

			if (row->note) {
				if (row->effect != 74) {
					voice->note = row->note;
					assert_op(row->sample, <, DMPLAYER_MAX_SAMPLES);
					if (row->sample) sample = voice->sample = &self->samples[row->sample];
				}
				voice->val1 = row->effect < 64 ? 1 : row->effect - 62;
				voice->val2 = row->param;
				idx = voice->step->transpose + sample->finetune;

				if (voice->val1 != 12) {
					voice->pitch = row->effect;

					if (voice->val1 == 1) {
						idx += voice->pitch;
						if (idx < 0) voice->period = 0;
						else voice->period = PERIODS[idx];
					}
				} else {
					voice->pitch = row->note;
					idx += voice->pitch;

					if (idx < 0) voice->period = 0;
						else voice->period = PERIODS[idx];
				}

				if (voice->val1 == 11) sample->arpeggio = voice->val2 & 7;

				if (voice->val1 != 12) {
					if (sample->wave > 31) {
						chan->pointer  = sample->super.pointer;
						chan->length   = sample->super.length;
						AmigaChannel_set_enabled(chan, 0);
						voice->mixPtr  = sample->super.pointer;
						voice->mixEnd  = sample->super.pointer + sample->super.length;
						voice->mixMute = 0;
					} else {
						dst = sample->wave << 7;
						chan->pointer = dst;
						chan->length  = sample->waveLen;
						if (voice->val1 != 10) AmigaChannel_set_enabled(chan, 0);

						if (self->numChannels == 4) {
							if (sample->effect != 0 && voice->val1 != 2 && voice->val1 != 4) {
								len  = dst + 128;
								src1 = sample->source1 << 7;
								assert_op(len, <=, AMIGA_MAX_MEMORY);
								for (j = dst; j < len; ++j) memory[j] = memory[src1++];
								// FIXME: we may have to increment vector_count_memory here

								sample->effectStep = 0;
								voice->effectCtr   = sample->effectSpeed;
							}
						}
					}
				}

				if (voice->val1 != 3 && voice->val1 != 4 && voice->val1 != 12) {
					voice->volumeCtr  = 1;
					voice->volumeStep = 0;
				}

				voice->arpeggioStep = 0;
				voice->pitchCtr     = sample->pitchDelay;
				voice->pitchStep    = 0;
				voice->portamento   = 0;
			}
		}

		switch (voice->val1) {
			case 0:
				break;
			case 5:   //pattern length
				value = voice->val2;
				if (value > 0 && value < 65) self->patternLen = value;
				break;
			case 6:   //song speed
				value  = voice->val2 & 15;
				value |= value << 4;
				if (voice->val2 == 0 || voice->val2 > 15) break;
				self->super.super.speed = value;
				break;
			case 7:   //led filter on
				self->super.amiga->filter->active = 1;
				break;
			case 8:   //led filter off
				self->super.amiga->filter->active = 0;
				break;
			case 13:  //shuffle
				voice->val1 = 0;
				value = voice->val2 & 0x0f;
				if (value == 0) break;
				value = voice->val2 & 0xf0;
				if (value == 0) break;
				self->super.super.speed = voice->val2;
				break;
			default:
				break;
		}
	}

	for (i = 0; i < self->numChannels; ++i) {
		voice  = &self->voices[i];
		sample = voice->sample;

		if (self->numChannels == 4) {
			chan = voice->channel;

			if (sample->wave < 32 && sample->effect && !sample->effectDone) {
				sample->effectDone = 1;

				if (voice->effectCtr) {
					voice->effectCtr--;
				} else {
					voice->effectCtr = sample->effectSpeed;
					dst = sample->wave << 7;

					switch (sample->effect) {
						case 1:   //filter
							assert_op(dst + 127, <, AMIGA_MAX_MEMORY);
							for (j = 0; j < 127; ++j) {
								value  = memory[dst];
								value += memory[dst + 1];
								memory[dst++] = value >> 1;
							}
							break;
						case 2:   //mixing
							src1 = sample->source1 << 7;
							src2 = sample->source2 << 7;
							idx  = sample->effectStep;
							len  = sample->waveLen;
							sample->effectStep = ++sample->effectStep & 127;
							assert_op(dst + len, <, AMIGA_MAX_MEMORY);
							// FIXME assert on the idx mem access
							for (j = 0; j < len; ++j) {
								value  = memory[src1++];
								value += memory[src2 + idx];
								memory[dst++] = value >> 1;
								idx = ++idx & 127;
							}
							break;
						case 3:   //scr left
							assert_op(dst + 127, <, AMIGA_MAX_MEMORY);
							value = memory[dst];
							for (j = 0; j < 127; ++j) memory[dst] = memory[++dst];
							memory[dst] = value;
							break;
						case 4:   //scr right
							dst += 127;
							assert_op(dst + 127, <, AMIGA_MAX_MEMORY);
							value = memory[dst];
							for (j = 0; j < 127; ++j) memory[dst] = memory[--dst];
							memory[dst] = value;
							break;
						case 5:   //upsample
							idx = value = dst;
							assert_op(dst + (64 * 2), <, AMIGA_MAX_MEMORY);
							assert_op(idx + 64, <, AMIGA_MAX_MEMORY);
							for (j = 0; j < 64; ++j) {
								memory[idx++] = memory[dst++];
								dst++;
							}
							idx = dst = value;
							idx += 64;
							assert_op(idx + 64, <, AMIGA_MAX_MEMORY);
							assert_op(dst + 64, <, AMIGA_MAX_MEMORY);
							for (j = 0; j < 64; ++j) memory[idx++] = memory[dst++];
							break;
						case 6:   //downsample
							src1 = dst + 64;
							dst += 128;
							assert_op(dst, <, AMIGA_MAX_MEMORY);
							assert_op(src1, <, AMIGA_MAX_MEMORY);
							for (j = 0; j < 64; ++j) {
								memory[--dst] = memory[--src1];
								memory[--dst] = memory[src1];
							}
							break;
						case 7:   //negate
							dst += sample->effectStep;
							assert_op(dst, <, AMIGA_MAX_MEMORY);
							memory[dst] = ~memory[dst] + 1;
							if (++sample->effectStep >= sample->waveLen) sample->effectStep = 0;
							break;
						case 8:   //madmix 1
							sample->effectStep = ++sample->effectStep & 127;
							src2 = (sample->source2 << 7) + sample->effectStep;
							assert_op(src2, <, AMIGA_MAX_MEMORY);
							idx  = memory[src2];
							len  = sample->waveLen;
							value = 3;
							
							assert_op(dst + (len * 1), <, AMIGA_MAX_MEMORY);
							for (j = 0; j < len; ++j) {
								src1 = memory[dst] + value;
								if (src1 < -128) src1 += 256;
								else if (src1 > 127) src1 -= 256;

								memory[dst++] = src1;
								value += idx;

								if (value < -128) value += 256;
								else if (value > 127) value -= 256;
							}
							break;
						case 9:   //addition
							src2 = sample->source2 << 7;
							len  = sample->waveLen;
							
							assert_op(dst + len, <, AMIGA_MAX_MEMORY);
							assert_op(src2 + len, <, AMIGA_MAX_MEMORY);
							for (j = 0; j < len; ++j) {
								value  = memory[src2++];
								value += memory[dst];
								if (value > 127) value -= 256;
								memory[dst++] = value;
							}
							break;
						case 10:  //filter 2
							assert_op(dst + 127, <, AMIGA_MAX_MEMORY);
							for (j = 0; j < 126; ++j) {
								value  = memory[dst++] * 3;
								value += memory[dst + 1];
								memory[dst] = value >> 2;
							}
							break;
						case 11:  //morphing
							src1 = sample->source1 << 7;
							src2 = sample->source2 << 7;
							len  = sample->waveLen;

							sample->effectStep = ++sample->effectStep & 127;
							value = sample->effectStep;
							if (value >= 64) value = 127 - value;
							idx = (value ^ 255) & 63;
							
							assert_op(dst + len, <, AMIGA_MAX_MEMORY);
							assert_op(src1 + len, <, AMIGA_MAX_MEMORY);
							assert_op(src2 + len, <, AMIGA_MAX_MEMORY);
							for (j = 0; j < len; ++j) {
								r  = memory[src1++] * value;
								r += memory[src2++] * idx;
								memory[dst++] = r >> 6;
							}
							break;
						case 12:  //morph f
							src1 = sample->source1 << 7;
							src2 = sample->source2 << 7;
							len  = sample->waveLen;

							sample->effectStep = ++sample->effectStep & 31;
							value = sample->effectStep;
							if (value >= 16) value = 31 - value;
							idx = (value ^ 255) & 15;

							assert_op(dst + len, <, AMIGA_MAX_MEMORY);
							assert_op(src1 + len, <, AMIGA_MAX_MEMORY);
							assert_op(src2 + len, <, AMIGA_MAX_MEMORY);

							for (j = 0; j < len; ++j) {
								r  = memory[src1++] * value;
								r += memory[src2++] * idx;
								memory[dst++] = r >> 4;
							}
							break;
						case 13:  //filter 3
							assert_op(dst + 127, <, AMIGA_MAX_MEMORY);
							for (j = 0; j < 126; ++j) {
								value  = memory[dst++];
								value += memory[dst + 1];
								memory[dst] = value >> 1;
							}
							break;
						case 14:  //polygate
							idx = dst + sample->effectStep;
							assert_op(idx, <, AMIGA_MAX_MEMORY);
							memory[idx] = ~memory[idx] + 1;
							idx = (sample->effectStep + sample->source2) & (sample->waveLen - 1);
							idx += dst;
							assert_op(idx, <, AMIGA_MAX_MEMORY);
							memory[idx] = ~memory[idx] + 1;
							if (++sample->effectStep >= sample->waveLen) sample->effectStep = 0;
							break;
						case 15:  //colgate
							idx = dst;
							assert_op(dst + 128, <, AMIGA_MAX_MEMORY);
							for (j = 0; j < 127; ++j) {
								value  = memory[dst];
								value += memory[dst + 1];
								memory[dst++] = value >> 1;
							}
							dst = idx;
							sample->effectStep++;

							if (sample->effectStep == sample->source2) {
								sample->effectStep = 0;
								idx = value = dst;
								assert_op(idx + 64, <, AMIGA_MAX_MEMORY);
								assert_op(dst + 64, <, AMIGA_MAX_MEMORY);
								for (j = 0; j < 64; ++j) {
									memory[idx++] = memory[dst++];
									dst++;
								}
								idx = dst = value;
								idx += 64;
								for (j = 0; j < 64; ++j) memory[idx++] = memory[dst++];
							}
							break;
						default:
							break;
					}
				}
			}
		} else {
			chan = (i < 3) ? voice->channel : &self->mixChannel;
		}

		if (voice->volumeCtr) {
			voice->volumeCtr--;

			if (voice->volumeCtr == 0) {
				voice->volumeCtr  = sample->volumeSpeed;
				voice->volumeStep = ++voice->volumeStep & 127;

				if (voice->volumeStep || sample->volumeLoop) {
					idx = voice->volumeStep + (sample->super.volume << 7);
					assert_op(idx, <, AMIGA_MAX_MEMORY);
					value = ~(memory[idx] + 129) + 1;

					voice->volume = (value & 255) >> 2;
					AmigaChannel_set_volume(chan, voice->volume);
				} else {
					voice->volumeCtr = 0;
				}
			}
		}
		value = voice->note;

		if (sample->arpeggio) {
			idx = voice->arpeggioStep + (sample->arpeggio << 5);
			value += self->arpeggios[idx];
			voice->arpeggioStep = ++voice->arpeggioStep & 31;
		}

		idx = value + voice->step->transpose + sample->finetune;
		voice->finalPeriod = PERIODS[idx];
		dst = voice->finalPeriod;

		if (voice->val1 == 1 || voice->val1 == 12) {
			value = ~voice->val2 + 1;
			voice->portamento += value;
			voice->finalPeriod += voice->portamento;

			if (voice->val2) {
				if ((value < 0 && voice->finalPeriod <= voice->period) || (value >= 0 && voice->finalPeriod >= voice->period)) {
					voice->portamento = voice->period - dst;
					voice->val2 = 0;
				}
			}
		}

		if (sample->pitch) {
			if (voice->pitchCtr) {
				voice->pitchCtr--;
			} else {
				idx = voice->pitchStep;
				voice->pitchStep = ++voice->pitchStep & 127;
				if (voice->pitchStep == 0) voice->pitchStep = sample->pitchLoop;

				idx += sample->pitch << 7;
				assert_op(idx, <, AMIGA_MAX_MEMORY);
				value = memory[idx];
				voice->finalPeriod += (~value + 1);
			}
		}
		AmigaChannel_set_period(chan, voice->finalPeriod);
	}

	if (self->numChannels > 4) {
		src1 = self->buffer1;
		self->buffer1 = self->buffer2;
		self->buffer2 = src1;

		chan = &self->super.amiga->channels[3];
		chan->pointer = src1;

		for (i = 3; i < 7; ++i) {
			voice = &self->voices[i];
			voice->mixStep = 0;

			if (voice->finalPeriod < 125) {
				voice->mixMute  = 1;
				voice->mixSpeed = 0;
			} else {
				j = ((voice->finalPeriod << 8) / self->mixPeriod) & 65535;
				src2 = ((256 / j) & 255) << 8;
				dst  = ((256 % j) << 8) & 16777215;
				voice->mixSpeed = (src2 | ((dst / j) & 255)) << 8;
			}

			if (voice->mixMute) voice->mixVolume = 0;
			else voice->mixVolume = voice->volume << 8;
		}

		for (i = 0; i < 350; ++i) {
			dst = 0;

			for (j = 3; j < 7; ++j) {
				unsigned idxx = (voice->mixPtr + (voice->mixStep >> 16));
				voice = &self->voices[j];
				assert_op(idxx, <, AMIGA_MAX_MEMORY);
				src2 = (memory[idxx] & 255) + voice->mixVolume;
				dst += self->volumes[src2];
				voice->mixStep += voice->mixSpeed;
			}
			assert_op(src1 + 1, <, AMIGA_MAX_MEMORY);
			memory[src1++] = self->averages[dst];
		}
		chan->length = 350;
		AmigaChannel_set_period(chan, self->mixPeriod);
		AmigaChannel_set_volume(chan, 64);
	}

	if (--(self->super.super.tick) == 0) {
		self->super.super.tick = self->super.super.speed & 15;
		self->super.super.speed  = (self->super.super.speed & 240) >> 4;
		self->super.super.speed |= (self->super.super.tick << 4);
		self->patternEnd = 1;
		self->patternPos++;

		if (self->patternPos == 64 || self->patternPos == self->patternLen) {
			self->patternPos = 0;
			self->stepEnd    = 1;
			self->trackPos  += 4;

			if (self->trackPos == self->song1->length) {
				self->trackPos = self->song1->loopStep;
				CoreMixer_set_complete(&self->super.amiga->super, 1);
			}
		}
	} else {
		self->patternEnd = 0;
		self->stepEnd = 0;
	}

	for (i = 0; i < self->numChannels; ++i) {
		voice = &self->voices[i];
		voice->mixPtr += voice->mixStep >> 16;

		sample = voice->sample;
		sample->effectDone = 0;

		if (voice->mixPtr >= voice->mixEnd) {
			if (sample->super.loop) {
				voice->mixPtr -= sample->super.repeat;
			} else {
				voice->mixPtr  = 0;
				voice->mixMute = 1;
			}
		}

		if (i < 4) {
			chan = voice->channel;
			AmigaChannel_set_enabled(chan, 1);
		}
	}
}

//override
void DMPlayer_initialize(struct DMPlayer* self) {
	struct AmigaChannel *chan = 0;
	int i = 0;
	int len = 0;
	struct DMVoice *voice = 0;
	
	//self->super->initialize();
	CorePlayer_initialize(&self->super.super);

	if (self->super.super.playSong > 7) self->super.super.playSong = 0;

	self->song1  = &self->songs[self->super.super.playSong];
	self->super.super.speed  = self->song1->speed & 0x0f;
	self->super.super.speed |= self->super.super.speed << 4;
	self->super.super.tick   = self->song1->speed;

	self->trackPos    = 0;
	self->patternPos  = 0;
	self->patternLen  = 64;
	self->patternEnd  = 1;
	self->stepEnd     = 1;
	self->numChannels = 4;

	for (; i < 7; ++i) {
		voice = &self->voices[i];
		DMVoice_initialize(voice);
		//voice->initialize();
		voice->sample = &self->samples[0];

		if (i < 4) {
			chan = &self->super.amiga->channels[i];
			AmigaChannel_set_enabled(chan, 0);
			chan->pointer = self->super.amiga->loopPtr;
			chan->length  = 2;
			AmigaChannel_set_period(chan, 124);
			AmigaChannel_set_volume(chan, 0);

			voice->channel = chan;
		}
	}

	if (self->super.super.version == DIGITALMUG_V2) {
		if ((self->super.super.playSong & 1) != 0) self->super.super.playSong--;
		self->song2 = &self->songs[self->super.super.playSong + 1];

		//self->mixChannel  = new AmigaChannel(7);
		AmigaChannel_ctor(&self->mixChannel, 7);
		self->numChannels = 7;

		chan = &self->super.amiga->channels[3];
		chan->mute    = 0;
		chan->pointer = self->buffer1;
		chan->length  = 350;
		AmigaChannel_set_period(chan, self->mixPeriod);
		AmigaChannel_set_volume(chan, 64);

		len = self->buffer1 + 700;
		assert_op(len, <, AMIGA_MAX_MEMORY);
		for (i = self->buffer1; i < len; ++i) self->super.amiga->memory[i] = 0;
	}
}

//override
void DMPlayer_loader(struct DMPlayer* self, struct ByteArray *stream) {
	int data = 0; 
	int i = 0; 
	char id[28];
	//index:Vector.<int>;
	int index[8] = {0};
	int instr = 0; 
	int j = 0; 
	int len = 0; 
	int position = 0; 
	struct AmigaRow *row = 0;
	struct DMSample *sample = 0;
	struct DMSong *song = 0;
	struct AmigaStep *step = 0;
	
	stream->readMultiByte(stream, id, 24);

	if (is_str(id, " MUGICIAN/SOFTEYES 1990 ")) self->super.super.version = DIGITALMUG_V1;
	else if (is_str(id, " MUGICIAN2/SOFTEYES 1990")) self->super.super.version = DIGITALMUG_V2;
	else return;

	ByteArray_set_position(stream, 28);
	//index = new Vector.<int>(8, true);
	for (; i < ARRAY_SIZE(index); ++i) index[i] = stream->readUnsignedInt(stream);

	ByteArray_set_position(stream, 76);

	for (i = 0; i < DMPLAYER_MAX_SONGS; ++i) {
		//song = new DMSong();
		song = &self->songs[i];
		DMSong_ctor(song);
		
		song->loop     = stream->readUnsignedByte(stream);
		song->loopStep = stream->readUnsignedByte(stream) << 2;
		song->speed    = stream->readUnsignedByte(stream);
		song->length   = stream->readUnsignedByte(stream) << 2;
		stream->readMultiByte(stream, song->title, 12);
		//self->songs[i] = song;
	}

	ByteArray_set_position(stream, 204);
	self->super.super.lastSong = DMPLAYER_MAX_SONGS - 1;
	
	int lastsong_set = 0;

	for (i = 0; i < DMPLAYER_MAX_SONGS; ++i) {
		song = &self->songs[i];
		len  = index[i] << 2;

		assert_op(len, <=, DMSONG_MAX_TRACKS);
		for (j = 0; j < len; ++j) {
			//step = new AmigaStep();
			step = &song->tracks[j];
			AmigaStep_ctor(step);
			step->pattern   = stream->readUnsignedByte(stream) << 6;
			step->transpose = stream->readByte(stream);
			//song->tracks[j] = step;
		}
		//song->tracks->fixed = true;
		if(!lastsong_set && song->length <= 4 && song->tracks[0].pattern == 0) {
			self->super.super.lastSong = i - 1;
			lastsong_set = 1;
		}
	}

	position = ByteArray_get_position(stream);
	ByteArray_set_position(stream, 60);
	len = stream->readUnsignedInt(stream);
	
	//self->samples = new Vector.<DMSample>(++len, true);
	len++;
	assert_op(len, <=, DMPLAYER_MAX_SAMPLES);
	
	ByteArray_set_position(stream, position);

	for (i = 1; i < len; ++i) {
		sample = &self->samples[i];
		DMSample_ctor(sample);
		
		//sample = new DMSample();
		sample->wave        = stream->readUnsignedByte(stream);
		sample->waveLen     = stream->readUnsignedByte(stream) << 1;
		sample->super.volume= stream->readUnsignedByte(stream);
		sample->volumeSpeed = stream->readUnsignedByte(stream);
		sample->arpeggio    = stream->readUnsignedByte(stream);
		sample->pitch       = stream->readUnsignedByte(stream);
		sample->effectStep  = stream->readUnsignedByte(stream);
		sample->pitchDelay  = stream->readUnsignedByte(stream);
		sample->finetune    = stream->readUnsignedByte(stream) << 6;
		sample->pitchLoop   = stream->readUnsignedByte(stream);
		sample->pitchSpeed  = stream->readUnsignedByte(stream);
		sample->effect      = stream->readUnsignedByte(stream);
		sample->source1     = stream->readUnsignedByte(stream);
		sample->source2     = stream->readUnsignedByte(stream);
		sample->effectSpeed = stream->readUnsignedByte(stream);
		sample->volumeLoop  = stream->readUnsignedByte(stream);
		//self->samples[i] = sample;
	}
	// FIXME this assumes a pointer!
	self->samples[0] = self->samples[1];

	position = ByteArray_get_position(stream);
	ByteArray_set_position(stream, 64);
	len = stream->readUnsignedInt(stream) << 7;
	ByteArray_set_position(stream, position);
	Amiga_store(self->super.amiga, stream, len, -1);
	//self->super.amiga->store(stream, len);

	position = ByteArray_get_position(stream);
	ByteArray_set_position(stream, 68);
	instr = stream->readUnsignedInt(stream);

	ByteArray_set_position(stream, 26);
	len = stream->readUnsignedShort(stream) << 6;
	
	assert_op(len, <=, DMPLAYER_MAX_PATTERNS);
	//patterns = new Vector.<AmigaRow>(len, true);
	
	ByteArray_set_position(stream, position + (instr << 5));

	if (instr) instr = position;

	for (i = 0; i < len; ++i) {
		//row = new AmigaRow();
		row = &self->patterns[i];
		AmigaRow_ctor(row);
		
		row->note   = stream->readUnsignedByte(stream);
		row->sample = stream->readUnsignedByte(stream) & 63;
		row->effect = stream->readUnsignedByte(stream);
		row->param  = stream->readByte(stream);
		//self->patterns[i] = row;
	}

	position = ByteArray_get_position(stream);
	ByteArray_set_position(stream, 72);

	if (instr) {
		len = stream->readUnsignedInt(stream);
		ByteArray_set_position(stream, position);
		data = Amiga_store(self->super.amiga, stream, len, -1);
		position = ByteArray_get_position(stream);

		Amiga_memory_set_length(self->super.amiga, self->super.amiga->vector_count_memory + 350);
		self->buffer1 = self->super.amiga->vector_count_memory;
		Amiga_memory_set_length(self->super.amiga, self->super.amiga->vector_count_memory + 350);
		self->buffer2 = self->super.amiga->vector_count_memory;
		Amiga_memory_set_length(self->super.amiga, self->super.amiga->vector_count_memory + 350);
		self->super.amiga->loopLen = 8;

		//len = self->samples->length;
		len = DMPLAYER_MAX_SAMPLES;

		for (i = 1; i < len; ++i) {
			sample = &self->samples[i];
			if (sample->wave < 32) continue;
			ByteArray_set_position(stream, instr + ((sample->wave - 32) << 5));

			sample->super.pointer = stream->readUnsignedInt(stream);
			sample->super.length  = stream->readUnsignedInt(stream) - sample->super.pointer;
			sample->super.loop    = stream->readUnsignedInt(stream);
			stream->readMultiByte(stream, sample->sample_name, 12);
			sample->super.name    = sample->sample_name;

			if (sample->super.loop) {
				sample->super.loop  -= sample->super.pointer;
				sample->super.repeat = sample->super.length - sample->super.loop;
				if ((sample->super.repeat & 1) != 0) sample->super.repeat--;
			} else {
				sample->super.loopPtr = self->super.amiga->vector_count_memory;
				sample->super.repeat  = 8;
			}

			if ((sample->super.pointer & 1) != 0) sample->super.pointer--;
			if ((sample->super.length  & 1) != 0) sample->super.length--;

			sample->super.pointer += data;
			if (!sample->super.loopPtr) sample->super.loopPtr = sample->super.pointer + sample->super.loop;
		}
	} else {
		position += stream->readUnsignedInt(stream);
	}
	ByteArray_set_position(stream, 24);

	if (stream->readUnsignedShort(stream) == 1) {
		ByteArray_set_position(stream, position);
		len = ByteArray_get_length(stream) - ByteArray_get_position(stream);
		if (len > 256) len = 256;
		for (i = 0; i < len; ++i) self->arpeggios[i] = stream->readUnsignedByte(stream);
	}
}

void tables(struct DMPlayer* self) {
	int i = 0;
	int idx = 0;
	int j = 0; 
	int pos = 0; 
	int step = 0; 
	int v1 = 0; 
	int v2 = 0; 
	int vol = 128;

	//averages  = new Vector.<int>(1024, true);
	//volumes   = new Vector.<int>(16384, true);
	self->mixPeriod = 203;

	for (; i < DMPLAYER_MAX_AVERAGES; ++i) {
		if (vol > 127) vol -= 256;
		self->averages[i] = vol;
		if (i > 383 && i < 639) vol = ++vol & 255;
	}

	for (i = 0; i < 64; ++i) {
		v1 = -128;
		v2 =  128;

		for (j = 0; j < 256; ++j) {
			vol = ((v1 * step) / 63) + 128;
			idx = pos + v2;
			self->volumes[idx] = vol & 255;

			if (i != 0 && i != 63 && v2 >= 128) --(self->volumes[idx]);
			v1++;
			v2 = ++v2 & 255;
		}
		pos += 256;
		step++;
	}
}


