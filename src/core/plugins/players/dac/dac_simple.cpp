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
#include "dac_simple.h"
#include "core/plugins/players/properties_meta.h"
#include "core/plugins/players/tracking.h"
#include "core/plugins/players/simple_orderlist.h"
//common includes
#include <make_ptr.h>
//library includes
#include <devices/dac/sample_factories.h>
//boost includes
#include <boost/ref.hpp>

namespace Module
{
  namespace DAC
  {
    class SimpleDataBuilderImpl : public SimpleDataBuilder
    {
    public:
      SimpleDataBuilderImpl(DAC::PropertiesHelper& props, const PatternsBuilder& builder)
        : Data(MakeRWPtr<SimpleModuleData>())
        , Properties(props)
        , Meta(props)
        , Patterns(builder)
      {
        Data->Patterns = Patterns.GetResult();
      }

      virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
      {
        return Meta;
      }

      virtual void SetInitialTempo(uint_t tempo)
      {
        Data->InitialTempo = tempo;
      }

      virtual void SetSamplesFrequency(uint_t freq)
      {
        Properties.SetSamplesFrequency(freq);
      }

      virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr content, bool is4Bit)
      {
        Data->Samples.Add(index, is4Bit
          ? Devices::DAC::CreateU4Sample(content, loop)
          : Devices::DAC::CreateU8Sample(content, loop));
      }

      virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
      {
        Data->Order = MakePtr<SimpleOrderList>(loop, positions.begin(), positions.end());
      }

      virtual Formats::Chiptune::PatternBuilder& StartPattern(uint_t index)
      {
        Patterns.SetPattern(index);
        return Patterns;
      }

      virtual void StartChannel(uint_t index)
      {
        Patterns.SetChannel(index);
      }

      virtual void SetRest()
      {
        Patterns.GetChannel().SetEnabled(false);
      }

      virtual void SetNote(uint_t note)
      {
        Patterns.GetChannel().SetEnabled(true);
        Patterns.GetChannel().SetNote(note);
      }

      virtual void SetSample(uint_t sample)
      {
        Patterns.GetChannel().SetSample(sample);
      }

      virtual SimpleModuleData::Ptr GetResult() const
      {
        return Data;
      }
    private:
      const SimpleModuleData::RWPtr Data;
      DAC::PropertiesHelper& Properties;
      MetaProperties Meta;
      PatternsBuilder Patterns;
    };

    SimpleDataBuilder::Ptr SimpleDataBuilder::Create(DAC::PropertiesHelper& props, const PatternsBuilder& builder)
    {
      return MakePtr<SimpleDataBuilderImpl>(boost::ref(props), builder);
    }

    class SimpleDataRenderer : public DAC::DataRenderer
    {
    public:
      SimpleDataRenderer(SimpleModuleData::Ptr data, uint_t channels)
        : Data(data)
        , Channels(channels)
      {
      }

      virtual void Reset()
      {
      }

      virtual void SynthesizeData(const TrackModelState& state, DAC::TrackBuilder& track)
      {
        if (0 == state.Quirk())
        {
          GetNewLineState(state, track);
        }
      }
    private:
      void GetNewLineState(const TrackModelState& state, DAC::TrackBuilder& track)
      {
        if (const Line::Ptr line = state.LineObject())
        {
          for (uint_t chan = 0; chan != Channels; ++chan)
          {
            if (const Cell::Ptr src = line->GetChannel(chan))
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
        : Data(data)
        , Properties(properties)
        , Info(CreateTrackInfo(Data, channels))
        , Channels(channels)
      {
      }

      virtual Information::Ptr GetInformation() const
      {
        return Info;
      }

      virtual Parameters::Accessor::Ptr GetProperties() const
      {
        return Properties;
      }

      virtual DAC::DataIterator::Ptr CreateDataIterator() const
      {
        const TrackStateIterator::Ptr iterator = CreateTrackStateIterator(Data);
        const DAC::DataRenderer::Ptr renderer = MakePtr<SimpleDataRenderer>(Data, Channels);
        return DAC::CreateDataIterator(iterator, renderer);
      }

      virtual void GetSamples(Devices::DAC::Chip::Ptr chip) const
      {
        for (uint_t idx = 0, lim = Data->Samples.Size(); idx != lim; ++idx)
        {
          chip->SetSample(idx, Data->Samples.Get(idx));
        }
      }
    private:
      const SimpleModuleData::Ptr Data;
      const Parameters::Accessor::Ptr Properties;
      const Information::Ptr Info;
      const uint_t Channels;
    };

    DAC::Chiptune::Ptr CreateSimpleChiptune(SimpleModuleData::Ptr data, Parameters::Accessor::Ptr properties, uint_t channels)
    {
      return MakePtr<SimpleChiptune>(data, properties, channels);
    }
  }
}
