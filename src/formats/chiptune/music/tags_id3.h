/**
 *
 * @file
 *
 * @brief  ID3 tags parser interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/builder_meta.h"

#include "binary/input_stream.h"

namespace Formats::Chiptune::Id3
{
  bool Parse(Binary::DataInputStream& stream, MetaBuilder& target);
}  // namespace Formats::Chiptune::Id3
