/**
 *
 * @file
 *
 * @brief  Analyzer interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <math/fixedpoint.h>
// std includes
#include <memory>

namespace Module
{
  //! @brief %Sound analyzer interface
  class Analyzer
  {
  public:
    //! Pointer type
    typedef std::shared_ptr<const Analyzer> Ptr;

    virtual ~Analyzer() = default;

    using LevelType = Math::FixedPoint<uint8_t, 100>;

    struct SpectrumState
    {
      SpectrumState(){};
      SpectrumState(const SpectrumState&) = delete;
      SpectrumState& operator=(const SpectrumState&) = delete;
      SpectrumState(SpectrumState&& rh) noexcept  // = default;
        : Data(std::move(rh.Data))
      {}

      using DataType = std::array<LevelType, 96>;

      SpectrumState(DataType&& rh) noexcept
        : Data(std::move(rh))
      {}

      void Set(uint_t band, LevelType val)
      {
        if (band < Data.size())
        {
          Data[band] = AccumulateLevel(Data[band], val);
        }
      }

      static LevelType AccumulateLevel(LevelType lh, LevelType rh)
      {
        return std::min(lh + rh, LevelType(1));
      }

      DataType Data;
    };

    virtual SpectrumState GetState() const = 0;
  };
}  // namespace Module
