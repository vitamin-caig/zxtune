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

#include "S2Voice.h"
#include "../flod_internal.h"

void S2Voice_defaults(struct S2Voice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void S2Voice_ctor(struct S2Voice* self, int index) {
	CLASS_CTOR_DEF(S2Voice);
	// original constructor code goes here
	self->index = index;
}

struct S2Voice* S2Voice_new(int index) {
	CLASS_NEW_BODY(S2Voice, index);
}


void S2Voice_initialize(struct S2Voice* self) {
	self->step           = null;
	self->row            = null;
	self->instr          = null;
	self->sample         = null;
	self->enabled        = 0;
	self->pattern        = 0;
	self->instrument     = 0;
	self->note           = 0;
	self->period         = 0;
	self->volume         = 0;
	self->original       = 0;
	self->adsrPos        = 0;
	self->sustainCtr     = 0;
	self->pitchBend      = 0;
	self->pitchBendCtr   = 0;
	self->noteSlideTo    = 0;
	self->noteSlideSpeed = 0;
	self->waveCtr        = 0;
	self->wavePos        = 0;
	self->arpeggioCtr    = 0;
	self->arpeggioPos    = 0;
	self->vibratoCtr     = 0;
	self->vibratoPos     = 0;
	self->speed          = 0;
}
