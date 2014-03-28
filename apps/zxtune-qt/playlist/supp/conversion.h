/**
* 
* @file
*
* @brief Conversion-related BL
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <parameters/accessor.h>

namespace Playlist
{
  namespace Item
  {
    namespace Conversion
    {
      struct Options
      {
        String Type;
        String FilenameTemplate;
        Parameters::Accessor::Ptr Params;
      };
    }
  }
}
