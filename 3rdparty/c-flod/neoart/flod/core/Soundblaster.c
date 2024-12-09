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

#include "Soundblaster.h"
#include "../flod_internal.h"
#include "../../../flashlib/Common.h"

//extends CoreMixer
void Soundblaster_defaults(struct Soundblaster* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void Soundblaster_ctor(struct Soundblaster* self) {
	CLASS_CTOR_DEF(Soundblaster);
	// original constructor code goes here
	PFUNC();
	//super();
	CoreMixer_ctor(&self->super);
	self->super.type = CM_SOUNDBLASTER;
	
	//vtable
	self->super.fast = Soundblaster_fast;
	self->super.accurate = Soundblaster_accurate;
}

struct Soundblaster* Soundblaster_new(void) {
	CLASS_NEW_BODY(Soundblaster);
}

void Soundblaster_setup(struct Soundblaster* self, unsigned int len) {
	DEBUGP("calling %s with %d channels\n", __FUNCTION__, len);
	assert_op(len, <=, SOUNDBLASTER_MAX_CHANNELS);
/*	int i = 1;
	self->channels = new Vector.<SBChannel>(len, true);
	self->channels[0] = new SBChannel();

	for (; i < len; ++i)
		channels[i] = channels[int(i - 1)].next = new SBChannel(); */
	unsigned i;
	for(i = 0; i < len; i++){
		SBChannel_ctor(&self->channels[i]);
		if(i) {
			self->channels[i - 1].next = &self->channels[i];
		}
	}
}

//override
void Soundblaster_initialize(struct Soundblaster* self) {
	PFUNC();
	struct SBChannel *chan = &self->channels[0];
	//super->initialize();
	//CoreMixer_initialize(&self->super);

	while (chan) {
		//chan->initialize();
		SBChannel_initialize(chan);
		chan = chan->next;
	}
}

#include "Hardware.h"

//override
void Soundblaster_fast(struct Soundblaster* self) {
	PFUNC();
	struct SBChannel *chan;
	//d:Vector.<Number>,
	Number* d;
	
	int i = 0;
	int mixed = 0;
	int mixLen = 0;
	int mixPos = 0;
	struct SBSample *s = NULL;
	struct Sample *sample = NULL;
	//int size = self->bufferSize;
	int size = CoreMixer_get_bufferSize(&self->super);
	int toMix = 0;
	Number value = NAN;

	if (self->super.completed) {
		if (!self->super.remains) return;
		size = self->super.remains;
	}

	while (mixed < size) {
		if (!self->super.samplesLeft) {
			self->super.player->process(self->super.player);
			self->super.player->fast(self->super.player);
			self->super.samplesLeft = self->super.samplesTick;

			if (self->super.completed) {
				size = mixed +self->super.samplesTick;

				if (size > CoreMixer_get_bufferSize(&self->super)) {
					self->super.remains = size - CoreMixer_get_bufferSize(&self->super);
					size = CoreMixer_get_bufferSize(&self->super);
				}
			}
		}

		toMix = self->super.samplesLeft;
		if ((mixed + toMix) >= size) toMix = size - mixed;
		mixLen = mixPos + toMix;
		chan = &self->channels[0];

		while (chan) {
			if (!chan->enabled) {
				chan = chan->next;
				continue;
			}

			s = chan->sample;
			d = s->data;
			assert_op(mixPos, <, COREMIXER_MAX_BUFFER);
			sample  = &self->super.buffer[mixPos];

			for (i = mixPos; i < mixLen; ++i) {
				if (chan->index != chan->pointer) {
					if (chan->index >= chan->length) {
						if (!s->loopMode) {
							chan->enabled = 0;
							break;
						} else {
							chan->pointer = s->loopStart + (chan->index - chan->length);
							chan->length  = s->length;

							if (s->loopMode == 2) {
								if (!chan->dir) {
									chan->dir = s->length + s->loopStart - 1;
								} else {
									chan->dir = 0;
								}
							}
						}
					} else chan->pointer = chan->index;

					if (!chan->mute) {
						unsigned int temp;
						temp = chan->dir ? chan->dir - chan->pointer : chan->pointer;
						assert_op(temp, <, SBSAMPLE_MAX_DATA);
						value = d[temp];

						chan->ldata = value * chan->lvol;
						chan->rdata = value * chan->rvol;
					} else {
						chan->ldata = 0.0;
						chan->rdata = 0.0;
					}
				}

				chan->index = chan->pointer + chan->delta;

				if ((chan->fraction += chan->speed) >= 1.0) {
					chan->index++;
					chan->fraction--;
				}

				sample->l += chan->ldata;
				sample->r += chan->rdata;
				sample = sample->next;
			}
			chan = chan->next;
		}

		mixPos = mixLen;
		mixed += toMix;
		self->super.samplesLeft -= toMix;
	}

	process_wave(self, size);

}

//override
void Soundblaster_accurate(struct Soundblaster* self) {
	PFUNC();
	struct SBChannel *chan = NULL;
	//d1:Vector.<Number>;
	//d2:Vector.<Number>;
	Number *d1 = null;
	Number *d2 = null;
	int delta = 0;
	int i = 0; 
	int mixed = 0; 
	int mixLen = 0; 
	int mixPos = 0; 
	Number mixValue = NAN;
	struct SBSample *s1;
	struct SBSample *s2;
	struct Sample *sample;
	//int size = bufferSize;
	int size = CoreMixer_get_bufferSize(&self->super);
	int toMix = 0; 
	Number value = NAN;

	if (self->super.completed) {
		if (!self->super.remains) return;
		size = self->super.remains;
	}

	while (mixed < size) {
		if (!self->super.samplesLeft) {
			self->super.player->process(self->super.player);
			self->super.player->accurate(self->super.player);
			self->super.samplesLeft = self->super.samplesTick;

			if (self->super.completed) {
				size = mixed + self->super.samplesTick;

				if (size > CoreMixer_get_bufferSize(&self->super)) {
					self->super.remains = size - CoreMixer_get_bufferSize(&self->super);
					size = CoreMixer_get_bufferSize(&self->super);
				}
			}
		}

		toMix = self->super.samplesLeft;
		if ((mixed + toMix) >= size) toMix = size - mixed;
		mixLen = mixPos + toMix;
		chan = &self->channels[0];

		while (chan) {
			if (!chan->enabled) {
				chan = chan->next;
				continue;
			}

			s1 = chan->sample;
			d1 = s1->data;
			s2 = chan->oldSample;
			if (s2) d2 = s2->data;
			
			assert_op(mixPos, <, COREMIXER_MAX_BUFFER);

			sample = &self->super.buffer[mixPos];

			for (i = mixPos; i < mixLen; ++i) {
				if(chan->mute)
					value = 0.0;
				else {
					assert_op(chan->pointer, <, SBSAMPLE_MAX_DATA);
					value = d1[chan->pointer];
				}
				//value = chan->mute ? 0.0 : d1[chan->pointer];
				assert_op(chan->pointer + chan->dir, <, SBSAMPLE_MAX_DATA);
				value += (d1[chan->pointer + chan->dir] - value) * chan->fraction;

				if ((chan->fraction += chan->speed) >= 1.0) {
					delta = (int) chan->fraction;
					chan->fraction -= delta;

					if (chan->dir > 0) {
						chan->pointer += delta;

						if (chan->pointer > chan->length) {
							chan->fraction += chan->pointer - chan->length;
							chan->pointer = chan->length;
						}
					} else {
						chan->pointer -= delta;

						if (chan->pointer < chan->length) {
							chan->fraction += chan->length - chan->pointer;
							chan->pointer = chan->length;
						}
					}
				}

				if (!chan->mixCounter) {
					sample->l += value * chan->lvol;
					sample->r += value * chan->rvol;

					if (chan->volCounter) {
						chan->lvol += chan->lvolDelta;
						chan->rvol += chan->rvolDelta;
						chan->volCounter--;
					} else if (chan->panCounter) {
						chan->lpan += chan->lpanDelta;
						chan->rpan += chan->rpanDelta;
						chan->panCounter--;

						chan->lvol = chan->volume * chan->lpan;
						chan->rvol = chan->volume * chan->rpan;
					}
				} else {
					if (s2) {
						if(chan->mute) {
							mixValue = 0.0;
						} else {
							assert_op(chan->oldPointer, <, SBSAMPLE_MAX_DATA);
							mixValue = d2[chan->oldPointer];
						}
						//mixValue = chan->mute ? 0.0 : d2[chan->oldPointer];
						assert_op(chan->oldPointer + chan->oldDir, <, SBSAMPLE_MAX_DATA);
						mixValue += (d2[chan->oldPointer + chan->oldDir] - mixValue) * chan->oldFraction;

						if ((chan->oldFraction += chan->oldSpeed) > 1) {
							delta = (int) chan->oldFraction;
							chan->oldFraction -= delta;

							if (chan->oldDir > 0) {
								chan->oldPointer += delta;

								if (chan->oldPointer > chan->oldLength) {
									chan->oldFraction += chan->oldPointer - chan->oldLength;
									chan->oldPointer = chan->oldLength;
								}
							} else {
								chan->oldPointer -= delta;

								if (chan->oldPointer < chan->oldLength) {
									chan->oldFraction += chan->oldLength - chan->oldPointer;
									chan->oldPointer = chan->oldLength;
								}
							}
						}

						sample->l += (value * chan->lmixRampU) + (mixValue * chan->lmixRampD);
						sample->r += (value * chan->rmixRampU) + (mixValue * chan->rmixRampD);

						chan->lmixRampD -= chan->lmixDeltaD;
						chan->rmixRampD -= chan->rmixDeltaD;
					} else {
						sample->l += (value * chan->lmixRampU);
						sample->r += (value * chan->rmixRampU);
					}

					chan->lmixRampU += chan->lmixDeltaU;
					chan->rmixRampU += chan->rmixDeltaU;
					chan->mixCounter--;

					if (chan->oldPointer == chan->oldLength) {
						if (!s2->loopMode) {
							s2 = null;
							chan->oldPointer = 0;
						} else if (s2->loopMode == 1) {
							chan->oldPointer = s2->loopStart;
							chan->oldLength  = s2->length;
						} else {
							if (chan->oldDir > 0) {
								chan->oldPointer = s2->length - 1;
								chan->oldLength  = s2->loopStart;
								chan->oldDir     = -1;
							} else {
								chan->oldFraction -= 1;
								chan->oldPointer   = s2->loopStart;
								chan->oldLength    = s2->length;
								chan->oldDir       = 1;
							}
						}
					}
				}

				if (chan->pointer == chan->length) {
					if (!s1->loopMode) {
						chan->enabled = 0;
						break;
					} else if (s1->loopMode == 1) {
						chan->pointer = s1->loopStart;
						chan->length  = s1->length;
					} else {
						if (chan->dir > 0) {
							chan->pointer = s1->length - 1;
							chan->length  = s1->loopStart;
							chan->dir     = -1;
						} else {
							chan->fraction -= 1;
							chan->pointer   = s1->loopStart;
							chan->length    = s1->length;
							chan->dir       = 1;
						}
					}
				}
				sample = sample->next;
			}
			chan = chan->next;
		}

		mixPos = mixLen;
		mixed += toMix;
		self->super.samplesLeft -= toMix;
	}
	
	process_wave(self, size);
}

