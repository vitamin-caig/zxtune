/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.0 - 2012/03/10

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.

  ---

  This is a simple demo showing how to use the FileLoader class to play a module, in any of the supported
  formats, loaded from the server.

  Warning: the server must be able to handle the correct file extension, if it can't just rename the file
  to filename->wav and it should work...
*/

#include "../neoart/flod/FileLoader.h"
#include "../neoart/flod/flod.h"


int main(int argc, char** argv) {
	struct FileLoader *loader;
	struct CorePlayer *player;
	struct ByteArray *data;
	
	loader = FileLoader_new();
	data = ByteArray_new();
	if(argc < 2) {
		puts("expect filename as arg!");
		return 1;
	}
	if(!ByteArray_open_file(data, argv[1])) {
		perror("failed to open file");
		return 1;
	}
	player = FileLoader_load(loader, data);
	if (player && player->version) {
		CorePlayer_record(player);
		CorePlayer_play(player, NULL);
	}
	EventDispatcher_event_loop();
	
	return 0;
}
