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

#include "S1Voice.h"
#include "../flod_internal.h"

void S1Voice_defaults(struct S1Voice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void S1Voice_ctor(struct S1Voice* self, int index) {
	CLASS_CTOR_DEF(S1Voice);
	// original constructor code goes here
	self->index = index;
}

struct S1Voice* S1Voice_new(int index) {
	CLASS_NEW_BODY(S1Voice, index);
}

void S1Voice_initialize(struct S1Voice* self) {
	self->step         =  0;
	self->row          =  0;
	self->sample       =  0;
	self->samplePtr    = -1;
	self->sampleLen    =  0;
	self->note         =  0;
	self->noteTimer    =  0;
	self->period       =  0x9999;
	self->volume       =  0;
	self->bendTo       =  0;
	self->bendSpeed    =  0;
	self->arpeggioCtr  =  0;
	self->envelopeCtr  =  0;
	self->pitchCtr     =  0;
	self->pitchFallCtr =  0;
	self->sustainCtr   =  0;
	self->phaseTimer   =  0;
	self->phaseSpeed   =  0;
	self->wavePos      =  0;
	self->waveList     =  0;
	self->waveTimer    =  0;
	self->waitCtr      =  0;
}