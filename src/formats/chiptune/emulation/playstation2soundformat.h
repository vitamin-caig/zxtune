/**
* 
* @file
*
* @brief  PSF2 VFS parser interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace Playstation2SoundFormat
    {
      const constexpr uint_t VERSION_ID = 2;

      class Builder
      {
      public:
        virtual ~Builder() = default;
        
        virtual void OnFile(String path, Binary::Container::Ptr content) = 0;
      };

      void ParseVFS(const Binary::Container& data, Builder& target);
    }

    Decoder::Ptr CreatePSF2Decoder();
  }
}
