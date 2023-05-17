/**
 *
 * @file
 *
 * @brief  Beeper support
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
// std includes
#include <memory>

namespace Devices::Beeper
{
  using TimeUnit = Time::Microsecond;
  using Stamp = Time::Instant<TimeUnit>;

  struct DataChunk
  {
    DataChunk(Stamp stamp, bool level)
      : TimeStamp(stamp)
      , Level(level)
    {}

    Stamp TimeStamp;
    bool Level = false;
  };

  class Device
  {
  public:
    using Ptr = std::shared_ptr<Device>;
    virtual ~Device() = default;

    /// Render multiple data chunks
    virtual void RenderData(const std::vector<DataChunk>& src) = 0;

    /// reset internal state to initial
    virtual void Reset() = 0;
  };

  // TODO: add StateSource if required
  class Chip : public Device
  {
  public:
    using Ptr = std::shared_ptr<Chip>;

    virtual Sound::Chunk RenderTill(Stamp till) = 0;
  };

  class ChipParameters
  {
  public:
    using Ptr = std::shared_ptr<const ChipParameters>;

    virtual ~ChipParameters() = default;

    virtual uint_t Version() const = 0;
    virtual uint64_t ClockFreq() const = 0;
    virtual uint_t SoundFreq() const = 0;
  };

  /// Virtual constructors
  Chip::Ptr CreateChip(ChipParameters::Ptr params);
}  // namespace Devices::Beeper
