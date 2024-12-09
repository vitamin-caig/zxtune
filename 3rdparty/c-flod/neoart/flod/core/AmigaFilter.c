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
#include "../flod_internal.h"
#include "../flod.h"
#include "AmigaFilter.h"
#include "Amiga.h"

void AmigaFilter_defaults(struct AmigaFilter* self) {
	CLASS_DEF_INIT();
	// static initializers go here
	self->forced = FORCE_OFF;
	self->l0 = self->l1 = self->l2 = self->l3 = self->l4 = NAN;
	self->r0 = self->r1 = self->r2 = self->l3 = self->r4 = NAN;
}

void AmigaFilter_ctor(struct AmigaFilter* self) {
	CLASS_CTOR_DEF(AmigaFilter);
	PFUNC();
	// original constructor code goes here
}

struct AmigaFilter* AmigaFilter_new(void) {
	CLASS_NEW_BODY(AmigaFilter);
}

void AmigaFilter_initialize(struct AmigaFilter* self) {
	PFUNC();
	self->l0 = self->l1 = self->l2 = self->l3 = self->l4 = 0.0f;
	self->r0 = self->r1 = self->r2 = self->r3 = self->r4 = 0.0f;
}

void AmigaFilter_process(struct AmigaFilter* self, int model, struct Sample* sample) {
	//PFUNC();
	const Number FL = 0.5213345843532200f, P0 = 0.4860348337215757f, P1 = 0.9314955486749749f;
	Number d = 1.0f - P0;
	#define e18 (1.e-18f)
	
	if (model == MODEL_A500) {
		self->l0 = P0 * sample->l + d * self->l0 + e18 - e18;
		self->r0 = P0 * sample->r + d * self->r0 + e18 - e18;
		d = 1.0f - P1;
		sample->l = self->l1 = P1 * self->l0 + d * self->l1;
		sample->r = self->r1 = P1 * self->r0 + d * self->r1;
	}

	if ((self->active | self->forced) > 0) {
		d = 1.0f - FL;
		self->l2 = FL * sample->l + d * self->l2 + e18 - e18;
		self->r2 = FL * sample->r + d * self->r2 + e18 - e18;
		self->l3 = FL * self->l2 + d * self->l3;
		self->r3 = FL * self->r2 + d * self->r3;
		sample->l = self->l4 = FL * self->l3 + d * self->l4;
		sample->r = self->r4 = FL * self->r3 + d * self->r4;
	}
	/* //clipping is done in the wave writer code as well
	if (sample->l > 1.0f) sample->l = 1.0f;
	else if (sample->l < -1.0f) sample->l = -1.0f;

	if (sample->r > 1.0f) sample->r = 1.0f;
	else if (sample->r < -1.0f) sample->r = -1.0f;
	*/
}

