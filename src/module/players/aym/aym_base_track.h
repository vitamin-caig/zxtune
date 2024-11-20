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

#include "module/renderer.h"

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

  class DataRenderer
  {
  public:
    using Ptr = std::unique_ptr<DataRenderer>;

    virtual ~DataRenderer() = default;

    virtual void SynthesizeData(const TrackModelState& state, TrackBuilder& track) = 0;
    virtual void Reset() = 0;
  };

  DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams, TrackStateIterator::Ptr iterator,
                                       DataRenderer::Ptr renderer);

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
      return *Patterns;
    }

    uint_t InitialTempo = 0;
    typename OrderListType::Ptr Order;
    PatternsSet::Ptr Patterns;
    SparsedObjectsStorage<SampleType> Samples;
    SparsedObjectsStorage<OrnamentType> Ornaments;
  };

  template<class ModuleData, class DataRenderer>
  class TrackingChiptune : public Chiptune
  {
  public:
    TrackingChiptune(typename ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(std::move(data))
      , Properties(std::move(properties))
    {}

    Time::Microseconds GetFrameDuration() const override
    {
      return BASE_FRAME_DURATION;
    }

    TrackModel::Ptr FindTrackModel() const override
    {
      return Data;
    }

    Module::StreamModel::Ptr FindStreamModel() const override
    {
      return {};
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      return Properties;
    }

    AYM::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams) const override
    {
      auto iterator = CreateTrackStateIterator(GetFrameDuration(), Data);
      auto renderer = MakePtr<DataRenderer>(Data);
      return AYM::CreateDataIterator(std::move(trackParams), std::move(iterator), std::move(renderer));
    }

  private:
    const typename ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
  };
}  // namespace Module::AYM
