/**
 *
 * @file
 *
 * @brief  Archived data accessors factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <formats/archived.h>

namespace Formats::Archived
{
  Decoder::Ptr CreateZipDecoder();
  Decoder::Ptr CreateRarDecoder();
  Decoder::Ptr CreateZXZipDecoder();
  Decoder::Ptr CreateSCLDecoder();
  Decoder::Ptr CreateTRDDecoder();
  Decoder::Ptr CreateHripDecoder();
  Decoder::Ptr CreateLhaDecoder();
  Decoder::Ptr CreateZXStateDecoder();
  Decoder::Ptr CreateUMXDecoder();
  Decoder::Ptr Create7zipDecoder();
}  // namespace Formats::Archived
