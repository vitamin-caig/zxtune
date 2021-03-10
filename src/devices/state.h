/**
 *
 * @file
 *
 * @brief  Device state source interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <math/fixedpoint.h>
// std includes
#include <array>
#include <memory>

namespace Devices
{
  using LevelType = Math::FixedPoint<uint8_t, 100>;

  struct DeviceState : std::array<LevelType, 96>
  {
    void Set(uint_t band, LevelType level)
    {
      if (band < size())
      {
        auto& val = (*this)[band];
        val = std::max(val, level);
      }
    }
  };

  class StateSource
  {
  public:
    using Ptr = std::shared_ptr<const StateSource>;
    virtual ~StateSource() = default;

    virtual DeviceState GetState() const = 0;
  };
}  // namespace Devices
