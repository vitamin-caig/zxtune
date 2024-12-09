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

#include "SBChannel.h"
#include "../flod_internal.h"

void SBChannel_defaults(struct SBChannel* self) {
	CLASS_DEF_INIT();
	// static initializers go here
	self->fraction = self->speed = self->oldFraction = self->oldSpeed = self->volume =
	self->lvol = self->rvol = self->lpan = self->rpan = self->ldata = self->rdata = 
	self->lmixRampU = self->lmixDeltaU = self->rmixRampU = self->rmixDeltaU = 
	self->lmixRampD = self->lmixDeltaD = self->rmixRampD = self->rmixDeltaD = 
	self->lvolDelta = self->rvolDelta = self->lpanDelta = self->rpanDelta = NAN;
}

void SBChannel_ctor(struct SBChannel* self) {
	CLASS_CTOR_DEF(SBChannel);
	// original constructor code goes here
}

struct SBChannel* SBChannel_new(void) {
	CLASS_NEW_BODY(SBChannel);
}

void SBChannel_initialize(struct SBChannel* self) {
	self->enabled     = 0;
	self->sample      = null;
	self->length      = 0;
	self->index       = 0;
	self->pointer     = 0;
	self->delta       = 0;
	self->fraction    = 0.0f;
	self->speed       = 0.0f;
	self->dir         = 0;
	self->oldSample   = null;
	self->oldLength   = 0;
	self->oldPointer  = 0;
	self->oldFraction = 0.0f;
	self->oldSpeed    = 0.0f;
	self->oldDir      = 0;
	self->volume      = 0.0f;
	self->lvol        = 0.0f;
	self->rvol        = 0.0f;
	self->panning     = 128;
	self->lpan        = 0.5f;
	self->rpan        = 0.5f;
	self->ldata       = 0.0f;
	self->rdata       = 0.0f;
	self->mixCounter  = 0;
	self->lmixRampU   = 0.0f;
	self->lmixDeltaU  = 0.0f;
	self->rmixRampU   = 0.0f;
	self->rmixDeltaU  = 0.0f;
	self->lmixRampD   = 0.0f;
	self->lmixDeltaD  = 0.0f;
	self->rmixRampD   = 0.0f;
	self->rmixDeltaD  = 0.0f;
	self->volCounter  = 0;
	self->lvolDelta   = 0.0f;
	self->rvolDelta   = 0.0f;
	self->panCounter  = 0;
	self->lpanDelta   = 0.0f;
	self->rpanDelta   = 0.0f;
}