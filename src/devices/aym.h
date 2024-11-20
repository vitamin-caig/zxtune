/**
 *
 * @file
 *
 * @brief  AY/YM support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "time/instant.h"

#include "types.h"

#include <array>
#include <memory>
#include <vector>

// supporting for AY/YM-based modules
namespace Devices::AYM
{
  const uint_t VOICES = 5;

  using TimeUnit = Time::Microsecond;
  using Stamp = Time::Instant<TimeUnit>;

  class Registers
  {
  public:
    // registers offsets in data
    enum Index
    {
      TONEA_L,
      TONEA_H,
      TONEB_L,
      TONEB_H,
      TONEC_L,
      TONEC_H,
      TONEN,
      MIXER,
      VOLA,
      VOLB,
      VOLC,
      TONEE_L,
      TONEE_H,
      ENV,
      // limiter
      TOTAL
    };

    // masks
    enum
    {
      // bits in REG_VOL*
      MASK_VOL = 0x0f,
      MASK_ENV = 0x10,
      // bits in REG_MIXER
      MASK_TONEA = 0x01,
      MASK_NOISEA = 0x08,
      MASK_TONEB = 0x02,
      MASK_NOISEB = 0x10,
      MASK_TONEC = 0x04,
      MASK_NOISEC = 0x20,
    };

    Registers()
      : Data()
    {}

    bool Empty() const
    {
      return 0 == Mask;
    }

    bool Has(Index reg) const
    {
      return 0 != (Mask & (1 << reg));
    }

    uint8_t& operator[](Index reg)
    {
      Mask |= 1 << reg;
      return Data[reg];
    }

    void Reset(Index reg)
    {
      Mask &= ~(1 << reg);
    }

    uint8_t operator[](Index reg) const
    {
      return Data[reg];
    }

    class IndicesIterator
    {
    public:
      explicit IndicesIterator(const Registers& regs)
        : Mask(regs.Mask & ((1 << TOTAL) - 1))
      {
        SkipUnset();
      }

      using BoolType = bool (IndicesIterator::*)() const;

      operator BoolType() const
      {
        return IsValid() ? &IndicesIterator::IsValid : nullptr;
      }

      Index operator*() const
      {
        return static_cast<Index>(Idx);
      }

      void operator++()
      {
        Next();
        SkipUnset();
      }

    private:
      bool IsValid() const
      {
        return Mask != 0;
      }

      bool HasValue() const
      {
        return 0 != (Mask & 1);
      }

      void Next()
      {
        ++Idx;
        Mask >>= 1;
      }

      void SkipUnset()
      {
        while (IsValid() && !HasValue())
        {
          Next();
        }
      }

    private:
      uint_t Mask;
      uint_t Idx = 0;
    };

  private:
    uint16_t Mask = 0;
    std::array<uint8_t, TOTAL> Data;
  };

  struct DataChunk
  {
    DataChunk() = default;

    Stamp TimeStamp;
    Registers Data;
  };

  class Device
  {
  public:
    using Ptr = std::unique_ptr<Device>;
    virtual ~Device() = default;

    /// Render single data chunk
    virtual void RenderData(const DataChunk& src) = 0;

    /// Render multiple data chunks
    virtual void RenderData(const std::vector<DataChunk>& src) = 0;

    /// reset internal state to initial
    virtual void Reset() = 0;
  };
}  // namespace Devices::AYM
