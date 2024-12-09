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

#include "FCVoice.h"
#include "../flod_internal.h"

void FCVoice_defaults(struct FCVoice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void FCVoice_ctor(struct FCVoice* self, int index) {
	CLASS_CTOR_DEF(FCVoice);
	// original constructor code goes here
	self->index = index;
}

struct FCVoice* FCVoice_new(int index) {
	CLASS_NEW_BODY(FCVoice, index);
}

void FCVoice_initialize(struct FCVoice* self) {
	self->sample         = null;
	self->enabled        = 0;
	self->pattern        = 0;
	self->soundTranspose = 0;
	self->transpose      = 0;
	self->patStep        = 0;
	self->frqStep        = 0;
	self->frqPos         = 0;
	self->frqSustain     = 0;
	self->frqTranspose   = 0;
	self->volStep        = 0;
	self->volPos         = 0;
	self->volCtr         = 1;
	self->volSpeed       = 1;
	self->volSustain     = 0;
	self->note           = 0;
	self->pitch          = 0;
	self->volume         = 0;
	self->pitchBendFlag  = 0;
	self->pitchBendSpeed = 0;
	self->pitchBendTime  = 0;
	self->portamentoFlag = 0;
	self->portamento     = 0;
	self->volBendFlag    = 0;
	self->volBendSpeed   = 0;
	self->volBendTime    = 0;
	self->vibratoFlag    = 0;
	self->vibratoSpeed   = 0;
	self->vibratoDepth   = 0;
	self->vibratoDelay   = 0;
	self->vibrato        = 0;
}

void FCVoice_volumeBend(struct FCVoice* self) {
	self->volBendFlag ^= 1;

	if (self->volBendFlag) {
		self->volBendTime--;
		self->volume += self->volBendSpeed;
		if (self->volume < 0 || self->volume > 64) self->volBendTime = 0;
	}
}
