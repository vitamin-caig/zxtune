/*
  Flod 4.1
  2012/04/30
  Christian Corti
  Neoart Costa Rica

  Last Update: Flod 4.1 - 2012/04/16

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This work is licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0 Unported License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
  Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
*/

#include "FileLoader.h"
#include "flod_internal.h"
#include "whittaker/DWPlayer.h"
#include "futurecomposer/FCPlayer.h"
#include "trackers/STPlayer.h"
#include "trackers/PTPlayer.h"
#include "fasttracker/F2Player.h"

/*
  import flash.utils.*;
  import neoart.flip.*;
  import neoart.flod.core.*;
  import neoart.flod.deltamusic.*;
  import neoart.flod.digitalmugician.*;
  import neoart.flod.fred.*;
  import neoart.flod.futurecomposer.*;
  import neoart.flod.hippel.*;
  import neoart.flod.hubbard.*;
  import neoart.flod.sidmon.*;
  import neoart.flod.soundfx.*;
  import neoart.flod.soundmon.*;
  import neoart.flod.trackers.*;
  import neoart.flod.fasttracker.*;
  import neoart.flod.whittaker.*;
*/


void FileLoader_defaults(struct FileLoader* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void FileLoader_ctor(struct FileLoader* self) {
	CLASS_CTOR_DEF(FileLoader);
	// original constructor code goes here
	self->amiga = Amiga_new();
	self->mixer = Soundblaster_new();
}

struct FileLoader* FileLoader_new(void) {
	CLASS_NEW_BODY(FileLoader);
}


const char* FileLoader_get_tracker(struct FileLoader* self) {
	return (self->player) ? TRACKERS[self->index + self->player->version] : TRACKERS[0];
}

struct CorePlayer *FileLoader_load(struct FileLoader* self, struct ByteArray *stream) {
	char *id = "";
	int value = 0;

	stream->endian = BAE_LITTLE;
	ByteArray_set_position(stream, 0);

#ifdef SUPPORT_COMPRESSION
	struct ZipFile archive = NULL;
	if (stream->readUnsignedInt(stream) == 67324752) {
		archive = new ZipFile(stream);
		stream = archive->uncompress(archive->entries[0]);
	}
#endif

	if (!stream) return null;

/*
	if (self->player && !(self->player is STPlayer)) {
		self->player->load(stream);
		if (self->player->version) return self->player;
	}
	*/

	if (ByteArray_get_length(stream) > 336) {
		/*
		stream->position = 38;
		id = stream->readMultiByte(20, CorePlayer->ENCODING);

		if (id == "FastTracker v2.00   " ||
		id == "FastTracker v 2.00  " ||
		id == "Sk@le Tracker"        ||
		id == "MadTracker 2.0"       ||
		id == "MilkyTracker        " ||
		id == "DigiBooster Pro 2.18" ||
		id->indexOf("OpenMPT") != -1) {
			*/

			self->player = (struct CorePlayer*) F2Player_new(self->mixer);
			CorePlayer_load(self->player, stream);

			if (self->player->version) {
				self->index = FASTTRACKER;
				return self->player;
			}
		//}
	}

	stream->endian = BAE_BIG;
	
	/*

	if (stream->length > 2105) {
		stream->position = 1080;
		id = stream->readMultiByte(4, CorePlayer->ENCODING);

		if (id == "M->K." || id == "FLT4") {
			self->player = new MKPlayer(self->amiga);
			self->player->load(stream);

			if (self->player->version) {
				index = NOISETRACKER;
				return self->player;
			}
		} else if (id == "FEST") {
			self->player = new HMPlayer(self->amiga);
			self->player->load(stream);

			if (self->player->version) {
				index = HISMASTER;
				return self->player;
			}
		}
	}
	
	*/

	if (ByteArray_get_length(stream) > 2105) {
		ByteArray_set_position(stream, 0);
		//ByteArray_set_position(stream, 1080);
		//id = stream->readMultiByte(4, CorePlayer->ENCODING);

		//if (id == "M.K." || id == "M!K!") {
			self->player = (struct CorePlayer*) PTPlayer_new(self->amiga);
			CorePlayer_load(self->player, stream);

			if (self->player->version) {
				self->index = PROTRACKER;
				return self->player;
			}
		//}
	}
	
	/*

	if (stream->length > 1685) {
		stream->position = 60;
		id = stream->readMultiByte(4, CorePlayer->ENCODING);

		if (id != "SONG") {
			stream->position = 124;
			id = stream->readMultiByte(4, CorePlayer->ENCODING);
		}

		if (id == "SONG" || id == "SO31") {
			self->player = new FXPlayer(self->amiga);
			self->player->load(stream);

			if (self->player->version) {
				index = SOUNDFX;
				return self->player;
			}
		}
	}

	if (stream->length > 4) {
		stream->position = 0;
		id = stream->readMultiByte(4, CorePlayer->ENCODING);

		if (id == "ALL ") {
			self->player = new D1Player(self->amiga);
			self->player->load(stream);

			if (self->player->version) {
				index = DELTAMUSIC;
				return self->player;
			}
		}
	}

	if (stream->length > 3018) {
		stream->position = 3014;
		id = stream->readMultiByte(4, CorePlayer->ENCODING);

		if (id == ".FNL") {
			self->player = new D2Player(self->amiga);
			self->player->load(stream);

			if (self->player->version) {
				index = DELTAMUSIC;
				return self->player;
			}
		}
	}

	if (stream->length > 30) {
		stream->position = 26;
		id = stream->readMultiByte(3, CorePlayer->ENCODING);

		if (id == "BPS" || id == "V.2" || id == "V.3") {
			self->player = new BPPlayer(self->amiga);
			self->player->load(stream);

			if (self->player->version) {
				index = BPSOUNDMON;
				return self->player;
			}
		}
	}
	*/
	if (ByteArray_get_length(stream) > 4) {
		char myid[4];
		ByteArray_set_position(stream, 0);
		stream->readMultiByte(stream, myid, 4);

		if (!memcmp(myid, "SMOD", 4) || !memcmp(myid, "FC14", 4)) {
			self->player = (struct CorePlayer*) FCPlayer_new(self->amiga);
			CorePlayer_load(self->player, stream);
			//self->player->load(stream);

			if (self->player->version) {
				self->index = FUTURECOMP;
				return self->player;
			}
		}
	}
	/*

	if (stream->length > 10) {
		stream->position = 0;
		id = stream->readMultiByte(9, CorePlayer->ENCODING);

		if (id == " MUGICIAN") {
			self->player = new DMPlayer(self->amiga);
			self->player->load(stream);

			if (self->player->version) {
				index = DIGITALMUG;
				return self->player;
			}
		}
	}

	if (stream->length > 86) {
		stream->position = 58;
		id = stream->readMultiByte(28, CorePlayer->ENCODING);

		if (id == "SIDMON II - THE MIDI VERSION") {
			self->player = new S2Player(self->amiga);
			self->player->load(stream);

			if (self->player->version) {
				index = SIDMON;
				return self->player;
			}
		}
	}

	if (stream->length > 2830) {
		stream->position = 0;
		value = stream->readUnsignedShort();

		if (value == 0x4efa) {
			self->player = new FEPlayer(self->amiga);
			self->player->load(stream);

			if (self->player->version) {
				index = FREDED;
				return self->player;
			}
		}
	}

	if (stream->length > 5220) {
		self->player = new S1Player(self->amiga);
		self->player->load(stream);

		if (self->player->version) {
			index = SIDMON;
			return self->player;
		}
	}

	stream->position = 0;
	value = stream->readUnsignedShort();
	stream->position = 0;
	id = stream->readMultiByte(4, CorePlayer->ENCODING);

	if (id == "COSO" || value == 0x6000 || value == 0x6002 || value == 0x600e || value == 0x6016) {
		self->player = new JHPlayer(self->amiga);
		self->player->load(stream);

		if (self->player->version) {
			index = HIPPEL;
			return self->player;
		}
	}
	*/
	ByteArray_set_position(stream, 0);
	value = stream->readUnsignedShort(stream);

	self->player = (struct CorePlayer*) DWPlayer_new(self->amiga);
	CorePlayer_load(self->player, stream);
	//self->player->load(stream);

	if (self->player->version) {
		self->index = WHITTAKER;
		return self->player;
	}

	ByteArray_set_position(stream, 0);
	value = stream->readUnsignedShort(stream);
	
	/*

	if (value == 0x6000) {
		self->player = new RHPlayer(self->amiga);
		self->player->load(stream);

		if (self->player->version) {
			index = HUBBARD;
			return self->player;
		}
	} */

	if (ByteArray_get_length(stream) > 1625) {
		self->player = (struct CorePlayer*) STPlayer_new(self->amiga);
		CorePlayer_load(self->player, stream);

		if (self->player->version) {
			self->index = SOUNDTRACKER;
			return self->player;
		}
	}

	//ByteArray_clear(stream);
	self->index = 0;
	return self->player = null;
}

const char* TRACKERS[] = {
        "Unknown Format",
        "Ultimate SoundTracker",
        "D->O.C. SoundTracker 9",
        "Master SoundTracker",
        "D->O.C. SoundTracker 2.0/2.2",
        "SoundTracker 2.3",
        "SoundTracker 2.4",
        "NoiseTracker 1.0",
        "NoiseTracker 1.1",
        "NoiseTracker 2.0",
        "ProTracker 1.0",
        "ProTracker 1.1/2.1",
        "ProTracker 1.2/2.0",
        "His Master's NoiseTracker",
        "SoundFX 1.0/1.7",
        "SoundFX 1.8",
        "SoundFX 1.945",
        "SoundFX 1.994/2.0",
        "BP SoundMon V1",
        "BP SoundMon V2",
        "BP SoundMon V3",
        "Delta Music 1.0",
        "Delta Music 2.0",
        "Digital Mugician",
        "Digital Mugician 7 Voices",
        "Future Composer 1.0/1.3",
        "Future Composer 1.4",
        "SidMon 1.0",
        "SidMon 2.0",
        "David Whittaker",
        "FredEd",
        "Jochen Hippel",
        "Jochen Hippel COSO",
        "Rob Hubbard",
        "FastTracker II",
        "Sk@leTracker",
        "MadTracker 2.0",
        "MilkyTracker",
        "DigiBooster Pro 2.18",
        "OpenMPT"
};