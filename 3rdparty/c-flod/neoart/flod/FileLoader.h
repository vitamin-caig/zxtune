#ifndef FILELOADER_H
#define FILELOADER_H

#include "core/CorePlayer.h"
#include "core/Amiga.h"
#include "core/Soundblaster.h"

enum FileLoader_Tracker {
      SOUNDTRACKER = 0,
      NOISETRACKER = 4,
      PROTRACKER   = 9,
      HISMASTER    = 12,
      SOUNDFX      = 13,
      BPSOUNDMON   = 17,
      DELTAMUSIC   = 20,
      DIGITALMUG   = 22,
      FUTURECOMP   = 24,
      SIDMON       = 26,
      WHITTAKER    = 28,
      FREDED       = 29,
      HIPPEL       = 30,
      HUBBARD      = 32,
      FASTTRACKER  = 33,
};

struct FileLoader {
	struct CorePlayer *player;
	int index;
	struct Amiga *amiga;
	struct Soundblaster *mixer;
};

extern const char* TRACKERS[];

void FileLoader_defaults(struct FileLoader* self);
void FileLoader_ctor(struct FileLoader* self);
struct FileLoader* FileLoader_new(void);

const char* FileLoader_get_tracker(struct FileLoader* self);
struct CorePlayer *FileLoader_load(struct FileLoader* self, struct ByteArray *stream);

#endif
