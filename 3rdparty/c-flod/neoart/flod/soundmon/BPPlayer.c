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

#include "BPPlayer.h"
#include "../flod_internal.h"

void BPPlayer_loader(struct BPPlayer* self, struct ByteArray *stream);
void BPPlayer_reset(struct BPPlayer* self);
void BPPlayer_initialize(struct BPPlayer* self);
void BPPlayer_process(struct BPPlayer* self);

static const unsigned short PERIODS[] = {
        6848,6464,6080,5760,5440,5120,4832,4576,4320,4064,3840,3616,
        3424,3232,3040,2880,2720,2560,2416,2288,2160,2032,1920,1808,
        1712,1616,1520,1440,1360,1280,1208,1144,1080,1016, 960, 904,
         856, 808, 760, 720, 680, 640, 604, 572, 540, 508, 480, 452,
         428, 404, 380, 360, 340, 320, 302, 286, 270, 254, 240, 226,
         214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
         107, 101,  95,  90,  85,  80,  76,  72,  68,  64,  60,  57,
};

static const signed short VIBRATO[] = {0,64,128,64,0,-64,-128,-64,};

void BPPlayer_defaults(struct BPPlayer* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void BPPlayer_ctor(struct BPPlayer* self, struct Amiga *amiga) {
	CLASS_CTOR_DEF(BPPlayer);
	// original constructor code goes here
	//super(amiga);
	AmigaPlayer_ctor(&self->super, amiga);

	//buffer  = new Vector.<int>(128, true);
	//samples = new Vector.<BPSample>(16, true);
	//voices  = new Vector.<BPVoice>(4, true);
	
	unsigned i = 0;
	for(; i < BPPLAYER_MAX_VOICES; i++) {
		BPVoice_ctor(&self->voices[i], i);
		if(i) self->voices[i - 1].next = &self->voices[i];
	}
	
	//vtable
	self->super.super.process = BPPlayer_process;
	self->super.super.loader = BPPlayer_loader;
	self->super.super.initialize = BPPlayer_initialize;
	self->super.super.reset = BPPlayer_reset;
	
	self->super.super.min_filesize = 30;
	
}

struct BPPlayer* BPPlayer_new(struct Amiga *amiga) {
	CLASS_NEW_BODY(BPPlayer, amiga);
}

//override
void BPPlayer_process(struct BPPlayer* self) {
	struct AmigaChannel *chan = 0;
	int data = 0;
	int dst = 0;
	int instr = 0;
	int len = 0;
	signed char *memory = self->super.amiga->memory;
	int note = 0;
	int option = 0;
	struct AmigaRow *row = 0;
	struct BPSample *sample = 0;
	int src = 0;
	struct BPStep *step = 0;
	struct BPVoice *voice = &self->voices[0];
	
	self->arpeggioCtr = --(self->arpeggioCtr) & 3;
	self->vibratoPos  = ++(self->vibratoPos)  & 7;

	while (voice) {
		chan = voice->channel;
		voice->period += voice->autoSlide;

		if (voice->vibrato) AmigaChannel_set_period(chan, voice->period + (VIBRATO[self->vibratoPos] / voice->vibrato));
		else AmigaChannel_set_period(chan, voice->period);

		chan->pointer = voice->samplePtr;
		chan->length  = voice->sampleLen;

		if (voice->arpeggio || voice->autoArpeggio) {
			note = voice->note;

			if (!self->arpeggioCtr)
			note += ((voice->arpeggio & 0xf0) >> 4) + ((voice->autoArpeggio & 0xf0) >> 4);
			else if (self->arpeggioCtr == 1)
			note += (voice->arpeggio & 0x0f) + (voice->autoArpeggio & 0x0f);
			
			unsigned idxx = note + 35;
			assert_op(idxx, <, ARRAY_SIZE(PERIODS));
			voice->period = PERIODS[idxx];
			AmigaChannel_set_period(chan, voice->period);
			voice->restart = 0;
		}

		if (!voice->synth || voice->sample < 0) {
			voice = voice->next;
			continue;
		}
		assert_op(voice->sample, <, BPPLAYER_MAX_SAMPLES);
		sample = &self->samples[voice->sample];

		if (voice->adsrControl) {
			if (--voice->adsrCtr == 0) {
				voice->adsrCtr = sample->adsrSpeed;
				unsigned idxx = sample->adsrTable + voice->adsrPtr;
				assert_op(idxx, <, AMIGA_MAX_MEMORY);
				data = (128 + memory[idxx]) >> 2;
				AmigaChannel_set_volume(chan, (data * voice->volume) >> 6);

				if (++voice->adsrPtr == sample->adsrLen) {
					voice->adsrPtr = 0;
					if (voice->adsrControl == 1) voice->adsrControl = 0;
				}
			}
		}

		if (voice->lfoControl) {
			if (--voice->lfoCtr == 0) {
				voice->lfoCtr = sample->lfoSpeed;
				unsigned idxx = sample->lfoTable + voice->lfoPtr;
				assert_op(idxx, <, AMIGA_MAX_MEMORY);
				data = memory[idxx];
				if (sample->lfoDepth) data = (data / sample->lfoDepth) >> 0;
				AmigaChannel_set_period(chan, voice->period + data);

				if (++voice->lfoPtr == sample->lfoLen) {
					voice->lfoPtr = 0;
					if (voice->lfoControl == 1) voice->lfoControl = 0;
				}
			}
		}

		if (voice->synthPtr < 0) {
			voice = voice->next;
			continue;
		}

		if (voice->egControl) {
			if (--voice->egCtr == 0) {
				voice->egCtr = sample->egSpeed;
				data = voice->egValue;
				unsigned idxx = sample->egTable + voice->egPtr;
				assert_op(idxx, <, AMIGA_MAX_MEMORY);
				voice->egValue = (128 + memory[idxx]) >> 3;

				if (voice->egValue != data) {
					src = (voice->index << 5) + data;
					dst = voice->synthPtr + data;

					if (voice->egValue < data) {
						data -= voice->egValue;
						len = dst - data;
						assert_op(dst, <=, AMIGA_MAX_MEMORY);
						assert_op(src, <=, BPPLAYER_MAX_BUFFER);
						for (; dst > len;) memory[--dst] = self->buffer[--src];
					} else {
						data = voice->egValue - data;
						len = dst + data;
						assert_op(len, <, AMIGA_MAX_MEMORY);
						assert_op(src + (len - dst), <, BPPLAYER_MAX_BUFFER);
						for (; dst < len;) memory[dst++] = ~self->buffer[src++] + 1;
					}
				}

				if (++voice->egPtr == sample->egLen) {
					voice->egPtr = 0;
					if (voice->egControl == 1) voice->egControl = 0;
				}
			}
		}

		switch (voice->fxControl) {
			default:
			case 0:
				break;
			case 1:   //averaging
				if (--voice->fxCtr == 0) {
					voice->fxCtr = sample->fxSpeed;
					dst = voice->synthPtr;
					len = voice->synthPtr + 32;

					assert_op(dst, <=, AMIGA_MAX_MEMORY);
					data = dst > 0 ? memory[dst - 1] : 0;
					
					assert_op(len + 1, <, AMIGA_MAX_MEMORY);
					for (; dst < len;) {
						data = (data + memory[dst + 1]) >> 1;
						memory[dst++] = data;
					}
				}
				break;
			case 2:   //inversion
				src = (voice->index << 5) + 31;
				len = voice->synthPtr + 32;
				data = sample->fxSpeed;

				for (dst = voice->synthPtr; dst < len; ++dst) {
					if (self->buffer[src] < memory[dst]) {
						memory[dst] -= data;
					} else if (self->buffer[src] > memory[dst]) {
						memory[dst] += data;
					}
					src--;
				}
				break;
			case 3:   //backward inversion
			case 5:   //backward transform
				src = voice->index << 5;
				len = voice->synthPtr + 32;
				data = sample->fxSpeed;

				for (dst = voice->synthPtr; dst < len; ++dst) {
					if (self->buffer[src] < memory[dst]) {
						memory[dst] -= data;
					} else if (self->buffer[src] > memory[dst]) {
						memory[dst] += data;
					}
					src++;
				}
				break;
			case 4:   //transform
				src = voice->synthPtr + 64;
				len = voice->synthPtr + 32;
				data = sample->fxSpeed;

				for (dst = voice->synthPtr; dst < len; ++dst) {
					if (memory[src] < memory[dst]) {
						memory[dst] -= data;
					} else if (memory[src] > memory[dst]) {
						memory[dst] += data;
					}
					src++;
				}
				break;
			case 6:   //wave change
				if (--voice->fxCtr == 0) {
					voice->fxControl = 0;
					voice->fxCtr = 1;
					src = voice->synthPtr + 64;
					len = voice->synthPtr + 32;
					for (dst = voice->synthPtr; dst < len; ++dst) memory[dst] = memory[src++];
				}
				break;
		}

		if (voice->modControl) {
			if (--voice->modCtr == 0) {
				voice->modCtr = sample->modSpeed;
				unsigned idxx = sample->modTable + voice->modPtr;
				assert_op(idxx, <, AMIGA_MAX_MEMORY);
				memory[voice->synthPtr + 32] = memory[idxx];

				if (++voice->modPtr == sample->modLen) {
					voice->modPtr = 0;
					if (voice->modControl == 1) voice->modControl = 0;
				}
			}
		}
		voice = voice->next;
	}

	if (--(self->super.super.tick) == 0) {
		self->super.super.tick = self->super.super.speed;
		voice = &self->voices[0];

		while (voice) {
			chan = voice->channel;
			voice->enabled = 0;
			
			unsigned idxx = (self->trackPos << 2) + voice->index;
			assert_op(idxx, <, BPPLAYER_MAX_TRACKS);

			step   = &self->tracks[idxx];
			idxx = self->patternPos + ((step->super.pattern - 1) << 4);
			assert_op(idxx, <, BPPLAYER_MAX_PATTERNS);
			
			row    = &self->patterns[idxx];
			note   = row->note;
			option = row->effect;
			data   = row->param;

			if (note) {
				voice->autoArpeggio = voice->autoSlide = voice->vibrato = 0;
				if (option != 10 || (data & 0xf0) == 0) note += step->super.transpose;
				voice->note = note;
				idxx = note + 35;
				assert_op(idxx, <, ARRAY_SIZE(PERIODS));
				voice->period = PERIODS[idxx];

				if (option < 13) voice->restart = voice->volumeDef = 1;
				else voice->restart = 0;

				instr = row->sample;
				if (instr == 0) instr = voice->sample;
				if (option != 10 || (data & 0x0f) == 0) instr += step->soundTranspose;

				if (option < 13 && (!voice->synth || (voice->sample != instr))) {
					voice->sample = instr;
					voice->enabled = 1;
				}
			}

			switch (option) {
				case 0:   //arpeggio once
					voice->arpeggio = data;
					break;
				case 1:   //set volume
					voice->volume = data;
					voice->volumeDef = 0;

					if (self->super.super.version < BPSOUNDMON_V3 || !voice->synth)
						AmigaChannel_set_volume(chan, voice->volume);
					break;
				case 2:   //set speed
					self->super.super.tick = self->super.super.speed = data;
					break;
				case 3:   //set filter
					self->super.amiga->filter->active = data;
					break;
				case 4:   //portamento up
					voice->period -= data;
					voice->arpeggio = 0;
					break;
				case 5:   //portamento down
					voice->period += data;
					voice->arpeggio = 0;
					break;
				case 6:   //set vibrato
					if (self->super.super.version == BPSOUNDMON_V3) voice->vibrato = data;
						else self->repeatCtr = data;
					break;
				case 7:   //step jump
					if (self->super.super.version == BPSOUNDMON_V3) {
						self->nextPos = data;
						self->jumpFlag = 1;
					} else if (self->repeatCtr == 0) {
						self->trackPos = data;
					}
					break;
				case 8:   //set auto slide
					voice->autoSlide = data;
					break;
					case 9:   //set auto arpeggio
					voice->autoArpeggio = data;
					if (self->super.super.version == BPSOUNDMON_V3) {
						voice->adsrPtr = 0;
						if (voice->adsrControl == 0) voice->adsrControl = 1;
					}
					break;
				case 11:  //change effect
					voice->fxControl = data;
					break;
				case 13:  //change inversion
					voice->autoArpeggio = data;
					voice->fxControl ^= 1;
					voice->adsrPtr = 0;
					if (voice->adsrControl == 0) voice->adsrControl = 1;
					break;
				case 14:  //no eg reset
					voice->autoArpeggio = data;
					voice->adsrPtr = 0;
					if (voice->adsrControl == 0) voice->adsrControl = 1;
					break;
				case 15:  //no eg and no adsr reset
					voice->autoArpeggio = data;
					break;
				default:
					break;
			}
			voice = voice->next;
		}

		if (self->jumpFlag) {
			self->trackPos   = self->nextPos;
			self->patternPos = self->jumpFlag = 0;
		} else if (++(self->patternPos) == 16) {
			self->patternPos = 0;

			if (++(self->trackPos) == self->length) {
				self->trackPos = 0;
				CoreMixer_set_complete(&self->super.amiga->super, 1);
			}
		}
		voice = &self->voices[0];

		while (voice) {
			chan = voice->channel;
			if (voice->enabled) {
				voice->enabled = 0;
				AmigaChannel_set_enabled(chan, 0);
			}
			if (voice->restart == 0) {
				voice = voice->next;
				continue;
			}

			if (voice->synthPtr > -1) {
				src = voice->index << 5;
				len = voice->synthPtr + 32;
				for (dst = voice->synthPtr; dst < len; ++dst) memory[dst] = self->buffer[src++];
				voice->synthPtr = -1;
			}
			voice = voice->next;
		}
		voice = &self->voices[0];

		while (voice) {
			if (voice->restart == 0 || voice->sample < 0) {
				voice = voice->next;
				continue;
			}
			chan = voice->channel;

			AmigaChannel_set_period(chan, voice->period);
			voice->restart = 0;
			assert_op(voice->sample, <, BPPLAYER_MAX_SAMPLES);
			sample = &self->samples[voice->sample];

			if (sample->synth) {
				voice->synth   = 1;
				voice->egValue = 0;
				voice->adsrPtr = voice->lfoPtr = voice->egPtr = voice->modPtr = 0;

				voice->adsrCtr = 1;
				voice->lfoCtr  = sample->lfoDelay + 1;
				voice->egCtr   = sample->egDelay  + 1;
				voice->fxCtr   = sample->fxDelay  + 1;
				voice->modCtr  = sample->modDelay + 1;

				voice->adsrControl = sample->adsrControl;
				voice->lfoControl  = sample->lfoControl;
				voice->egControl   = sample->egControl;
				voice->fxControl   = sample->fxControl;
				voice->modControl  = sample->modControl;

				chan->pointer = voice->samplePtr = sample->super.pointer;
				chan->length  = voice->sampleLen = sample->super.length;

				if (voice->adsrControl) {
					data = (128 + memory[sample->adsrTable]) >> 2;

					if (voice->volumeDef) {
						voice->volume = sample->super.volume;
						voice->volumeDef = 0;
					}

					AmigaChannel_set_volume(chan, (data * voice->volume) >> 6);
				} else {
					AmigaChannel_set_volume(chan, voice->volumeDef ? sample->super.volume : voice->volume);
				}

				if (voice->egControl || voice->fxControl || voice->modControl) {
					voice->synthPtr = sample->super.pointer;
					dst = voice->index << 5;
					len = voice->synthPtr + 32;
					for (src = voice->synthPtr; src < len; ++src) self->buffer[dst++] = memory[src];
				}
			} else {
				voice->synth = voice->lfoControl = 0;

				if (sample->super.pointer < 0) {
					voice->samplePtr = self->super.amiga->loopPtr;
					voice->sampleLen = 2;
				} else {
					chan->pointer = sample->super.pointer;
					AmigaChannel_set_volume(chan, voice->volumeDef ? sample->super.volume : voice->volume);

					if (sample->super.repeat != 2) {
						voice->samplePtr = sample->super.loopPtr;
						chan->length = voice->sampleLen = sample->super.repeat;
					} else {
						voice->samplePtr = self->super.amiga->loopPtr;
						voice->sampleLen = 2;
						chan->length = sample->super.length;
					}
				}
			}
			voice->enabled = 1;
			AmigaChannel_set_enabled(chan, 1);
			voice = voice->next;
		}
	}
}

//override
void BPPlayer_initialize(struct BPPlayer* self) {
	int i = 0;
	struct BPVoice *voice = &self->voices[0];
	
	CorePlayer_initialize(&self->super.super);
	//self->super->initialize();

	self->super.super.speed       = 6;
	self->super.super.tick        = 1;
	self->trackPos    = 0;
	self->patternPos  = 0;
	self->nextPos     = 0;
	self->jumpFlag    = 0;
	self->repeatCtr   = 0;
	self->arpeggioCtr = 1;
	self->vibratoPos  = 0;

	for (i = 0; i < 128; ++i) self->buffer[i] = 0;

	while (voice) {
		BPVoice_initialize(voice);
		voice->channel   = &self->super.amiga->channels[voice->index];
		voice->samplePtr = self->super.amiga->loopPtr;
		voice = voice->next;
	}
}

//override
void BPPlayer_reset(struct BPPlayer* self) {
	int i = 0; 
	int len = 0; 
	int pos = 0; 
	struct BPVoice *voice = &self->voices[0];

	while (voice) {
		if (voice->synthPtr > -1) {
			pos = voice->index << 5;
			len = voice->synthPtr + 32;

			for (i = voice->synthPtr; i < len; ++i)
			self->super.amiga->memory[i] = self->buffer[pos++];
		}

		voice = voice->next;
	}
}

//override
void BPPlayer_loader(struct BPPlayer* self, struct ByteArray *stream) {
	int higher = 0; 
	int i = 0;
	char id[4];
	int len = 0; 
	struct AmigaRow *row = 0;
	struct BPSample *sample = 0;
	struct BPStep *step = 0;
	int tables = 0;
	
	self->super.super.title = self->title_buffer;
	stream->readMultiByte(stream, self->title_buffer, 26);

	stream->readMultiByte(stream, id, 4);
	if (is_str(id, "BPSM")) {
		self->super.super.version = BPSOUNDMON_V1;
	} else {
		if (is_str(id, "V.2")) self->super.super.version = BPSOUNDMON_V2;
		else if (is_str(id, "V.3")) self->super.super.version = BPSOUNDMON_V3;
		else return;

		ByteArray_set_position(stream, 29);
		tables = stream->readUnsignedByte(stream);
	}

	self->length = stream->readUnsignedShort(stream);

	for (; ++i < 16;) {
		//sample = new BPSample();
		sample = &self->samples[i];
		BPSample_ctor(sample);

		if (stream->readUnsignedByte(stream) == 0xff) {
			sample->synth   = 1;
			sample->table   = stream->readUnsignedByte(stream);
			sample->super.pointer = sample->table << 6;
			sample->super.length  = stream->readUnsignedShort(stream) << 1;

			sample->adsrControl = stream->readUnsignedByte(stream);
			sample->adsrTable   = stream->readUnsignedByte(stream) << 6;
			sample->adsrLen     = stream->readUnsignedShort(stream);
			sample->adsrSpeed   = stream->readUnsignedByte(stream);
			sample->lfoControl  = stream->readUnsignedByte(stream);
			sample->lfoTable    = stream->readUnsignedByte(stream) << 6;
			sample->lfoDepth    = stream->readUnsignedByte(stream);
			sample->lfoLen      = stream->readUnsignedShort(stream);

			if (self->super.super.version < BPSOUNDMON_V3) {
				stream->readByte(stream);
				sample->lfoDelay  = stream->readUnsignedByte(stream);
				sample->lfoSpeed  = stream->readUnsignedByte(stream);
				sample->egControl = stream->readUnsignedByte(stream);
				sample->egTable   = stream->readUnsignedByte(stream) << 6;
				stream->readByte(stream);
				sample->egLen     = stream->readUnsignedShort(stream);
				stream->readByte(stream);
				sample->egDelay   = stream->readUnsignedByte(stream);
				sample->egSpeed   = stream->readUnsignedByte(stream);
				sample->fxSpeed   = 1;
				sample->modSpeed  = 1;
				sample->super.volume    = stream->readUnsignedByte(stream);
				ByteArray_set_position_rel(stream, +6);
			} else {
				sample->lfoDelay   = stream->readUnsignedByte(stream);
				sample->lfoSpeed   = stream->readUnsignedByte(stream);
				sample->egControl  = stream->readUnsignedByte(stream);
				sample->egTable    = stream->readUnsignedByte(stream) << 6;
				sample->egLen      = stream->readUnsignedShort(stream);
				sample->egDelay    = stream->readUnsignedByte(stream);
				sample->egSpeed    = stream->readUnsignedByte(stream);
				sample->fxControl  = stream->readUnsignedByte(stream);
				sample->fxSpeed    = stream->readUnsignedByte(stream);
				sample->fxDelay    = stream->readUnsignedByte(stream);
				sample->modControl = stream->readUnsignedByte(stream);
				sample->modTable   = stream->readUnsignedByte(stream) << 6;
				sample->modSpeed   = stream->readUnsignedByte(stream);
				sample->modDelay   = stream->readUnsignedByte(stream);
				sample->super.volume     = stream->readUnsignedByte(stream);
				sample->modLen     = stream->readUnsignedShort(stream);
			}
		} else {
			ByteArray_set_position_rel(stream, -1);
			sample->synth  = 0;
			sample->super.name = sample->sample_name; 
			stream->readMultiByte(stream, sample->sample_name, 24);
			
			sample->super.length = stream->readUnsignedShort(stream) << 1;

			if (sample->super.length) {
				sample->super.loop   = stream->readUnsignedShort(stream);
				sample->super.repeat = stream->readUnsignedShort(stream) << 1;
				sample->super.volume = stream->readUnsignedShort(stream);

				if ((sample->super.loop + sample->super.repeat) >= sample->super.length)
				sample->super.repeat = sample->super.length - sample->super.loop;
			} else {
				sample->super.pointer--;
				sample->super.repeat = 2;
				ByteArray_set_position_rel(stream, +6);
			}
		}
		//self->samples[i] = sample;
	}

	len = self->length << 2;
	assert_op(len, <=, BPPLAYER_MAX_TRACKS);
	//tracks = new Vector.<BPStep>(len, true);

	for (i = 0; i < len; ++i) {
		//step = new BPStep();
		step = &self->tracks[i];
		BPStep_ctor(step);
		
		step->super.pattern = stream->readUnsignedShort(stream);
		step->soundTranspose = stream->readByte(stream);
		step->super.transpose = stream->readByte(stream);
		if (step->super.pattern > higher) higher = step->super.pattern;
		//self->tracks[i] = step;
	}

	len = higher << 4;
	assert_op(len, <=, BPPLAYER_MAX_PATTERNS);
	//patterns = new Vector.<AmigaRow>(len, true);

	for (i = 0; i < len; ++i) {
		//row = new AmigaRow();
		row = &self->patterns[i];
		AmigaRow_ctor(row);
		
		row->note   = stream->readByte(stream);
		row->sample = stream->readUnsignedByte(stream);
		row->effect = row->sample & 0x0f;
		row->sample = (row->sample & 0xf0) >> 4;
		row->param  = stream->readByte(stream);
		//self->patterns[i] = row;
	}

	Amiga_store(self->super.amiga, stream, tables << 6, -1);

	for (i = 0; ++i < 16;) {
		sample = &self->samples[i];
		if (sample->synth || !sample->super.length) continue;
		sample->super.pointer = Amiga_store(self->super.amiga, stream, sample->super.length, -1);
		sample->super.loopPtr = sample->super.pointer + sample->super.loop;
	}
}
