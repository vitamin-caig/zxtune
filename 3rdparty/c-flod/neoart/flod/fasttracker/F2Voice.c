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

#include "F2Voice.h"
#include "../flod_internal.h"

// for flags enum
#include "F2Player.h"

// FIXME only needed for round()
#include <math.h>


static const signed char AUTOVIBRATO[] = {
          0, -2, -3, -5, -6, -8, -9,-11,-12,-14,-16,-17,-19,-20,-22,-23,
        -24,-26,-27,-29,-30,-32,-33,-34,-36,-37,-38,-39,-41,-42,-43,-44,
        -45,-46,-47,-48,-49,-50,-51,-52,-53,-54,-55,-56,-56,-57,-58,-59,
        -59,-60,-60,-61,-61,-62,-62,-62,-63,-63,-63,-64,-64,-64,-64,-64,
        -64,-64,-64,-64,-64,-64,-63,-63,-63,-62,-62,-62,-61,-61,-60,-60,
        -59,-59,-58,-57,-56,-56,-55,-54,-53,-52,-51,-50,-49,-48,-47,-46,
        -45,-44,-43,-42,-41,-39,-38,-37,-36,-34,-33,-32,-30,-29,-27,-26,
        -24,-23,-22,-20,-19,-17,-16,-14,-12,-11, -9, -8, -6, -5, -3, -2,
          0,  2,  3,  5,  6,  8,  9, 11, 12, 14, 16, 17, 19, 20, 22, 23,
         24, 26, 27, 29, 30, 32, 33, 34, 36, 37, 38, 39, 41, 42, 43, 44,
         45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59,
         59, 60, 60, 61, 61, 62, 62, 62, 63, 63, 63, 64, 64, 64, 64, 64,
         64, 64, 64, 64, 64, 64, 63, 63, 63, 62, 62, 62, 61, 61, 60, 60,
         59, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46,
         45, 44, 43, 42, 41, 39, 38, 37, 36, 34, 33, 32, 30, 29, 27, 26,
         24, 23, 22, 20, 19, 17, 16, 14, 12, 11,  9,  8,  6,  5,  3,  2,
};

static const unsigned char VIBRATO[] = {
          0, 24, 49, 74, 97,120,141,161,180,197,212,224,235,244,250,253,
        255,253,250,244,235,224,212,197,180,161,141,120, 97, 74, 49, 24,
};

