/**
 *
 * @file
 *
 * @brief  Backends parameters names
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "sound/parameters.h"

namespace Parameters::ZXTune::Sound::Backends
{
  constexpr auto Prefix = Sound::Prefix + "backends"_id;

  //! @brief Semicolon-delimited backends identifiers order
  struct Order : Value<StringType>
  {
    static constexpr auto Key = Prefix + "order"_id;
    static constexpr const char* Default = "";
  };

  //! @brief Any file-based backend parameters namespace
  namespace File
  {
    constexpr auto Prefix = Backends::Prefix + "file"_id;

    //! @brief Output filename template
    //! @see core/module_attrs.h for possibly supported field names
    struct Filename : Value<StringType>
    {
      static constexpr auto Key = Prefix + "filename"_id;
    };

    //! @brief Buffers count to asynchronous saving; zero means synchronous
    struct Buffers : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "buffers"_id;
      static constexpr Type Default = 0;
    };
  }  // namespace File

  //! @brief %Win32 backend parameters namespace
  namespace Win32
  {
    constexpr auto Prefix = Backends::Prefix + "win32"_id;

    //! @brief 0-based output device index
    struct Device : Value<int_t>
    {
      static constexpr auto Key = Prefix + "device"_id;
      static constexpr Type Default = -1;
    };

    //! @brief Buffers count
    struct Buffers : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "buffers"_id;
      static constexpr Type Default = 3, Min = 3, Max = 10;
    };
  }  // namespace Win32

  //! @brief %Oss backend parameters
  namespace Oss
  {
    constexpr auto Prefix = Backends::Prefix + "oss"_id;

    //! @brief Device filename
    struct Device : Value<StringType>
    {
      static constexpr auto Key = Prefix + "device"_id;
      static constexpr const char* Default = "/dev/dsp";
    };

    //! @brief Mixer filename
    struct Mixer : Value<StringType>
    {
      static constexpr auto Key = Prefix + "mixer"_id;
      static constexpr const char* Default = "/dev/mixer";
    };
  }  // namespace Oss

  //! @brief %Alsa backend parameters
  namespace Alsa
  {
    constexpr auto Prefix = Backends::Prefix + "alsa"_id;

    //! @brief Device name
    struct Device : Value<StringType>
    {
      static constexpr auto Key = Prefix + "device"_id;
      static constexpr const char* Default = "default";
    };

    //! @brief Mixer name
    struct Mixer : Value<StringType>
    {
      static constexpr auto Key = Prefix + "mixer"_id;
      static constexpr const char* Default = "";
    };

    //! @brief Latency in mS
    struct Latency : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "latency"_id;
      static constexpr Type Default = 100, Min = 20, Max = 10'000;
    };
  }  // namespace Alsa

  //! @brief %Sdl backend parameters
  namespace Sdl
  {
    constexpr auto Prefix = Backends::Prefix + "sdl"_id;

    //! @brief Buffers count
    struct Buffers : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "buffers"_id;
      static constexpr Type Default = 5, Min = 2, Max = 10;
    };
  }  // namespace Sdl

  //! @brief %DirectSound backend parameters
  namespace DirectSound
  {
    constexpr auto Prefix = Backends::Prefix + "dsound"_id;

    //! @brief Device uuid (empty for primary)
    struct Device : Value<StringType>
    {
      static constexpr auto Key = Prefix + "device"_id;
      static constexpr const char* Default = "";
    };

    //! @brief Latency in mS
    struct Latency : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "latency"_id;
      static constexpr Type Default = 100, Min = 20, Max = 10'000;
    };
  }  // namespace DirectSound

  //! @brief %Mp3 backend parameters
  namespace Mp3
  {
    constexpr auto Prefix = Backends::Prefix + "mp3"_id;

    //! @brief Operational mode
    struct Mode : Value<StringType>
    {
      static constexpr auto Key = Prefix + "mode"_id;
      static constexpr const char* CBR = "cbr";
      static constexpr const char* VBR = "vbr";
      static constexpr const char* ABR = "abr";
      static constexpr const char* Default = CBR;
    };

    //! @brief Bitrate in kbps
    struct Bitrate : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "bitrate"_id;
      static constexpr Type Default = 128, Min = 32, Max = 320;
    };

    //! @brief VBR quality 9..0
    struct Quality : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "quality"_id;
      static constexpr Type Default = 5, Min = 0, Max = 9;
    };

    //! @brief Channels encoding mode
    struct Channels : Value<StringType>
    {
      static constexpr auto Key = Prefix + "channels"_id;
      static constexpr const char* Default = "default";
      static constexpr const char* Stereo = "stereo";
      static constexpr const char* JointStereo = "jointstereo";
      static constexpr const char* Mono = "mono";
    };
  }  // namespace Mp3

  //! @brief %Ogg backend parameters
  namespace Ogg
  {
    constexpr auto Prefix = Backends::Prefix + "ogg"_id;

    //! @brief Working mode
    struct Mode : Value<StringType>
    {
      static constexpr auto Key = Prefix + "mode"_id;
      static constexpr const char* Quality = "quality";
      static constexpr const char* ABR = "abr";
      static constexpr const char* Default = Quality;
    };

    //! @brief VBR quality 1..10
    struct Quality : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "quality"_id;
      static constexpr Type Default = 6, Min = 0, Max = 10;
    };

    //! @brief ABR bitrate in kbps
    struct Bitrate : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "bitrate"_id;
      static constexpr Type Default = 128, Min = 48, Max = 500;
    };
  }  // namespace Ogg

  //! @brief %Flac backend parameters
  namespace Flac
  {
    constexpr auto Prefix = Backends::Prefix + "flac"_id;

    //! @brief Compression level 0..8
    struct Compression : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "compression"_id;
      static constexpr Type Default = 5, Min = 0, Max = 8;
    };

    //! @brief Block size in samples
    struct Blocksize : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "blocksize"_id;
    };
  }  // namespace Flac

  //! @brief %OpenAl backend parameters namespace
  namespace OpenAl
  {
    constexpr auto Prefix = Backends::Prefix + "openal"_id;

    //! @brief Device name
    struct Device : Value<StringType>
    {
      static constexpr auto Key = Prefix + "device"_id;
      static constexpr const char* Default = "";
    };

    //! @brief Buffers count
    struct Buffers : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "buffers"_id;
      static constexpr Type Default = 5;
    };
  }  // namespace OpenAl
}  // namespace Parameters::ZXTune::Sound::Backends
