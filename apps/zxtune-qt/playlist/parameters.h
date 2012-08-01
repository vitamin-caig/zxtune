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
      const std::string NAMESPACE_NAME("Playlist");

      const NameType PREFIX = ZXTuneQT::PREFIX + NAMESPACE_NAME;

      //@{
      //! @name Use deep scanning while processing input files

      //! Parameter name
      const NameType DEEP_SCANNING = PREFIX + "DeepScanning";
      //! Default value
      const IntType DEEP_SCANNING_DEFAULT = 1;
      //@}

      //@{
      //! @name Loop playlist playback

      //! Parameter name
      const NameType LOOPED = PREFIX + "Loop";
      //! Default value
      const IntType LOOPED_DEFAULT = 0;
      //@}

      //@{
      //! @name Randomize playlist playback

      //! Parameter name
      const NameType RANDOMIZED = PREFIX + "Random";
      //! Default value
      const IntType RANDOMIZED_DEFAULT = 0;
      //@}
    }
  }
}
#endif //ZXTUNE_QT_PLAYLIST_PARAMETERS_H_DEFINED
