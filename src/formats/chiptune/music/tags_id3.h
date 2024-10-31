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

namespace Binary
{
  class DataInputStream;
}  // namespace Binary
namespace Formats::Chiptune
{
  class MetaBuilder;
}  // namespace Formats::Chiptune

namespace Formats::Chiptune::Id3
{
  bool Parse(Binary::DataInputStream& stream, MetaBuilder& target);
}  // namespace Formats::Chiptune::Id3
