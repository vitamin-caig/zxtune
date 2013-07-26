/*
Abstract:
  Simple digital tracks playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_DAC_DIGITAL_DEFINED
#define CORE_PLUGINS_PLAYERS_DAC_DIGITAL_DEFINED

//local includes
#include "dac_base.h"
#include "core/plugins/players/tracking.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/simple_orderlist.h"
//library includes
#include <devices/dac/sample_factories.h>
#include <formats/chiptune/digital/digital.h>

namespace Module
{
  namespace DAC
  {
    class ModuleData : public TrackModel
    {
    public:
      typedef boost::shared_ptr<const ModuleData> Ptr;

      ModuleData()
        : InitialTempo()
      {
      }

      virtual uint_t GetInitialTempo() const
      {
        return InitialTempo;
      }

      virtual const OrderList& GetOrder() const
      {
        return *Order;
      }

      virtual const PatternsSet& GetPatterns() const
      {
        return *Patterns;
      }

      uint_t InitialTempo;
      OrderList::Ptr Order;
      PatternsSet::Ptr Patterns;
      SparsedObjectsStorage<Devices::DAC::Sample::Ptr> Samples;
    };

    class DataBuilder : public Formats::Chiptune::Digital::Builder
    {
      DataBuilder(PropertiesBuilder& props, const PatternsBuilder& builder)
        : Data(boost::make_shared<ModuleData>())
        , Properties(props)
        , Patterns(builder)
      {
        Data->Patterns = Patterns.GetResult();
      }
    public:
      template<uint_t Channels>
      static std::auto_ptr<DataBuilder> Create(PropertiesBuilder& props)
      {
        return std::auto_ptr<DataBuilder>(new DataBuilder(props, PatternsBuilder::Create<Channels>()));
      }

      virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
      {
        return Properties;
      }

      virtual void SetInitialTempo(uint_t tempo)
      {
        Data->InitialTempo = tempo;
      }

      virtual void SetSamplesFrequency(uint_t freq)
      {
        Properties.SetSamplesFreq(freq);
      }

      virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr content, bool is4Bit)
      {
        Data->Samples.Add(index, is4Bit
          ? Devices::DAC::CreateU4Sample(content, loop)
          : Devices::DAC::CreateU8Sample(content, loop));
      }

      virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
      {
        Data->Order = boost::make_shared<SimpleOrderList>(loop, positions.begin(), positions.end());
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

      ModuleData::Ptr GetResult() const
      {
        return Data;
      }
    private:
      const boost::shared_ptr<ModuleData> Data;
      PropertiesBuilder& Properties;
      PatternsBuilder Patterns;
    };

    class SimpleDataRenderer : public DAC::DataRenderer
    {
    public:
      SimpleDataRenderer(ModuleData::Ptr data, uint_t channels)
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
      const ModuleData::Ptr Data;
      const uint_t Channels;
    };

    class SimpleChiptune : public DAC::Chiptune
    {
    public:
      SimpleChiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties, uint_t channels)
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
        const DAC::DataRenderer::Ptr renderer = boost::make_shared<SimpleDataRenderer>(Data, Channels);
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
      const ModuleData::Ptr Data;
      const Parameters::Accessor::Ptr Properties;
      const Information::Ptr Info;
      const uint_t Channels;
    };
  }
}

#endif //CORE_PLUGINS_PLAYERS_DAC_DIGITAL_DEFINED
