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
  enum EndCmd
  {
    // Synthetic command marking the end of a pattern; ignored by playback
    END_MARKER = 0x7f
  };

  class SimpleModuleData : public TrackModelData<OrderList>
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

    void SetupSamples(Devices::DAC::Chip& chip) const;

    const uint_t ChannelsCount;
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

  DAC::Chiptune::Ptr CreateSimpleChiptune(SimpleModuleData::Ptr data,
                                          Chiptune::PresentPatternFunction presentPattern = nullptr);

  template<class RendererType, class ModelPtr>
  class TrackingChiptune : public Chiptune
  {
    using ModelType = std::remove_const_t<typename ModelPtr::element_type>;

  public:
    explicit TrackingChiptune(ModelPtr model, PresentPatternFunction presentPattern = nullptr)
      : Model(std::move(model))
      , PresentPattern(presentPattern)
    {}

    TrackModel::Ptr GetTrackModel() const override
    {
      return Model;
    }

    DataIterator::Ptr CreateDataIterator() const override
    {
      return MakePtr<TrackIterator<ModelType, ModelTrackCursor, Synth<RendererType>>>(Model);
    }

    void GetSamples(Devices::DAC::Chip& chip) const override
    {
      Model->SetupSamples(chip);
    }

    PresentPatternFunction GetPresentPattern() const override
    {
      return PresentPattern;
    }

  private:
    const ModelPtr Model;
    const PresentPatternFunction PresentPattern;
  };

  template<class RendererType, class ModelPtr>
  Chiptune::Ptr CreateTrackingChiptune(ModelPtr model, Chiptune::PresentPatternFunction presentPattern = nullptr)
  {
    return MakePtr<TrackingChiptune<RendererType, ModelPtr>>(std::move(model), presentPattern);
  }
}  // namespace Module::DAC
