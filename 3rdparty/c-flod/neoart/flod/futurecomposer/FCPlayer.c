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


#include "FCPlayer.h"
#include "../flod_internal.h"

static const unsigned short PERIODS[] = {
        1712,1616,1524,1440,1356,1280,1208,1140,1076,1016, 960, 906,
         856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
         428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
         214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
         113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113,
        3424,3232,3048,2880,2712,2560,2416,2280,2152,2032,1920,1812,
        1712,1616,1524,1440,1356,1280,1208,1140,1076,1016, 960, 906,
         856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
         428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
         214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
         113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113, 113,
};

const signed char WAVES[] = {
          16,  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,
          16,  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,
           8,   8,   8,   8,   8,   8,   8,   8,  16,   8,  16,  16,   8,   8,  24, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56,  63,
          55,  47,  39,  31,  23,  15,   7,  -1,   7,  15,  23,  31,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
          55,  47,  39,  31,  23,  15,   7,  -1,   7,  15,  23,  31,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72,  47,  39,  31,  23,  15,   7,  -1,   7,  15,  23,  31,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72, -80,  39,  31,  23,  15,   7,  -1,   7,  15,  23,  31,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72, -80, -88,  31,  23,  15,   7,  -1,   7,  15,  23,  31,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72, -80, -88, -96,  23,  15,   7,  -1,   7,  15,  23,  31,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72, -80, -88, -96,-104,  15,   7,  -1,   7,  15,  23,  31,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72, -80, -88, -96,-104,-112,   7,  -1,   7,  15,  23,  31,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72, -80, -88, -96,-104,-112,-120,  -1,   7,  15,  23,  31,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72, -80, -88, -96,-104,-112,-120,-128,   7,  15,  23,  31,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72, -80, -88, -96,-104,-112,-120,-128,-120,  15,  23,  31,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72, -80, -88, -96,-104,-112,-120,-128,-120,-112,  23,  31,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72, -80, -88, -96,-104,-112,-120,-128,-120,-112,-104,  31,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72, -80, -88, -96,-104,-112,-120,-128,-120,-112,-104, -96,  39,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72, -80, -88, -96,-104,-112,-120,-128,-120,-112,-104, -96, -88,  47,  55, -64,
         -64, -48, -40, -32, -24, -16,  -8,   0,  -8, -16, -24, -32, -40, -48, -56, -64,
         -72, -80, -88, -96,-104,-112,-120,-128,-120,-112,-104, -96, -88, -80,  55,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127, 127,
         127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
         127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
        -127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
        -127,-127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
        -127,-127,-127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
        -127,-127,-127,-127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
        -127,-127,-127,-127,-127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
        -127,-127,-127,-127,-127,-127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
        -127,-127,-127,-127,-127,-127,-127, 127, 127, 127, 127, 127, 127, 127, 127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127, 127, 127, 127, 127, 127, 127, 127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127, 127, 127, 127, 127, 127, 127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127, 127, 127, 127, 127, 127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127, 127, 127, 127, 127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
        -127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127, 127, 127, 127,-128,
        -128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,
        -128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128, 127, 127,-128,
        -128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,
        -128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128, 127,-128,
        -128,-128,-128,-128,-128,-128,-128, 127, 127, 127, 127, 127, 127, 127, 127,-128,
        -128,-128,-128,-128,-128,-128, 127, 127, 127, 127, 127, 127, 127, 127, 127,-128,
        -128,-128,-128,-128,-128, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-128,
        -128,-128,-128,-128, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-128,
        -128,-128,-128, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-128,
        -128,-128, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-128,
        -128, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-128,
        -128, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,-128,
        -128,-112,-104, -96, -88, -80, -72, -64, -56, -48, -40, -32, -24, -16,  -8,   0,
           8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 127,-128,
        -128, -96, -80, -64, -48, -32, -16,   0,  16,  32,  48,  64,  80,  96, 112,  69,
          69, 121, 125, 122, 119, 112, 102,  97,  88,  83,  77,  44,  32,  24,  18,   4,
         -37, -45, -51, -58, -68, -75, -82, -88, -93, -99,-103,-109,-114,-117,-118,  69,
          69, 121, 125, 122, 119, 112, 102,  91,  75,  67,  55,  44,  32,  24,  18,   4,
          -8, -24, -37, -49, -58, -66, -80, -88, -92, -98,-102,-107,-108,-115,-125,   0,
           0,  64,  96, 127,  96,  64,  32,   0, -32, -64, -96,-128, -96, -64, -32,   0,
           0,  64,  96, 127,  96,  64,  32,   0, -32, -64, -96,-128, -96, -64, -32,-128,
        -128,-112,-104, -96, -88, -80, -72, -64, -56, -48, -40, -32, -24, -16,  -8,   0,
           8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 127,-128,
        -128, -96, -80, -64, -48, -32, -16,   0,  16,  32,  48,  64,  80,  96, 112,

	// FIXME figure out why the last row has only 15 entries
};

