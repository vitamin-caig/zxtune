/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.0 - 2012/03/05

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "F2Player.h"
#include "F2Player_const.h"
#include "../flod_internal.h"
#include "../flod.h"

//FIXME only needed for pow
#include <math.h>

static void envelope(struct F2Voice* voice, struct F2Envelope* envelope, struct F2Data* data);
static int amiga(int note, int finetune);
static void retrig(struct F2Voice* voice);

void F2Player_defaults(struct F2Player* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

/* mixer default value = null */
void F2Player_ctor(struct F2Player* self, struct Soundblaster *mixer) {
	CLASS_CTOR_DEF(F2Player);
	// original constructor code goes here
	SBPlayer_ctor(&self->super, mixer);
	//super(mixer);
	//vtable
	self->super.super.accurate = F2Player_accurate;
	self->super.super.fast = F2Player_fast;
	self->super.super.initialize = F2Player_initialize;
	self->super.super.loader = F2Player_loader;
	self->super.super.process = F2Player_process;
	
	self->super.super.min_filesize = 360;
}

struct F2Player* F2Player_new(struct Soundblaster *mixer) {
	CLASS_NEW_BODY(F2Player, mixer);
}

//override
void F2Player_process(struct F2Player* self) {
	int com = 0;
	struct F2Point *curr = NULL;
	struct F2Instrument *instr = NULL;
	int i = 0;
	int jumpFlag = 0;
	struct F2Point *next = NULL;
	int paramx = 0;
	int paramy = 0;
	int porta = 0;
	struct F2Row *row = NULL;
	struct F2Sample *sample = NULL;
	int slide = 0;
	int value = 0;
	struct F2Voice *voice = &self->voices[0];

	if (!self->super.super.tick) {
		if (self->nextOrder >= 0) self->order = self->nextOrder;
		if (self->nextPosition >= 0) self->position = self->nextPosition;

		self->nextOrder = self->nextPosition = -1;
		assert_op(self->order, <, SBPLAYER_MAX_TRACKS);
		unsigned idx = self->super.track[self->order];
		assert_op(idx, <, F2PLAYER_MAX_PATTERNS);
		self->pattern = &self->patterns[idx];

		while (voice) {
			idx = self->position + voice->index;
			assert_op(idx, <, F2PATTERN_MAX_ROWS);
			row = &self->pattern->rows[idx];
			com = row->volume >> 4;
			porta = (int)(row->effect == FX_TONE_PORTAMENTO || row->effect == FX_TONE_PORTA_VOLUME_SLIDE || com == VX_TONE_PORTAMENTO);
			paramx = row->param >> 4;
			voice->keyoff = 0;

			if (voice->arpDelta) {
				voice->arpDelta = 0;
				voice->flags |= UPDATE_PERIOD;
			}

			if (row->instrument) {
				voice->instrument = 
					((row->instrument < self->vector_count_instruments)
					? &self->instruments[row->instrument] : null);
					
				F2Envelope_reset(&voice->volEnvelope);
				F2Envelope_reset(&voice->panEnvelope);
				voice->flags |= (UPDATE_VOLUME | UPDATE_PANNING | SHORT_RAMP);
			} else if (row->note == KEYOFF_NOTE || (row->effect == FX_KEYOFF && !row->param)) {
				voice->fadeEnabled = 1;
				voice->keyoff = 1;
			}

			if (row->note && row->note != KEYOFF_NOTE) {
				if (voice->instrument) {
					instr  = voice->instrument;
					value  = row->note - 1;
					assert_op(value, <, F2INSTRUMENT_MAX_NOTESAMPLES);
					idx = instr->noteSamples[value];
					assert_op(idx, <, F2INSTRUMENT_MAX_SAMPLES);
					sample = &instr->samples[idx];
					value += sample->relative;

					if (value >= LOWER_NOTE && value <= HIGHER_NOTE) {
						if (!porta) {
							voice->note = value;
							voice->sample = sample;

							if (row->instrument) {
								voice->volEnabled = instr->volEnabled;
								voice->panEnabled = instr->panEnabled;
								voice->flags |= UPDATE_ALL;
							} else {
								voice->flags |= (UPDATE_PERIOD | UPDATE_TRIGGER);
							}
						}

						if (row->instrument) {
							F2Voice_reset(voice);
							voice->fadeDelta = instr->fadeout;
						} else {
							voice->finetune = (sample->finetune >> 3) << 2;
						}

						if (row->effect == FX_EXTENDED_EFFECTS && paramx == EX_SET_FINETUNE)
							voice->finetune = ((row->param & 15) - 8) << 3;

						if (self->linear) {
							value = ((120 - value) << 6) - voice->finetune;
						} else {
							value = amiga(value, voice->finetune);
						}

						if (!porta) {
							voice->period = value;
							voice->glissPeriod = 0;
						} else {
							voice->portaPeriod = value;
						}
					}
				} else {
					voice->volume = 0;
					voice->flags = (UPDATE_VOLUME | SHORT_RAMP);
				}
			} else if (voice->vibratoReset) {
				if (row->effect != FX_VIBRATO && row->effect != FX_VIBRATO_VOLUME_SLIDE) {
					voice->vibDelta = 0;
					voice->vibratoReset = 0;
					voice->flags |= UPDATE_PERIOD;
				}
			}

			if (row->volume) {
				if (row->volume >= 16 && row->volume <= 80) {
					voice->volume = row->volume - 16;
					voice->flags |= (UPDATE_VOLUME | SHORT_RAMP);
				} else {
					paramy = row->volume & 15;

					switch (com) {
						case VX_FINE_VOLUME_SLIDE_DOWN:
							voice->volume -= paramy;
							if (voice->volume < 0) voice->volume = 0;
							voice->flags |= UPDATE_VOLUME;
							break;
						case VX_FINE_VOLUME_SLIDE_UP:
							voice->volume += paramy;
							if (voice->volume > 64) voice->volume = 64;
							voice->flags |= UPDATE_VOLUME;
							break;
						case VX_SET_VIBRATO_SPEED:
							if (paramy) voice->vibratoSpeed = paramy;
							break;
						case VX_VIBRATO:
							if (paramy) voice->vibratoDepth = paramy << 2;
							break;
						case VX_SET_PANNING:
							voice->panning = paramy << 4;
							voice->flags |= UPDATE_PANNING;
							break;
						case VX_TONE_PORTAMENTO:
							if (paramy) voice->portaSpeed = paramy << 4;
							break;
						default:
							break;
					}
				}
			}

			if (row->effect) {
				paramy = row->param & 15;

				switch (row->effect) {
					case FX_PORTAMENTO_UP:
						if (row->param) voice->portaU = row->param << 2;
						break;
					case FX_PORTAMENTO_DOWN:
						if (row->param) voice->portaD = row->param << 2;
						break;
					case FX_TONE_PORTAMENTO:
						if (row->param && com != VX_TONE_PORTAMENTO) voice->portaSpeed = row->param;
						break;
					case FX_VIBRATO:
						voice->vibratoReset = 1;
						break;
					case FX_TONE_PORTA_VOLUME_SLIDE:
						if (row->param) voice->volSlide = row->param;
						break;
					case FX_VIBRATO_VOLUME_SLIDE:
						if (row->param) voice->volSlide = row->param;
						voice->vibratoReset = 1;
						break;
					case FX_TREMOLO:
						if (paramx) voice->tremoloSpeed = paramx;
						if (paramy) voice->tremoloDepth = paramy;
						break;
					case FX_SET_PANNING:
						voice->panning = row->param;
						voice->flags |= UPDATE_PANNING;
						break;
					case FX_SAMPLE_OFFSET:
						if (row->param) voice->sampleOffset = row->param << 8;

						if (voice->sampleOffset >= voice->sample->super.length) {
							voice->volume = 0;
							voice->sampleOffset = 0;
							voice->flags &= ~(UPDATE_PERIOD | UPDATE_TRIGGER);
							voice->flags |=  (UPDATE_VOLUME | SHORT_RAMP);
						}
						break;
					case FX_VOLUME_SLIDE:
						if (row->param) voice->volSlide = row->param;
						break;
					case FX_POSITION_JUMP:
						self->nextOrder = row->param;

						if (self->nextOrder >= self->super.length) self->complete = 1;
						else self->nextPosition = 0;

						jumpFlag      = 1;
						self->patternOffset = 0;
						break;
					case FX_SET_VOLUME:
						voice->volume = row->param;
						voice->flags |= (UPDATE_VOLUME | SHORT_RAMP);
						break;
					case FX_PATTERN_BREAK:
						self->nextPosition  = ((paramx * 10) + paramy) * self->super.super.channels;
						self->patternOffset = 0;

						if (!jumpFlag) {
							self->nextOrder = self->order + 1;

							if (self->nextOrder >= self->super.length) {
								self->complete = 1;
								self->nextPosition = -1;
							}
						}
						break;
					case FX_EXTENDED_EFFECTS:
						switch (paramx) {
							case EX_FINE_PORTAMENTO_UP:
								if (paramy) voice->finePortaU = paramy << 2;
								voice->period -= voice->finePortaU;
								voice->flags |= UPDATE_PERIOD;
								break;
							case EX_FINE_PORTAMENTO_DOWN:
								if (paramy) voice->finePortaD = paramy << 2;
								voice->period += voice->finePortaD;
								voice->flags |= UPDATE_PERIOD;
								break;
							case EX_GLISSANDO_CONTROL:
								voice->glissando = paramy;
								break;
							case EX_VIBRATO_CONTROL:
								voice->waveControl = (voice->waveControl & 0xf0) | paramy;
								break;
							case EX_PATTERN_LOOP:
								if (!paramy) {
									voice->patternLoopRow = self->patternOffset = self->position;
								} else {
									if (!voice->patternLoop) {
										voice->patternLoop = paramy;
									} else {
										voice->patternLoop--;
									}

									if (voice->patternLoop)
										self->nextPosition = voice->patternLoopRow;
								}
								break;
							case EX_TREMOLO_CONTROL:
								voice->waveControl = (voice->waveControl & 0x0f) | (paramy << 4);
								break;
							case EX_FINE_VOLUME_SLIDE_UP:
								if (paramy) voice->fineSlideU = paramy;
								voice->volume += voice->fineSlideU;
								voice->flags |= UPDATE_VOLUME;
								break;
							case EX_FINE_VOLUME_SLIDE_DOWN:
								if (paramy) voice->fineSlideD = paramy;
								voice->volume -= voice->fineSlideD;
								voice->flags |= UPDATE_VOLUME;
								break;
							case EX_NOTE_DELAY:
								voice->delay = voice->flags;
								voice->flags = 0;
								break;
							case EX_PATTERN_DELAY:
								self->patternDelay = paramy * self->super.timer;
								break;
							default:
								break;
						}
						break;
					case FX_SET_SPEED:
						if (!row->param) break;
						if (row->param < 32) self->super.timer = row->param;
						else self->super.mixer->super.samplesTick = 110250 / row->param;
						break;
					case FX_SET_GLOBAL_VOLUME:
						self->super.master = row->param;
						if (self->super.master > 64) self->super.master = 64;
						voice->flags |= UPDATE_VOLUME;
						break;
					case FX_GLOBAL_VOLUME_SLIDE:
						if (row->param) voice->volSlideMaster = row->param;
						break;
					case FX_SET_ENVELOPE_POSITION:
						if (!voice->instrument || !voice->instrument->volEnabled) break;
						instr  = voice->instrument;
						value  = row->param;
						paramx = instr->volData.total;

						for (i = 0; i < paramx; ++i)
						if (value < instr->volData.points[i].frame) break;

						voice->volEnvelope.position = --i;
						paramx--;

						if ((instr->volData.flags & ENVELOPE_LOOP) && i == instr->volData.loopEnd) {
							i = voice->volEnvelope.position = instr->volData.loopStart;
							value = instr->volData.points[i].frame;
							voice->volEnvelope.frame = value;
						}

						if (i >= paramx) {
							voice->volEnvelope.value = instr->volData.points[paramx].value;
							voice->volEnvelope.stopped = 1;
						} else {
							voice->volEnvelope.stopped = 0;
							voice->volEnvelope.frame = value;
							if (value > instr->volData.points[i].frame) voice->volEnvelope.position++;

							assert_op(i + 1, <, F2DATA_MAX_POINTS);
							curr = &instr->volData.points[i];
							next = &instr->volData.points[++i];
							value = next->frame - curr->frame;

							voice->volEnvelope.delta = value ? ((next->value - curr->value) << 8) / value : 0;
							voice->volEnvelope.fraction = (curr->value << 8);
						}
						break;
					case FX_PANNING_SLIDE:
						if (row->param) voice->panSlide = row->param;
						break;
					case FX_MULTI_RETRIG_NOTE:
						if (paramx) voice->retrigx = paramx;
						if (paramy) voice->retrigy = paramy;

						if (!row->volume && voice->retrigy) {
							com = self->super.super.tick + 1;
							if (com % voice->retrigy) break;
							if (row->volume > 80 && voice->retrigx) retrig(voice);
						}
						break;
					case FX_TREMOR:
						if (row->param) {
							voice->tremorOn  = ++paramx;
							voice->tremorOff = ++paramy + paramx;
						}
						break;
					case FX_EXTRA_FINE_PORTAMENTO:
						if (paramx == 1) {
							if (paramy) voice->xtraPortaU = paramy;
							voice->period -= voice->xtraPortaU;
							voice->flags |= UPDATE_PERIOD;
						} else if (paramx == 2) {
							if (paramy) voice->xtraPortaD = paramy;
							voice->period += voice->xtraPortaD;
							voice->flags |= UPDATE_PERIOD;
						}
						break;
				}
			}
			voice = voice->next;
		}
	} else {
		while (voice) {
			row = &self->pattern->rows[self->position + voice->index];

			if (voice->delay) {
				if ((row->param & 15) == self->super.super.tick) {
					voice->flags = voice->delay;
					voice->delay = 0;
				} else {
					voice = voice->next;
					continue;
				}
			}

			if (row->volume) {
				paramx = row->volume >> 4;
				paramy = row->volume & 15;

				switch (paramx) {
					case VX_VOLUME_SLIDE_DOWN:
						voice->volume -= paramy;
						if (voice->volume < 0) voice->volume = 0;
						voice->flags |= UPDATE_VOLUME;
						break;
					case VX_VOLUME_SLIDE_UP:
						voice->volume += paramy;
						if (voice->volume > 64) voice->volume = 64;
						voice->flags |= UPDATE_VOLUME;
						break;
					case VX_VIBRATO:
						F2Voice_vibrato(voice);
						break;
					case VX_PANNING_SLIDE_LEFT:
						voice->panning -= paramy;
						if (voice->panning < 0) voice->panning = 0;
						voice->flags |= UPDATE_PANNING;
						break;
					case VX_PANNING_SLIDE_RIGHT:
						voice->panning += paramy;
						if (voice->panning > 255) voice->panning = 255;
						voice->flags |= UPDATE_PANNING;
						break;
					case VX_TONE_PORTAMENTO:
						if (voice->portaPeriod) F2Voice_tonePortamento(voice);
						break;
					default: 
						break;
				}
			}

			paramx = row->param >> 4;
			paramy = row->param & 15;

			switch (row->effect) {
				case FX_ARPEGGIO:
					if (!row->param) break;
					value = (self->super.super.tick - self->super.timer) % 3;
					if (value < 0) value += 3;
					if (self->super.super.tick == 2 && self->super.timer == 18) value = 0;

					if (!value) {
						voice->arpDelta = 0;
					} else if (value == 1) {
						if (self->linear) {
							voice->arpDelta = -(paramy << 6);
						} else {
							value = amiga(voice->note + paramy, voice->finetune);
							voice->arpDelta = value - voice->period;
						}
					} else {
						if (self->linear) {
							voice->arpDelta = -(paramx << 6);
						} else {
							value = amiga(voice->note + paramx, voice->finetune);
							voice->arpDelta = value - voice->period;
						}
					}

					voice->flags |= UPDATE_PERIOD;
					break;
				case FX_PORTAMENTO_UP:
					voice->period -= voice->portaU;
					if (voice->period < 0) voice->period = 0;
					voice->flags |= UPDATE_PERIOD;
					break;
				case FX_PORTAMENTO_DOWN:
					voice->period += voice->portaD;
					if (voice->period > 9212) voice->period = 9212;
					voice->flags |= UPDATE_PERIOD;
					break;
				case FX_TONE_PORTAMENTO:
					if (voice->portaPeriod) F2Voice_tonePortamento(voice);
					break;
				case FX_VIBRATO:
					if (paramx) voice->vibratoSpeed = paramx;
					if (paramy) voice->vibratoDepth = paramy << 2;
					F2Voice_vibrato(voice);
					break;
				case FX_TONE_PORTA_VOLUME_SLIDE:
					slide = 1;
					if (voice->portaPeriod) F2Voice_tonePortamento(voice);
					break;
				case FX_VIBRATO_VOLUME_SLIDE:
					slide = 1;
					F2Voice_vibrato(voice);
					break;
				case FX_TREMOLO:
					F2Voice_tremolo(voice);
					break;
				case FX_VOLUME_SLIDE:
					slide = 1;
					break;
				case FX_EXTENDED_EFFECTS:
					switch (paramx) {
						case EX_RETRIG_NOTE:
							if ((self->super.super.tick % paramy) == 0) {
								F2Envelope_reset(&voice->volEnvelope);
								F2Envelope_reset(&voice->panEnvelope);
								voice->flags |= (UPDATE_VOLUME | UPDATE_PANNING | UPDATE_TRIGGER);
							}
							break;
						case EX_NOTE_CUT:
							if (self->super.super.tick == paramy) {
								voice->volume = 0;
								voice->flags |= UPDATE_VOLUME;
							}
							break;
						default:
							break;
					}
					break;
				case FX_GLOBAL_VOLUME_SLIDE:
					paramx = voice->volSlideMaster >> 4;
					paramy = voice->volSlideMaster & 15;

					if (paramx) {
						self->super.master += paramx;
						if (self->super.master > 64) self->super.master = 64;
						voice->flags |= UPDATE_VOLUME;
					} else if (paramy) {
						self->super.master -= paramy;
						if (self->super.master < 0) self->super.master = 0;
						voice->flags |= UPDATE_VOLUME;
					}
					break;
				case FX_KEYOFF:
					if (self->super.super.tick == row->param) {
						voice->fadeEnabled = 1;
						voice->keyoff = 1;
					}
					break;
				case FX_PANNING_SLIDE:
					paramx = voice->panSlide >> 4;
					paramy = voice->panSlide & 15;

					if (paramx) {
						voice->panning += paramx;
						if (voice->panning > 255) voice->panning = 255;
						voice->flags |= UPDATE_PANNING;
					} else if (paramy) {
						voice->panning -= paramy;
						if (voice->panning < 0) voice->panning = 0;
						voice->flags |= UPDATE_PANNING;
					}
					break;
				case FX_MULTI_RETRIG_NOTE:
					com = self->super.super.tick;
					if (!row->volume) com++;
					if (com % voice->retrigy) break;

					if ((!row->volume || row->volume > 80) && voice->retrigx)
						retrig(voice);
					
					voice->flags |= UPDATE_TRIGGER;
					break;
				case FX_TREMOR:
					F2Voice_tremor(voice);
					break;
				default:
					break;
			}

			if (slide) {
				paramx = voice->volSlide >> 4;
				paramy = voice->volSlide & 15;
				slide = 0;

				if (paramx) {
					voice->volume += paramx;
					voice->flags |= UPDATE_VOLUME;
				} else if (paramy) {
					voice->volume -= paramy;
					voice->flags |= UPDATE_VOLUME;
				}
			}
			voice = voice->next;
		}
	}

	if (++(self->super.super.tick) >= (self->super.timer + self->patternDelay)) {
		self->patternDelay = self->super.super.tick = 0;

		if (self->nextPosition < 0) {
			self->nextPosition = self->position + self->super.super.channels;

			if (self->nextPosition >= self->pattern->size || self->complete) {
				self->nextOrder = self->order + 1;
				self->nextPosition = self->patternOffset;

				if (self->nextOrder >= self->super.length) {
					self->nextOrder = self->super.restart;
					CoreMixer_set_complete(&self->super.mixer->super, 1);
				}
			}
		}
	}
}

/* not sure what this does... */
static inline Number linear_transform(int linear, int delta) {
	Number ret;
	if (linear) {
		//chan->speed  = (int)((548077568 * pow(2, ((4608 - delta) / 768))) / 44100) / 65536;
		ret  = (int)((548077568.f * pow(2.f, ((float) (4608 - delta) / 768.f))) / 44100.f) / 65536.f;
	} else {
		//chan->speed  = (int)((65536 * (14317456 / delta)) / 44100) / 65536;
		ret  = (int)((65536.f * (14317456.f / (float) delta)) / 44100.f) / 65536.f;
	}
	return ret;
}


//override
void F2Player_fast(struct F2Player* self) {
	struct SBChannel *chan = NULL;
	int delta = 0; 
	int flags = 0; 
	struct F2Instrument *instr = NULL;
	int panning = 0;
	struct F2Voice *voice = &self->voices[0];
	Number volume = NAN;

	while (voice) {
		chan  = voice->channel;
		flags = voice->flags;
		voice->flags = 0;

		if (flags & UPDATE_TRIGGER) {
			chan->index    = voice->sampleOffset;
			chan->pointer  = -1;
			chan->dir      =  0;
			chan->fraction =  0;
			chan->sample   = &voice->sample->super;
			chan->length   = voice->sample->super.length;

			//FIXME: this test is bogus since sample->data is a static array
			chan->enabled = chan->sample->data ? 1 : 0;
			voice->playing = voice->instrument;
			voice->sampleOffset = 0;
		}

		instr = voice->playing;
		delta = instr->vibratoSpeed ? F2Voice_autoVibrato(voice) : 0;

		volume = voice->volume + voice->volDelta;

		if (instr->volEnabled) {
			if (voice->volEnabled && !voice->volEnvelope.stopped)
				envelope(voice, &voice->volEnvelope, &instr->volData);

			volume = (int)(volume * voice->volEnvelope.value) >> 6;
			flags |= UPDATE_VOLUME;

			if (voice->fadeEnabled) {
				voice->fadeVolume -= voice->fadeDelta;

				if (voice->fadeVolume < 0) {
					volume = 0;

					voice->fadeVolume  = 0;
					voice->fadeEnabled = 0;

					voice->volEnvelope.value   = 0;
					voice->volEnvelope.stopped = 1;
					voice->panEnvelope.stopped = 1;
				} else {
					volume = (int) (volume * voice->fadeVolume) >> 16;
				}
			}
		} else if (voice->keyoff) {
			volume = 0;
			flags |= UPDATE_VOLUME;
		}

		panning = voice->panning;

		if (instr->panEnabled) {
			if (voice->panEnabled && !voice->panEnvelope.stopped)
				envelope(voice, &voice->panEnvelope, &instr->panData);

			panning = (voice->panEnvelope.value << 2);
			flags |= UPDATE_PANNING;

			if (panning < 0) panning = 0;
			else if (panning > 255) panning = 255;
		}

		if (flags & UPDATE_VOLUME) {
			if (volume < 0) volume = 0;
			else if (volume > 64) volume = 64;

			chan->volume = VOLUMES[(int)(volume * self->super.master) >> 6];
			chan->lvol = chan->volume * chan->lpan;
			chan->rvol = chan->volume * chan->rpan;
		}

		if (flags & UPDATE_PANNING) {
			chan->panning = panning;
			chan->lpan = PANNING[256 - panning];
			chan->rpan = PANNING[panning];

			chan->lvol = chan->volume * chan->lpan;
			chan->rvol = chan->volume * chan->rpan;
		}

		if (flags & UPDATE_PERIOD) {
			delta += voice->period + voice->arpDelta + voice->vibDelta;
			
			chan->speed = linear_transform(self->linear, delta);

			chan->delta  = chan->speed;
			chan->speed -= chan->delta;
		}
		voice = voice->next;
	}
}

//override
void F2Player_accurate(struct F2Player* self) {
	struct SBChannel *chan = NULL;
	int delta = 0;
	int flags = 0; 
	struct F2Instrument *instr = NULL;
	Number lpan = NAN;
	Number lvol = NAN; 
	int panning = 0; 
	Number rpan = NAN; 
	Number rvol = NAN; 
	struct F2Voice *voice = &self->voices[0];
	Number volume = NAN; 

	while (voice) {
		chan  = voice->channel;
		flags = voice->flags;
		voice->flags = 0;

		if (flags & UPDATE_TRIGGER) {
			if (chan->sample) {
				flags |= SHORT_RAMP;
				chan->mixCounter = 220;
				chan->oldSample  = null;
				chan->oldPointer = -1;

				if (chan->enabled) {
					chan->oldDir      = chan->dir;
					chan->oldFraction = chan->fraction;
					chan->oldSpeed    = chan->speed;
					chan->oldSample   = chan->sample;
					chan->oldPointer  = chan->pointer;
					chan->oldLength   = chan->length;

					chan->lmixRampD  = chan->lvol;
					chan->lmixDeltaD = chan->lvol / 220;
					chan->rmixRampD  = chan->rvol;
					chan->rmixDeltaD = chan->rvol / 220;
				}
			}

			chan->dir = 1;
			chan->fraction = 0;
			chan->sample  = &voice->sample->super;
			chan->pointer = voice->sampleOffset;
			chan->length  = voice->sample->super.length;

			//FIXME: this test is bogus since sample->data is a static array
			chan->enabled = chan->sample->data ? 1 : 0;
			voice->playing = voice->instrument;
			voice->sampleOffset = 0;
		}

		instr = voice->playing;
		delta = instr->vibratoSpeed ? F2Voice_autoVibrato(voice) : 0;

		volume = voice->volume + voice->volDelta;

		if (instr->volEnabled) {
			if (voice->volEnabled && !voice->volEnvelope.stopped)
				envelope(voice, &voice->volEnvelope, &instr->volData);

			volume = (int)(volume * voice->volEnvelope.value) >> 6;
			flags |= UPDATE_VOLUME;

			if (voice->fadeEnabled) {
				voice->fadeVolume -= voice->fadeDelta;

				if (voice->fadeVolume < 0) {
					volume = 0;

					voice->fadeVolume  = 0;
					voice->fadeEnabled = 0;

					voice->volEnvelope.value   = 0;
					voice->volEnvelope.stopped = 1;
					voice->panEnvelope.stopped = 1;
				} else {
					volume = (int)(volume * voice->fadeVolume) >> 16;
				}
			}
		} else if (voice->keyoff) {
			volume = 0;
			flags |= UPDATE_VOLUME;
		}

		panning = voice->panning;

		if (instr->panEnabled) {
			if (voice->panEnabled && !voice->panEnvelope.stopped)
				envelope(voice, &voice->panEnvelope, &instr->panData);

			panning = (voice->panEnvelope.value << 2);
			flags |= UPDATE_PANNING;

			if (panning < 0) panning = 0;
			else if (panning > 255) panning = 255;
		}

		if (!chan->enabled) {
			chan->volCounter = 0;
			chan->panCounter = 0;
			voice = voice->next;
			continue;
		}

		if (flags & UPDATE_VOLUME) {
			if (volume < 0) volume = 0;
			else if (volume > 64) volume = 64;
			
			unsigned vidx = (int)(volume * self->super.master) >> 6;
			assert_op(vidx, <, ARRAY_SIZE(VOLUMES));
			volume = VOLUMES[vidx];
			lvol = volume * PANNING[256 - panning];
			rvol = volume * PANNING[panning];

			if (volume != chan->volume && !chan->mixCounter) {
				chan->volCounter = (flags & SHORT_RAMP) ? 220 : self->super.mixer->super.samplesTick;

				chan->lvolDelta = (lvol - chan->lvol) / chan->volCounter;
				chan->rvolDelta = (rvol - chan->rvol) / chan->volCounter;
			} else {
				chan->lvol = lvol;
				chan->rvol = rvol;
			}
			chan->volume = volume;
		}

		if (flags & UPDATE_PANNING) {
			lpan = PANNING[256 - panning];
			rpan = PANNING[panning];

			if (panning != chan->panning && !chan->mixCounter && !chan->volCounter) {
				chan->panCounter = self->super.mixer->super.samplesTick;

				chan->lpanDelta = (lpan - chan->lpan) / chan->panCounter;
				chan->rpanDelta = (rpan - chan->rpan) / chan->panCounter;
			} else {
				chan->lpan = lpan;
				chan->rpan = rpan;
			}
			chan->panning = panning;
		}

		if (flags & UPDATE_PERIOD) {
			delta += voice->period + voice->arpDelta + voice->vibDelta;
			chan->speed = linear_transform(self->linear, delta);
		}

		if (chan->mixCounter) {
			chan->lmixRampU  = 0.0;
			chan->lmixDeltaU = chan->lvol / 220;
			chan->rmixRampU  = 0.0;
			chan->rmixDeltaU = chan->rvol / 220;
		}
		voice = voice->next;
	}
}

//override
void F2Player_initialize(struct F2Player* self) {
	SBPlayer_initialize(&self->super);
	//self->super->initialize();

	self->super.timer   = self->super.super.speed;
	self->order         =  0;
	self->position      =  0;
	self->nextOrder     = -1;
	self->nextPosition  = -1;
	self->patternDelay  =  0;
	self->patternOffset =  0;
	self->complete      =  0;
	self->super.master  = 64;

	assert_op(self->super.super.channels, <=, F2PLAYER_MAX_VOICES);
	
	//self->voices = new Vector.<F2Voice>(self->super.super.channels, true);
	
	unsigned int i;

	for (i = 0; i < self->super.super.channels; ++i) {
		struct F2Voice *voice = &self->voices[i];
		F2Voice_ctor(voice, i);
		//voice = new F2Voice(i);

		voice->channel = &self->super.mixer->channels[i];
		voice->playing = &self->instruments[0];
		voice->sample  = &voice->playing->samples[0];

		//self->voices[i] = voice;
		if (i) self->voices[i - 1].next = voice;
	}
}

//override
void F2Player_loader(struct F2Player* self, struct ByteArray *stream) {
	int header = 0;
	int i = 0;
	char id[24];
	int iheader = 0;
	struct F2Instrument *instr = NULL;
	int ipos = 0;
	int j = 0; 
	int len = 0; 
	struct F2Pattern *pattern = NULL;
	int pos = 0; 
	int reserved = 22;
	struct F2Row *row = NULL;
	int rows = 0; 
	struct F2Sample *sample = NULL;
	int value = 0;
	
	if (ByteArray_get_length(stream) <= self->super.super.min_filesize) return;
	ByteArray_set_position(stream, 0);
	stream->readMultiByte(stream, id, 17);
	if (!is_str(id, "Extended Module: ")) return;
	assert_op(stream->pos, ==, 17);

	//self->super.super.title = stream->readMultiByte(stream, 20, ENCODING);
	stream->readMultiByte(stream, self->title_buffer, 20);
	self->super.super.title = self->title_buffer;
	
	ByteArray_set_position_rel(stream, +1);
	//id = stream->readMultiByte(stream, 20, ENCODING);
	stream->readMultiByte(stream, id, 20);
	
	//FIXME its probably better to just test for "Extended Module" at the stream start
	if (is_str(id, "FastTracker v2.00   ") || is_str(id, "FastTracker v 2.00  ")) {
		self->super.super.version = 1;
	} else if (is_str(id, "Sk@le Tracker")) {
		reserved = 2;
		self->super.super.version = 2;
	} else if (is_str(id, "MadTracker 2.0")) {
		self->super.super.version = 3;
	} else if (is_str(id, "MilkyTracker        ")) {
		self->super.super.version = 4;
	} else if (is_str(id, "DigiBooster Pro 2.18")) {
		self->super.super.version = 5;
	// FIXME maybe this tests if the string is *contained*, not starting with
	//} else if (id->indexOf("OpenMPT") != -1) {
	} else if (is_str(id, "OpenMPT")) {	
		self->super.super.version = 6;
	} else if (is_str(id, "MOD2XM 1.0")) {
		self->super.super.version = 7;
	} else if (is_str(id, "XMLiTE")) {
		self->super.super.version = 9;
	} else {
		self->super.super.version = 8;
		id[20] = 0;
		printf("warning: unknown tracker %s\n", id);
	}

	stream->readUnsignedShort(stream);

	header   = stream->readUnsignedInt(stream);
	self->super.length   = stream->readUnsignedShort(stream);
	self->super.restart  = stream->readUnsignedShort(stream);
	self->super.super.channels = stream->readUnsignedShort(stream);

	value = rows = stream->readUnsignedShort(stream);
	
	//self->instruments = new Vector.<F2Instrument>(stream->readUnsignedShort() + 1, true);
	self->vector_count_instruments = stream->readUnsignedShort(stream) + 1;
	assert_op(self->vector_count_instruments, <=, F2PLAYER_MAX_INSTRUMENTS);

	self->linear = stream->readUnsignedShort(stream);
	self->super.super.speed  = stream->readUnsignedShort(stream);
	self->super.super.tempo  = stream->readUnsignedShort(stream);

	//self->track = new Vector.<int>(length, true);
	assert_op(self->super.length, <=, SBPLAYER_MAX_TRACKS);

	for (i = 0; i < self->super.length; ++i) {
		j = stream->readUnsignedByte(stream);
		if (j >= value) rows = j + 1;
		self->super.track[i] = j;
	}
	
	assert_op(rows, <=, F2PLAYER_MAX_PATTERNS);
	//self->patterns = new Vector.<F2Pattern>(rows, true);

	if (rows != value) {
		pattern = &self->patterns[rows - 1];
		F2Pattern_ctor(pattern, 64, self->super.super.channels);
		//pattern = new F2Pattern(64, self->super.super.channels);
		j = pattern->size;
		assert_op(j, <, F2PATTERN_MAX_ROWS);
		for (i = 0; i < j; ++i) {
			//pattern->rows[i] = new F2Row();
			F2Row_ctor(&pattern->rows[i]);
		}
		//self->patterns[--rows] = pattern;
		rows--;
	}

	pos = header + 60;
	ByteArray_set_position(stream, pos);
	len = value;

	assert_op(len, <=, F2PLAYER_MAX_PATTERNS);
	for (i = 0; i < len; ++i) {
		header = stream->readUnsignedInt(stream);
		ByteArray_set_position_rel(stream, +1);

		//pattern = new F2Pattern(stream->readUnsignedShort(stream), self->super.super.channels);
		pattern = &self->patterns[i];
		F2Pattern_ctor(pattern, stream->readUnsignedShort(stream), self->super.super.channels);
		
		rows = pattern->size;

		value = stream->readUnsignedShort(stream);
		ByteArray_set_position(stream, pos + header);
		ipos = ByteArray_get_position(stream) + value;

		if (value) {
			assert_op(rows, <=, F2PATTERN_MAX_ROWS);
			for (j = 0; j < rows; ++j) {
				//row = new F2Row();
				row = &pattern->rows[j];
				F2Row_ctor(row);
				
				value = stream->readUnsignedByte(stream);

				if (value & 128) {
					if (value &  1) row->note       = stream->readUnsignedByte(stream);
					if (value &  2) row->instrument = stream->readUnsignedByte(stream);
					if (value &  4) row->volume     = stream->readUnsignedByte(stream);
					if (value &  8) row->effect     = stream->readUnsignedByte(stream);
					if (value & 16) row->param      = stream->readUnsignedByte(stream);
				} else {
					row->note       = value;
					row->instrument = stream->readUnsignedByte(stream);
					row->volume     = stream->readUnsignedByte(stream);
					row->effect     = stream->readUnsignedByte(stream);
					row->param      = stream->readUnsignedByte(stream);
				}

				if (row->note != KEYOFF_NOTE) 
					if (row->note > 96) row->note = 0;
					
				//pattern->rows[j] = row;
			}
		} else {
			assert_op(rows, <=, F2PATTERN_MAX_ROWS);
			for (j = 0; j < rows; ++j) {
				//pattern->rows[j] = new F2Row();
				F2Row_ctor(&pattern->rows[j]);
			}
		}

		//self->patterns[i] = pattern;
		pos = ByteArray_get_position(stream);
		if (pos != ipos) {
			pos = ipos;
			ByteArray_set_position(stream, pos);
		}
	}

	ipos = ByteArray_get_position(stream);
	//len = self->instruments->length;
	len = self->vector_count_instruments;

	for (i = 1; i < len; ++i) {
		iheader = stream->readUnsignedInt(stream);
		if ((ByteArray_get_position(stream) + iheader) >= ByteArray_get_length(stream)) break;

		
		//FIXME: the original code did not assign the instr to the self->instruments array
		instr = &self->instruments[i];
		F2Instrument_ctor(instr);
		//instr = new F2Instrument();
		
		// FIXME : forgotten to call F2Data_ctor on instr voldata and pandata ?
		// the code below the loop which also called F2Instrument_new did it...
		
		//instr->name = stream->readMultiByte(stream,22, ENCODING);
		stream->readMultiByte(stream, self->instrument_names[i], 22);
		instr->name = self->instrument_names[i];
		
		ByteArray_set_position_rel(stream, +1);

		value = stream->readUnsignedShort(stream);
		if (value > 16) value = 16;
		header = stream->readUnsignedInt(stream);
		if (reserved == 2 && header != 64) header = 64;

		if (value) {
			assert_op(value, <=, F2INSTRUMENT_MAX_SAMPLES);
			//instr->samples = new Vector.<F2Sample>(value, true);

			for (j = 0; j < 96; ++j)
				instr->noteSamples[j] = stream->readUnsignedByte(stream);
			for (j = 0; j < 12; ++j) {
				//instr->volData.points[j] = new F2Point(stream->readUnsignedShort(), stream->readUnsignedShort());
				unsigned short x = stream->readUnsignedShort(stream);
				unsigned short y = stream->readUnsignedShort(stream);
				F2Point_ctor(&instr->volData.points[j], x, y);
			}
			for (j = 0; j < 12; ++j) {
				unsigned short x = stream->readUnsignedShort(stream);
				unsigned short y = stream->readUnsignedShort(stream);
				F2Point_ctor(&instr->panData.points[j], x, y);
				//instr->panData.points[j] = new F2Point(stream->readUnsignedShort(), stream->readUnsignedShort());
			}

			instr->volData.total     = stream->readUnsignedByte(stream);
			instr->panData.total     = stream->readUnsignedByte(stream);
			instr->volData.sustain   = stream->readUnsignedByte(stream);
			instr->volData.loopStart = stream->readUnsignedByte(stream);
			instr->volData.loopEnd   = stream->readUnsignedByte(stream);
			instr->panData.sustain   = stream->readUnsignedByte(stream);
			instr->panData.loopStart = stream->readUnsignedByte(stream);
			instr->panData.loopEnd   = stream->readUnsignedByte(stream);
			instr->volData.flags     = stream->readUnsignedByte(stream);
			instr->panData.flags     = stream->readUnsignedByte(stream);

			if (instr->volData.flags & ENVELOPE_ON) instr->volEnabled = 1;
			if (instr->panData.flags & ENVELOPE_ON) instr->panEnabled = 1;

			instr->vibratoType  = stream->readUnsignedByte(stream);
			instr->vibratoSweep = stream->readUnsignedByte(stream);
			instr->vibratoDepth = stream->readUnsignedByte(stream);
			instr->vibratoSpeed = stream->readUnsignedByte(stream);
			instr->fadeout      = stream->readUnsignedShort(stream) << 1;

			ByteArray_set_position_rel(stream, reserved);
			pos = ByteArray_get_position(stream);
			//self->instruments[i] = instr;
			
			assert_op(value, <=, F2INSTRUMENT_MAX_SAMPLES);

			for (j = 0; j < value; ++j) {
				//sample = new F2Sample();
				sample = &instr->samples[j];
				F2Sample_ctor(sample);
				
				sample->super.length    = stream->readUnsignedInt(stream);
				sample->super.loopStart = stream->readUnsignedInt(stream);
				sample->super.loopLen   = stream->readUnsignedInt(stream);
				sample->super.volume    = stream->readUnsignedByte(stream);
				sample->finetune  = stream->readByte(stream);
				sample->super.loopMode  = stream->readUnsignedByte(stream);
				sample->panning   = stream->readUnsignedByte(stream);
				sample->relative  = stream->readByte(stream);

				ByteArray_set_position_rel(stream, +1);
				
				stream->readMultiByte(stream, sample->name_buffer, 22);
				sample->super.name = sample->name_buffer;
				
				//sample->super.name = stream->readMultiByte(stream, 22, ENCODING);
				
				//instr->samples[j] = sample;
				
				pos += header;
				ByteArray_set_position(stream, pos);
				//stream->position = (pos += header);
			}

			for (j = 0; j < value; ++j) {
				sample = &instr->samples[j];
				if (!sample->super.length) continue;
				pos = ByteArray_get_position(stream) + sample->super.length;

				if (sample->super.loopMode & 16) {
					sample->super.bits       = 16;
					sample->super.loopMode  ^= 16;
					sample->super.length    >>= 1;
					sample->super.loopStart >>= 1;
					sample->super.loopLen   >>= 1;
				}

				if (!sample->super.loopLen) sample->super.loopMode = 0;
				SBSample_store(&sample->super, stream);
				//sample->store(stream);
				if (sample->super.loopMode) sample->super.length = sample->super.loopStart + sample->super.loopLen;
				ByteArray_set_position(stream, pos);
			}
		} else {
			ByteArray_set_position(stream, ipos + iheader);
		}

		ipos = ByteArray_get_position(stream);
		//FIXME: our stream position can never exceed its length
		if (ipos >= ByteArray_get_length(stream)) break;
	}

	//instr = new F2Instrument();
	instr = &self->instruments[0];
	F2Instrument_ctor(instr);
	
	//instr->volData = new F2Data();
	//instr->panData = new F2Data();
	F2Data_ctor(&instr->volData);
	F2Data_ctor(&instr->panData);
	
	//instr->samples = new Vector.<F2Sample>(1, true);

	for (i = 0; i < F2DATA_MAX_POINTS; ++i) {
		//instr->volData.points[i] = new F2Point();
		//instr->panData.points[i] = new F2Point();
		F2Point_ctor(&instr->volData.points[i], 0, 0);
		F2Point_ctor(&instr->panData.points[i], 0, 0);
	}

	//sample = new F2Sample();
	sample = &instr->samples[0];
	F2Sample_ctor(sample);
	
	sample->super.length = 220;
	//sample->data = new Vector.<Number>(220, true);

	for (i = 0; i < 220; ++i) sample->super.data[i] = 0.0;

	//instr->samples[0] = sample;
	//self->instruments[0] = instr;
}

static void envelope(struct F2Voice *voice, struct F2Envelope *envelope, struct F2Data *data) {
	int pos = envelope->position;
	struct F2Point *curr = NULL;
	struct F2Point *next = NULL;
	
	assert_op(pos, <, F2DATA_MAX_POINTS);
	curr = &data->points[pos];

	if (envelope->frame == curr->frame) {
		if ((data->flags & ENVELOPE_LOOP) && pos == data->loopEnd) {
			pos  = envelope->position = data->loopStart;
			assert_op(pos, <, F2DATA_MAX_POINTS);
			
			curr = &data->points[pos];
			envelope->frame = curr->frame;
		}

		if (pos == (data->total - 1)) {
			envelope->value = curr->value;
			envelope->stopped = 1;
			return;
		}

		if ((data->flags & ENVELOPE_SUSTAIN) && pos == data->sustain && !voice->fadeEnabled) {
			envelope->value = curr->value;
			return;
		}

		envelope->position++;
		assert_op(envelope->position, <, F2DATA_MAX_POINTS);
		next = &data->points[envelope->position];

		int divisor = next->frame - curr->frame;
		//FIXME: examine how as3 handles a div by zero
		envelope->delta = ((next->value - curr->value) << 8) / (divisor ? divisor : 1);
		envelope->fraction = (curr->value << 8);
	} else {
		envelope->fraction += envelope->delta;
	}

	envelope->value = (envelope->fraction >> 8);
	envelope->frame++;
}

static int amiga(int note, int finetune) {
	Number delta = 0.0;
	int period = PERIODS[++note];

	if (finetune < 0) {
		delta = (float) (PERIODS[--note] - period) / 64.f;
	} else if (finetune > 0) {
		delta = (float) (period - PERIODS[++note]) / 64.f;
	}

	return period - (delta * finetune);
}
    
static void retrig(struct F2Voice *voice) {
	switch (voice->retrigx) {
		case 1:
			voice->volume--;
			break;
		case 2:
			voice->volume++;
			break;
		case 3:
			voice->volume -= 4;
			break;
		case 4:
			voice->volume -= 8;
			break;
		case 5:
			voice->volume -= 16;
			break;
		case 6:
			voice->volume = (voice->volume << 1) / 3;
			break;
		case 7:
			voice->volume >>= 1;
			break;
		case 8:
			voice->volume = voice->sample->super.volume;
			break;
		case 9:
			voice->volume++;
			break;
		case 10:
			voice->volume += 2;
			break;
		case 11:
			voice->volume += 4;
			break;
		case 12:
			voice->volume += 8;
			break;
		case 13:
			voice->volume += 16;
			break;
		case 14:
			voice->volume = (voice->volume * 3) >> 1;
			break;
		case 15:
			voice->volume <<= 1;
			break;
		default:
			break;
	}

	if (voice->volume < 0) voice->volume = 0;
	else if (voice->volume > 64) voice->volume = 64;

	voice->flags |= UPDATE_VOLUME;
}

