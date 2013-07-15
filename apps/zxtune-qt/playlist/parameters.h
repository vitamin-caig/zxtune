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

      namespace Cache
      {
        const std::string NAMESPACE_NAME("Cache");

        const NameType PREFIX = Playlist::PREFIX + NAMESPACE_NAME;

        //@{
        //! @name Cache size limit in bytes

        //! Parameter name
        const NameType MEMORY_LIMIT_MB = PREFIX + "Memory";
        //! Default value
        const IntType MEMORY_LIMIT_MB_DEFAULT = 10;
        //@}

        //@{
        //! @name Cache size limit in files

        //! Parameter name
        const NameType FILES_LIMIT = PREFIX + "Files";
        //! Default value
        const IntType FILES_LIMIT_DEFAULT = 1000;
        //@}
      }

      namespace Store
      {
        const std::string NAMESPACE_NAME("Store");

        const NameType PREFIX = Playlist::PREFIX + NAMESPACE_NAME;

        //@{

        //! @name Store full properties set instead of only changed

        //! Parameter name
        const NameType PROPERTIES = PREFIX + "Properties";
        //! Default value
        const IntType PROPERTIES_DEFAULT = 0;
        //@}
      }
    }
  }
}
#endif //ZXTUNE_QT_PLAYLIST_PARAMETERS_H_DEFINED
