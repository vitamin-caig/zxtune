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

//common includes
#include <parameters.h>

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
        extern const NameType PREFIX;

        //! @brief Semicolon-delimited backends identifiers order
        extern const NameType ORDER;

        //! @brief Any file-based backend parameters namespace
        namespace File
        {
          //! @brief Parameters#ZXTune#Sound#Backends#File namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name Any file-based backend basic parameters

          //! @brief Output filename template
          //! @see core/module_attrs.h for possibly supported field names
          extern const NameType FILENAME;

          //! @brief Buffers count to asynchronous saving
          //! @note Not zero if use asynchronous saving
          extern const NameType BUFFERS;
          //@}
        }

        //! @brief %Win32 backend parameters namespace
        namespace Win32
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Win32 namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name Win32 backend parameters

          //! Default value
          const IntType DEVICE_DEFAULT = -1;
          //! 0-based output device index
          extern const NameType DEVICE;

          //! Default value
          const IntType BUFFERS_DEFAULT = 3;
          //! Buffers count
          extern const NameType BUFFERS;
          //@}
        }

        //! @brief %Oss backend parameters
        namespace Oss
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Oss namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name OSS backend parameters

          //! Default value
          const Char DEVICE_DEFAULT[] = {'/', 'd', 'e', 'v', '/', 'd', 's', 'p', '\0'};
          //! Device filename
          extern const NameType DEVICE;

          //! Default value
          const Char MIXER_DEFAULT[] = {'/', 'd', 'e', 'v', '/', 'm', 'i', 'x', 'e', 'r', '\0'};
          //! Mixer filename
          extern const NameType MIXER;
          //@}
        }

        //! @brief %Alsa backend parameters
        namespace Alsa
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Alsa namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name ALSA backend parameters

          //! Default value
          const Char DEVICE_DEFAULT[] = {'d', 'e', 'f', 'a', 'u', 'l', 't', '\0'};
          //! Device name
          extern const NameType DEVICE;

          //! Mixer name
          extern const NameType MIXER;

          //! Default value
          const IntType BUFFERS_DEFAULT = 3;
          //! Buffers count
          extern const NameType BUFFERS;
          //@}
        }

        //! @brief %Sdl backend parameters
        namespace Sdl
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Sdl namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name SDL backend parameters

          //! Default value
          const IntType BUFFERS_DEFAULT = 5;
          //! Buffers count
          extern const NameType BUFFERS;
          //@}
        }

        //! @brief %DirectSound backend parameters
        namespace DirectSound
        {
          //! @brief Parameters#ZXTune#Sound#Backends#DirectSound namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name DirectSound backend parameters

          //! Device uuid (empty for primary)
          extern const NameType DEVICE;

          //! Default value
          const IntType LATENCY_DEFAULT = 100;
          //! Buffers count
          extern const NameType LATENCY;
          //@}
        }

        //! @brief %Mp3 backend parameters
        namespace Mp3
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Mp3 namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name Mp3 backend parameters

          const Char MODE_CBR[] = {'c','b','r',0};
          const Char MODE_VBR[] = {'v','b','r',0};
          const Char MODE_ABR[] = {'a','b','r',0};
          //! Default
          const Char MODE_DEFAULT[] = {'c','b','r',0};
          //! Operational mode
          extern const NameType MODE;

          //! Default value
          const IntType BITRATE_DEFAULT = 128;
          //! Bitrate in kbps
          extern const NameType BITRATE;

          //! Default value
          const IntType QUALITY_DEFAULT = 5;
          //! VBR quality 9..0
          extern const NameType QUALITY;

          const Char CHANNELS_DEFAULT[] = {'d','e','f','a','u','l','t',0};
          const Char CHANNELS_STEREO[] = {'s','t','e','r','e','o',0};
          const Char CHANNELS_JOINTSTEREO[] = {'j','o','i','n','t','s','t','e','r','e','o',0};
          const Char CHANNELS_MONO[] = {'m','o','n','o',0};
          //! Channels encoding mode
          extern const NameType CHANNELS;
          //@}
        }

        //! @brief %Ogg backend parameters
        namespace Ogg
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Ogg namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name Ogg backend parameters

          const Char MODE_QUALITY[] = {'q','u','a','l','i','t','y',0};
          const Char MODE_ABR[] = {'a','b','r',0};
          //! Default value
          const Char MODE_DEFAULT[] = {'q','u','a','l','i','t','y',0};;
          // Working mode
          extern const NameType MODE;

          //! Default value
          const IntType QUALITY_DEFAULT = 6;
          //! VBR quality 1..10
          extern const NameType QUALITY;

          //! Default value
          const IntType BITRATE_DEFAULT = 128;
          //! ABR bitrate in kbps
          extern const NameType BITRATE;
          //@}
        }

        //! @brief %Flac backend parameters
        namespace Flac
        {
          //! @brief Parameters#ZXTune#Sound#Backends#Flac namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name Flac backend parameters

          //! Default value
          const IntType COMPRESSION_DEFAULT = 5;
          //! Compression level 0..8
          extern const NameType COMPRESSION;

          //! Block size in samples
          extern const NameType BLOCKSIZE;
          //@}
        }
      }
    }
  }
}
#endif //__SOUND_BACKENDS_PARAMETERS_H_DEFINED__
