/**
*
* @file      sound/backends_parameters.h
* @brief     Backends parameters names
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __SOUND_BACKENDS_PARAMETERS_H_DEFINED__
#define __SOUND_BACKENDS_PARAMETERS_H_DEFINED__

//local includes
#include "sound_parameters.h"

namespace Parameters
{
  namespace ZXTune
  {
    namespace Sound
    {
      //! @brief Backend-specific parameters namespace
      namespace Backends
      {
        //! @brief Parameters#ZXTune#Sound#Backends namespace prefix
        const NameType PREFIX = Sound::PREFIX + "backends";

        //! @brief Semicolon-delimited backends identifiers order
        const NameType ORDER = PREFIX + "order";

        //! @brief Any file-based backend parameters namespace
        namespace File
        {
          //! @brief Parameters#ZXTune#Sound#Backends#File namespace prefix
          const NameType PREFIX = Backends::PREFIX + "file";

          //@{
          //! @name Any file-based backend basic parameters

          //! @brief Output filename template
          //! @see core/module_attrs.h for possibly supported field names
          const NameType FILENAME = PREFIX + "filename";

          //! @brief Rewrite output files if exist
          //! @note Not zero if rewrite
          const NameType OVERWRITE = PREFIX + "overwrite";

          //! @brief Buffers count to asynchronous saving
          //! @note Not zero if use asynchronous saving
          const NameType BUFFERS = PREFIX + "buffers";
          //@}
        }

        //! @brief %Win32 backend parameters namespace
        namespace Win32
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Win32 namespace prefix
          const NameType PREFIX = Backends::PREFIX + "win32";

          //@{
          //! @name Win32 backend parameters

          //! Default value
          const IntType DEVICE_DEFAULT = -1;
          //! 0-based output device index
          const NameType DEVICE = PREFIX + "device";

          //! Default value
          const IntType BUFFERS_DEFAULT = 3;
          //! Buffers count
          const NameType BUFFERS = PREFIX + "buffers";
          //@}
        }

        //! @brief %Oss backend parameters
        namespace Oss
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Oss namespace prefix
          const NameType PREFIX = Backends::PREFIX + "oss";

          //@{
          //! @name OSS backend parameters

          //! Default value
          const Char DEVICE_DEFAULT[] = {'/', 'd', 'e', 'v', '/', 'd', 's', 'p', '\0'};
          //! Device filename
          const NameType DEVICE = PREFIX + "device";

          //! Default value
          const Char MIXER_DEFAULT[] = {'/', 'd', 'e', 'v', '/', 'm', 'i', 'x', 'e', 'r', '\0'};
          //! Mixer filename
          const NameType MIXER = PREFIX + "mixer";
          //@}
        }

        //! @brief %Alsa backend parameters
        namespace Alsa
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Alsa namespace prefix
          const NameType PREFIX = Backends::PREFIX + "alsa";

          //@{
          //! @name ALSA backend parameters

          //! Default value
          const Char DEVICE_DEFAULT[] = {'d', 'e', 'f', 'a', 'u', 'l', 't', '\0'};
          //! Device name
          const NameType DEVICE = PREFIX + "device";

          //! Mixer name
          const NameType MIXER = PREFIX + "mixer";

          //! Default value
          const IntType BUFFERS_DEFAULT = 3;
          //! Buffers count
          const NameType BUFFERS = PREFIX + "buffers";
          //@}
        }

        //! @brief %Sdl backend parameters
        namespace Sdl
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Sdl namespace prefix
          const NameType PREFIX = Backends::PREFIX + "sdl";

          //@{
          //! @name SDL backend parameters

          //! Default value
          const IntType BUFFERS_DEFAULT = 5;
          //! Buffers count
          const NameType BUFFERS = PREFIX + "buffers";
          //@}
        }

        //! @brief %DirectSound backend parameters
        namespace DirectSound
        {
          //! @brief Parameters#ZXTune#Sound#Backends#DirectSound namespace prefix
          const NameType PREFIX = Backends::PREFIX + "dsound";

          //@{
          //! @name DirectSound backend parameters

          //! Device uuid (empty for primary)
          const NameType DEVICE = PREFIX + "device";

          //! Default value
          const IntType LATENCY_DEFAULT = 100;
          //! Buffers count
          const NameType LATENCY = PREFIX + "latency";
          //@}
        }

        //! @brief %Mp3 backend parameters
        namespace Mp3
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Mp3 namespace prefix
          const NameType PREFIX = Backends::PREFIX + "mp3";

          //@{
          //! @name Mp3 backend parameters

          const Char MODE_CBR[] = {'c','b','r',0};
          const Char MODE_VBR[] = {'v','b','r',0};
          const Char MODE_ABR[] = {'a','b','r',0};
          //! Default
          const Char MODE_DEFAULT[] = {'c','b','r',0};
          //! Operational mode
          const NameType MODE = PREFIX + "mode";

          //! Default value
          const IntType BITRATE_DEFAULT = 128;
          //! Bitrate in kbps
          const NameType BITRATE = PREFIX + "bitrate";

          //! Default value
          const IntType QUALITY_DEFAULT = 5;
          //! VBR quality 9..0
          const NameType QUALITY = PREFIX + "quality";
          //@}
        }

        //! @brief %Ogg backend parameters
        namespace Ogg
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Ogg namespace prefix
          const NameType PREFIX = Backends::PREFIX + "ogg";

          //@{
          //! @name Ogg backend parameters

          const Char MODE_QUALITY[] = {'q','u','a','l','i','t','y',0};
          const Char MODE_ABR[] = {'a','b','r',0};
          //! Default value
          const Char MODE_DEFAULT[] = {'q','u','a','l','i','t','y',0};;
          // Working mode
          const NameType MODE = PREFIX + "mode";

          //! Default value
          const IntType QUALITY_DEFAULT = 6;
          //! VBR quality 1..10
          const NameType QUALITY = PREFIX + "quality";

          //! Default value
          const IntType BITRATE_DEFAULT = 128;
          //! ABR bitrate in kbps
          const NameType BITRATE = PREFIX + "bitrate";
          //@}
        }

        //! @brief %Flac backend parameters
        namespace Flac
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Flac namespace prefix
          const NameType PREFIX = Backends::PREFIX + "flac";

          //@{
          //! @name Flac backend parameters

          //! Default value
          const IntType COMPRESSION_DEFAULT = 5;
          //! Compression level 0..8
          const NameType COMPRESSION = PREFIX + "compression";

          //! Block size in samples
          const NameType BLOCKSIZE = PREFIX + "blocksize";
          //@}
        }
      }
    }
  }
}
#endif //__SOUND_BACKENDS_PARAMETERS_H_DEFINED__
