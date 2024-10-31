/**
 *
 * @file
 *
 * @brief  Simple DAC-based tracks support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "devices/dac/sample.h"
#include "formats/chiptune/digital/digital.h"  // IWYU pragma: export
#include "module/players/dac/dac_chiptune.h"
#include "module/players/tracking.h"  // IWYU pragma: export

#include "parameters/accessor.h"

#include "types.h"

#include <memory>

namespace Devices::DAC
{
  class Chip;
}  // namespace Devices::DAC

namespace Module::DAC
{
  class PropertiesHelper;

  class SimpleModuleData : public TrackModel
  {
  public:
    using Ptr = std::shared_ptr<const SimpleModuleData>;
    using RWPtr = std::shared_ptr<SimpleModuleData>;

    explicit SimpleModuleData(uint_t channels)
      : ChannelsCount(channels)
    {}

    uint_t GetChannelsCount() const override
    {
      return ChannelsCount;
    }

    uint_t GetInitialTempo() const override
    {
      return InitialTempo;
    }

    const OrderList& GetOrder() const override
    {
      return *Order;
    }

    const PatternsSet& GetPatterns() const override
    {
      return *Patterns;
    }

    void SetupSamples(Devices::DAC::Chip& chip) const;

    const uint_t ChannelsCount;
    uint_t InitialTempo = 0;
    OrderList::Ptr Order;
    PatternsSet::Ptr Patterns;
    SparsedObjectsStorage<Devices::DAC::Sample::Ptr> Samples;
  };

  class SimpleDataBuilder : public Formats::Chiptune::Digital::Builder
  {
  public:
    using Ptr = std::unique_ptr<SimpleDataBuilder>;

    virtual SimpleModuleData::Ptr CaptureResult() = 0;

    static Ptr Create(DAC::PropertiesHelper& props, PatternsBuilder builder,
                      uint_t channels);  // TODO: rework external dependency from builder
  };

  template<uint_t Channels>
  static SimpleDataBuilder::Ptr CreateSimpleDataBuilder(DAC::PropertiesHelper& props)
  {
    return SimpleDataBuilder::Create(props, PatternsBuilder::Create<Channels>(), Channels);
  }

  DAC::Chiptune::Ptr CreateSimpleChiptune(SimpleModuleData::Ptr data, Parameters::Accessor::Ptr properties);
}  // namespace Module::DAC
