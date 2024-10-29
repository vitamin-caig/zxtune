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
#include <types.h>
// library includes
#include <sound/chunk.h>
#include <time/instant.h>
#include <tools/data_streaming.h>
// std includes
#include <array>
#include <memory>

namespace Devices::FM
{
  const uint_t VOICES = 3;

  using TimeUnit = Time::Microsecond;
  using Stamp = Time::Instant<TimeUnit>;

  class Register
  {
  public:
    Register() = default;

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
    uint_t Val = 0;
  };

  using Registers = std::vector<Register>;

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

    /// render single data chunk
    virtual void RenderData(const DataChunk& src) = 0;

    /// reset internal state to initial
    virtual void Reset() = 0;
  };

  // Describes real device
  class Chip : public Device
  {
  public:
    using Ptr = std::unique_ptr<Chip>;

    /// Render rest data and return result
    virtual Sound::Chunk RenderTill(Stamp stamp) = 0;
  };

  class ChipParameters
  {
  public:
    using Ptr = std::unique_ptr<const ChipParameters>;

    virtual ~ChipParameters() = default;

    virtual uint_t Version() const = 0;
    virtual uint64_t ClockFreq() const = 0;
    virtual uint_t SoundFreq() const = 0;
  };

  /// Virtual constructors
  Chip::Ptr CreateChip(ChipParameters::Ptr params);
}  // namespace Devices::FM
