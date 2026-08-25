/**
 *
 * @file
 *
 * @brief  vgmstream-based formats support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/factory.h"

#include "formats/multitrack/decoder.h"

#include "string_view.h"

namespace Module::VGMStream
{
  enum class PluginType
  {
    SIMPLE,
    MULTIFILE,
    MULTITRACK
  };

  ExternalParsingFactory::Ptr CreateFactory(StringView id, StringView description, StringView suffix, PluginType type);
  MultitrackFactory::Ptr CreateMultitrackFactory(StringView id, StringView description, StringView suffix);
}  // namespace Module::VGMStream

namespace Formats::Multitrack
{
  Decoder::Ptr CreateVGMStreamDecoder(StringView format, StringView description, StringView suffix);
}
