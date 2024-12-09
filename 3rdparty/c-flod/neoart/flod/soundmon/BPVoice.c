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

#include "BPVoice.h"
#include "../flod_internal.h"

void BPVoice_defaults(struct BPVoice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void BPVoice_ctor(struct BPVoice* self, int index) {
	CLASS_CTOR_DEF(BPVoice);
	// original constructor code goes here
	self->index = index;
}

struct BPVoice* BPVoice_new(int index) {
	CLASS_NEW_BODY(BPVoice, index);
}


void BPVoice_initialize(struct BPVoice* self) {
	self->channel      =  null,
	self->enabled      =  0;
	self->restart      =  0;
	self->note         =  0;
	self->period       =  0;
	self->sample       =  0;
	self->samplePtr    =  0;
	self->sampleLen    =  2;
	self->synth        =  0;
	self->synthPtr     = -1;
	self->arpeggio     =  0;
	self->autoArpeggio =  0;
	self->autoSlide    =  0;
	self->vibrato      =  0;
	self->volume       =  0;
	self->volumeDef    =  0;
	self->adsrControl  =  0;
	self->adsrPtr      =  0;
	self->adsrCtr      =  0;
	self->lfoControl   =  0;
	self->lfoPtr       =  0;
	self->lfoCtr       =  0;
	self->egControl    =  0;
	self->egPtr        =  0;
	self->egCtr        =  0;
	self->egValue      =  0;
	self->fxControl    =  0;
	self->fxCtr        =  0;
	self->modControl   =  0;
	self->modPtr       =  0;
	self->modCtr       =  0;
}
