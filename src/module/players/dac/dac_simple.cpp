/**
* 
* @file
*
* @brief  Simple DAC-based tracks support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/dac/dac_simple.h"
//common includes
#include <make_ptr.h>
//library includes
#include <devices/dac/sample_factories.h>
#include <module/players/properties_meta.h>
#include <module/players/tracking.h>
#include <module/players/simple_orderlist.h>
//std includes
#include <utility>

namespace Module
{
  namespace DAC
  {
    class SimpleDataBuilderImpl : public SimpleDataBuilder
    {
    public:
      SimpleDataBuilderImpl(DAC::PropertiesHelper& props, PatternsBuilder builder)
        : Properties(props)
        , Meta(props)
        , Patterns(std::move(builder))
        , Data(MakeRWPtr<SimpleModuleData>())
      {
      }

      Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
      {
        return Meta;
      }

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
        Data->Samples.Add(index, is4Bit
          ? Devices::DAC::CreateU4Sample(content, loop)
          : Devices::DAC::CreateU8Sample(content, loop));
      }

      void SetPositions(Formats::Chiptune::Digital::Positions positions) override
      {
        Data->Order = MakePtr<SimpleOrderList>(positions.Loop, std::move(positions.Lines));
      }

      Formats::Chiptune::PatternBuilder& StartPattern(uint_t index) override
      {
        Patterns.SetPattern(index);
        return Patterns;
      }

      void StartChannel(uint_t index) override
      {
        Patterns.SetChannel(index);
      }

      void SetRest() override
      {
        Patterns.GetChannel().SetEnabled(false);
      }

      void SetNote(uint_t note) override
      {
        Patterns.GetChannel().SetEnabled(true);
        Patterns.GetChannel().SetNote(note);
      }

      void SetSample(uint_t sample) override
      {
        Patterns.GetChannel().SetSample(sample);
      }

      SimpleModuleData::Ptr CaptureResult() override
      {
        Data->Patterns = Patterns.CaptureResult();
        return std::move(Data);
      }
    private:
      DAC::PropertiesHelper& Properties;
      MetaProperties Meta;
      PatternsBuilder Patterns;
      SimpleModuleData::RWPtr Data;
    };

    SimpleDataBuilder::Ptr SimpleDataBuilder::Create(DAC::PropertiesHelper& props, PatternsBuilder builder)
    {
      return MakePtr<SimpleDataBuilderImpl>(props, std::move(builder));
    }

    class SimpleDataRenderer : public DAC::DataRenderer
    {
    public:
      SimpleDataRenderer(SimpleModuleData::Ptr data, uint_t channels)
        : Data(std::move(data))
        , Channels(channels)
      {
      }

      void Reset() override
      {
      }

      void SynthesizeData(const TrackModelState& state, DAC::TrackBuilder& track) override
      {
        if (0 == state.Quirk())
        {
          GetNewLineState(state, track);
        }
      }
    private:
      void GetNewLineState(const TrackModelState& state, DAC::TrackBuilder& track)
      {
        if (const auto line = state.LineObject())
        {
          for (uint_t chan = 0; chan != Channels; ++chan)
          {
            if (const auto src = line->GetChannel(chan))
            {
              ChannelDataBuilder builder = track.GetChannel(chan);
              GetNewChannelState(*src, builder);
            }
          }
        }
      }

      void GetNewChannelState(const Cell& src, ChannelDataBuilder& builder)
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
      const SimpleModuleData::Ptr Data;
      const uint_t Channels;
    };

    class SimpleChiptune : public DAC::Chiptune
    {
    public:
      SimpleChiptune(SimpleModuleData::Ptr data, Parameters::Accessor::Ptr properties, uint_t channels)
        : Data(std::move(data))
        , Properties(std::move(properties))
        , Info(CreateTrackInfo(Data, channels))
        , Channels(channels)
      {
      }

      TrackInformation::Ptr GetInformation() const override
      {
        return Info;
      }

      Parameters::Accessor::Ptr GetProperties() const override
      {
        return Properties;
      }

      DAC::DataIterator::Ptr CreateDataIterator() const override
      {
        auto iterator = CreateTrackStateIterator(Data);
        auto renderer = MakePtr<SimpleDataRenderer>(Data, Channels);
        return DAC::CreateDataIterator(std::move(iterator), std::move(renderer));
      }

      void GetSamples(Devices::DAC::Chip::Ptr chip) const override
      {
        for (uint_t idx = 0, lim = Data->Samples.Size(); idx != lim; ++idx)
        {
          chip->SetSample(idx, Data->Samples.Get(idx));
        }
      }
    private:
      const SimpleModuleData::Ptr Data;
      const Parameters::Accessor::Ptr Properties;
      const TrackInformation::Ptr Info;
      const uint_t Channels;
    };

    DAC::Chiptune::Ptr CreateSimpleChiptune(SimpleModuleData::Ptr data, Parameters::Accessor::Ptr properties, uint_t channels)
    {
      return MakePtr<SimpleChiptune>(std::move(data), std::move(properties), channels);
    }
  }
}
