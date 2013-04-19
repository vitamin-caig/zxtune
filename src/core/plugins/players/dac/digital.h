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
#include <core/module_holder.h>
#include <devices/dac_sample_factories.h>
#include <formats/chiptune/digital.h>
#include <sound/mixer_factory.h>

namespace ZXTune
{
  namespace Module
  {
    namespace DAC
    {
      class ModuleData : public TrackModel
      {
      public:
        typedef boost::shared_ptr<ModuleData> RWPtr;
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
        DataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props, const PatternsBuilder& builder)
          : Data(data)
          , Properties(props)
          , Builder(builder)
        {
          Data->Patterns = Builder.GetPatterns();
        }
      public:
        template<uint_t Channels>
        static std::auto_ptr<Formats::Chiptune::Digital::Builder> Create(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
        {
          return std::auto_ptr<Formats::Chiptune::Digital::Builder>(new DataBuilder(data, props, PatternsBuilder::Create<Channels>()));
        }

        virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
        {
          return *Properties;
        }

        virtual void SetInitialTempo(uint_t tempo)
        {
          Data->InitialTempo = tempo;
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
          Builder.SetPattern(index);
          return Builder;
        }

        virtual void StartChannel(uint_t index)
        {
          Builder.SetChannel(index);
        }

        virtual void SetRest()
        {
          Builder.GetChannel().SetEnabled(false);
        }

        virtual void SetNote(uint_t note)
        {
          Builder.GetChannel().SetEnabled(true);
          Builder.GetChannel().SetNote(note);
        }

        virtual void SetSample(uint_t sample)
        {
          Builder.GetChannel().SetSample(sample);
        }
      private:
        const ModuleData::RWPtr Data;
        const ModuleProperties::RWPtr Properties;
        PatternsBuilder Builder;
      };

      //TODO: remote this template namespace
      template<uint_t Channels>
      struct Digital
      {
        class Renderer : public ZXTune::Module::Renderer
        {
        public:
          Renderer(Parameters::Accessor::Ptr params, ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
            : Data(data)
            , Params(DAC::TrackParameters::Create(params))
            , Device(device)
            , Iterator(CreateTrackStateIterator(Data))
            , LastRenderTime(0)
          {
      #ifndef NDEBUG
      //perform self-test
            Devices::DAC::DataChunk chunk;
            while (Iterator->IsValid())
            {
              RenderData(chunk);
              Iterator->NextFrame(false);
            }
            Reset();
      #endif
          }

          virtual TrackState::Ptr GetTrackState() const
          {
            return Iterator->GetStateObserver();
          }

          virtual Analyzer::Ptr GetAnalyzer() const
          {
            return DAC::CreateAnalyzer(Device);
          }

          virtual bool RenderFrame()
          {
            if (Iterator->IsValid())
            {
              LastRenderTime += Params->FrameDuration();
              Devices::DAC::DataChunk chunk;
              RenderData(chunk);
              chunk.TimeStamp = LastRenderTime;
              Device->RenderData(chunk);
              Device->Flush();
              Iterator->NextFrame(Params->Looped());
            }
            return Iterator->IsValid();
          }

          virtual void Reset()
          {
            Device->Reset();
            Iterator->Reset();
            LastRenderTime = Time::Microseconds();
          }

          virtual void SetPosition(uint_t frame)
          {
            const TrackState::Ptr state = Iterator->GetStateObserver();
            if (frame < state->Frame())
            {
              //reset to beginning in case of moving back
              Iterator->Reset();
            }
            //fast forward
            Devices::DAC::DataChunk chunk;
            while (state->Frame() < frame && Iterator->IsValid())
            {
              //do not update tick for proper rendering
              RenderData(chunk);
              Iterator->NextFrame(false);
            }
          }

        private:
          void RenderData(Devices::DAC::DataChunk& chunk)
          {
            const TrackModelState::Ptr state = Iterator->GetStateObserver();
            DAC::TrackBuilder track;
            SynthesizeData(*state, track);
            track.GetResult(chunk.Channels);
          }

          void SynthesizeData(const TrackModelState& state, DAC::TrackBuilder& track)
          {
            if (0 == state.Quirk())
            {
              GetNewLineState(state, track);
            }
          }

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
          const DAC::TrackParameters::Ptr Params;
          const Devices::DAC::Chip::Ptr Device;
          const TrackStateIterator::Ptr Iterator;
          Time::Microseconds LastRenderTime;
        };

        class Holder : public ZXTune::Module::Holder
        {
        public:
          Holder(ModuleData::Ptr data, ModuleProperties::Ptr properties, uint_t baseFreq)
            : Data(data)
            , Properties(properties)
            , Info(CreateTrackInfo(Data, Channels))
            , BaseFreq(baseFreq)
          {
          }

          virtual Information::Ptr GetModuleInformation() const
          {
            return Info;
          }

          virtual Parameters::Accessor::Ptr GetModuleProperties() const
          {
            return Properties;
          }

          virtual Module::Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
          {
            const typename Sound::FixedChannelsMixer<Channels>::Ptr mixer = Sound::CreateMixer(params, Sound::FixedChannelsSample<Channels>());
            mixer->SetTarget(target);
            const Devices::DAC::Receiver::Ptr receiver = DAC::CreateReceiver(mixer);
            const Devices::DAC::ChipParameters::Ptr chipParams = DAC::CreateChipParameters(params);
            const Devices::DAC::Chip::Ptr chip(Devices::DAC::CreateChip(Channels, BaseFreq, chipParams, receiver));
            for (uint_t idx = 0, lim = Data->Samples.Size(); idx != lim; ++idx)
            {
              chip->SetSample(idx, Data->Samples.Get(idx));
            }
            return boost::make_shared<Renderer>(params, Data, chip);
          }
        private:
          const ModuleData::Ptr Data;
          const ModuleProperties::Ptr Properties;
          const Information::Ptr Info;
          const uint_t BaseFreq;
        };
      };
    }
  }
}

#endif //CORE_PLUGINS_PLAYERS_DAC_DIGITAL_DEFINED
