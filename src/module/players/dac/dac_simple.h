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

#include "formats/chiptune/digital/digital.h"
#include "module/players/dac/dac_base.h"
#include "module/players/dac/dac_properties_helper.h"

#include "make_ptr.h"

namespace Module::DAC
{
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
      return Patterns;
    }

    void SetupSamples(Devices::DAC::Chip& chip) const;

    const uint_t ChannelsCount;
    uint_t InitialTempo = 0;
    OrderList::Ptr Order;
    PatternsSet Patterns;
    SparsedObjectsStorage<Devices::DAC::Sample::Ptr> Samples;
  };

  class SimpleDataBuilder : public Formats::Chiptune::Digital::Builder
  {
  public:
    using Ptr = std::unique_ptr<SimpleDataBuilder>;

    virtual SimpleModuleData::Ptr CaptureResult() = 0;

    static Ptr Create(DAC::PropertiesHelper& props,
                      uint_t channels);  // TODO: rework external dependency from builder
  };

  template<uint_t Channels>
  static SimpleDataBuilder::Ptr CreateSimpleDataBuilder(DAC::PropertiesHelper& props)
  {
    return SimpleDataBuilder::Create(props, Channels);
  }

  DAC::Chiptune::Ptr CreateSimpleChiptune(SimpleModuleData::Ptr data, Parameters::Accessor::Ptr properties);

  template<class DataPtr, class DataRenderer>
  class TrackingChiptune : public Chiptune
  {
  public:
    TrackingChiptune(DataPtr data, Parameters::Accessor::Ptr properties)
      : Data(std::move(data))
      , Properties(std::move(properties))
    {}

    TrackModel::Ptr GetTrackModel() const override
    {
      return Data;
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      return Properties;
    }

    DataIterator::Ptr CreateDataIterator() const override
    {
      auto iterator = CreateTrackStateIterator(GetFrameDuration(), Data);
      auto renderer = MakePtr<DataRenderer>(Data);
      return DAC::CreateDataIterator(std::move(iterator), std::move(renderer));
    }

    void GetSamples(Devices::DAC::Chip& chip) const override
    {
      Data->SetupSamples(chip);
    }

  private:
    const DataPtr Data;
    const Parameters::Accessor::Ptr Properties;
  };

  template<class DataRenderer, class DataPtr>
  Chiptune::Ptr CreateTrackingChiptune(DataPtr data, Parameters::Accessor::Ptr properties)
  {
    return MakePtr<TrackingChiptune<DataPtr, DataRenderer>>(std::move(data), std::move(properties));
  }
}  // namespace Module::DAC
