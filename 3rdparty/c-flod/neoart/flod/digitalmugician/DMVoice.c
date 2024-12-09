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

#include "DMVoice.h"
#include "../flod_internal.h"

void DMVoice_defaults(struct DMVoice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void DMVoice_ctor(struct DMVoice* self) {
	CLASS_CTOR_DEF(DMVoice);
	// original constructor code goes here
}

struct DMVoice* DMVoice_new(void) {
	CLASS_NEW_BODY(DMVoice);
}

void DMVoice_initialize(struct DMVoice *self) {
	self->sample       = NULL;
	self->step         = NULL;
	self->note         = 0;
	self->period       = 0;
	self->val1         = 0;
	self->val2         = 0;
	self->finalPeriod  = 0;
	self->arpeggioStep = 0;
	self->effectCtr    = 0;
	self->pitch        = 0;
	self->pitchCtr     = 0;
	self->pitchStep    = 0;
	self->portamento   = 0;
	self->volume       = 0;
	self->volumeCtr    = 0;
	self->volumeStep   = 0;
	self->mixMute      = 1;
	self->mixPtr       = 0;
	self->mixEnd       = 0;
	self->mixSpeed     = 0;
	self->mixStep      = 0;
	self->mixVolume    = 0;
}
