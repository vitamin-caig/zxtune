/**
* 
* @file
*
* @brief  LHA compressor support interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <formats/packed.h>

namespace Formats
{
  namespace Packed
  {
    namespace Lha
    {
      Container::Ptr DecodeRawData(const Binary::Container& input, const String& method, std::size_t outputSize);
      Container::Ptr DecodeRawDataAtLeast(const Binary::Container& input, const String& method, std::size_t sizeHint);
    }
  }
}
