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

#include "FXVoice.h"
#include "../flod_internal.h"

void FXVoice_defaults(struct FXVoice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void FXVoice_ctor(struct FXVoice* self, int index) {
	CLASS_CTOR_DEF(FXVoice);
	// original constructor code goes here
	self->index = index;
}

struct FXVoice* FXVoice_new(int index) {
	CLASS_NEW_BODY(FXVoice, index);
}


void FXVoice_initialize(struct FXVoice* self) {
	self->channel     = null;
	self->sample      = null;
	self->enabled     = 0;
	self->period      = 0;
	self->effect      = 0;
	self->param       = 0;
	self->volume      = 0;
	self->last        = 0;
	self->slideCtr    = 0;
	self->slideDir    = 0;
	self->slideParam  = 0;
	self->slidePeriod = 0;
	self->slideSpeed  = 0;
	self->stepPeriod  = 0;
	self->stepSpeed   = 0;
	self->stepWanted  = 0;
}
