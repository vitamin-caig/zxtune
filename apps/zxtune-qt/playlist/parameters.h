/**
* 
* @file
*
* @brief Playlist parameters definition
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

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

      //@}
      //! @name Index of last played tab 
      const NameType INDEX = PREFIX + "Index";
      
      //! @name Index of last played track
      const NameType TRACK = PREFIX + "Track";
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
