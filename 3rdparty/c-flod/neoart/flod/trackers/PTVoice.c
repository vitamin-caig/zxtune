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

#include "PTVoice.h"
#include "../flod_internal.h"

void PTVoice_defaults(struct PTVoice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void PTVoice_ctor(struct PTVoice* self, int index) {
	CLASS_CTOR_DEF(PTVoice);
	// original constructor code goes here
	self->index = index;
}

struct PTVoice* PTVoice_new(int index) {
	CLASS_NEW_BODY(PTVoice, index);
}

void PTVoice_initialize(struct PTVoice* self) {
	self->channel      = null;
	self->sample       = null;
	self->enabled      = 0;
	self->loopCtr      = 0;
	self->loopPos      = 0;
	self->step         = 0;
	self->period       = 0;
	self->effect       = 0;
	self->param        = 0;
	self->volume       = 0;
	self->pointer      = 0;
	self->length       = 0;
	self->loopPtr      = 0;
	self->repeat       = 0;
	self->finetune     = 0;
	self->offset       = 0;
	self->portaDir     = 0;
	self->portaPeriod  = 0;
	self->portaSpeed   = 0;
	self->glissando    = 0;
	self->tremoloParam = 0;
	self->tremoloPos   = 0;
	self->tremoloWave  = 0;
	self->vibratoParam = 0;
	self->vibratoPos   = 0;
	self->vibratoWave  = 0;
	self->funkPos      = 0;
	self->funkSpeed    = 0;
	self->funkWave     = 0;
}
