/**
 *
 * @file
 *
 * @brief  Multitrack players factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <formats/multitrack.h>
#include <module/players/factory.h>

namespace Module
{
  class MultitrackFactory : public BaseFactory<Formats::Multitrack::Container>
  {
  public:
    // Need to be shared
    using Ptr = std::shared_ptr<const MultitrackFactory>;
  };
}  // namespace Module
