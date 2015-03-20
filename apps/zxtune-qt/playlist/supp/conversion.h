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
        typedef boost::shared_ptr<const Options> Ptr;
        
        Options(const String& type, const String& filenameTemplate, Parameters::Accessor::Ptr params)
          : Type(type)
          , FilenameTemplate(filenameTemplate)
          , Params(params)
        {
        }
        
        const String Type;
        const String FilenameTemplate;
        const Parameters::Accessor::Ptr Params;
      };
    }
  }
}
