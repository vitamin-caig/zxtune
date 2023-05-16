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

// local includes
#include "module/players/dac/dac_base.h"
#include "module/players/dac/dac_properties_helper.h"
// library includes
#include <formats/chiptune/digital/digital.h>

namespace Module::DAC
{
  class SimpleModuleData : public TrackModel
  {
  public:
    typedef std::shared_ptr<const SimpleModuleData> Ptr;
    typedef std::shared_ptr<SimpleModuleData> RWPtr;

    explicit SimpleModuleData(uint_t channels)
      : ChannelsCount(channels)
      , InitialTempo()
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

    const uint_t ChannelsCount;
    uint_t InitialTempo;
    OrderList::Ptr Order;
    PatternsSet::Ptr Patterns;
    SparsedObjectsStorage<Devices::DAC::Sample::Ptr> Samples;
  };

  class SimpleDataBuilder : public Formats::Chiptune::Digital::Builder
  {
  public:
    typedef std::unique_ptr<SimpleDataBuilder> Ptr;

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
