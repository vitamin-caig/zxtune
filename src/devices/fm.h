/**
 *
 * @file
 *
 * @brief  FM support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <data_streaming.h>
#include <types.h>
// library includes
#include <sound/chunk.h>
#include <time/instant.h>
// std includes
#include <array>

namespace Devices::FM
{
  const uint_t VOICES = 3;

  using TimeUnit = Time::Microsecond;
  using Stamp = Time::Instant<TimeUnit>;

  class Register
  {
  public:
    Register()
      : Val()
    {}

    Register(uint_t idx, uint_t val)
      : Val((idx << 8) | val)
    {}

    uint_t Index() const
    {
      return (Val >> 8) & 0xff;
    }

    uint_t Value() const
    {
      return Val & 0xff;
    }

  protected:
    uint_t Val;
  };

  typedef std::vector<Register> Registers;

  struct DataChunk
  {
    DataChunk()
      : TimeStamp()
    {}

    Stamp TimeStamp;
    Registers Data;
  };

  class Device
  {
  public:
    typedef std::shared_ptr<Device> Ptr;
    virtual ~Device() = default;

    /// render single data chunk
    virtual void RenderData(const DataChunk& src) = 0;

    /// reset internal state to initial
    virtual void Reset() = 0;
  };

  // Describes real device
  class Chip : public Device
  {
  public:
    using Ptr = std::shared_ptr<Chip>;

    /// Render rest data and return result
    virtual Sound::Chunk RenderTill(Stamp stamp) = 0;
  };

  class ChipParameters
  {
  public:
    typedef std::shared_ptr<const ChipParameters> Ptr;

    virtual ~ChipParameters() = default;

    virtual uint_t Version() const = 0;
    virtual uint64_t ClockFreq() const = 0;
    virtual uint_t SoundFreq() const = 0;
  };

  /// Virtual constructors
  Chip::Ptr CreateChip(ChipParameters::Ptr params);
}  // namespace Devices::FM
