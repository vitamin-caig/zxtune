/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.1 - 2012/04/21

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "SBPlayer.h"
#include "../flod_internal.h"
//extends CorePlayer

void SBPlayer_defaults(struct SBPlayer* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

//mixer default value: NULL
void SBPlayer_ctor(struct SBPlayer* self, struct Soundblaster* mixer) {
	CLASS_CTOR_DEF(SBPlayer);
	// original constructor code goes here
	PFUNC();
	self->mixer = mixer ? mixer : Soundblaster_new();
	//super(self->mixer);
	CorePlayer_ctor(&self->super, (struct CoreMixer*) self->mixer);

	self->super.endian  = BAE_LITTLE;
	self->super.quality = 1;

	//add vtable
	self->super.setup = SBPlayer_setup;
	self->super.set_volume = SBPlayer_set_volume;
	self->super.toggle = SBPlayer_toggle;
	self->super.initialize = SBPlayer_initialize;
}

struct SBPlayer* SBPlayer_new(struct Soundblaster* mixer) {
	CLASS_NEW_BODY(SBPlayer, mixer);
}


//override
void SBPlayer_set_volume(struct SBPlayer* self, Number value) {
	PFUNC();
	if (value < 0.0) value = 0.0;
		else if (value > 1.0) value = 1.0;

	self->master = value * 64;
}

//override
void SBPlayer_toggle(struct SBPlayer* self, int index) {
	PFUNC();
	self->mixer->channels[index].mute ^= 1;
}

//override
void SBPlayer_setup(struct SBPlayer* self) {
	PFUNC();
	Soundblaster_setup(self->mixer, self->super.channels);
	//self->mixer->setup(self->super.channels);
}

//override
void SBPlayer_initialize(struct SBPlayer* self) {
	PFUNC();
	//super->initialize();
	CorePlayer_initialize(&self->super);
	self->timer  = self->super.speed;
	self->master = 64;
}
