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

#include "sound/sound_parameters.h"

namespace Parameters::ZXTune::Sound::Backends
{
  //! @brief Parameters#ZXTune#Sound#Backends namespace prefix
  const auto PREFIX = Sound::PREFIX + "backends"_id;

  //! @brief Semicolon-delimited backends identifiers order
  const auto ORDER = PREFIX + "order"_id;

  //! @brief Any file-based backend parameters namespace
  namespace File
  {
    //! @brief Parameters#ZXTune#Sound#Backends#File namespace prefix
    const auto PREFIX = Backends::PREFIX + "file"_id;

    //@{
    //! @name Any file-based backend basic parameters

    //! @brief Output filename template
    //! @see core/module_attrs.h for possibly supported field names
    const auto FILENAME = PREFIX + "filename"_id;

    //! @brief Buffers count to asynchronous saving
    //! @note Not zero if use asynchronous saving
    const auto BUFFERS = PREFIX + "buffers"_id;
    //@}
  }  // namespace File

  //! @brief %Win32 backend parameters namespace
  namespace Win32
  {
    //! @brief Parameters#ZXTune#Sound#Backends#Win32 namespace prefix
    const auto PREFIX = Backends::PREFIX + "win32"_id;

    //@{
    //! @name Win32 backend parameters

    //! Default value
    const IntType DEVICE_DEFAULT = -1;
    //! 0-based output device index
    const auto DEVICE = PREFIX + "device"_id;

    //! Default value
    const IntType BUFFERS_DEFAULT = 3;
    //! Buffers count
    const auto BUFFERS = PREFIX + "buffers"_id;
    //@}
  }  // namespace Win32

  //! @brief %Oss backend parameters
  namespace Oss
  {
    //! @brief Parameters#ZXTune#Sound#Backends#Oss namespace prefix
    const auto PREFIX = Backends::PREFIX + "oss"_id;

    //@{
    //! @name OSS backend parameters

    //! Default value
    const auto DEVICE_DEFAULT = "/dev/dsp";
    //! Device filename
    const auto DEVICE = PREFIX + "device"_id;

    //! Default value
    const auto MIXER_DEFAULT = "/dev/mixer";
    //! Mixer filename
    const auto MIXER = PREFIX + "mixer"_id;
    //@}
  }  // namespace Oss

  //! @brief %Alsa backend parameters
  namespace Alsa
  {
    //! @brief Parameters#ZXTune#Sound#Backends#Alsa namespace prefix
    const auto PREFIX = Backends::PREFIX + "alsa"_id;

    //@{
    //! @name ALSA backend parameters

    //! Default value
    const auto DEVICE_DEFAULT = "default";
    //! Device name
    const auto DEVICE = PREFIX + "device"_id;

    //! Mixer name
    const auto MIXER = PREFIX + "mixer"_id;

    //! Default value
    const IntType LATENCY_DEFAULT = 100;
    //! Latency in mS
    const auto LATENCY = PREFIX + "latency"_id;
    //@}
  }  // namespace Alsa

  //! @brief %Sdl backend parameters
  namespace Sdl
  {
    //! @brief Parameters#ZXTune#Sound#Backends#Sdl namespace prefix
    const auto PREFIX = Backends::PREFIX + "sdl"_id;

    //@{
    //! @name SDL backend parameters

    //! Default value
    const IntType BUFFERS_DEFAULT = 5;
    //! Buffers count
    const auto BUFFERS = PREFIX + "buffers"_id;
    //@}
  }  // namespace Sdl

  //! @brief %DirectSound backend parameters
  namespace DirectSound
  {
    //! @brief Parameters#ZXTune#Sound#Backends#DirectSound namespace prefix
    const auto PREFIX = Backends::PREFIX + "dsound"_id;

    //@{
    //! @name DirectSound backend parameters

    //! Device uuid (empty for primary)
    const auto DEVICE = PREFIX + "device"_id;

    //! Default value
    const IntType LATENCY_DEFAULT = 100;
    //! Latency in mS
    const auto LATENCY = PREFIX + "latency"_id;
    //@}
  }  // namespace DirectSound

  //! @brief %Mp3 backend parameters
  namespace Mp3
  {
    //! @brief Parameters#ZXTune#Sound#Backends#Mp3 namespace prefix
    const auto PREFIX = Backends::PREFIX + "mp3"_id;

    //@{
    //! @name Mp3 backend parameters

    const auto MODE_CBR = "cbr";
    const auto MODE_VBR = "vbr";
    const auto MODE_ABR = "abr";
    //! Default
    const auto MODE_DEFAULT = MODE_CBR;
    //! Operational mode
    const auto MODE = PREFIX + "mode"_id;

    //! Default value
    const IntType BITRATE_DEFAULT = 128;
    //! Bitrate in kbps
    const auto BITRATE = PREFIX + "bitrate"_id;

    //! Default value
    const IntType QUALITY_DEFAULT = 5;
    //! VBR quality 9..0
    const auto QUALITY = PREFIX + "quality"_id;

    const auto CHANNELS_DEFAULT = "default";
    const auto CHANNELS_STEREO = "stereo";
    const auto CHANNELS_JOINTSTEREO = "jointstereo";
    const auto CHANNELS_MONO = "mono";
    //! Channels encoding mode
    const auto CHANNELS = PREFIX + "channels"_id;
    //@}
  }  // namespace Mp3

  //! @brief %Ogg backend parameters
  namespace Ogg
  {
    //! @brief Parameters#ZXTune#Sound#Backends#Ogg namespace prefix
    const auto PREFIX = Backends::PREFIX + "ogg"_id;

    //@{
    //! @name Ogg backend parameters

    const auto MODE_QUALITY = "quality";
    const auto MODE_ABR = "abr";
    //! Default value
    const auto MODE_DEFAULT = MODE_QUALITY;

    // Working mode
    const auto MODE = PREFIX + "mode"_id;

    //! Default value
    const IntType QUALITY_DEFAULT = 6;
    //! VBR quality 1..10
    const auto QUALITY = PREFIX + "quality"_id;

    //! Default value
    const IntType BITRATE_DEFAULT = 128;
    //! ABR bitrate in kbps
    const auto BITRATE = PREFIX + "bitrate"_id;
    //@}
  }  // namespace Ogg

  //! @brief %Flac backend parameters
  namespace Flac
  {
    //! @brief Parameters#ZXTune#Sound#Backends#Flac namespace prefix
    const auto PREFIX = Backends::PREFIX + "flac"_id;

    //@{
    //! @name Flac backend parameters

    //! Default value
    const IntType COMPRESSION_DEFAULT = 5;
    //! Compression level 0..8
    const auto COMPRESSION = PREFIX + "compression"_id;

    //! Block size in samples
    const auto BLOCKSIZE = PREFIX + "blocksize"_id;
    //@}
  }  // namespace Flac

  //! @brief %OpenAl backend parameters namespace
  namespace OpenAl
  {
    //! @brief Parameters#ZXTune#Sound#Backends#OpenAl namespace prefix
    const auto PREFIX = Backends::PREFIX + "openal"_id;

    //@{
    //! @name OpenAl backend parameters

    //! Default value
    const auto DEVICE_DEFAULT = "";
    //! Device name
    const auto DEVICE = PREFIX + "device"_id;

    //! Default value
    const IntType BUFFERS_DEFAULT = 5;
    //! Buffers count
    const auto BUFFERS = PREFIX + "buffers"_id;
    //@}
  }  // namespace OpenAl
}  // namespace Parameters::ZXTune::Sound::Backends
