/**
 *
 * @file
 *
 * @brief  Simple DAC-based tracks support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/dac/dac_simple.h"

#include "devices/dac/sample_factories.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"
#include "module/players/track_builder.h"
#include "module/players/tracking.h"

#include "make_ptr.h"

#include <utility>

namespace Module::DAC
{
  void SimpleModuleData::SetupSamples(Devices::DAC::Chip& chip) const
  {
    for (uint_t idx = 0, lim = Samples.Size(); idx != lim; ++idx)
    {
      if (const auto& smp = Samples.Get(idx))
      {
        chip.SetSample(idx, *smp);
      }
    }
  }

  class DataChannelBuilder : public TrackChannelBuilder<Formats::Chiptune::Digital::ChannelBuilder>
  {};

  using LineBuilder = TrackLineBuilder<Formats::Chiptune::Digital::ChannelBuilder, DataChannelBuilder>;
  using PatternsBuilder = TrackPatternsBuilder<Formats::Chiptune::Digital::ChannelBuilder, DataChannelBuilder>;

  class SimpleDataBuilderImpl : public TrackDataBuilder<SimpleDataBuilder, SimpleModuleData, PatternsBuilder>
  {
  public:
    SimpleDataBuilderImpl(DAC::PropertiesHelper& props, uint_t channels)
      : TrackDataBuilder(props, MakeRWPtr<SimpleModuleData>(channels))
      , Properties(props)
    {}

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSamplesFrequency(uint_t freq) override
    {
      Properties.SetSamplesFrequency(freq);
    }

    void SetSample(uint_t index, std::size_t loop, Binary::View content, bool is4Bit) override
    {
      Data->Samples.Add(index, is4Bit ? Devices::DAC::CreateU4Sample(content, loop)
                                      : Devices::DAC::CreateU8Sample(content, loop));
    }

    void SetPositions(Formats::Chiptune::Digital::Positions positions) override
    {
      Data->Order = MakePtr<SimpleOrderList>(positions.Loop, std::move(positions.Lines));
    }

  private:
    DAC::PropertiesHelper& Properties;
  };

  SimpleDataBuilder::Ptr SimpleDataBuilder::Create(DAC::PropertiesHelper& props, uint_t channels)
  {
    return MakePtr<SimpleDataBuilderImpl>(props, channels);
  }

  class SimpleDataRenderer
  {
  public:
    explicit SimpleDataRenderer(const SimpleModuleData& data)
      : Data(data)
    {}

    void Reset() {}

    void SynthesizeData(const TrackState& state, DAC::TrackBuilder& track)
    {
      if (0 == state.Quirk)
      {
        GetNewLineState(state, track);
      }
    }

  private:
    void GetNewLineState(const TrackState& state, DAC::TrackBuilder& track)
    {
      if (const auto* const line = Data.GetLine(state))
      {
        line->ForEachChannel([&](auto chan, const auto& src) { GetNewChannelState(src, track.GetChannel(chan)); });
      }
    }

    static void GetNewChannelState(const Cell& src, ChannelDataBuilder builder)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        builder.SetEnabled(*enabled);
        if (!*enabled)
        {
          builder.SetPosInSample(0);
        }
      }
      if (const uint_t* note = src.GetNote())
      {
        builder.SetNote(*note);
        builder.SetPosInSample(0);
      }
      if (const uint_t* sample = src.GetSample())
      {
        builder.SetSampleNum(*sample);
        builder.SetPosInSample(0);
      }
    }

  private:
    const SimpleModuleData& Data;
  };

  class SimpleChiptune : public DAC::Chiptune
  {
  public:
    explicit SimpleChiptune(SimpleModuleData::Ptr data)
      : Data(std::move(data))
    {}

    TrackModel::Ptr GetTrackModel() const override
    {
      return Data;
    }

    DataIterator::Ptr CreateDataIterator() const override
    {
      return MakePtr<TrackIterator<SimpleModuleData, ModelTrackCursor, Synth<SimpleDataRenderer>>>(Data);
    }

    void GetSamples(Devices::DAC::Chip& chip) const override
    {
      Data->SetupSamples(chip);
    }

  private:
    const SimpleModuleData::Ptr Data;
  };

  DAC::Chiptune::Ptr CreateSimpleChiptune(SimpleModuleData::Ptr data)
  {
    return MakePtr<SimpleChiptune>(std::move(data));
  }
}  // namespace Module::DAC
