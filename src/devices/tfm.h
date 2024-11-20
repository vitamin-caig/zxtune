/**
 *
 * @file
 *
 * @brief  TurboFM support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "devices/fm.h"
#include "sound/chunk.h"
#include "time/instant.h"

#include "types.h"

#include <memory>
#include <vector>

namespace Devices::TFM
{
  const uint_t CHIPS = 2;
  const uint_t VOICES = FM::VOICES * CHIPS;

  using Devices::FM::TimeUnit;
  using Devices::FM::Stamp;

  class Register : public FM::Register
  {
  public:
    Register() = default;

    Register(uint_t chip, FM::Register reg)
      : FM::Register(reg)
    {
      Val |= chip << 16;
    }

    Register(uint_t chip, uint_t idx, uint_t val)
      : FM::Register(idx, val)
    {
      Val |= chip << 16;
    }

    uint_t Chip() const
    {
      return (Val >> 16) & 0xff;
    }
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

    virtual void RenderData(const DataChunk& src) = 0;
    virtual void Reset() = 0;
  };

  class Chip : public Device
  {
  public:
    using Ptr = std::unique_ptr<Chip>;

    /// Render rest data and return result
    virtual Sound::Chunk RenderTill(Stamp stamp) = 0;
  };

  using FM::ChipParameters;
  Chip::Ptr CreateChip(ChipParameters::Ptr params);
}  // namespace Devices::TFM
