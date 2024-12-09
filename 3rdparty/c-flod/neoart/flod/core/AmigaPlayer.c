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

#include "AmigaPlayer.h"
#include "../flod_internal.h"

void AmigaPlayer_defaults(struct AmigaPlayer* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void AmigaPlayer_ctor(struct AmigaPlayer* self, struct Amiga* amiga) {
	CLASS_CTOR_DEF(AmigaPlayer);
	PFUNC();
	// original constructor code goes here
	self->amiga = amiga ? amiga : Amiga_new();
	//super(self->amiga);
	CorePlayer_ctor(&self->super, (struct CoreMixer*) self->amiga);

	self->super.channels = 4;
	self->super.endian   = BAE_BIG;
	AmigaPlayer_set_ntsc(self, 0);
	self->super.speed    = 6;
	self->super.tempo    = 125;

	//add vtable
	self->super.set_ntsc = (void*) AmigaPlayer_set_ntsc;
	self->super.set_stereo = (void*) AmigaPlayer_set_stereo;
	self->super.set_volume = (void*) AmigaPlayer_set_volume;
	self->super.toggle = (void*) AmigaPlayer_toggle;
}

struct AmigaPlayer* AmigaPlayer_new(struct Amiga* amiga) {
	CLASS_NEW_BODY(AmigaPlayer, amiga);
}

//override
void AmigaPlayer_set_ntsc(struct AmigaPlayer* self, int value) {
	PFUNC();
	self->standard = value;

	if (value) {
		self->amiga->clock = 81.1688208;
		self->amiga->super.samplesTick = 735;
	} else {
		self->amiga->clock = 80.4284580;
		self->amiga->super.samplesTick = 882;
	}
}

//override
void AmigaPlayer_set_stereo(struct AmigaPlayer* self, Number value) {
	PFUNC();
	struct AmigaChannel *chan = &self->amiga->channels[0];

	if (value < 0.0) value = 0.0;
	else if (value > 1.0) value = 1.0;

	while (chan) {
		chan->level = value * chan->panning;
		chan = chan->next;
	}
}

//override
void AmigaPlayer_set_volume(struct AmigaPlayer* self, Number value) {
	PFUNC();
	if (value < 0.0) value = 0.0;
	else if (value > 1.0) value = 1.0;

	self->amiga->master = value * 0.00390625;
}

//override
void AmigaPlayer_toggle(struct AmigaPlayer* self, int index) {
	PFUNC();
	self->amiga->channels[index].mute ^= 1;
}

  
