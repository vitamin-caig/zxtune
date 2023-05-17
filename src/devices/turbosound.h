/**
 *
 * @file
 *
 * @brief  TurboSound support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <devices/aym/chip.h>

namespace Devices::TurboSound
{
  const uint_t CHIPS = 2;
  const uint_t VOICES = AYM::VOICES * CHIPS;

  using AYM::TimeUnit;
  using AYM::Stamp;

  using Registers = std::array<AYM::Registers, CHIPS>;

  struct DataChunk
  {
    DataChunk()
      : TimeStamp()
      , Data()
    {}

    Stamp TimeStamp;
    Registers Data;
  };

  class Device
  {
  public:
    using Ptr = std::shared_ptr<Device>;
    virtual ~Device() = default;

    virtual void RenderData(const DataChunk& src) = 0;
    virtual void RenderData(const std::vector<DataChunk>& src) = 0;
    virtual void Reset() = 0;
  };

  class Chip : public Device
  {
  public:
    using Ptr = std::shared_ptr<Chip>;

    virtual Sound::Chunk RenderTill(Stamp till) = 0;
  };

  using AYM::ChipParameters;
  using AYM::MixerType;

  Chip::Ptr CreateChip(ChipParameters::Ptr params, MixerType::Ptr mixer);
}  // namespace Devices::TurboSound
