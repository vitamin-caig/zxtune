/**
* 
* @file
*
* @brief  Multitrack archives support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <formats/archived.h>
#include <formats/multitrack.h>

namespace Formats
{
  namespace Archived
  {
    Formats::Archived::Decoder::Ptr CreateMultitrackArchiveDecoder(const String& description, Formats::Multitrack::Decoder::Ptr delegate);
  }
}