void F2Voice_defaults(struct F2Voice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void F2Voice_ctor(struct F2Voice* self, int index) {
	CLASS_CTOR_DEF(F2Voice);
	// original constructor code goes here
	self->index = index;
	F2Envelope_ctor(&self->volEnvelope);
	F2Envelope_ctor(&self->panEnvelope);
}

struct F2Voice* F2Voice_new(int index) {
	CLASS_NEW_BODY(F2Voice, index);
}


void F2Voice_reset(struct F2Voice* self) {
	self->volume   = self->sample->super.volume;
	self->panning  = self->sample->panning;
	self->finetune = (self->sample->finetune >> 3) << 2;
	self->keyoff   = 0;
	self->volDelta = 0;

	self->fadeEnabled = 0;      
	self->fadeDelta   = 0;
	self->fadeVolume  = 65536;

	self->autoVibratoPos = 0;
	self->autoSweep      = 1;
	self->autoSweepPos   = 0;
	self->vibDelta       = 0;
	self->portaPeriod    = 0;
	self->vibratoReset   = 0;

	if ((self->waveControl & 15) < 4) self->vibratoPos = 0;
	if ((self->waveControl >> 4) < 4) self->tremoloPos = 0;
}

int F2Voice_autoVibrato(struct F2Voice* self) {
	int delta = 0;

	self->autoVibratoPos = (self->autoVibratoPos + self->playing->vibratoSpeed) & 255;

	switch (self->playing->vibratoType) {
		case 0:
			delta = AUTOVIBRATO[self->autoVibratoPos];
			break;
		case 1:
			if (self->autoVibratoPos < 128) delta = -64;
			else delta = 64;
			break;
		case 2:
			delta = ((64 + (self->autoVibratoPos >> 1)) & 127) - 64;
			break;
		case 3:
			delta = ((64 - (self->autoVibratoPos >> 1)) & 127) - 64;
			break;
		default:
			break;
	}

	delta *= self->playing->vibratoDepth;

	if (self->autoSweep) {
		if (!self->playing->vibratoSweep) {
			self->autoSweep = 0;
		} else {
			if (self->autoSweepPos > self->playing->vibratoSweep) {
				if (self->autoSweepPos & 2) delta *= (self->autoSweepPos / self->playing->vibratoSweep);
				self->autoSweep = 0;
			} else {
				delta *= ((++(self->autoSweepPos)) / self->playing->vibratoSweep);
			}
		}
	}

	self->flags |= UPDATE_PERIOD;
	return (delta >> 6);
}

void F2Voice_tonePortamento(struct F2Voice* self) {
	if (!self->glissPeriod) self->glissPeriod = self->period;

	// FIXME BLOAT these two blocks are equivalent, execpt for - / + and <= / >=
	if (self->period < self->portaPeriod) {
		self->glissPeriod += self->portaSpeed << 2;

		// FIXME round expects a double
		if (!self->glissando) self->period = self->glissPeriod;
		else self->period = (int)(round((float) self->glissPeriod / 64.f)) << 6;
		//else self->period = round(self->glissPeriod / 64) << 6;

		if (self->period >= self->portaPeriod) {
			self->period = self->portaPeriod;
			self->glissPeriod = self->portaPeriod = 0;
		}
	} else if (self->period > self->portaPeriod) {
		self->glissPeriod -= self->portaSpeed << 2;

		if (!self->glissando) self->period = self->glissPeriod;
		else self->period = (int)(round((float) self->glissPeriod / 64.f)) << 6;
		//else self->period = round(self->glissPeriod / 64) << 6;

		if (self->period <= self->portaPeriod) {
			self->period = self->portaPeriod;
			self->glissPeriod = self->portaPeriod = 0;
		}
	}

	self->flags |= UPDATE_PERIOD;
}

void F2Voice_tremolo(struct F2Voice* self) {
	int delta = 255;
	int position = self->tremoloPos & 31;

	switch ((self->waveControl >> 4) & 3) {
		case 0:
			delta = VIBRATO[position];
			break;
		case 1:
			delta = position << 3;
			break;
		default:
			break;
	}

	self->volDelta = (delta * self->tremoloDepth) >> 6;
	if (self->tremoloPos > 31) self->volDelta = -self->volDelta;
	self->tremoloPos = (self->tremoloPos + self->tremoloSpeed) & 63;

	self->flags |= UPDATE_VOLUME;
}

void F2Voice_tremor(struct F2Voice* self) {
	if (self->tremorPos == self->tremorOn) {
		self->tremorVolume = self->volume;
		self->volume = 0;
		self->flags |= UPDATE_VOLUME;
	} else if (self->tremorPos == self->tremorOff) {
		self->tremorPos = 0;
		self->volume = self->tremorVolume;
		self->flags |= UPDATE_VOLUME;
	}
	++(self->tremorPos);
}

void F2Voice_vibrato(struct F2Voice* self) {
	int delta = 255;
	int position = self->vibratoPos & 31;

	switch (self->waveControl & 3) {
		case 0:
			delta = VIBRATO[position];
			break;
		case 1:
			delta = position << 3;
			if (self->vibratoPos > 31) delta = 255 - delta;
			break;
		default:
			break;
	}

	self->vibDelta = (delta * self->vibratoDepth) >> 7;
	if (self->vibratoPos > 31) self->vibDelta = -self->vibDelta;
	self->vibratoPos = (self->vibratoPos + self->vibratoSpeed) & 63;

	self->flags |= UPDATE_PERIOD;
}

