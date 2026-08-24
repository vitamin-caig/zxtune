/**
 *
 * @file
 *
 * @brief  Dump type definition
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "types.h"

#include <vector>

namespace Binary
{
  //! @brief Plain data type
  // Make in uncopyable
  class Dump : public std::vector<uint8_t>
  {
    using Base = std::vector<uint8_t>;

  public:
    using Base::Base;
    Dump(const Base&) = delete;
    Dump(Dump&& t)
      : Base(std::move(t))
    {}

    Dump& operator=(const Base&) = delete;
    Dump& operator=(Dump&& t)
    {
      Base::operator=(std::move(t));
      return *this;
    }
  };
}  // namespace Binary
