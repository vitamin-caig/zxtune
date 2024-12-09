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

#include "F2Pattern.h"
#include "../flod_internal.h"
#include "../flod.h"

void F2Pattern_defaults(struct F2Pattern* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void F2Pattern_ctor(struct F2Pattern* self, unsigned length, unsigned channels) {
	CLASS_CTOR_DEF(F2Pattern);
	// original constructor code goes here
	self->size = length * channels;
	//rows = new Vector.<F2Row>(size, true);
	assert_op(self->size, <=, F2PATTERN_MAX_ROWS);
	self->length = length;
}

struct F2Pattern* F2Pattern_new(unsigned length, unsigned channels) {
	CLASS_NEW_BODY(F2Pattern, length, channels);
}

