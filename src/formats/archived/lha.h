/*
Abstract:
  Lha structures declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef FORMATS_ARCHIVED_LHA_SUPP_H_DEFINED
#define FORMATS_ARCHIVED_LHA_SUPP_H_DEFINED

//library includes
#include <binary/container.h>

namespace Formats
{
  namespace Archived
  {
    namespace Lha
    {
      Binary::Container::Ptr DecodeRawData(const Binary::Container& input, const std::string& method, std::size_t outputSize);
    }
  }
}

#endif //FORMATS_ARCHIVED_LHA_SUPP_H_DEFINED
