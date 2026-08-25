/**
 *
 * @file
 *
 * @brief  Chiptune container helper
 *
 * @author vitamin.caig@gmail.com
 *
 **/
#pragma once

#include "formats/chiptune/container.h"

namespace Formats::Chiptune
{
  Container::Ptr CreateKnownCrcContainer(Binary::Container::Ptr data, uint_t crc);
  Container::Ptr CreateCalculatingCrcContainer(Binary::Container::Ptr data, std::size_t offset, std::size_t size);
  Container::Ptr CreateCalculatingCrcContainer(const Binary::Container& data,
                                               std::size_t crcCalculatingLimit = 10485760);
}  // namespace Formats::Chiptune
