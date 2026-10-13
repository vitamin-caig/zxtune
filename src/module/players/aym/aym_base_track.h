/**
 *
 * @file
 *
 * @brief  AYM-based track chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/aym/aym_chiptune.h"
#include "module/players/tracking.h"

#include "parameters/tracking_helper.h"

namespace Module::AYM
{
  const uint_t TRACK_CHANNELS = 3;

  class ChannelBuilder
  {
  public:
    ChannelBuilder(uint_t chan, const FrequencyTable& table, Devices::AYM::Registers& data)
      : Channel(chan)
      , Table(table)
      , Data(data)
    {}

    void SetTone(int_t halfTones, int_t offset);
    void SetTone(uint_t tone);
    void SetLevel(int_t level);
    void DisableTone();
    void EnableEnvelope();
    void DisableNoise();

  private:
    const uint_t Channel;
    const FrequencyTable& Table;
    Devices::AYM::Registers& Data;
  };

  class TrackBuilder
  {
  public:
    explicit TrackBuilder(const FrequencyTable& table)
      : Table(table)
    {
      Data[Devices::AYM::Registers::MIXER] = 0;
    }

    void SetNoise(uint_t level);
    void SetEnvelopeType(uint_t type);
    void SetEnvelopeTone(uint_t tone);

    uint_t GetFrequency(int_t halfTone) const;
    int_t GetSlidingDifference(int_t halfToneFrom, int_t halfToneTo) const;

    ChannelBuilder GetChannel(uint_t chan)
    {
      return {chan, Table, Data};
    }

    const Devices::AYM::Registers& GetResult() const
    {
      return Data;
    }

  private:
    const FrequencyTable& Table;
    Devices::AYM::Registers Data;
  };

  template<class RendererType>
  class Synth
  {
  public:
    using Data = Devices::AYM::Registers;

    template<class ModelType>
    Synth(const ModelType& model, AYM::TrackParameters::Ptr params)
      : Renderer(model)
      , Params(std::move(params))
    {}

    void Reset()
    {
      Renderer.Reset();
      Params.Reset();
    }

    Devices::AYM::Registers Synthesize(const TrackState& state)
    {
      if (Params.IsChanged())
      {
        Params->FreqTable(Table);
      }
      AYM::TrackBuilder builder(Table);
      Renderer.SynthesizeData(state, builder);
      return builder.GetResult();
    }

  private:
    RendererType Renderer;
    Parameters::TrackingHelper<AYM::TrackParameters> Params;
    FrequencyTable Table;
  };

  template<class OrderListType, class SampleType, class OrnamentType>
  class ModuleData : public TrackModel
  {
  public:
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;

    ModuleData() = default;

    uint_t GetChannelsCount() const override
    {
      return TRACK_CHANNELS;
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

    uint_t InitialTempo = 0;
    typename OrderListType::Ptr Order;
    PatternsSet Patterns;
    SparsedObjectsStorage<SampleType> Samples;
    SparsedObjectsStorage<OrnamentType> Ornaments;
  };

  template<class RendererType, class ModelPtr>
  class TrackingChiptune : public Chiptune
  {
    using ModelType = std::remove_const_t<typename ModelPtr::element_type>;

  public:
    explicit TrackingChiptune(ModelPtr model)
      : Model(std::move(model))
    {}

    Time::Microseconds GetFrameDuration() const override
    {
      return BASE_FRAME_DURATION;
    }

    FramedInformation GetInformation() const override
    {
      return CreateTrackInfo(*Model);
    }

    DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams) const override
    {
      return MakePtr<TrackIterator<ModelType, ModelTrackCursor, Synth<RendererType>>>(Model, std::move(trackParams));
    }

  private:
    const ModelPtr Model;
  };

  template<class RendererType, class ModelPtr>
  Chiptune::Ptr CreateTrackingChiptune(ModelPtr model)
  {
    return MakePtr<TrackingChiptune<RendererType, ModelPtr>>(std::move(model));
  }
}  // namespace Module::AYM
