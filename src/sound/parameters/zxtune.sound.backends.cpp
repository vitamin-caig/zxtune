/*
Abstract:
  Parameters::ZXTune::Sound::Backends implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <zxtune.h>
#include <sound/backends_parameters.h>
#include <sound/sound_parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    namespace Sound
    {
      namespace Backends
      {
        extern const NameType PREFIX = Sound::PREFIX + "backends";

        extern const NameType ORDER = PREFIX + "order";

        namespace File
        {
          extern const NameType PREFIX = Backends::PREFIX + "file";

          extern const NameType FILENAME = PREFIX + "filename";
          extern const NameType OVERWRITE = PREFIX + "overwrite";
					extern const NameType BUFFERS = PREFIX + "buffers";
        }

        namespace Win32
        {
          extern const NameType PREFIX = Backends::PREFIX + "win32";

          extern const NameType DEVICE = PREFIX + "device";
          extern const NameType BUFFERS = PREFIX + "buffers";
        }

        namespace Oss
        {
          extern const NameType PREFIX = Backends::PREFIX + "oss";

          extern const NameType DEVICE = PREFIX + "device";
          extern const NameType MIXER = PREFIX + "mixer";
        }

        namespace Alsa
        {
          extern const NameType PREFIX = Backends::PREFIX + "alsa";

          extern const NameType DEVICE = PREFIX + "device";
          extern const NameType MIXER = PREFIX + "mixer";
          extern const NameType BUFFERS = PREFIX + "buffers";
        }

        namespace Sdl
        {
          extern const NameType PREFIX = Backends::PREFIX + "sdl";

          extern const NameType BUFFERS = PREFIX + "buffers";
        }

        namespace DirectSound
        {
          extern const NameType PREFIX = Backends::PREFIX + "dsound";

          extern const NameType DEVICE = PREFIX + "device";
          extern const NameType LATENCY = PREFIX + "latency";
        }

        namespace Mp3
        {
          extern const NameType PREFIX = Backends::PREFIX + "mp3";

          extern const NameType MODE = PREFIX + "mode";
          extern const NameType BITRATE = PREFIX + "bitrate";
          extern const NameType QUALITY = PREFIX + "quality";
        }

        namespace Ogg
        {
          extern const NameType PREFIX = Backends::PREFIX + "ogg";

          extern const NameType MODE = PREFIX + "mode";
          extern const NameType QUALITY = PREFIX + "quality";
          extern const NameType BITRATE = PREFIX + "bitrate";
        }

        namespace Flac
        {
          extern const NameType PREFIX = Backends::PREFIX + "flac";

          extern const NameType COMPRESSION = PREFIX + "compression";
          extern const NameType BLOCKSIZE = PREFIX + "blocksize";
        }
      }
    }
  }
}
