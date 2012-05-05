/*
Abstract:
  Playlist parameters definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_PARAMETERS_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_PARAMETERS_H_DEFINED

//local includes
#include "app_parameters.h"

namespace Parameters
{
  namespace ZXTuneQT
  {
    namespace Playlist
    {
      const Char NAMESPACE_NAME[] = {'P','l','a','y','l','i','s','t','\0'};

      const NameType PREFIX = NameType(Parameters::ZXTuneQT::PREFIX) + NAMESPACE_NAME + NAMESPACE_DELIMITER;

      //@{
      //! @name Use deep scanning while processing input files
      const Char DEEP_SCANNING_PARAMETER[] = {'D','e','e','p','S','c','a','n','n','i','n','g','\0'};

      //! Parameter name
      const NameType DEEP_SCANNING = PREFIX + DEEP_SCANNING_PARAMETER;
      //! Default value
      const IntType DEEP_SCANNING_DEFAULT = 1;
      //@}

      //@{
      //! @name Loop playlist playback
      const Char LOOPED_PARAMETER[] = {'L','o','o','p','\0'};

      //! Parameter name
      const NameType LOOPED = PREFIX + LOOPED_PARAMETER;
      //! Default value
      const IntType LOOPED_DEFAULT = 0;
      //@}

      //@{
      //! @name Randomize playlist playback
      const Char RANDOMIZED_PARAMETER[] = {'R','a','n','d','o','m','\0'};

      //! Parameter name
      const NameType RANDOMIZED = PREFIX + RANDOMIZED_PARAMETER;
      //! Default value
      const IntType RANDOMIZED_DEFAULT = 0;
      //@}
    }
  }
}
#endif //ZXTUNE_QT_PLAYLIST_PARAMETERS_H_DEFINED