static void my_bytearray_set_position(struct ByteArray* self, int newpos, int len) {
	if(newpos > len) INT3;
	ByteArray_set_position(self, newpos);
}

#define xByteArray_set_position(ba, pos) my_bytearray_set_position(ba, (pos), ba ## _used )
#define xByteArray_get_length(ba) (ba ## _used)

void FCPlayer_process(struct FCPlayer* self);
void FCPlayer_initialize(struct FCPlayer *self);
void FCPlayer_loader(struct FCPlayer *self, struct ByteArray *stream);


void FCPlayer_defaults(struct FCPlayer* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

// amiga default value NULL
void FCPlayer_ctor(struct FCPlayer* self, struct Amiga *amiga) {
	CLASS_CTOR_DEF(FCPlayer);
	// original constructor code goes here
	AmigaPlayer_ctor(&self->super, amiga);
	//super(amiga);
	//PERIODS->fixed = true;
	//WAVES->fixed   = true;

	/*
	voices = new Vector.<FCVoice>(4, true);

	voices[0] = new FCVoice(0);
	voices[0].next = voices[1] = new FCVoice(1);
	voices[1].next = voices[2] = new FCVoice(2);
	voices[2].next = voices[3] = new FCVoice(3);	*/
	unsigned i = 0;
	for(; i < FCPLAYER_MAX_VOICES; i++) {
		FCVoice_ctor(&self->voices[i], i);
		if(i) self->voices[i - 1].next = &self->voices[i];
	}
	
	//add vtable
	self->super.super.process = FCPlayer_process;
	self->super.super.loader = FCPlayer_loader;
	self->super.super.initialize = FCPlayer_initialize;
	
	// TODO: not very accurate...
	self->super.super.min_filesize = 16;
	
}

struct FCPlayer* FCPlayer_new(struct Amiga *amiga) {
	CLASS_NEW_BODY(FCPlayer, amiga);
}

static int my_bytesAvailable(struct ByteArray* b, unsigned b_count) {
	if(b->pos > b_count)  { INT3; }
	//assert_op (b->pos, <=, b_count);
	return b_count - b->pos;
}

//override
void FCPlayer_process(struct FCPlayer* self) {
	int base = 0; 
	struct AmigaChannel *chan = NULL;
	int delta = 0;
	int i = 0;
	int info = 0;
	int loopEffect = 0;
	int loopSustain = 0;
	int period = 0;
	struct AmigaSample *sample = NULL;
	int temp = 0;
	struct FCVoice *voice = &self->voices[0];

	if (--(self->super.super.tick) == 0) {
		base = ByteArray_get_position(self->seqs);

		while (voice) {
			chan = voice->channel;

			unsigned newpos = voice->pattern + voice->patStep;
			if(newpos < xByteArray_get_length(self->pats)) {
				xByteArray_set_position(self->pats, newpos);
				temp = self->pats->readUnsignedByte(self->pats);
			} else  {
				temp = 0;
			}

			if (voice->patStep >= 64 || temp == 0x49) {
				if (ByteArray_get_position(self->seqs) == self->length) {
					xByteArray_set_position(self->seqs, 0);
					CoreMixer_set_complete(&self->super.amiga->super, 1);
					//self->super.amiga->complete = 1;
				}

				voice->patStep = 0;
				// blue-funk.fc4, paralax.fc3, Hellsector-5.fc4 
				// all cause an eof read here, but they play well in uade.
				// the actionscript code triggers an eof exception as well.
				if(my_bytesAvailable(self->seqs, self->seqs_used) >= 3) {
					voice->pattern = self->seqs->readUnsignedByte(self->seqs) << 6;
					voice->transpose = self->seqs->readByte(self->seqs);
					voice->soundTranspose = self->seqs->readByte(self->seqs);
				}
				if(voice->pattern < xByteArray_get_length(self->pats)) {
					xByteArray_set_position(self->pats, voice->pattern);
					temp = self->pats->readUnsignedByte(self->pats);
				} else {
					temp = 0;
				}
			}
			
			if(voice->pattern < xByteArray_get_length(self->pats))
				info = self->pats->readUnsignedByte(self->pats);
			
			xByteArray_set_position(self->frqs, 0);
			xByteArray_set_position(self->vols, 0);

			if (temp != 0) {
				voice->note = temp & 0x7f;
				voice->pitch = 0;
				voice->portamento = 0;
				AmigaChannel_set_enabled(chan, 0);
				voice->enabled = 0;
				//voice->enabled = chan->enabled = 0;

				temp = 8 + (((info & 0x3f) + voice->soundTranspose) << 6);
				if (temp >= 0 && temp < xByteArray_get_length(self->vols)) 
					xByteArray_set_position(self->vols, temp);

				voice->volStep = 0;
				voice->volSpeed = voice->volCtr = self->vols->readUnsignedByte(self->vols);
				voice->volSustain = 0;

				voice->frqPos = 8 + (self->vols->readUnsignedByte(self->vols) << 6);
				voice->frqStep = 0;
				voice->frqSustain = 0;

				voice->vibratoFlag  = 0;
				voice->vibratoSpeed = self->vols->readUnsignedByte(self->vols);
				voice->vibratoDepth = voice->vibrato = self->vols->readUnsignedByte(self->vols);
				voice->vibratoDelay = self->vols->readUnsignedByte(self->vols);
				voice->volPos = ByteArray_get_position(self->vols);
			}

			if (info & 0x40) {
				voice->portamento = 0;
			} else if (info & 0x80) {
				voice->portamento = ByteArray_getUnsignedByte(self->pats, ByteArray_get_position(self->pats) + 1);
				//voice->portamento = self->pats[(ByteArray_get_position(self->pats) + 1)];
				if (self->super.super.version == FUTURECOMP_10) voice->portamento <<= 1;
			}

			voice->patStep += 2;
			voice = voice->next;
		}

		if (ByteArray_get_position(self->seqs) != base) {
			if (ByteArray_get_position(self->seqs) == xByteArray_get_length(self->seqs)) {
				xByteArray_set_position(self->seqs, 0);
				CoreMixer_set_complete(&self->super.amiga->super, 1);			
			} else {
				temp = self->seqs->readUnsignedByte(self->seqs);
				if (temp) self->super.super.speed = temp;
			}
		}
		self->super.super.tick = self->super.super.speed;
	}
	voice = &self->voices[0];

	while (voice) {
		chan = voice->channel;

		do {
			loopSustain = 0;

			if (voice->frqSustain) {
				voice->frqSustain--;
				break;
			}
			unsigned nextpos = voice->frqPos + voice->frqStep;
			// fix for kerni-the_end.fc4
			if(nextpos > xByteArray_get_length(self->frqs)) 
				nextpos = xByteArray_get_length(self->frqs);
			
			xByteArray_set_position(self->frqs, nextpos);

			do {
				unsigned temp_idx;
				loopEffect = 0;
				if (!my_bytesAvailable(self->frqs, self->frqs_used)) break;
				info = self->frqs->readUnsignedByte(self->frqs);
				if (info == 0xe1) break;

				if (info == 0xe0) {
					voice->frqStep = self->frqs->readUnsignedByte(self->frqs) & 0x3f;
					xByteArray_set_position(self->frqs, voice->frqPos + voice->frqStep);
					info = self->frqs->readUnsignedByte(self->frqs);
				}

				switch (info) {
					case 0xe2:  //set wave
						AmigaChannel_set_enabled(chan, 0);
						voice->enabled = 1;
						voice->volCtr  = 1;
						voice->volStep = 0;
						// FIXME break forgotten or fallthrough?
					case 0xe4:  //change wave:
						temp_idx = self->frqs->readUnsignedByte(self->frqs);
						assert_op(temp_idx, <, self->samples_max);
						sample = &self->samples[temp_idx];
						//FIXME we need a lookup table for assigned samples
						if (sample) {
							chan->pointer = sample->pointer;
							chan->length  = sample->length;
						} else {
							voice->enabled = 0;
						}
						voice->sample = sample;
						voice->frqStep += 2;
						break;
					case 0xe9:  //set pack
						temp = 100 + (self->frqs->readUnsignedByte(self->frqs) * 10);
						temp_idx = (temp + self->frqs->readUnsignedByte(self->frqs));
						assert_op(temp_idx, <, self->samples_max);
						sample = &self->samples[temp_idx];

						if (sample) {
							AmigaChannel_set_enabled(chan, 0);
							chan->pointer = sample->pointer;
							chan->length  = sample->length;
							voice->enabled = 1;
						}

						voice->sample = sample;
						voice->volCtr = 1;
						voice->volStep = 0;
						voice->frqStep += 3;
						break;
					case 0xe7:  //new sequence
						loopEffect = 1;
						voice->frqPos = 8 + (self->frqs->readUnsignedByte(self->frqs) << 6);
						if (voice->frqPos >= xByteArray_get_length(self->frqs)) voice->frqPos = 0;
						voice->frqStep = 0;
						xByteArray_set_position(self->frqs, voice->frqPos);
						break;
					case 0xea:  //pitch bend
						voice->pitchBendSpeed = self->frqs->readByte(self->frqs);
						voice->pitchBendTime  = self->frqs->readUnsignedByte(self->frqs);
						voice->frqStep += 3;
						break;
					case 0xe8:  //sustain
						loopSustain = 1;
						voice->frqSustain = self->frqs->readUnsignedByte(self->frqs);
						voice->frqStep += 2;
						break;
					case 0xe3:  //new vibrato
						voice->vibratoSpeed = self->frqs->readUnsignedByte(self->frqs);
						voice->vibratoDepth = self->frqs->readUnsignedByte(self->frqs);
						voice->frqStep += 3;
						break;
					default:
						break;
				}

				if (!loopSustain && !loopEffect) {
					xByteArray_set_position(self->frqs, voice->frqPos + voice->frqStep);
					voice->frqTranspose = self->frqs->readByte(self->frqs);
					voice->frqStep++;
				}
			} while (loopEffect);
		} while (loopSustain);

		if (voice->volSustain) {
			voice->volSustain--;
		} else {
			if (voice->volBendTime) {
				FCVoice_volumeBend(voice);
			} else {
				if (--(voice->volCtr) == 0) {
					voice->volCtr = voice->volSpeed;

					do {
						loopEffect = 0;
						xByteArray_set_position(self->vols, voice->volPos + voice->volStep);
						if (!my_bytesAvailable(self->vols, self->vols_used)) break;
						info = self->vols->readUnsignedByte(self->vols);
						if (info == 0xe1) break;

						switch (info) {
							case 0xea: //volume slide
								voice->volBendSpeed = self->vols->readByte(self->vols);
								voice->volBendTime  = self->vols->readUnsignedByte(self->vols);
								voice->volStep += 3;
								FCVoice_volumeBend(voice);
								break;
							case 0xe8: //volume sustain
								voice->volSustain = self->vols->readUnsignedByte(self->vols);
								voice->volStep += 2;
								break;
							case 0xe0: //volume loop
								loopEffect = 1;
								temp = self->vols->readUnsignedByte(self->vols) & 0x3f;
								voice->volStep = temp - 5;
								break;
							default:
								voice->volume = info;
								voice->volStep++;
								break;
						}
					} while (loopEffect);
				}
			}
		}
		info = voice->frqTranspose;
		if (info >= 0) info += (voice->note + voice->transpose);
		info &= 0x7f;
		assert_op(info, <, ARRAY_SIZE(PERIODS));
		period = PERIODS[info];

		if (voice->vibratoDelay) {
			voice->vibratoDelay--;
		} else {
			temp = voice->vibrato;

			if (voice->vibratoFlag) {
				delta = voice->vibratoDepth << 1;
				temp += voice->vibratoSpeed;

				if (temp > delta) {
					temp = delta;
					voice->vibratoFlag = 0;
				}
			} else {
				temp -= voice->vibratoSpeed;

				if (temp < 0) {
					temp = 0;
					voice->vibratoFlag = 1;
				}
			}
			voice->vibrato = temp;
			temp -= voice->vibratoDepth;
			base = (info << 1) + 160;

			while (base < 256) {
				temp <<= 1;
				base += 24;
			}
			period += temp;
		}
		voice->portamentoFlag ^= 1;

		if (voice->portamentoFlag && voice->portamento) {
			if (voice->portamento > 0x1f)
				voice->pitch += voice->portamento & 0x1f;
			else
				voice->pitch -= voice->portamento;
		}
		voice->pitchBendFlag ^= 1;

		if (voice->pitchBendFlag && voice->pitchBendTime) {
			voice->pitchBendTime--;
			voice->pitch -= voice->pitchBendSpeed;
		}
		period += voice->pitch;

		if (period < 113) period = 113;
		else if (period > 3424) period = 3424;

		AmigaChannel_set_period(chan, period);
		AmigaChannel_set_volume(chan, voice->volume);

		if (voice->sample) {
			sample = voice->sample;
			AmigaChannel_set_enabled(chan, voice->enabled);
			chan->pointer = sample->loopPtr;
			chan->length  = sample->repeat;
		}
		voice = voice->next;
	}
}

//override
void FCPlayer_initialize(struct FCPlayer *self) {
	struct FCVoice *voice = &self->voices[0];
	//self->super->initialize();
	// FIXME check if thats correct
	CorePlayer_initialize(&self->super.super);

	xByteArray_set_position(self->seqs, 0);
	xByteArray_set_position(self->pats, 0);
	xByteArray_set_position(self->vols, 0);
	xByteArray_set_position(self->frqs, 0);

	while (voice) {
		FCVoice_initialize(voice);
		voice->channel = &self->super.amiga->channels[voice->index];

		voice->pattern = self->seqs->readUnsignedByte(self->seqs) << 6;
		voice->transpose = self->seqs->readByte(self->seqs);
		voice->soundTranspose = self->seqs->readByte(self->seqs);

		voice = voice->next;
	}

	self->super.super.speed = self->seqs->readUnsignedByte(self->seqs);
	if (!self->super.super.speed) self->super.super.speed = 3;
	self->super.super.tick = self->super.super.speed;
}


//override
void FCPlayer_loader(struct FCPlayer *self, struct ByteArray *stream) {
	int i = 0;
	char id[4]; //:String, 
	int j = 0; 
	int len = 0; 
	int offset = 0; 
	int position = 0; 
	struct AmigaSample *sample = NULL;
	int size = 0;
	int temp = 0;
	int total = 0;
	stream->readMultiByte(stream, id, 4);

	if (id[0] == 'S' && id[1] == 'M' && id[2] == 'O' && id[3] == 'D')
		self->super.super.version = FUTURECOMP_10;
	else if (id[0] == 'F' && id[1] == 'C' && id[2] == '1' && id[3] == '4')
		self->super.super.version = FUTURECOMP_14;
	else return;
	
	// FIXME: need to find out which endianess AS3 defaults to, 
	// probably the one of the local sys

#define init_static_ba(ba, sz) \
	do { \
		ba = &( ba ## _struct); \
		ByteArray_ctor(ba); \
		ByteArray_open_mem(ba, (ba ## _buffer), sz); \
		ba ## _used = 0;  \
	} while (0)
	
	init_static_ba(self->seqs, FCPLAYER_SEQS_BUFFERSIZE);
	init_static_ba(self->vols, FCPLAYER_VOLS_BUFFERSIZE);
	init_static_ba(self->frqs, FCPLAYER_FRQS_BUFFERSIZE);
	init_static_ba(self->pats, FCPLAYER_PATS_BUFFERSIZE);
	
	// WARNING need to take special care about all length accesses
	// of those 4 bytearrays, since they're expected to dynamically grow on each write
	// we in turn have a static size.
	// thus whenever we write into one of them we have to bookkeep how many bytes have been written.
	// we do this by adding to self->xxxx_used.

	ByteArray_set_position(stream, 4);
	self->length = stream->readUnsignedInt(stream);
	ByteArray_set_position(stream, (self->super.super.version == FUTURECOMP_10 ? 100 : 180));
	//self->seqs = new ByteArray();
	
	self->seqs_used += stream->readBytes(stream, self->seqs, 0, self->length);
	self->length /= 13;

	ByteArray_set_position(stream, 12);
	len = stream->readUnsignedInt(stream);
	ByteArray_set_position(stream, 8);
	ByteArray_set_position(stream, stream->readUnsignedInt(stream));
	//FIXME
	//self->pats = new ByteArray();

	self->pats_used += stream->readBytes(stream, self->pats, 0, len);

	xByteArray_set_position(self->pats, xByteArray_get_length(self->pats));
	
	self->pats_used += self->pats->writeByte(self->pats, 0);
	xByteArray_set_position(self->pats, 0);

	ByteArray_set_position(stream, 20);
	len = stream->readUnsignedInt(stream);
	ByteArray_set_position(stream, 16);
	ByteArray_set_position(stream, stream->readUnsignedInt(stream));
	//FIXME
	//self->frqs = new ByteArray();
	self->frqs_used += self->frqs->writeInt(self->frqs, 0x01000000);
	self->frqs_used += self->frqs->writeInt(self->frqs, 0x000000e1);
	self->frqs_used += stream->readBytes(stream, self->frqs, 8, len);

	xByteArray_set_position(self->frqs, xByteArray_get_length(self->frqs));
	self->frqs_used += self->frqs->writeByte(self->frqs, 0xe1);
	xByteArray_set_position(self->frqs, 0);

	ByteArray_set_position(stream, 28);
	len = stream->readUnsignedInt(stream);
	ByteArray_set_position(stream, 24);
	ByteArray_set_position(stream, stream->readUnsignedInt(stream));
	// FIXME
	//self->vols = new ByteArray();
	self->vols_used += self->vols->writeInt(self->vols, 0x01000000);
	self->vols_used += self->vols->writeInt(self->vols, 0x000000e1);
	self->vols_used += stream->readBytes(stream, self->vols, 8, len);

	ByteArray_set_position(stream, 32);
	// sampleOffset in http://xmms-fc.cvs.sourceforge.net/viewvc/xmms-fc/libfc14audiodecoder/src/FC.cpp?revision=1.2&view=markup
	size = stream->readUnsignedInt(stream);
	ByteArray_set_position(stream, 40);

	if (self->super.super.version == FUTURECOMP_10) {
		//samples = new Vector.<AmigaSample>(57, true);
		self->samples_max = 57;
		offset = 0;
	} else {
		//samples = new Vector.<AmigaSample>(200, true);
		self->samples_max = 200;
		offset = 2;
	}

	for (i = 0; i < 10; ++i) {
		len = stream->readUnsignedShort(stream) << 1;

		if (len > 0) {
			if(size == stream->size) continue; // fix for hydra_great_saga.fc3
			position = ByteArray_get_position(stream);
			ByteArray_set_position(stream, size);
			stream->readMultiByte(stream, id, 4);

			if (id[0] == 'S' && id[1] == 'S' && id[2] == 'M' && id[3] == 'P') {
				temp = len;

				for (j = 0; j < 10; ++j) {
					stream->readInt(stream);
					len = stream->readUnsignedShort(stream) << 1;

					if (len > 0) {
						struct AmigaSample temp_sample;
						sample = &temp_sample;
						AmigaSample_ctor(sample);
						//sample = new AmigaSample();
						sample->length = len + 2;
						sample->loop   = stream->readUnsignedShort(stream);
						sample->repeat = stream->readUnsignedShort(stream) << 1;

						if ((sample->loop + sample->repeat) > sample->length)
						sample->repeat = sample->length - sample->loop;

						if ((size + sample->length) > ByteArray_get_length(stream))
						sample->length = ByteArray_get_length(stream) - size;

						sample->pointer = Amiga_store(self->super.amiga, stream, sample->length, size + total);
						sample->loopPtr = sample->pointer + sample->loop;
						unsigned dest_index = (100 + (i * 10) + j);
						assert_op(dest_index, <, self->samples_max);
						self->samples[dest_index] = *sample;
						total += sample->length;
						ByteArray_set_position_rel(stream, 6);
					} else {
						ByteArray_set_position_rel(stream, 10);
					}
				}

				size += (temp + 2);
				ByteArray_set_position(stream, position + 4);
			} else {
				ByteArray_set_position(stream, position);
				struct AmigaSample temp_sample;
				sample = &temp_sample;
				AmigaSample_ctor(sample);
				//sample = new AmigaSample();
				sample->length = len + offset;
				sample->loop   = stream->readUnsignedShort(stream);
				sample->repeat = stream->readUnsignedShort(stream) << 1;

				if ((sample->loop + sample->repeat) > sample->length)
					sample->repeat = sample->length - sample->loop;

				if ((size + sample->length) > ByteArray_get_length(stream))
					sample->length = ByteArray_get_length(stream) - size;

				sample->pointer = Amiga_store(self->super.amiga, stream, sample->length, size);
				sample->loopPtr = sample->pointer + sample->loop;
				assert_op(i, <, self->samples_max);
				self->samples[i] = *sample;
				size += sample->length;
			}
		} else {
			ByteArray_set_position_rel(stream, 4);
		}
	}

	if (self->super.super.version == FUTURECOMP_10) {
		offset = 0; temp = 47;

		for (i = 10; i < 57; ++i) {
			struct AmigaSample temp_sample;
			sample = &temp_sample;
			AmigaSample_ctor(sample);
			//sample = new AmigaSample();
			assert_op(offset + 1, <, sizeof(WAVES));
			sample->length  = WAVES[offset++] << 1;
			sample->loop    = 0;
			sample->repeat  = sample->length;
			
			//position = self->super.amiga->memory->length;
			position = self->super.amiga->vector_count_memory;
			sample->pointer = position;
			sample->loopPtr = position;
			assert_op(i, <, self->samples_max);
			self->samples[i] = *sample;

			len = position + sample->length;
			
			assert_op(len, <, AMIGA_MAX_MEMORY);
			if(len > self->super.amiga->vector_count_memory)
				self->super.amiga->vector_count_memory = len;
			assert_op((len - position), >=, 0);
			assert_op(temp + (len - position), <=, sizeof(WAVES)); // <= because of x++
			for (j = position; j < len; ++j)
				self->super.amiga->memory[j] = WAVES[temp++];
		}
	} else {
		ByteArray_set_position(stream, 36);
		size = stream->readUnsignedInt(stream);
		ByteArray_set_position(stream, 100);

		for (i = 10; i < 90; ++i) {
			len = stream->readUnsignedByte(stream) << 1;
			if (len < 2) continue;

			struct AmigaSample temp_sample;
			sample = &temp_sample;
			AmigaSample_ctor(sample);
			//sample = new AmigaSample();
			sample->length = len;
			sample->loop   = 0;
			sample->repeat = sample->length;

			if ((size + sample->length) > ByteArray_get_length(stream))
				sample->length = ByteArray_get_length(stream) - size;

			sample->pointer = Amiga_store(self->super.amiga, stream, sample->length, size);
			sample->loopPtr = sample->pointer;
			assert_op(i, <, self->samples_max);
			self->samples[i] = *sample;
			size += sample->length;
		}
	}

	self->length *= 13;
}


