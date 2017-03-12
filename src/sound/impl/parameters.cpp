/**
*
* @file
*
* @brief  Parameters::ZXTune::Sound and tested
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <zxtune.h>
#include <sound/backends_parameters.h>
#include <sound/sound_parameters.h>
#include <sound/gain.h>

namespace
{
  static_assert(Sound::Gain::CHANNELS == 2, "Incompatible sound channels count");

  struct MixerValue
  {
    const char* const Name;
    const Parameters::IntType DefVal;
  };

  const MixerValue MIXERS[4][4][2] =
  {
    //1-channel
    {
      { {"1.0_0", 100}, {"1.0_1", 100} },
      { {nullptr,         0}, {nullptr,         0} },
      { {nullptr,         0}, {nullptr,         0} },
      { {nullptr,         0}, {nullptr,         0} },
    },
    //2-channel
    {
      { {"2.0_0", 100}, {"2.0_1",   0} },
      { {"2.1_0",   0}, {"2.1_1", 100} },
      { {nullptr,         0}, {nullptr,         0} },
      { {nullptr,         0}, {nullptr,         0} },
    },
    //3-channel
    {
      { {"3.0_0", 100}, {"3.0_1",   0} },
      { {"3.1_0",  60}, {"3.1_1",  60} },
      { {"3.2_0",   0}, {"3.2_1", 100} },
      { {nullptr,         0}, {nullptr,         0} },
    },
    //4-channel
    {
      { {"4.0_0", 100}, {"4.0_1",   0} },
      { {"4.1_0", 100}, {"4.1_1",   0} },
      { {"4.2_0",   0}, {"4.2_1", 100} },
      { {"4.3_0",   0}, {"4.3_1", 100} },
    },
  };
}

namespace Parameters
{
  namespace ZXTune
  {
    namespace Sound
    {
      extern const NameType PREFIX = ZXTune::PREFIX + "sound";

      extern const NameType FREQUENCY = PREFIX + "frequency";
      extern const NameType FRAMEDURATION = PREFIX + "frameduration";
      extern const NameType LOOPED = PREFIX + "looped";
      extern const NameType FADEIN = PREFIX + "fadein";
      extern const NameType FADEOUT = PREFIX + "fadeout";

      namespace Mixer
      {
        extern const NameType PREFIX = Sound::PREFIX + "mixer";

        NameType LEVEL(uint_t totalChannels, uint_t inChannel, uint_t outChannel)
        {
          return PREFIX + std::string(MIXERS[totalChannels - 1][inChannel][outChannel].Name);
        }

        //! @brief Function to get defaul percent-based level
        IntType LEVEL_DEFAULT(uint_t totalChannels, uint_t inChannel, uint_t outChannel)
        {
          return MIXERS[totalChannels - 1][inChannel][outChannel].DefVal;
        }
      }

      namespace Backends
      {
        extern const NameType PREFIX = Sound::PREFIX + "backends";

        extern const NameType ORDER = PREFIX + "order";

        namespace File
        {
          extern const NameType PREFIX = Backends::PREFIX + "file";

          extern const NameType FILENAME = PREFIX + "filename";
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
          extern const NameType LATENCY = PREFIX + "latency";
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
          extern const NameType CHANNELS = PREFIX + "channels";
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

        namespace OpenAl
        {
          extern const NameType PREFIX = Backends::PREFIX + "openal";

          extern const NameType DEVICE = PREFIX + "device";
          extern const NameType BUFFERS = PREFIX + "buffers";
        }
      }
    }
  }
}
