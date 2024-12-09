/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.1 - 2012/04/09

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "../flod_internal.h"
#include "AmigaChannel.h"

void AmigaChannel_defaults(struct AmigaChannel* self) {
	CLASS_DEF_INIT();
	// static initializers go here
	self->panning  = 1.0;
	
	self->timer = self->level = self->ldata = self->rdata = NAN;
}

void AmigaChannel_ctor(struct AmigaChannel* self, int index) {
	CLASS_CTOR_DEF(AmigaChannel);
	// original constructor code goes here
	PFUNC();
	if (((++index) & 2) == 0) self->panning = -(self->panning);
	self->level = self->panning;	
}

struct AmigaChannel* AmigaChannel_new(int index) {
	CLASS_NEW_BODY(AmigaChannel, index);
}

int AmigaChannel_get_enabled(struct AmigaChannel* self) {
	return self->audena;
}

void AmigaChannel_set_enabled(struct AmigaChannel* self, int value) {
	if (value == self->audena) return;

	self->audena = value;
	self->audloc = self->pointer;
	self->audcnt = self->pointer + self->length;

	self->timer = 1.0;
	if (value) self->delay += 2;
}

void AmigaChannel_set_period(struct AmigaChannel* self, int value) {
	if (value < 0) value = 0;
	else if(value > 65535) value = 65535;

	self->audper = value;
}

void AmigaChannel_set_volume(struct AmigaChannel* self, int value) {
	if (value < 0) value = 0;
	else if (value > 64) value = 64;

	self->audvol = value;
}

void AmigaChannel_resetData(struct AmigaChannel* self) {
	PFUNC();
	self->ldata = 0.0;
	self->rdata = 0.0;
}

void AmigaChannel_initialize(struct AmigaChannel* self) {
	PFUNC();
	self->audena = 0;
	self->audcnt = 0;
	self->audloc = 0;
	self->audper = 50;
	self->audvol = 0;

	self->timer = 0.0;
	self->ldata = 0.0;
	self->rdata = 0.0;

	self->delay   = 0;
	self->pointer = 0;
	self->length  = 0;
}
