/**
 *
 * @file
 *
 * @brief  Chiptune container helper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  Container::Ptr CreateKnownCrcContainer(Binary::Container::Ptr data, uint_t crc);
  Container::Ptr CreateCalculatingCrcContainer(Binary::Container::Ptr data, std::size_t offset, std::size_t size);
}  // namespace Formats::Chiptune
