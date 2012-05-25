/*
Abstract:
  Lha access functions declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef FORMATS_PACKED_LHA_SUPP_H_DEFINED
#define FORMATS_PACKED_LHA_SUPP_H_DEFINED

//library includes
#include <formats/packed.h>

namespace Formats
{
  namespace Packed
  {
    namespace Lha
    {
      Container::Ptr DecodeRawData(const Binary::Container& input, const std::string& method, std::size_t outputSize);
    }
  }
}

#endif //FORMATS_PACKED_LHA_SUPP_H_DEFINED
