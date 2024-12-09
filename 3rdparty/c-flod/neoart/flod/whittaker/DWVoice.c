/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.0 - 2012/02/22

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "DWVoice.h"
#include "../flod_internal.h"

void DWVoice_defaults(struct DWVoice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void DWVoice_ctor(struct DWVoice* self, int index, int bitFlag) {
	CLASS_CTOR_DEF(DWVoice);
	// original constructor code goes here
	PFUNC();
	self->index = index;
	self->bitFlag = bitFlag;	
}

struct DWVoice* DWVoice_new(int index, int bitFlag) {
	CLASS_NEW_BODY(DWVoice, index, bitFlag);
}

void DWVoice_initialize(struct DWVoice* self) {
	PFUNC();
	self->channel       = null;
	self->sample        = null;
	self->trackPtr      = 0;
	self->trackPos      = 0;
	self->patternPos    = 0;
	self->frqseqPtr     = 0;
	self->frqseqPos     = 0;
	self->volseqPtr     = 0;
	self->volseqPos     = 0;
	self->volseqSpeed   = 0;
	self->volseqCounter = 0;
	self->halve         = 0;
	self->speed         = 0;
	self->tick          = 1;
	self->busy          = -1;
	self->flags         = 0;
	self->note          = 0;
	self->period        = 0;
	self->transpose     = 0;
	self->portaDelay    = 0;
	self->portaDelta    = 0;
	self->portaSpeed    = 0;
	self->vibrato       = 0;
	self->vibratoDelta  = 0;
	self->vibratoSpeed  = 0;
	self->vibratoDepth  = 0;
}

