/**
 *
 * @file
 *
 * @brief  Vorbis tags parser interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/builder_meta.h"

#include "binary/input_stream.h"

namespace Formats::Chiptune::Vorbis
{
  void ParseComment(Binary::DataInputStream& payload, MetaBuilder& target);
}  // namespace Formats::Chiptune::Vorbis
