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

namespace Binary
{
  class DataInputStream;
}  // namespace Binary
namespace Formats::Chiptune
{
  class MetaBuilder;
}  // namespace Formats::Chiptune

namespace Formats::Chiptune::Vorbis
{
  void ParseComment(Binary::DataInputStream& payload, MetaBuilder& target);
}  // namespace Formats::Chiptune::Vorbis
